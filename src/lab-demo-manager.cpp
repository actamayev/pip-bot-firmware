#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/motor_driver.h"
#include "./include/lab_demo_manager.h"
#include "./include/encoder_manager.h"
#include "./include/pid_controller.h"

LabDemoManager::LabDemoManager() 
    : isExecutingCommand(false), 
      hasNextCommand(false),
      currentLeftSpeed(0),
      currentRightSpeed(0),
      targetLeftSpeed(0),
      targetRightSpeed(0),
      nextLeftSpeed(0),
      nextRightSpeed(0),
      startLeftCount(0),
      startRightCount(0),
      pidEnabled(true),
      lastPIDUpdateTime(0),
      adjustedLeftPWM(0),
      adjustedRightPWM(0) {
      
    // Initialize PID controller
    // Parameters: Kp, Ki, Kd, min output, max output
    speedMatchPID = new PIDController(0.5, 0.1, 0.05, -50, 50);
}

void LabDemoManager::handleBinaryMessage(const char* data) {
    if (data[0] != 1) {  // 1 = motor control
        Serial.printf("Unknown message type: %d\n", data[0]);
        return;
    }

    // Extract 16-bit signed integers (little-endian)
    int16_t leftSpeed = (static_cast<uint8_t>(data[2]) << 8) | static_cast<uint8_t>(data[1]);
    int16_t rightSpeed = (static_cast<uint8_t>(data[4]) << 8) | static_cast<uint8_t>(data[3]);
    
    updateMotorSpeeds(leftSpeed, rightSpeed);
}

void LabDemoManager::updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed) {
    // Constrain speeds
    leftSpeed = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);
    
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

void LabDemoManager::executeCommand(int16_t leftSpeed, int16_t rightSpeed) {
    // Save command details
    currentLeftSpeed = leftSpeed;
    currentRightSpeed = rightSpeed;
    targetLeftSpeed = leftSpeed;
    targetRightSpeed = rightSpeed;

    // Initialize adjusted PWM values to match input speeds
    adjustedLeftPWM = leftSpeed;
    adjustedRightPWM = rightSpeed;

    // Reset PID controller when starting a new command
    if (pidEnabled) {
        speedMatchPID->reset();
        
        // Set the setpoint to 0 (we want the difference between wheels to be 0)
        speedMatchPID->setSetpoint(0);
    }

    // Get initial encoder counts directly
    startLeftCount = encoderManager._leftEncoder.getCount();
    startRightCount = encoderManager._rightEncoder.getCount();

    Serial.printf("Motors updated - Left: %d, Right: %d\n", leftSpeed, rightSpeed);

    // Apply motor controls with initial speeds
    if (leftSpeed == 0) {
        motorDriver.left_motor_stop();
    } else if (leftSpeed > 0) {
        motorDriver.left_motor_forward(leftSpeed);
    } else {
        motorDriver.left_motor_backward(-leftSpeed);
    }

    if (rightSpeed == 0) {
        motorDriver.right_motor_stop();
    } else if (rightSpeed > 0) {
        motorDriver.right_motor_forward(rightSpeed);
    } else {
        motorDriver.right_motor_backward(-rightSpeed);
    }

    // Update LED based on motor direction
    if (leftSpeed == 0 && rightSpeed == 0) {
        rgbLed.turn_led_off();
    } else if (leftSpeed > 0 && rightSpeed > 0) {
        rgbLed.set_led_blue();
    } else if (leftSpeed < 0 && rightSpeed < 0) {
        rgbLed.set_led_red();
    } else {
        rgbLed.set_led_green();
    }
    
    isExecutingCommand = true;
    lastPIDUpdateTime = millis();
}

void LabDemoManager::processPendingCommands() {
    // Apply PID adjustments if enabled
    if (pidEnabled && isExecutingCommand) {
        Serial.println("here");
        adjustMotorSpeeds();
    }
    
    if (!isExecutingCommand) {
        // If we have a next command, execute it
        if (hasNextCommand) {
            executeCommand(nextLeftSpeed, nextRightSpeed);
            hasNextCommand = false;
        }
        return;
    }

    // Is this a movement command (non-zero speed)?
    bool isMovementCommand = (currentLeftSpeed != 0 || currentRightSpeed != 0);

    // For stop commands, mark as completed immediately and execute next command
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
    
    // Optional debugging (only printed once every 500ms)
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 500) {
        Serial.printf("Encoder deltas - Left: %lld, Right: %lld (Target: %d)\n", 
                     leftDelta, rightDelta, MIN_ENCODER_PULSES);
        lastDebugTime = millis();
    }
    
    // Command is complete if either encoder has moved enough
    if (leftDelta >= MIN_ENCODER_PULSES || rightDelta >= MIN_ENCODER_PULSES) {
        Serial.printf("Command completed with pulses - Left: %lld, Right: %lld\n", 
                     leftDelta, rightDelta);
        
        isExecutingCommand = false;
        
        if (hasNextCommand) {
            executeCommand(nextLeftSpeed, nextRightSpeed);
            hasNextCommand = false;
        }
    }
}

void LabDemoManager::adjustMotorSpeeds() {
    Serial.printf("pidEnabled", pidEnabled);
    Serial.printf("isExecutingCommand", isExecutingCommand);

    if (!pidEnabled || !isExecutingCommand) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastPIDUpdateTime < PID_UPDATE_INTERVAL) {
        return; // Not time to update yet
    }
    
    lastPIDUpdateTime = currentTime;
    
    // Get current wheel speeds from encoders
    WheelRPMs rpms = encoderManager.getBothWheelRPMs();
    Serial.printf("rpms.leftWheelRPM %d", rpms.leftWheelRPM);
    Serial.printf("rpms.rightWheelRPM %d", rpms.rightWheelRPM);

    // Skip PID if both motors are stopped
    if (targetLeftSpeed == 0 && targetRightSpeed == 0) {
        return;
    }
    
    // Skip PID if one motor is intentionally stopped
    if (targetLeftSpeed == 0 || targetRightSpeed == 0) {
        return;
    }
    
    // Skip PID if going in opposite directions (turning in place)
    if ((targetLeftSpeed > 0 && targetRightSpeed < 0) || 
        (targetLeftSpeed < 0 && targetRightSpeed > 0)) {
        return;
    }
    
    // Calculate speed difference (error) - we want them to be equal
    float speedDifference = rpms.rightWheelRPM - rpms.leftWheelRPM;
    
    // When going backward, invert the difference since negative RPM values
    if (targetLeftSpeed < 0 && targetRightSpeed < 0) {
        speedDifference = -speedDifference;
    }
    
    // Compute PID correction
    float correction = speedMatchPID->compute(speedDifference);
    
    // Apply the correction to adjusted PWM values
    adjustedLeftPWM = targetLeftSpeed + (int16_t)correction;
    adjustedRightPWM = targetRightSpeed - (int16_t)correction;
    
    // Constrain to valid PWM range
    adjustedLeftPWM = constrain(adjustedLeftPWM, -255, 255);
    adjustedRightPWM = constrain(adjustedRightPWM, -255, 255);
    
    // Debug output
    Serial.printf("PID: L_RPM=%.2f, R_RPM=%.2f, Diff=%.2f, Corr=%.2f, L_PWM=%d, R_PWM=%d\n",
                 rpms.leftWheelRPM, rpms.rightWheelRPM, speedDifference, 
                 correction, adjustedLeftPWM, adjustedRightPWM);
    
    // Apply the adjusted speeds
    if (adjustedLeftPWM == 0) {
        motorDriver.left_motor_stop();
    } else if (adjustedLeftPWM > 0) {
        motorDriver.left_motor_forward(adjustedLeftPWM);
    } else {
        motorDriver.left_motor_backward(-adjustedLeftPWM);
    }

    if (adjustedRightPWM == 0) {
        motorDriver.right_motor_stop();
    } else if (adjustedRightPWM > 0) {
        motorDriver.right_motor_forward(adjustedRightPWM);
    } else {
        motorDriver.right_motor_backward(-adjustedRightPWM);
    }
}
