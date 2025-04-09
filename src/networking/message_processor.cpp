#include "./message_processor.h"

MessageProcessor::MessageProcessor() 
    : isExecutingCommand(false), 
      hasNextCommand(false),
      currentLeftSpeed(0),
      currentRightSpeed(0),
      nextLeftSpeed(0),
      nextRightSpeed(0),
      startLeftCount(0),
      startRightCount(0),
      commandStartTime(0) {
}

void MessageProcessor::handleMotorControl(const uint8_t* data) {
    // Extract 16-bit signed integers (little-endian)
    int16_t leftSpeed = static_cast<int16_t>(data[1] | (data[2] << 8));
    int16_t rightSpeed = static_cast<int16_t>(data[3] | (data[4] << 8));
    
    updateMotorSpeeds(leftSpeed, rightSpeed);
}

void MessageProcessor::handleSoundCommand(SoundType soundType) {
    // Play the requested tune
    switch(soundType) {
        case SoundType::ALERT:
            Serial.println("Playing Alert sound");
            // Call your alert sound function
            // speaker.alert();
            break;

        case SoundType::BEEP:
            Serial.println("Playing Beep sound");
            // Call your beep sound function
            // speaker.beep();
            break;

        case SoundType::CHIME:
            Serial.println("Playing Chime sound");
            speaker.chime();
            break;

        default:
            Serial.printf("Unknown tune type: %d\n", static_cast<int>(soundType));
            break;
    }
}

void MessageProcessor::handleSpeakerMute(SpeakerStatus status) {
    if (status == SpeakerStatus::MUTED) {
        Serial.println("Muting speaker");
        speaker.mute();
    } else if (status == SpeakerStatus::UNMUTED) {
        Serial.println("Unmuting speaker");
        speaker.unmute();
    } else {
        Serial.printf("Unknown mute state: %d\n", static_cast<int>(status));
    }
}

void MessageProcessor::updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed) {
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

void MessageProcessor::executeCommand(int16_t leftSpeed, int16_t rightSpeed) {
    // Save command details
    currentLeftSpeed = leftSpeed;
    currentRightSpeed = rightSpeed;

    // Get initial encoder counts directly
    startLeftCount = encoderManager._leftEncoder.getCount();
    startRightCount = encoderManager._rightEncoder.getCount();

    // Start the command timer
    commandStartTime = millis();

    Serial.printf("Motors updated - Left: %d, Right: %d\n", leftSpeed, rightSpeed);

    motorDriver.set_motor_speeds(leftSpeed, rightSpeed);

    // Enable straight driving correction for forward/backward movement
    if ((leftSpeed == 255 && rightSpeed == 255) || (leftSpeed == -255 && rightSpeed == -255)) {
        StraightLineDrive::getInstance().enable();
    } else {
        StraightLineDrive::getInstance().disable();
    }

    isExecutingCommand = true;
}

void MessageProcessor::processPendingCommands() {
    if (BalanceController::getInstance().isEnabled()) {
        BalanceController::getInstance().update();
        return;
    }

    motorDriver.update_motor_speeds(true);
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
    
    // Check for command completion conditions:
    // 1. Either encoder has moved enough
    // 2. Command has timed out (1 second)
    bool encoderThresholdMet = (leftDelta >= MIN_ENCODER_PULSES || rightDelta >= MIN_ENCODER_PULSES);
    bool commandTimedOut = (millis() - commandStartTime) >= COMMAND_TIMEOUT_MS;
    
    if (encoderThresholdMet || commandTimedOut) {
        if (commandTimedOut) {
            Serial.println("Command timed out after 1 second - possible motor stall");
        } else {
            Serial.printf("Command completed with pulses - Left: %lld, Right: %lld\n", 
                        leftDelta, rightDelta);
        }
        
        isExecutingCommand = false;
        
        if (hasNextCommand) {
            executeCommand(nextLeftSpeed, nextRightSpeed);
            hasNextCommand = false;
        }
    }
}

void MessageProcessor::handleBalanceCommand(BalanceStatus status) {
    if (status == BalanceStatus::BALANCED) {
        BalanceController::getInstance().enable();
    } else {
        BalanceController::getInstance().disable();
    }
}

void MessageProcessor::handleLightCommand(LightAnimationStatus lightAnimationStatus) {
    if (lightAnimationStatus == LightAnimationStatus::NO_ANIMATION) {
        ledAnimations.stopAnimation();
    } else if (lightAnimationStatus == LightAnimationStatus::BREATHING) {
        ledAnimations.startBreathing(2000);
    } else if (lightAnimationStatus == LightAnimationStatus::RAINBOW) {
        ledAnimations.startRainbow(2000);
    } else if (lightAnimationStatus == LightAnimationStatus::STROBE) {
        ledAnimations.startStrobing(100);
    } else if (lightAnimationStatus == LightAnimationStatus::TURN_OFF) {
        ledAnimations.turnOff();
    } else if (lightAnimationStatus == LightAnimationStatus::FADE_OUT) {
        ledAnimations.fadeOut();
    }
}

void MessageProcessor::handleChangePidsCommand(NewBalancePids newBalancePids) {
    BalanceController::getInstance().updateBalancePids(newBalancePids);
}

void MessageProcessor::handleNewLightColors(NewLightColors newLightColors) {
    // Cast from float to uint8_t, assuming values are already in 0-255 range
    uint8_t topLeftR = (uint8_t)newLightColors.topLeftRed;
    uint8_t topLeftG = (uint8_t)newLightColors.topLeftGreen;
    uint8_t topLeftB = (uint8_t)newLightColors.topLeftBlue;
    
    uint8_t topRightR = (uint8_t)newLightColors.topRightRed;
    uint8_t topRightG = (uint8_t)newLightColors.topRightGreen;
    uint8_t topRightB = (uint8_t)newLightColors.topRightBlue;
    
    uint8_t middleLeftR = (uint8_t)newLightColors.middleLeftRed;
    uint8_t middleLeftG = (uint8_t)newLightColors.middleLeftGreen;
    uint8_t middleLeftB = (uint8_t)newLightColors.middleLeftBlue;
    
    uint8_t middleRightR = (uint8_t)newLightColors.middleRightRed;
    uint8_t middleRightG = (uint8_t)newLightColors.middleRightGreen;
    uint8_t middleRightB = (uint8_t)newLightColors.middleRightBlue;
    
    uint8_t backLeftR = (uint8_t)newLightColors.backLeftRed;
    uint8_t backLeftG = (uint8_t)newLightColors.backLeftGreen;
    uint8_t backLeftB = (uint8_t)newLightColors.backLeftBlue;
    
    uint8_t backRightR = (uint8_t)newLightColors.backRightRed;
    uint8_t backRightG = (uint8_t)newLightColors.backRightGreen;
    uint8_t backRightB = (uint8_t)newLightColors.backRightBlue;
    
    // Set each LED to its corresponding color
    rgbLed.set_top_left_led(topLeftR, topLeftG, topLeftB);
    rgbLed.set_top_right_led(topRightR, topRightG, topRightB);
    rgbLed.set_middle_left_led(middleLeftR, middleLeftG, middleLeftB);
    rgbLed.set_middle_right_led(middleRightR, middleRightG, middleRightB);
    rgbLed.set_back_left_led(backLeftR, backLeftG, backLeftB);
    rgbLed.set_back_right_led(backRightR, backRightG, backRightB);
}
