#include "turning_manager.h"
#include "networking/serial_queue_manager.h"

bool TurningManager::startTurn(float degrees) {
    if (currentState != TurningState::IDLE) return false; // Turn already in progress
    
    if (abs(degrees) < 0.1f) return false; // Invalid turn angle
    
    TARGET_TURN_ANGLE = degrees;
    initializeTurn();
    return true;
}

void TurningManager::initializeTurn() {
    // Reset all state
    turnDirectionChanges = 0;
    cumulativeRotation = 0.0f;
    rotationTrackingInitialized = false;
    currentError = 0.0f;
    currentVelocity = 0.0f;
    lastTime = 0;
    turnCompletionStartTime = 0;
    turnCompletionConfirmed = false;
    inSafetyPause = false;
    getRotationError();
    
    // Determine direction from sign of degrees
    targetDirection = (TARGET_TURN_ANGLE > 0) ? TurningDirection::CLOCKWISE : TurningDirection::COUNTER_CLOCKWISE;
    
    currentState = TurningState::TURNING;
    startTurnMotors();
    
    // char logMessage[64];
    // snprintf(logMessage, sizeof(logMessage), "Starting turn: %.1f degrees %s", 
    //          degrees, (degrees > 0) ? "CW" : "CCW");
    // SerialQueueManager::getInstance().queueMessage(logMessage);
}

void TurningManager::update() {
    if (currentState == TurningState::IDLE) return;
    
    updateVelocity();
    
    // Handle safety pause
    if (inSafetyPause) {
        if (millis() - safetyPauseStartTime < SAFETY_PAUSE_DURATION) return;
        inSafetyPause = false;
        turnDirectionChanges = 0; // Reset counter - give fresh chance
        SerialQueueManager::getInstance().queueMessage("Safety pause complete - resuming turn");
    }
    
    // Check for safety triggers (only when not already paused)
    if (!inSafetyPause && checkSafetyTriggers()) {
        triggerSafetyPause();
        return;
    }

    unsigned long currentTime = millis();
    
    // Update error based on current mode
    updateCumulativeRotation();
    getRotationError();
    
    adaptPWMLimits(calculatePIDSpeed());
    
    // Check if we're close enough to brake
    if (abs(currentError) > DEAD_ZONE) {
        // Reset confirmation timer if we move out of dead zone
        if (turnCompletionStartTime != 0) {
            turnCompletionStartTime = 0;
            turnCompletionConfirmed = false;
        }
    } else {
        motorDriver.brake_both_motors();
        
        // Start confirmation timer if not already started
        if (turnCompletionStartTime == 0) {
            turnCompletionStartTime = currentTime;
        }
        
        // Check if we've been in dead zone for confirmation time AND velocity has settled
        if (currentTime - turnCompletionStartTime >= COMPLETION_CONFIRMATION_TIME) {
            if (!turnCompletionConfirmed && abs(currentVelocity) < 20.0f) {
                turnCompletionConfirmed = true;
                completeNavigation(true);
                SerialQueueManager::getInstance().queueMessage("Turn completed");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(250));
        return;
    }
    
    // Determine required direction
    TurningDirection requiredDirection = (currentError > 0) ? TurningDirection::CLOCKWISE : TurningDirection::COUNTER_CLOCKWISE;
    
    // Calculate base PID speed
    uint8_t newPidSpeed = calculatePIDSpeed();
    
    if (newPidSpeed == 0) {
        motorDriver.brake_both_motors();
        vTaskDelay(pdMS_TO_TICKS(20));
        return;
    }

    // Change direction if needed
    if (requiredDirection == currentDirection) {
        setTurnSpeed(newPidSpeed);
    } else {
        // Add small delay when changing direction to reduce oscillation
        if (currentDirection != TurningDirection::NONE) {
            motorDriver.stop_both_motors();
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        
        // Track direction changes for safety
        if (currentDirection != TurningDirection::NONE) {
            turnDirectionChanges++;
        }
        
        currentDirection = requiredDirection;
        setTurnSpeed(newPidSpeed);
    }
}

void TurningManager::updateCumulativeRotation() {
    float currentHeading = -SensorDataBuffer::getInstance().getLatestYaw();
    
    if (!rotationTrackingInitialized) {
        lastHeadingForRotation = currentHeading;
        rotationTrackingInitialized = true;
        return;
    }
    
    float headingDelta = currentHeading - lastHeadingForRotation;
    
    // Handle wrap-around (shortest path for each step)
    while (headingDelta > 180.0f) headingDelta -= 360.0f;
    while (headingDelta < -180.0f) headingDelta += 360.0f;
    
    cumulativeRotation += headingDelta;
    lastHeadingForRotation = currentHeading;
}

void TurningManager::getRotationError() {
    float remainingRotation = TARGET_TURN_ANGLE - cumulativeRotation;
    
    // For the final approach, we want to slow down
    currentError = remainingRotation;
    
    // Determine direction based on remaining rotation
    if (abs(remainingRotation) < DEAD_ZONE) {
        currentError = 0.0f;  // Close enough
    }
}

void TurningManager::updateVelocity() {
    float currentHeading = -SensorDataBuffer::getInstance().getLatestYaw();
    unsigned long currentTime = millis();
    
    if (lastTime != 0) {
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        float deltaHeading = currentHeading - lastHeading;
        
        // Handle wrap-around
        while (deltaHeading > 180.0f) deltaHeading -= 360.0f;
        while (deltaHeading < -180.0f) deltaHeading += 360.0f;
        
        currentVelocity = deltaHeading / deltaTime;
    }
    lastHeading = currentHeading;
    lastTime = currentTime;
}

uint8_t TurningManager::calculatePIDSpeed() {
    const float k = 0.2f;
    const float midpoint = 60.0f;

    float sigmoidValue = 1.0f / (1.0f + exp(-k * (abs(currentError) - midpoint)));
    
    // Check if we need active braking
    if (abs(currentError) < 5.0f && abs(currentVelocity) > 80.0f) {
        return 0;
    }

    // Velocity-based approach: reduce speed if approaching target too fast
    if (abs(currentError) < 20.0f && abs(currentVelocity) > 100.0f) {
        sigmoidValue *= 0.6f;
    }

    const float maxResponse = currentMaxPWM - currentMinPWM;
    uint8_t targetPWM = currentMinPWM + (sigmoidValue * maxResponse);
    targetPWM = constrain(targetPWM, currentMinPWM, currentMaxPWM);
    
    // Final approach boost for friction surfaces
    if (abs(currentError) > DEAD_ZONE && abs(currentError) < 8.0f) {
        uint8_t frictionMinPWM = currentMinPWM + 5; // Boost minimum for final approach
        targetPWM = max(targetPWM, frictionMinPWM);
        targetPWM = min(targetPWM, currentMaxPWM); // Still respect max limit
    }
    
    return targetPWM;
}

void TurningManager::adaptPWMLimits(uint8_t commandedPWM) {
    unsigned long currentTime = millis();
    
    if (abs(currentError) < 15.0f) return;
    // Rate limit adaptations to prevent oscillation
    if (currentTime - lastAdaptationTime < ADAPTATION_RATE_LIMIT) return;
    
    float absVelocity = abs(currentVelocity);
    bool adapted = false;
    
    // Increase limits if commanding high PWM but velocity too low
    if (commandedPWM >= currentMaxPWM * 0.9f && absVelocity < targetMinVelocity) {
        currentMaxPWM = min(255, currentMaxPWM + 5);
        currentMinPWM = min(currentMaxPWM - 10, currentMinPWM + 2);
        adapted = true;
    }
    // Decrease max if commanding max PWM but velocity too high
    else if (commandedPWM >= currentMaxPWM * 0.9f && absVelocity > targetMaxVelocity) {
        currentMaxPWM = max(currentMinPWM + 10, currentMaxPWM - 3);
        adapted = true;
    }
    // Increase min if commanding min PWM but velocity too low
    else if (commandedPWM <= currentMinPWM * 1.1f && absVelocity < targetMinVelocity) {
        currentMinPWM = min(currentMaxPWM - 10, currentMinPWM + 3);
        adapted = true;
    }
    
    if (adapted) {
        lastAdaptationTime = currentTime;
    }
}

bool TurningManager::checkSafetyTriggers() {
    // Check high approach velocity
    if (abs(currentVelocity) > 1.0f) {
        float timeToTarget = abs(currentError) / abs(currentVelocity) * 1000.0f; // Convert to ms
        if (timeToTarget < 75.0f && abs(currentError) > 2.0f) {
            SerialQueueManager::getInstance().queueMessage("Safety: Approaching too fast!");
            return true;
        }
    }
    
    // Check direction changes
    if (turnDirectionChanges >= MAX_DIRECTION_CHANGES) {
        char logMessage[48];
        snprintf(logMessage, sizeof(logMessage), "Safety: Too many direction changes (%d)", turnDirectionChanges);
        SerialQueueManager::getInstance().queueMessage(logMessage);
        return true;
    }
    
    return false;
}

void TurningManager::triggerSafetyPause() {
    inSafetyPause = true;
    safetyPauseStartTime = millis();
    motorDriver.brake_both_motors();
    
    // Reset PWM to safe defaults
    currentMinPWM = safetyDefaultMinPWM;
    currentMaxPWM = safetyDefaultMaxPWM;
    
    SerialQueueManager::getInstance().queueMessage("Safety pause activated - resetting PWM and braking");
}

void TurningManager::startTurnMotors() {
    uint8_t speed = calculatePIDSpeed();
    currentDirection = targetDirection;
    setTurnSpeed(speed);
}

void TurningManager::setTurnSpeed(uint8_t speed) {
    // Check if we're in final approach where stiction is problematic
    bool inFinalApproach = (abs(currentError) > DEAD_ZONE && abs(currentError) < 8.0f);
    bool needsStictionFighting = inFinalApproach && abs(currentVelocity) < 10.0f; // Very low velocity indicates stiction
    
    if (needsStictionFighting) {
        // Use immediate control with boosted minimum PWM to fight stiction
        uint8_t stictionSpeed = max(speed, static_cast<uint8_t>(currentMinPWM + 10));
        if (currentDirection == TurningDirection::CLOCKWISE) {
            motorDriver.set_motor_speeds_immediate(stictionSpeed, -stictionSpeed);
        } else if (currentDirection == TurningDirection::COUNTER_CLOCKWISE) {
            motorDriver.set_motor_speeds_immediate(-stictionSpeed, stictionSpeed);
        }
    } else {
        // Use immediate control for all turning - no ramping needed for turns
        if (currentDirection == TurningDirection::CLOCKWISE) {
            motorDriver.set_motor_speeds_immediate(speed, -speed);
        } else if (currentDirection == TurningDirection::COUNTER_CLOCKWISE) {
            motorDriver.set_motor_speeds_immediate(-speed, speed);
        }
    }
}

void TurningManager::completeNavigation(bool absoluteBrake) {
    if (absoluteBrake) {
        motorDriver.brake_both_motors();
    } else {
        motorDriver.brake_if_moving();
    }
    currentDirection = TurningDirection::NONE;
    targetDirection = TurningDirection::NONE;
    currentState = TurningState::IDLE;
    turnCompletionStartTime = 0;
    turnCompletionConfirmed = false;
    
    // Reset rotation tracking
    cumulativeRotation = 0.0;
    rotationTrackingInitialized = false;
    TARGET_TURN_ANGLE = 0.0f;
}
