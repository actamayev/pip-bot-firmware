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

// TODO 8/19/25: Need to figure out how to take the motors out of braking after 1 second of being in brake without spinning them backwards when releasing from brake
void MotorDriver::brake_if_moving() {
    // TODO: Re-implement logic when we have encoders:
    brake_both_motors();
    return;
    // Get current wheel speeds from sensor data buffer
    // WheelRPMs rpms = SensorDataBuffer::getInstance().getLatestWheelRPMs();

    // // Check if left motor is moving
    // if (abs(rpms.leftWheelRPM) > MOTOR_STOPPED_THRESHOLD) {
    //     // Left motor is moving, apply brake
    //     brake_left_motor();
    // }
    // // // Check if right motor is moving
    // if (abs(rpms.rightWheelRPM) > MOTOR_STOPPED_THRESHOLD) {
    //     // Right motor is moving, apply brake
    //     brake_right_motor();
    // }
}

// Use this to set speeds directly (without needing ramp up, or waiting for next command).
// The next command is used to make sure the current command is fully complete (ie for micro-turns in the garage)
void MotorDriver::set_motor_speeds(int16_t leftTarget, int16_t rightTarget, bool shouldRampUp) {
    // Store target speeds but don't change actual speeds immediately
    _targetLeftPwm = constrain(leftTarget, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    _targetRightPwm = constrain(rightTarget, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    _shouldRampUp = shouldRampUp;
}

void MotorDriver::update() {
    bool speedsChanged = false;

    if (_shouldRampUp) {
        // Gradually ramp left motor speed toward target
        if (_actualLeftPwm < _targetLeftPwm) {
            _actualLeftPwm = min(static_cast<int16_t>(_actualLeftPwm + SPEED_RAMP_STEP), _targetLeftPwm);
            speedsChanged = true;
        } else if (_actualLeftPwm > _targetLeftPwm) {
            _actualLeftPwm = max(static_cast<int16_t>(_actualLeftPwm - SPEED_RAMP_STEP), _targetLeftPwm);
            speedsChanged = true;
        }

        // Gradually ramp right motor speed toward target
        if (_actualRightPwm < _targetRightPwm) {
            _actualRightPwm = min(static_cast<int16_t>(_actualRightPwm + SPEED_RAMP_STEP), _targetRightPwm);
            speedsChanged = true;
        } else if (_actualRightPwm > _targetRightPwm) {
            _actualRightPwm = max(static_cast<int16_t>(_actualRightPwm - SPEED_RAMP_STEP), _targetRightPwm);
            speedsChanged = true;
        }
    } else {
        // Skip ramping and set speeds immediately
        if (_actualLeftPwm != _targetLeftPwm) {
            _actualLeftPwm = _targetLeftPwm;
            speedsChanged = true;
        }
        
        if (_actualRightPwm != _targetRightPwm) {
            _actualRightPwm = _targetRightPwm;
            speedsChanged = true;
        }
    }

    int16_t leftAdjusted = _actualLeftPwm;
    int16_t rightAdjusted = _actualRightPwm;

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

void MotorDriver::set_motor_speeds_immediate(int16_t leftTarget, int16_t rightTarget) {
    // Constrain speeds
    int16_t leftConstrained = constrain(leftTarget, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    int16_t rightConstrained = constrain(rightTarget, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    
    // Apply immediately without any ramping or state management
    if (leftConstrained == 0) {
        brake_left_motor();
    } else if (leftConstrained > 0) {
        left_motor_forward(leftConstrained);
    } else {
        left_motor_backward(-leftConstrained);
    }

    if (rightConstrained == 0) {
        brake_right_motor();
    } else if (rightConstrained > 0) {
        right_motor_forward(rightConstrained);
    } else {
        right_motor_backward(-rightConstrained);
    }
}

void MotorDriver::updateMotorPwm(int16_t leftPwm, int16_t rightPwm) {
    // Constrain speeds
    leftPwm = constrain(leftPwm, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    rightPwm = constrain(rightPwm, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    
    // If we're not executing a command, start this one immediately
    if (!isExecutingCommand) {
        executeCommand(leftPwm, rightPwm);
    } else {
        // Store as next command
        hasNextCommand = true;
        nextLeftPwm = leftPwm;
        nextRightPwm = rightPwm;
    }
}

void MotorDriver::executeCommand(int16_t leftPwm, int16_t rightPwm) {
    // Save command details
    _commandLeftPwm = leftPwm;
    _commandRightPwm = rightPwm;

    // Get initial encoder counts from sensor data buffer
    auto startingCounts = SensorDataBuffer::getInstance().getLatestEncoderCounts();
    startLeftCount = startingCounts.first;
    startRightCount = startingCounts.second;

    // Start the command timer
    commandStartTime = millis();

    set_motor_speeds(leftPwm, rightPwm, true); // ramp is default true for commands that are executed in series (ie driving in the garage)

    // Enable straight driving correction for forward movement only. 
    // 4/12/25: Removing straight line drive for backward movement. need to bring back eventually
    // if ((leftSpeed > 0 && rightSpeed > 0) && (leftSpeed == rightSpeed)) {
    //     StraightLineDrive::getInstance().enable();
    // } else {
    //     StraightLineDrive::getInstance().disable();
    // }

    isExecutingCommand = true;
    hasNextCommand = false;
}

void MotorDriver::processPendingCommands() {
    // If a demo is running, don't process motor commands
    if (DemoManager::getInstance().isAnyDemoActive()) return;

    if (!isExecutingCommand) {
        // If we have a next command, execute it
        if (hasNextCommand) {
            executeCommand(nextLeftPwm, nextRightPwm);
        }
        return;
    }

    bool isMovementCommand = (_commandLeftPwm != 0 || _commandRightPwm != 0);

    if (!isMovementCommand) {
        isExecutingCommand = false;
        
        if (hasNextCommand) {
            executeCommand(nextLeftPwm, nextRightPwm);
        }
        return;
    }

    // Get current encoder counts from sensor data buffer
    auto currentCounts = SensorDataBuffer::getInstance().getLatestEncoderCounts();
    int64_t currentLeftCount = currentCounts.first;
    int64_t currentRightCount = currentCounts.second;
    
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
        executeCommand(nextLeftPwm, nextRightPwm);
    }
}

void MotorDriver::resetCommandState(bool absoluteBrake) {
    // Clear all command state first to prevent any queued commands from executing
    _targetLeftPwm = 0;
    _targetRightPwm = 0;
    _actualLeftPwm = 0;
    _actualRightPwm = 0;
    isExecutingCommand = false;
    _commandLeftPwm = 0;
    _commandRightPwm = 0;
    startLeftCount = 0;
    startRightCount = 0;
    commandStartTime = 0;
    hasNextCommand = false;
    nextLeftPwm = 0;
    nextRightPwm = 0;
    
    // Apply brakes after clearing state
    if (absoluteBrake) {
        motorDriver.brake_both_motors();
    } else {
        motorDriver.brake_if_moving();
    }
}
