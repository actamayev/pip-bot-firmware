#include "./motor_driver.h"

MotorDriver motorDriver;

MotorDriver::MotorDriver() {
    // Initialize motor pins
    pinMode(LEFT_MOTOR_PIN_IN_1, OUTPUT);
    pinMode(LEFT_MOTOR_PIN_IN_2, OUTPUT);
    pinMode(RIGHT_MOTOR_PIN_IN_1, OUTPUT);
    pinMode(RIGHT_MOTOR_PIN_IN_2, OUTPUT);
}

void MotorDriver::stop_both_motors() {
    left_motor_stop();
    right_motor_stop();
}

void MotorDriver::left_motor_forward(uint8_t speed) {
    analogWrite(LEFT_MOTOR_PIN_IN_2, 0); // Explicitly clear forward pin
    analogWrite(LEFT_MOTOR_PIN_IN_1, speed);
}

void MotorDriver::left_motor_backward(uint8_t speed) {
    analogWrite(LEFT_MOTOR_PIN_IN_1, 0); // Explicitly clear backward pin
    analogWrite(LEFT_MOTOR_PIN_IN_2, speed);
}

void MotorDriver::left_motor_stop() {
    analogWrite(LEFT_MOTOR_PIN_IN_1, 0);
    analogWrite(LEFT_MOTOR_PIN_IN_2, 0);
}

void MotorDriver::right_motor_forward(uint8_t speed) {
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 0); // Explicitly clear forward pin
    analogWrite(RIGHT_MOTOR_PIN_IN_1, speed);
}

void MotorDriver::right_motor_backward(uint8_t speed) {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 0); // Explicitly clear backward pin
    analogWrite(RIGHT_MOTOR_PIN_IN_2, speed);
}

void MotorDriver::right_motor_stop() {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 0);
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 0);
}

// These methods explicitly hold the motor in brake.
void MotorDriver::brake_left_motor() {
    analogWrite(LEFT_MOTOR_PIN_IN_1, 255);
    analogWrite(LEFT_MOTOR_PIN_IN_2, 255);
    _leftMotorBraking = true;
    // Serial.println("Braking left motor");
}

void MotorDriver::brake_right_motor() {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 255);
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 255);
    _rightMotorBraking = true;
    // Serial.println("Braking right motor");
}

void MotorDriver::brake_both_motors() {
    brake_left_motor();
    brake_right_motor();
}

void MotorDriver::release_left_brake() {
    left_motor_stop();
    _leftMotorBraking = false;
    // Serial.println("Released left brake");
}

void MotorDriver::release_right_brake() {
    right_motor_stop();
    _rightMotorBraking = false;
    // Serial.println("Released right brake");
}

void MotorDriver::brake_if_moving() {
    // Get current wheel speeds from encoder manager
    WheelRPMs rpms = encoderManager.getBothWheelRPMs();
    
    // Check if left motor is moving
    if (abs(rpms.leftWheelRPM) > MOTOR_STOPPED_THRESHOLD) {
        // Left motor is moving, apply brake
        brake_left_motor();
    } else if (_leftMotorBraking) {
        // Left motor already stopped but brake still applied, release it
        release_left_brake();
    }
    
    // Check if right motor is moving
    if (abs(rpms.rightWheelRPM) > MOTOR_STOPPED_THRESHOLD) {
        // Right motor is moving, apply brake
        brake_right_motor();
    } else if (_rightMotorBraking) {
        // Right motor already stopped but brake still applied, release it
        release_right_brake();
    }
}

void MotorDriver::set_motor_speeds(int16_t leftTarget, int16_t rightTarget) {
    // Store target speeds but don't change actual speeds immediately
    _targetLeftSpeed = constrain(leftTarget, -255, 255);
    _targetRightSpeed = constrain(rightTarget, -255, 255);
}

void MotorDriver::update_motor_speeds(bool should_ramp_up) {
    bool speedsChanged = false;

    if (should_ramp_up) {
        // Gradually ramp left motor speed toward target
        if (_currentLeftSpeed < _targetLeftSpeed) {
            _currentLeftSpeed = min(static_cast<int16_t>(_currentLeftSpeed + SPEED_RAMP_STEP), _targetLeftSpeed);
            speedsChanged = true;
        } else if (_currentLeftSpeed > _targetLeftSpeed) {
            _currentLeftSpeed = max(static_cast<int16_t>(_currentLeftSpeed - SPEED_RAMP_STEP), _targetLeftSpeed);
            speedsChanged = true;
        }

        // Gradually ramp right motor speed toward target
        if (_currentRightSpeed < _targetRightSpeed) {
            _currentRightSpeed = min(static_cast<int16_t>(_currentRightSpeed + SPEED_RAMP_STEP), _targetRightSpeed);
            speedsChanged = true;
        } else if (_currentRightSpeed > _targetRightSpeed) {
            _currentRightSpeed = max(static_cast<int16_t>(_currentRightSpeed - SPEED_RAMP_STEP), _targetRightSpeed);
            speedsChanged = true;
        }
    } else {
        // Skip ramping and set speeds immediately
        if (_currentLeftSpeed != _targetLeftSpeed) {
            _currentLeftSpeed = _targetLeftSpeed;
            speedsChanged = true;
        }
        
        if (_currentRightSpeed != _targetRightSpeed) {
            _currentRightSpeed = _targetRightSpeed;
            speedsChanged = true;
        }
    }

    int16_t leftAdjusted = _currentLeftSpeed;
    int16_t rightAdjusted = _currentRightSpeed;

    if (StraightLineDrive::getInstance().isEnabled()) {
        StraightLineDrive::getInstance().update(leftAdjusted, rightAdjusted);
    }

    if (_leftMotorBraking || _rightMotorBraking) {
        WheelRPMs rpms = encoderManager.getBothWheelRPMs();
        
        // If left motor is braking and has stopped, release the brake
        if (_leftMotorBraking && abs(rpms.leftWheelRPM) < MOTOR_STOPPED_THRESHOLD) {
            release_left_brake();
        }
        
        // If right motor is braking and has stopped, release the brake
        if (_rightMotorBraking && abs(rpms.rightWheelRPM) < MOTOR_STOPPED_THRESHOLD) {
            release_right_brake();
        }
    }

    // Only update motor controls if speeds have changed
    if (speedsChanged || StraightLineDrive::getInstance().isEnabled()) {
        // Apply the current speeds
        if (leftAdjusted == 0) {
            if (!_leftMotorBraking) {
                brake_left_motor();
            }
        } else if (leftAdjusted > 0) {
            _leftMotorBraking = false;
            left_motor_forward(leftAdjusted);
        } else {
            _leftMotorBraking = false;
            left_motor_backward(-leftAdjusted);
        }

        if (rightAdjusted == 0) {
            if (!_rightMotorBraking) {
                brake_right_motor();
            }
        } else if (rightAdjusted > 0) {
            _rightMotorBraking = false;
            right_motor_forward(rightAdjusted);
        } else {
            _rightMotorBraking = false;
            right_motor_backward(-rightAdjusted);
        }
    }
}

void MotorDriver::force_reset_motors() {
    // Force release any brakes
    if (_leftMotorBraking) {
        release_left_brake();
    }
    if (_rightMotorBraking) {
        release_right_brake();
    }
    // Reset speed targets
    _targetLeftSpeed = 0;
    _targetRightSpeed = 0;
    _currentLeftSpeed = 0;
    _currentRightSpeed = 0;
}
