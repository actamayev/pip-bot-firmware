#include "turning_manager.h"

bool TurningManager::start_turn(float degrees) {
    if (currentState != TurningState::IDLE) {
        SerialQueueManager::get_instance().queue_message("Turn already in progress!");
        return false; // Turn already in progress
    }

    if (abs(degrees) < 0.1f) {
        SerialQueueManager::get_instance().queue_message("Invalid turn angle!");
        return false; // Invalid turn angle
    }

    targetTurnAngle = degrees;
    initialize_turn();
    return true;
}

void TurningManager::initialize_turn() {
    // Initialize turn
    cumulativeRotation = 0.0f;
    rotationTrackingInitialized = false;
    currentVelocity = 0.0f;
    targetVelocity = 0.0f;
    lastHeading = 0.0f;
    lastHeadingForRotation = 0.0f;
    lastTime = 0;
    completionStartTime = 0;
    completionConfirmed = false;
    currentDirection = TurningDirection::NONE;
    currentPWM = 0;
    integralTerm = 0.0f;
    lastIntegralTime = 0;
    kpContribution = 0.0f;
    kiContribution = 0.0f;
    overshootBrakeStartTime = 0;

    currentState = TurningState::TURNING;

    char logMessage[64];
    snprintf(logMessage, sizeof(logMessage), "Starting turn: %.1f degrees", targetTurnAngle);
    SerialQueueManager::get_instance().queue_message(logMessage);
}

void TurningManager::update() {
    if (currentState == TurningState::IDLE) return;

    // Update velocity and rotation tracking
    update_velocity();
    update_cumulative_rotation();

    // Handle overshoot braking
    if (currentState == TurningState::OVERSHOOT_BRAKING) {
        if (millis() - overshootBrakeStartTime >= OVERSHOOT_BRAKE_DURATION) {
            currentState = TurningState::TURNING;
            SerialQueueManager::get_instance().queue_message("Overshoot braking complete - resuming");
        } else {
            motorDriver.brake_both_motors();
            return;
        }
    }

    // Calculate remaining angle
    float remainingAngle = calculate_remaining_angle();

    // Check for overshoot at high speed
    if (check_overshoot(remainingAngle)) {
        char logMessage[80];
        snprintf(logMessage, sizeof(logMessage), "High-speed overshoot detected! Braking for %dms", (int)OVERSHOOT_BRAKE_DURATION);
        SerialQueueManager::get_instance().queue_message(logMessage);
        currentState = TurningState::OVERSHOOT_BRAKING;
        overshootBrakeStartTime = millis();
        motorDriver.brake_both_motors();
        return;
    }

    // Calculate target velocity based on remaining angle
    targetVelocity = calculate_target_velocity(remainingAngle);

    // Calculate velocity error
    float velocityError = calculate_velocity_error();

    // Calculate PWM
    currentPWM = calculate_pwm(velocityError);

    // Apply motor control
    apply_motor_control(currentPWM, velocityError);

    // Check completion
    if (check_completion()) {
        reset_turn_state();
        SerialQueueManager::get_instance().queue_message("Turn completed");
        return;
    }

    // Update debug info
    _debugInfo.targetAngle = targetTurnAngle;
    _debugInfo.cumulativeRotation = cumulativeRotation;
    _debugInfo.remainingAngle = remainingAngle;
    _debugInfo.currentVelocity = currentVelocity;
    _debugInfo.targetVelocity = targetVelocity;
    _debugInfo.velocityError = velocityError;
    _debugInfo.currentPWM = currentPWM;
    _debugInfo.kpContribution = kpContribution;
    _debugInfo.kiContribution = kiContribution;
    _debugInfo.inOvershootBraking = (currentState == TurningState::OVERSHOOT_BRAKING);
}

void TurningManager::update_velocity() {
    float currentHeading = -SensorDataBuffer::get_instance().get_latest_yaw();
    unsigned long current_time = millis();

    if (lastTime != 0) {
        float deltaTime = (current_time - lastTime) / 1000.0f;
        if (deltaTime > 0) {
            float deltaHeading = currentHeading - lastHeading;

            // Handle wrap-around
            while (deltaHeading > 180.0f)
                deltaHeading -= 360.0f;
            while (deltaHeading < -180.0f)
                deltaHeading += 360.0f;

            currentVelocity = deltaHeading / deltaTime;
        }
    }

    lastHeading = currentHeading;
    lastTime = current_time;
}

void TurningManager::update_cumulative_rotation() {
    float currentHeading = -SensorDataBuffer::get_instance().get_latest_yaw();

    if (!rotationTrackingInitialized) {
        lastHeadingForRotation = currentHeading;
        rotationTrackingInitialized = true;
        return;
    }

    float headingDelta = currentHeading - lastHeadingForRotation;

    // Handle wrap-around (shortest path for each step)
    while (headingDelta > 180.0f)
        headingDelta -= 360.0f;
    while (headingDelta < -180.0f)
        headingDelta += 360.0f;

    cumulativeRotation += headingDelta;
    lastHeadingForRotation = currentHeading;
}

float TurningManager::calculate_remaining_angle() const {
    return targetTurnAngle - cumulativeRotation;
}

float TurningManager::calculate_target_velocity(float remainingAngle) const {
    float absRemaining = abs(remainingAngle);
    float targetVel;

    if (absRemaining > 5.0f) {
        // Use cruise velocity when far from target
        targetVel = CRUISE_VELOCITY;
    } else {
        // Use minimum velocity when close to target
        targetVel = MIN_VELOCITY;
    }

    // Apply correct sign based on remaining angle direction
    return (remainingAngle > 0) ? targetVel : -targetVel;
}

float TurningManager::calculate_velocity_error() const {
    return targetVelocity - currentVelocity;
}

uint16_t TurningManager::calculate_pwm(float velocityError) {
    // Calculate deltaTime for integral term
    unsigned long current_time = millis();
    float deltaTime = 0.0f;

    if (lastIntegralTime != 0) {
        deltaTime = (current_time - lastIntegralTime) / 1000.0f;

        // Accumulate integral term
        integralTerm += velocityError * deltaTime;

        // Integral windup protection
        float maxIntegral = MAX_MOTOR_PWM / KI_VELOCITY;
        integralTerm = constrain(integralTerm, -maxIntegral, maxIntegral);
    }

    lastIntegralTime = current_time;

    // Calculate individual contributions
    kpContribution = KP_VELOCITY * velocityError;
    kiContribution = KI_VELOCITY * integralTerm;

    // PID control
    float pwm = kpContribution + kiContribution;
    pwm = abs(pwm);

    return constrain((int)pwm, 0, MAX_MOTOR_PWM);
}

bool TurningManager::check_completion() {
    float remainingAngle = calculate_remaining_angle();

    // Check if within position and velocity thresholds
    if (abs(remainingAngle) <= COMPLETION_POSITION_THRESHOLD && abs(currentVelocity) <= COMPLETION_VELOCITY_THRESHOLD) {
        // Start confirmation timer if not already started
        if (completionStartTime == 0) {
            completionStartTime = millis();
            char logMessage[80];
            snprintf(logMessage, sizeof(logMessage), "Approaching completion: Remaining=%.2f°, Vel=%.2f°/s", remainingAngle, currentVelocity);
            SerialQueueManager::get_instance().queue_message(logMessage);
        }

        // Check if we've been in completion zone for required time
        if (millis() - completionStartTime >= COMPLETION_CONFIRMATION_TIME) {
            if (!completionConfirmed) {
                completionConfirmed = true;
                motorDriver.brake_both_motors();
                char logMessage[80];
                snprintf(logMessage, sizeof(logMessage), "Turn complete! Final error: %.2f°", remainingAngle);
                SerialQueueManager::get_instance().queue_message(logMessage);
                return true;
            }
        }
    } else {
        // Reset completion timer if we move out of zone
        if (completionStartTime != 0) {
            completionStartTime = 0;
            completionConfirmed = false;
            SerialQueueManager::get_instance().queue_message("Moved out of completion zone, resetting timer");
        }
    }

    return false;
}

void TurningManager::apply_motor_control(uint16_t pwm, float velocityError) {
    // Determine required direction from TARGET velocity sign (not error sign!)
    TurningDirection requiredDirection = (targetVelocity > 0) ? TurningDirection::CLOCKWISE : TurningDirection::COUNTER_CLOCKWISE;

    // If target velocity is essentially zero, stop
    if (abs(targetVelocity) < 1.0f) {
        motorDriver.brake_both_motors();
        return;
    }

    // Allow direction changes when needed (overshoot correction)
    if (requiredDirection != currentDirection && currentDirection != TurningDirection::NONE) {
        SerialQueueManager::get_instance().queue_message("Direction change (overshoot correction)");

        // Stop briefly before changing direction
        motorDriver.brake_both_motors();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Apply motor commands
    currentDirection = requiredDirection;

    if (requiredDirection == TurningDirection::CLOCKWISE) {
        motorDriver.set_motor_speeds_immediate(pwm, -pwm);
    } else {
        motorDriver.set_motor_speeds_immediate(-pwm, pwm);
    }
}

bool TurningManager::check_overshoot(float remainingAngle) {
    // Backup: reactive check for actual overshoot
    bool movingAwayFromTarget = (remainingAngle > 0 && currentVelocity < 0) || (remainingAngle < 0 && currentVelocity > 0);
    if (movingAwayFromTarget) {
        SerialQueueManager::get_instance().queue_message("REACTIVE: Already overshot!");
        return true;
    }

    // Predictive: time-to-target approach
    if (abs(currentVelocity) > MIN_VELOCITY) {
        float timeToTarget = abs(remainingAngle) / abs(currentVelocity) * 1000.0f; // ms

        if (timeToTarget < 10.0f && abs(remainingAngle) > COMPLETION_POSITION_THRESHOLD + 1.0f) {
            char logMessage[120];
            snprintf(logMessage, sizeof(logMessage), "PREDICTIVE: Too fast! Time to target: %.1fms, Remaining: %.1f°, Vel: %.1f°/s", timeToTarget,
                     remainingAngle, currentVelocity);
            SerialQueueManager::get_instance().queue_message(logMessage);
            return true;
        }
    }

    return false;
}

void TurningManager::reset_turn_state() {
    motorDriver.brake_both_motors();
    currentState = TurningState::IDLE;
    currentDirection = TurningDirection::NONE;
    targetTurnAngle = 0.0f;
    cumulativeRotation = 0.0f;
    rotationTrackingInitialized = false;
    currentVelocity = 0.0f;
    targetVelocity = 0.0f;
    lastHeading = 0.0f;
    lastHeadingForRotation = 0.0f;
    lastTime = 0;
    completionStartTime = 0;
    completionConfirmed = false;
    currentPWM = 0;
    integralTerm = 0.0f;
    lastIntegralTime = 0;
    kpContribution = 0.0f;
    kiContribution = 0.0f;
    overshootBrakeStartTime = 0;
}

void TurningManager::complete_navigation() {
    reset_turn_state();
}
