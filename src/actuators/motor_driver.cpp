#include "motor_driver.h"

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
}

void MotorDriver::brake_right_motor() {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 255);
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 255);
}

void MotorDriver::brake_both_motors() {
    brake_left_motor();
    brake_right_motor();
}

void MotorDriver::brake_if_moving() {
    // Get current wheel speeds from encoder manager
    WheelRPMs rpms = encoderManager.getBothWheelRPMs();

    SerialQueueManager::getInstance().queueMessage("Left Wheel RPM" + String(rpms.leftWheelRPM));
    SerialQueueManager::getInstance().queueMessage("Left Wheel RPM" + String(rpms.leftWheelRPM));
    // Check if left motor is moving
    if (abs(rpms.leftWheelRPM) > MOTOR_STOPPED_THRESHOLD) {
        // Left motor is moving, apply brake
        brake_left_motor();
    }
    // // Check if right motor is moving
    if (abs(rpms.rightWheelRPM) > MOTOR_STOPPED_THRESHOLD) {
        // Right motor is moving, apply brake
        brake_right_motor();
    }
}

void MotorDriver::set_motor_speeds(int16_t leftTarget, int16_t rightTarget, bool shouldRampUp) {
    // Store target speeds but don't change actual speeds immediately
    _targetLeftSpeed = constrain(leftTarget, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    _targetRightSpeed = constrain(rightTarget, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    _shouldRampUp = shouldRampUp;
}

void MotorDriver::update() {
    bool speedsChanged = false;

    if (_shouldRampUp) {
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

    // if (StraightLineDrive::getInstance().isEnabled()) {
    //     StraightLineDrive::getInstance().update(leftAdjusted, rightAdjusted);
    // }

    // Only update motor controls if speeds have changed
    // TODO 8/18/25: Bring this back after SLD works
    // if (speedsChanged || StraightLineDrive::getInstance().isEnabled()) {
    if (speedsChanged) {
        // Apply the current speeds
        if (leftAdjusted == 0) {
            brake_left_motor();
        } else if (leftAdjusted > 0) {
            left_motor_forward(leftAdjusted);
        } else {
            left_motor_backward(-leftAdjusted);
        }

        if (rightAdjusted == 0) {
            brake_right_motor();
        } else if (rightAdjusted > 0) {
            right_motor_forward(rightAdjusted);
        } else {
            right_motor_backward(-rightAdjusted);
        }
    }
}

void MotorDriver::force_reset_motors() {
    brake_if_moving();
    // if (_leftMotorBraking) {
    //     release_left_brake();
    // }
    // if (_rightMotorBraking) {
    //     release_right_brake();
    // }
    // left_motor_stop();
    // right_motor_stop();

    // Reset speed targets
    _targetLeftSpeed = 0;
    _targetRightSpeed = 0;
    _currentLeftSpeed = 0;
    _currentRightSpeed = 0;
}

void MotorDriver::updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed) {
    // Constrain speeds
    leftSpeed = constrain(leftSpeed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    rightSpeed = constrain(rightSpeed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    
    // If we're not executing a command, start this one immediately
    if (!isExecutingCommand) {
        executeCommand(leftSpeed, rightSpeed);
    } else {
        // Store as next command
        hasNextCommand = true;
        nextLeftSpeed = leftSpeed;
        nextRightSpeed = rightSpeed;
    }
}

void MotorDriver::executeCommand(int16_t leftSpeed, int16_t rightSpeed) {
    // Save command details
    currentLeftSpeed = leftSpeed;
    currentRightSpeed = rightSpeed;

    // Get initial encoder counts directly
    startLeftCount = encoderManager._leftEncoder.getCount();
    startRightCount = encoderManager._rightEncoder.getCount();

    // Start the command timer
    commandStartTime = millis();

    set_motor_speeds(leftSpeed, rightSpeed, true);

    // Enable straight driving correction for forward movement only. 
    // 4/12/25: Removing straight line drive for backward movement. need to bring back eventually
    // if ((leftSpeed > 0 && rightSpeed > 0) && (leftSpeed == rightSpeed)) {
    //     StraightLineDrive::getInstance().enable();
    // } else {
    //     StraightLineDrive::getInstance().disable();
    // }

    isExecutingCommand = true;
}

void MotorDriver::processPendingCommands() {
    DemoManager::getInstance().update();

    // If a demo is running, don't process motor commands
    if (DemoManager::getInstance().isAnyDemoActive()) return;

    if (!isExecutingCommand) {
        // If we have a next command, execute it
        if (hasNextCommand) {
            executeCommand(nextLeftSpeed, nextRightSpeed);
            hasNextCommand = false;
        }
        return;
    }

    bool isMovementCommand = (currentLeftSpeed != 0 || currentRightSpeed != 0);

    if (!isMovementCommand) {
        isExecutingCommand = false;
        
        if (hasNextCommand) {
            executeCommand(nextLeftSpeed, nextRightSpeed);
            hasNextCommand = false;
        }
        return;
    }

    // Get current encoder counts
    int64_t currentLeftCount = encoderManager._leftEncoder.getCount();
    int64_t currentRightCount = encoderManager._rightEncoder.getCount();
    
    // Calculate absolute change in encoder counts
    int64_t leftDelta = abs(currentLeftCount - startLeftCount);
    int64_t rightDelta = abs(currentRightCount - startRightCount);
    
    // Check for command completion conditions:
    bool encoderThresholdMet = (leftDelta >= MIN_ENCODER_PULSES || rightDelta >= MIN_ENCODER_PULSES);
    bool commandTimedOut = (millis() - commandStartTime) >= COMMAND_TIMEOUT_MS;
    
    if (encoderThresholdMet || commandTimedOut) {
        if (commandTimedOut) {
            SerialQueueManager::getInstance().queueMessage("Command timed out after 1 second - possible motor stall");
        } else {
            // SerialQueueManager::getInstance().queueMessage("Command completed with pulses - Left: %lld, Right: %lld\n", 
            //             leftDelta, rightDelta);
        }
        
        isExecutingCommand = false;
    }
    if (hasNextCommand) {
        executeCommand(nextLeftSpeed, nextRightSpeed);
        hasNextCommand = false;
    }
}

void MotorDriver::resetCommandState() {
    isExecutingCommand = false;
    hasNextCommand = false;
    currentLeftSpeed = 0;
    currentRightSpeed = 0;
    nextLeftSpeed = 0;
    nextRightSpeed = 0;
    startLeftCount = 0;
    startRightCount = 0;
    commandStartTime = 0;
}
