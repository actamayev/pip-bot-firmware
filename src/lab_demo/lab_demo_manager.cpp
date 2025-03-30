#include "../utils/config.h"
#include "./lab_demo_manager.h"
#include "../actuators/rgb_led.h"
#include "../actuators/motor_driver.h"
#include "../sensors/encoder_manager.h"
// #include "../actuators/speaker.h"

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

// Add this to lab_demo_manager.cpp
void LabDemoManager::handleSoundMessage(const char* data) {
    uint8_t tuneType = static_cast<uint8_t>(data[1]);

    // Play the requested tune
    switch(tuneType) {
        case TUNE_ALERT:
            Serial.println("Playing Alert sound");
            // Call your alert sound function
            // speaker.alert();
            break;

        case TUNE_BEEP:
            Serial.println("Playing Beep sound");
            // Call your beep sound function
            // speaker.beep();
            break;

        case TUNE_CHIME:
            Serial.println("Playing Chime sound");
            // speaker.chime();
            break;

        default:
            Serial.printf("Unknown tune type: %d\n", tuneType);
            break;
    }
}

void LabDemoManager::handleSpeakerMuteMessage(const char* data) {
    // Get mute state (0 = mute, 1 = unmute)
    uint8_t muteState = static_cast<uint8_t>(data[1]);
    
    if (muteState == 0) {
        Serial.println("Muting speaker");
        // speaker.mute();
    } else if (muteState == 1) {
        Serial.println("Unmuting speaker");
        // speaker.unmute();
    } else {
        Serial.printf("Unknown mute state: %d\n", muteState);
    }
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

    // Use ramping to prevent back-EMF spikes
    motorDriver.set_motor_speeds(leftSpeed, rightSpeed);

    // Update LED based on motor direction (unchanged)
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
    motorDriver.update_motor_speeds();
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

void LabDemoManager::handleChimeCommand() {
    // Play the chime sound
    // speaker.chime();

    Serial.println("Chime command executed");
}
