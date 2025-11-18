#include "turning_manager.h"

#include <math.h>

#include <cmath>

bool TurningManager::start_turn(float degrees) {
    TurningManager& instance = TurningManager::get_instance();
    if (instance._currentState != TurningState::IDLE) {
        SerialQueueManager::get_instance().queue_message("Turn already in progress!");
        return false; // Turn already in progress
    }

    if (abs(degrees) < 0.1f) {
        SerialQueueManager::get_instance().queue_message("Invalid turn angle!");
        return false; // Invalid turn angle
    }

    instance._targetTurnAngle = degrees;
    initialize_turn();
    return true;
}

void TurningManager::initialize_turn() {
    TurningManager& instance = TurningManager::get_instance();
    // Initialize turn
    instance._cumulativeRotation = 0.0f;
    instance._rotationTrackingInitialized = false;
    instance._currentVelocity = 0.0f;
    instance._targetVelocity = 0.0f;
    instance._lastHeading = 0.0f;
    instance._lastHeadingForRotation = 0.0f;
    instance._lastTime = 0;
    instance._completionStartTime = 0;
    instance._completionConfirmed = false;
    instance._currentDirection = TurningDirection::NONE;
    instance._currentPWM = 0;
    instance._integralTerm = 0.0f;
    instance._lastIntegralTime = 0;
    instance._kpContribution = 0.0f;
    instance._kiContribution = 0.0f;
    instance._overshootBrakeStartTime = 0;

    instance._currentState = TurningState::TURNING;

    char log_message[64];
    snprintf(log_message, sizeof(log_message), "Starting turn: %.1f degrees", instance._targetTurnAngle);
    SerialQueueManager::get_instance().queue_message(log_message);
}

void TurningManager::update() {
    TurningManager& instance = TurningManager::get_instance();
    if (instance._currentState == TurningState::IDLE) {
        return;
    }

    // Update velocity and rotation tracking
    update_velocity();
    update_cumulative_rotation();

    // Handle overshoot braking
    if (instance._currentState == TurningState::OVERSHOOT_BRAKING) {
        if (millis() - instance._overshootBrakeStartTime >= OVERSHOOT_BRAKE_DURATION) {
            instance._currentState = TurningState::TURNING;
            SerialQueueManager::get_instance().queue_message("Overshoot braking complete - resuming");
        } else {
            motor_driver.brake_both_motors();
            return;
        }
    }

    // Calculate remaining angle
    float remaining_angle = instance.calculate_remaining_angle();

    // Check for overshoot at high speed
    if (check_overshoot(remaining_angle)) {
        char log_message[80];
        snprintf(log_message, sizeof(log_message), "High-speed overshoot detected! Braking for %dms", static_cast<int>(OVERSHOOT_BRAKE_DURATION));
        SerialQueueManager::get_instance().queue_message(log_message);
        instance._currentState = TurningState::OVERSHOOT_BRAKING;
        instance._overshootBrakeStartTime = millis();
        motor_driver.brake_both_motors();
        return;
    }

    // Calculate target velocity based on remaining angle
    instance._targetVelocity = calculate_target_velocity(remaining_angle);

    // Calculate velocity error
    float velocity_error = instance.calculate_velocity_error();

    // Calculate PWM
    instance._currentPWM = calculate_pwm(velocity_error);

    // Apply motor control
    apply_motor_control(instance._currentPWM, velocity_error);

    // Check completion
    if (instance.check_completion()) {
        instance.reset_turn_state();
        SerialQueueManager::get_instance().queue_message("Turn completed");
        return;
    }

    // Update debug info
    instance._debugInfo.targetAngle = instance._targetTurnAngle;
    instance._debugInfo.cumulativeRotation = instance._cumulativeRotation;
    instance._debugInfo.remainingAngle = remaining_angle;
    instance._debugInfo.currentVelocity = instance._currentVelocity;
    instance._debugInfo.targetVelocity = instance._targetVelocity;
    instance._debugInfo.velocityError = velocity_error;
    instance._debugInfo.currentPWM = instance._currentPWM;
    instance._debugInfo.kpContribution = instance._kpContribution;
    instance._debugInfo.kiContribution = instance._kiContribution;
    instance._debugInfo.inOvershootBraking = (instance._currentState == TurningState::OVERSHOOT_BRAKING);
}

void TurningManager::update_velocity() {
    TurningManager& instance = TurningManager::get_instance();
    float current_heading = -SensorDataBuffer::get_instance().get_latest_yaw();
    const uint32_t CURRENT_TIME = millis();

    if (instance._lastTime != 0) {
        float delta_time = (CURRENT_TIME - instance._lastTime) / 1000.0f;
        if (delta_time > 0) {
            float delta_heading = current_heading - instance._lastHeading;

            // Handle wrap-around
            while (delta_heading > 180.0f) {
                delta_heading -= 360.0f;
            }
            while (delta_heading < -180.0f) {
                delta_heading += 360.0f;
            }

            instance._currentVelocity = delta_heading / delta_time;
        }
    }

    instance._lastHeading = current_heading;
    instance._lastTime = CURRENT_TIME;
}

void TurningManager::update_cumulative_rotation() {
    TurningManager& instance = TurningManager::get_instance();
    float current_heading = -SensorDataBuffer::get_instance().get_latest_yaw();

    if (!instance._rotationTrackingInitialized) {
        instance._lastHeadingForRotation = current_heading;
        instance._rotationTrackingInitialized = true;
        return;
    }

    float heading_delta = current_heading - instance._lastHeadingForRotation;

    // Handle wrap-around (shortest path for each step)
    while (heading_delta > 180.0f) {
        heading_delta -= 360.0f;
    }
    while (heading_delta < -180.0f) {
        heading_delta += 360.0f;
    }

    instance._cumulativeRotation += heading_delta;
    instance._lastHeadingForRotation = current_heading;
}

float TurningManager::calculate_remaining_angle() const {
    return _targetTurnAngle - _cumulativeRotation;
}

float TurningManager::calculate_target_velocity(float remaining_angle) {
    float abs_remaining = abs(remaining_angle);
    float target_vel = NAN;

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
    return _targetVelocity - _currentVelocity;
}

uint16_t TurningManager::calculate_pwm(float velocity_error) {
    TurningManager& instance = TurningManager::get_instance();
    // Calculate deltaTime for integral term
    const uint32_t CURRENT_TIME = millis();
    float delta_time = 0.0f;

    if (instance._lastIntegralTime != 0) {
        delta_time = (CURRENT_TIME - instance._lastIntegralTime) / 1000.0f;

        // Accumulate integral term
        instance._integralTerm += velocity_error * delta_time;

        // Integral windup protection
        float max_integral = MAX_MOTOR_PWM / KI_VELOCITY;
        instance._integralTerm = constrain(instance._integralTerm, -max_integral, max_integral);
    }

    instance._lastIntegralTime = CURRENT_TIME;

    // Calculate individual contributions
    instance._kpContribution = KP_VELOCITY * velocity_error;
    instance._kiContribution = KI_VELOCITY * instance._integralTerm;

    // PID control
    float pwm = instance._kpContribution + instance._kiContribution;
    pwm = abs(pwm);

    return constrain((int)pwm, 0, MAX_MOTOR_PWM);
}

bool TurningManager::check_completion() {
    float remaining_angle = calculate_remaining_angle();

    // Check if within position and velocity thresholds
    if (abs(remaining_angle) <= COMPLETION_POSITION_THRESHOLD && abs(_currentVelocity) <= COMPLETION_VELOCITY_THRESHOLD) {
        // Start confirmation timer if not already started
        if (_completionStartTime == 0) {
            _completionStartTime = millis();
            char log_message[80];
            snprintf(log_message, sizeof(log_message), "Approaching completion: Remaining=%.2f°, Vel=%.2f°/s", remaining_angle, _currentVelocity);
            SerialQueueManager::get_instance().queue_message(log_message);
        }

        // Check if we've been in completion zone for required time
        if (millis() - _completionStartTime >= COMPLETION_CONFIRMATION_TIME) {
            if (!_completionConfirmed) {
                _completionConfirmed = true;
                motor_driver.brake_both_motors();
                char log_message[80];
                snprintf(log_message, sizeof(log_message), "Turn complete! Final error: %.2f°", remaining_angle);
                SerialQueueManager::get_instance().queue_message(log_message);
                return true;
            }
        }
    } else {
        // Reset completion timer if we move out of zone
        if (_completionStartTime != 0) {
            _completionStartTime = 0;
            _completionConfirmed = false;
            SerialQueueManager::get_instance().queue_message("Moved out of completion zone, resetting timer");
        }
    }

    return false;
}

void TurningManager::apply_motor_control(uint16_t pwm, float velocity_error) {
    TurningManager& instance = TurningManager::get_instance();
    // Determine required direction from TARGET velocity sign (not error sign!)
    TurningDirection required_direction = (instance._targetVelocity > 0) ? TurningDirection::CLOCKWISE : TurningDirection::COUNTER_CLOCKWISE;

    // If target velocity is essentially zero, stop
    if (abs(instance._targetVelocity) < 1.0f) {
        motor_driver.brake_both_motors();
        return;
    }

    // Allow direction changes when needed (overshoot correction)
    if (required_direction != instance._currentDirection && instance._currentDirection != TurningDirection::NONE) {
        SerialQueueManager::get_instance().queue_message("Direction change (overshoot correction)");

        // Stop briefly before changing direction
        motor_driver.brake_both_motors();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Apply motor commands
    instance._currentDirection = required_direction;

    if (required_direction == TurningDirection::CLOCKWISE) {
        motor_driver.set_motor_speeds_immediate(pwm, -pwm);
    } else {
        motor_driver.set_motor_speeds_immediate(-pwm, pwm);
    }
}

bool TurningManager::check_overshoot(float remaining_angle) {
    TurningManager& instance = TurningManager::get_instance();
    // Backup: reactive check for actual overshoot
    bool moving_away_from_target = (remaining_angle > 0 && instance._currentVelocity < 0) || (remaining_angle < 0 && instance._currentVelocity > 0);
    if (moving_away_from_target) {
        SerialQueueManager::get_instance().queue_message("REACTIVE: Already overshot!");
        return true;
    }

    // Predictive: time-to-target approach
    if (abs(instance._currentVelocity) > MIN_VELOCITY) {
        float time_to_target = abs(remaining_angle) / abs(instance._currentVelocity) * 1000.0f; // ms

        if (time_to_target < 10.0f && abs(remaining_angle) > COMPLETION_POSITION_THRESHOLD + 1.0f) {
            char log_message[120];
            snprintf(log_message, sizeof(log_message), "PREDICTIVE: Too fast! Time to target: %.1fms, Remaining: %.1f°, Vel: %.1f°/s", time_to_target,
                     remaining_angle, instance._currentVelocity);
            SerialQueueManager::get_instance().queue_message(log_message);
            return true;
        }
    }

    return false;
}

void TurningManager::reset_turn_state() {
    motor_driver.brake_both_motors();
    _currentState = TurningState::IDLE;
    _currentDirection = TurningDirection::NONE;
    _targetTurnAngle = 0.0f;
    _cumulativeRotation = 0.0f;
    _rotationTrackingInitialized = false;
    _currentVelocity = 0.0f;
    _targetVelocity = 0.0f;
    _lastHeading = 0.0f;
    _lastHeadingForRotation = 0.0f;
    _lastTime = 0;
    _completionStartTime = 0;
    _completionConfirmed = false;
    _currentPWM = 0;
    _integralTerm = 0.0f;
    _lastIntegralTime = 0;
    _kpContribution = 0.0f;
    _kiContribution = 0.0f;
    _overshootBrakeStartTime = 0;
}

void TurningManager::complete_navigation() {
    reset_turn_state();
}
