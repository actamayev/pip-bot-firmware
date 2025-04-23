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

void MotorDriver::brake_right_motor() {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 255); // Explicitly clear backward pin
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 255);
}

void MotorDriver::brake_left_motor() {
    analogWrite(LEFT_MOTOR_PIN_IN_1, 255); // Explicitly clear backward pin
    analogWrite(LEFT_MOTOR_PIN_IN_2, 255);
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

    // Only update motor controls if speeds have changed
    if (speedsChanged || StraightLineDrive::getInstance().isEnabled()) {
        // Apply the current speeds
        if (leftAdjusted == 0) {
            brake_left_motor();
            // left_motor_stop();
        } else if (leftAdjusted > 0) {
            left_motor_forward(leftAdjusted);
        } else {
            left_motor_backward(-leftAdjusted);
        }

        if (rightAdjusted == 0) {
            brake_right_motor();
            // right_motor_stop();
        } else if (rightAdjusted > 0) {
            right_motor_forward(rightAdjusted);
        } else {
            right_motor_backward(-rightAdjusted);
        }
    }
}
