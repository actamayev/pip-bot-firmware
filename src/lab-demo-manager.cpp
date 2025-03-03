#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/motor_driver.h"
#include "./include/lab_demo_manager.h"
#include "./include/encoder_manager.h"

LabDemoManager::LabDemoManager() 
    : isExecutingCommand(false), 
      hasNextCommand(false),
      currentLeftSpeed(0),
      currentRightSpeed(0),
      nextLeftSpeed(0),
      nextRightSpeed(0),
      startLeftCount(0),
      startRightCount(0) {
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
    
    // Get initial encoder counts directly
    startLeftCount = encoderManager._leftEncoder.getCount();
    startRightCount = encoderManager._rightEncoder.getCount();
    
    Serial.printf("Motors updated - Left: %d, Right: %d\n", leftSpeed, rightSpeed);

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
}

void LabDemoManager::processPendingCommands() {
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
