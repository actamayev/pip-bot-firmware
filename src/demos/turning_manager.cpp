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

    // Update debug info
    _debugInfo.targetAngle = TARGET_TURN_ANGLE;
    _debugInfo.cumulativeRotation = cumulativeRotation;
    _debugInfo.currentError = currentError;
    _debugInfo.currentVelocity = currentVelocity;
    _debugInfo.currentMinPWM = currentMinPWM;
    _debugInfo.currentMaxPWM = currentMaxPWM;
    _debugInfo.directionChanges = turnDirectionChanges;
    _debugInfo.inSafetyPause = inSafetyPause;
    
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
    adaptPWMLimits();
    
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
                completeNavigation();
                SerialQueueManager::getInstance().queueMessage("Turn completed");
            }
        }
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
    const float k = 0.4f;
    const float midpoint = 50.0f;
    float sigmoidValue = 1.0f / (1.0f + exp(-k * (abs(currentError) - midpoint)));
    
    // Simple velocity limiting (less aggressive)
    if (abs(currentError) < 15.0f && abs(currentVelocity) > 120.0f) {
        sigmoidValue *= 0.7f;  // Only in final 15°
    }
    
    const float maxResponse = currentMaxPWM - currentMinPWM;
    uint8_t targetPWM = currentMinPWM + (sigmoidValue * maxResponse);
    
    // **SIMPLE FIX: Ensure minimum PWM in approach zone**
    if (abs(currentError) < 45.0f && abs(currentError) > DEAD_ZONE) {
        uint8_t minApproachPWM = currentMinPWM + 20;  // Always enough to move
        targetPWM = max(targetPWM, minApproachPWM);
    }
    
    return constrain(targetPWM, currentMinPWM, currentMaxPWM);
}

void TurningManager::detectStiction() {
    unsigned long currentTime = millis();
    bool isStuck = (abs(currentVelocity) < STICTION_VELOCITY_THRESHOLD);
    
    if (isStuck) {
        if (stictionDetectionStartTime == 0) {
            stictionDetectionStartTime = currentTime;
        } else if (currentTime - stictionDetectionStartTime > STICTION_DETECTION_TIME) {
            if (!stictionDetected) {
                stictionDetected = true;
                stictionBoostLevel = 1;
                SerialQueueManager::getInstance().queueMessage("Stiction detected - applying boost");
            } else {
                // Gradually increase boost level for persistent stiction
                if (currentTime - stictionDetectionStartTime > STICTION_DETECTION_TIME * (stictionBoostLevel + 1)) {
                    stictionBoostLevel = min(5, stictionBoostLevel + 1);
                }
            }
        }
    } else {
        // Moving well, reset detection timer but keep boost for a bit
        stictionDetectionStartTime = 0;
        if (abs(currentVelocity) > STICTION_VELOCITY_THRESHOLD * 2) {
            // Only clear stiction flag if we're moving well
            stictionDetected = false;
            stictionBoostLevel = 0;
        }
    }
}

void TurningManager::resetStictionDetection() {
    stictionDetected = false;
    stictionBoostLevel = 0;
    stictionDetectionStartTime = 0;
}

void TurningManager::adaptPWMLimits() {
    unsigned long currentTime = millis();
    
    // REMOVED: if (abs(currentError) < 15.0f) return;
    // Rate limit adaptations to prevent oscillation
    if (currentTime - lastAdaptationTime < ADAPTATION_RATE_LIMIT) return;
    
    float absVelocity = abs(currentVelocity);
    bool adapted = false;
    
    // IMPROVED: More aggressive adaptation in extended approach zone
    bool inFinalApproach = abs(currentError) < 30.0f;  // Extended from 15° to 30°
    uint8_t adaptationRate = inFinalApproach ? 4 : 2; // Faster adaptation in final approach
    uint8_t maxIncrease = inFinalApproach ? 8 : 5;    // Larger jumps in final approach

    uint8_t commandedPWM = calculatePIDSpeed();
    
    // Increase limits if commanding high PWM but velocity too low
    if (commandedPWM >= currentMaxPWM * 0.9f && absVelocity < targetMinVelocity) {
        currentMaxPWM = min(255, currentMaxPWM + maxIncrease);
        currentMinPWM = min(currentMaxPWM - 10, currentMinPWM + adaptationRate);
        adapted = true;
    }
    // Decrease max if commanding max PWM but velocity too high
    else if (commandedPWM >= currentMaxPWM * 0.9f && absVelocity > targetMaxVelocity) {
        currentMaxPWM = max(currentMinPWM + 10, currentMaxPWM - 3);
        adapted = true;
    }
    // IMPROVED: More aggressive min PWM increase when stuck in final approach
    else if (commandedPWM <= currentMinPWM * 1.1f && absVelocity < targetMinVelocity) {
        uint8_t minIncrease = inFinalApproach ? 5 : 3; // More aggressive in final approach
        currentMinPWM = min(currentMaxPWM - 10, currentMinPWM + minIncrease);
        adapted = true;
    }
    // NEW: Special case for final approach stiction
    else if (inFinalApproach && stictionDetected && absVelocity < STICTION_VELOCITY_THRESHOLD) {
        // Force increase both limits when stiction is detected
        currentMinPWM = min(currentMaxPWM - 5, currentMinPWM + 6);
        currentMaxPWM = min(255, currentMaxPWM + 4);
        adapted = true;
    }
    
    if (adapted) {
        lastAdaptationTime = currentTime;
        
        // Debug logging for final approach
        if (inFinalApproach) {
            char logMessage[80];
            snprintf(logMessage, sizeof(logMessage), 
                "Final PWM adapt: min=%d, max=%d, vel=%.1f, err=%.1f", 
                currentMinPWM, currentMaxPWM, currentVelocity, currentError);
            SerialQueueManager::getInstance().queueMessage(logMessage);
        }
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
    // Check if we're in extended approach where stiction is problematic
    bool inFinalApproach = (abs(currentError) > DEAD_ZONE && abs(currentError) < 25.0f);  // Extended range
    bool needsStictionFighting = inFinalApproach && abs(currentVelocity) < 15.0f; // Slightly higher threshold
    
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

void TurningManager::completeNavigation() {
    // Note: We are not doing brake_if_moving because 
    motorDriver.brake_both_motors();
    currentDirection = TurningDirection::NONE;
    targetDirection = TurningDirection::NONE;
    currentState = TurningState::IDLE;
    turnCompletionStartTime = 0;
    turnCompletionConfirmed = false;

    // Reset rotation tracking
    cumulativeRotation = 0.0;
    rotationTrackingInitialized = false;
    TARGET_TURN_ANGLE = 0.0f;

    // Reset control variables to prevent restart loops
    currentError = 0.0f;
    currentVelocity = 0.0f;
    lastHeading = 0.0f;
    lastTime = 0;
    turnDirectionChanges = 0;
    inSafetyPause = false;
    lastHeadingForRotation = 0.0f;
}
