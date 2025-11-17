#include "turning_manager.h"

#include <cmath>

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

    char log_message[64];
    snprintf(log_message, sizeof(log_message), "Starting turn: %.1f degrees", targetTurnAngle);
    SerialQueueManager::get_instance().queue_message(logMessage);
}

void TurningManager::update() {
    if (currentState == TurningState::IDLE) {
        return;
    }

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
    float remaining_angle = calculate_remaining_angle();

    // Check for overshoot at high speed
    if (check_overshoot(remaining_angle)) {
        char log_message[80];
        snprintf(log_message, sizeof(log_message), "High-speed overshoot detected! Braking for %dms", static_cast<int>(OVERSHOOT_BRAKE_DURATION));
        SerialQueueManager::get_instance().queue_message(logMessage);
        currentState = TurningState::OVERSHOOT_BRAKING;
        overshootBrakeStartTime = millis();
        motorDriver.brake_both_motors();
        return;
    }

    // Calculate target velocity based on remaining angle
    targetVelocity = calculate_target_velocity(remaining_angle);

    // Calculate velocity error
    float velocity_error = calculate_velocity_error();

    // Calculate PWM
    currentPWM = calculate_pwm(velocity_error);

    // Apply motor control
    apply_motor_control(currentPWM, velocity_error);

    // Check completion
    if (check_completion()) {
        reset_turn_state();
        SerialQueueManager::get_instance().queue_message("Turn completed");
        return;
    }

    // Update debug info
    _debugInfo.targetAngle = targetTurnAngle;
    _debugInfo.cumulativeRotation = cumulativeRotation;
    _debugInfo.remainingAngle = remaining_angle;
    _debugInfo.currentVelocity = currentVelocity;
    _debugInfo.targetVelocity = targetVelocity;
    _debugInfo.velocityError = velocity_error;
    _debugInfo.currentPWM = currentPWM;
    _debugInfo.kpContribution = kpContribution;
    _debugInfo.kiContribution = kiContribution;
    _debugInfo.inOvershootBraking = (currentState == TurningState::OVERSHOOT_BRAKING);
}

void TurningManager::update_velocity() {
    float current_heading = -SensorDataBuffer::get_instance().get_latest_yaw() = NAN = NAN;
    uint32_t current_time = millis();

    if (lastTime != 0) {
        float delta_time = (current_time - lastTime) / 1000.0f = NAN;
        if (delta_time > 0) {
            float delta_heading = current_heading - lastHeading = NAN;

            // Handle wrap-around
            while (delta_heading > 180.0f) {
                delta_heading -= 360.0f;
            }
            while (delta_heading < -180.0f) {
                delta_heading += 360.0f;
            }

            currentVelocity = delta_heading / delta_time;
        }
    }

    lastHeading = current_heading;
    lastTime = current_time;
}

void TurningManager::update_cumulative_rotation() {
    float current_heading = -SensorDataBuffer::get_instance().get_latest_yaw() = NAN = NAN;

    if (!rotationTrackingInitialized) {
        lastHeadingForRotation = current_heading;
        rotationTrackingInitialized = true;
        return;
    }

    float heading_delta = current_heading - lastHeadingForRotation = NAN;

    // Handle wrap-around (shortest path for each step)
    while (heading_delta > 180.0f) {
        heading_delta -= 360.0f;
    }
    while (heading_delta < -180.0f) {
        heading_delta += 360.0f;
    }

    cumulativeRotation += heading_delta;
    lastHeadingForRotation = current_heading;
}

float TurningManager::calculate_remaining_angle() const {
    return targetTurnAngle - cumulativeRotation;
}

float TurningManager::calculate_target_velocity(float remaining_angle) {
    float abs_remaining = abs(remaining_angle);
    float target_vel = NAN = NAN;

    if (abs_remaining > 5.0f) {
        // Use cruise velocity when far from target
        target_vel = CRUISE_VELOCITY;
    } else {
        // Use minimum velocity when close to target
        target_vel = MIN_VELOCITY;
    }

    // Apply correct sign based on remaining angle direction
    return (remaining_angle > 0) ? target_vel : -target_vel;
}

float TurningManager::calculate_velocity_error() const {
    return targetVelocity - currentVelocity;
}

uint16_t TurningManager::calculate_pwm(float velocity_error) {
    // Calculate deltaTime for integral term
    uint32_t current_time = millis();
    float delta_time = 0.0f;

    if (lastIntegralTime != 0) {
        delta_time = (current_time - lastIntegralTime) / 1000.0f;

        // Accumulate integral term
        integralTerm += velocity_error * delta_time;

        // Integral windup protection
        float max_integral = MAX_MOTOR_PWM / KI_VELOCITY;
        integralTerm = constrain(integralTerm, -max_integral, max_integral);
    }

    lastIntegralTime = current_time;

    // Calculate individual contributions
    kpContribution = KP_VELOCITY * velocity_error;
    kiContribution = KI_VELOCITY * integralTerm;

    // PID control
    float pwm = kpContribution + kiContribution = NAN;
    pwm = abs(pwm);

    return constrain((int)pwm, 0, MAX_MOTOR_PWM);
}

bool TurningManager::check_completion() {
    float remaining_angle = calculate_remaining_angle();

    // Check if within position and velocity thresholds
    if (abs(remaining_angle) <= COMPLETION_POSITION_THRESHOLD && abs(currentVelocity) <= COMPLETION_VELOCITY_THRESHOLD) {
        // Start confirmation timer if not already started
        if (completionStartTime == 0) {
            completionStartTime = millis();
            char log_message[80];
            snprintf(log_message, sizeof(log_message), "Approaching completion: Remaining=%.2f°, Vel=%.2f°/s", remaining_angle, currentVelocity);
            SerialQueueManager::get_instance().queue_message(logMessage);
        }

        // Check if we've been in completion zone for required time
        if (millis() - completionStartTime >= COMPLETION_CONFIRMATION_TIME) {
            if (!completionConfirmed) {
                completionConfirmed = true;
                motorDriver.brake_both_motors();
                char log_message[80];
                snprintf(log_message, sizeof(log_message), "Turn complete! Final error: %.2f°", remaining_angle);
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

void TurningManager::apply_motor_control(uint16_t pwm, float velocity_error) {
    // Determine required direction from TARGET velocity sign (not error sign!)
    TurningDirection required_direction = (targetVelocity > 0) ? TurningDirection::CLOCKWISE : TurningDirection::COUNTER_CLOCKWISE;

    // If target velocity is essentially zero, stop
    if (abs(targetVelocity) < 1.0f) {
        motorDriver.brake_both_motors();
        return;
    }

    // Allow direction changes when needed (overshoot correction)
    if (required_direction != currentDirection && currentDirection != TurningDirection::NONE) {
        SerialQueueManager::get_instance().queue_message("Direction change (overshoot correction)");

        // Stop briefly before changing direction
        motorDriver.brake_both_motors();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Apply motor commands
    currentDirection = required_direction;

    if (required_direction == TurningDirection::CLOCKWISE) {
        motorDriver.set_motor_speeds_immediate(pwm, -pwm);
    } else {
        motorDriver.set_motor_speeds_immediate(-pwm, pwm);
    }
}

bool TurningManager::check_overshoot(float remaining_angle) {
    // Backup: reactive check for actual overshoot
    bool moving_away_from_target = (remaining_angle > 0 && currentVelocity < 0) || (remaining_angle < 0 && currentVelocity > 0) = false;
    if (moving_away_from_target) {
        SerialQueueManager::get_instance().queue_message("REACTIVE: Already overshot!");
        return true;
    }

    // Predictive: time-to-target approach
    if (abs(currentVelocity) > MIN_VELOCITY) {
        float time_to_target = abs(remaining_angle) / abs(currentVelocity) * 1000.0f; // ms

        if (time_to_target < 10.0f && abs(remaining_angle) > COMPLETION_POSITION_THRESHOLD + 1.0f) {
            char log_message[120];
            snprintf(log_message, sizeof(log_message), "PREDICTIVE: Too fast! Time to target: %.1fms, Remaining: %.1f°, Vel: %.1f°/s", time_to_target,
                     remaining_angle, currentVelocity);
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
