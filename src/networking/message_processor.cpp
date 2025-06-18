#include "message_processor.h"

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
            SerialQueueManager::getInstance().queueMessage("Playing Alert sound");
            // Call your alert sound function
            // speaker.alert();
            break;

        case SoundType::BEEP:
            SerialQueueManager::getInstance().queueMessage("Playing Beep sound");
            // Call your beep sound function
            // speaker.beep();
            break;

        case SoundType::CHIME:
            SerialQueueManager::getInstance().queueMessage("Playing Chime sound");
            speaker.chime();
            break;

        default:
            // SerialQueueManager::getInstance().queueMessage("Unknown tune type: %d\n", static_cast<int>(soundType));
            break;
    }
}

void MessageProcessor::handleSpeakerMute(SpeakerStatus status) {
    speaker.setMuted(status == SpeakerStatus::MUTED);
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

    // SerialQueueManager::getInstance().queueMessage("Motors updated - Left: %d, Right: %d\n", leftSpeed, rightSpeed);

    motorDriver.set_motor_speeds(leftSpeed, rightSpeed);

    // Enable straight driving correction for forward movement only. 
    // 4/12/25: Removing straight line drive for backward movement. need to bring back eventually
    // if ((leftSpeed > 0 && rightSpeed > 0) && (leftSpeed == rightSpeed)) {
    //     StraightLineDrive::getInstance().enable();
    // } else {
    //     StraightLineDrive::getInstance().disable();
    // }

    isExecutingCommand = true;
}

void MessageProcessor::processPendingCommands() {
    DemoManager::getInstance().update();

    // If a demo is running, don't process motor commands
    if (DemoManager::getInstance().isAnyDemoActive()) return;

    motorDriver.update_motor_speeds(true);
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
    
    // Optional debugging (only printed once every 500ms)
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 500) {
        // SerialQueueManager::getInstance().queueMessage("Encoder deltas - Left: %lld, Right: %lld (Target: %d)\n", 
        //              leftDelta, rightDelta, MIN_ENCODER_PULSES);
        lastDebugTime = millis();
    }
    
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
        
        if (hasNextCommand) {
            executeCommand(nextLeftSpeed, nextRightSpeed);
            hasNextCommand = false;
        }
    }
}

void MessageProcessor::handleBalanceCommand(BalanceStatus status) {
    if (status == BalanceStatus::BALANCED) {
        DemoManager::getInstance().startDemo(Demo::BALANCE_CONTROLLER);
    } else {
        // If balance controller is currently running, stop it
        if (DemoManager::getInstance().getCurrentDemo() == Demo::BALANCE_CONTROLLER) {
            DemoManager::getInstance().stopCurrentDemo();
        }
    }
}

void MessageProcessor::handleObstacleAvoidanceCommand(ObstacleAvoidanceStatus status) {
    if (status == ObstacleAvoidanceStatus::AVOID) {
        DemoManager::getInstance().startDemo(Demo::OBSTACLE_AVOIDER);
    } else {
        // If obstacle avoider is currently running, stop it
        if (DemoManager::getInstance().getCurrentDemo() == Demo::OBSTACLE_AVOIDER) {
            DemoManager::getInstance().stopCurrentDemo();
        }
    }
}

void MessageProcessor::handleLightCommand(LightAnimationStatus lightAnimationStatus) {
    if (lightAnimationStatus == LightAnimationStatus::NO_ANIMATION) {
        ledAnimations.stopAnimation();
    } else if (lightAnimationStatus == LightAnimationStatus::BREATHING) {
        ledAnimations.startBreathing();
    } else if (lightAnimationStatus == LightAnimationStatus::RAINBOW) {
        ledAnimations.startRainbow();
    } else if (lightAnimationStatus == LightAnimationStatus::STROBE) {
        ledAnimations.startStrobing();
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

    uint8_t rightHeadlightRed = (uint8_t)newLightColors.rightHeadlightRed;
    uint8_t rightHeadlightGreen = (uint8_t)newLightColors.rightHeadlightGreen;
    uint8_t rightHeadlightBlue = (uint8_t)newLightColors.rightHeadlightBlue;

    uint8_t leftHeadlightRed = (uint8_t)newLightColors.leftHeadlightRed;
    uint8_t leftHeadlightGreen = (uint8_t)newLightColors.leftHeadlightGreen;
    uint8_t leftHeadlightBlue = (uint8_t)newLightColors.leftHeadlightBlue;
    
    // Set each LED to its corresponding color
    rgbLed.set_top_left_led(topLeftR, topLeftG, topLeftB);
    rgbLed.set_top_right_led(topRightR, topRightG, topRightB);
    rgbLed.set_middle_left_led(middleLeftR, middleLeftG, middleLeftB);
    rgbLed.set_middle_right_led(middleRightR, middleRightG, middleRightB);
    rgbLed.set_back_left_led(backLeftR, backLeftG, backLeftB);
    rgbLed.set_back_right_led(backRightR, backRightG, backRightB);
    rgbLed.set_left_headlight(leftHeadlightRed, leftHeadlightGreen, leftHeadlightBlue);
    rgbLed.set_right_headlight(rightHeadlightRed, rightHeadlightGreen, rightHeadlightBlue);
}

void MessageProcessor::handleGetSavedWiFiNetworks() {
    SerialQueueManager::getInstance().queueMessage("Retrieving saved WiFi networks...");
    
    // Get saved networks from WiFiManager
    std::vector<WiFiCredentials> savedNetworks = WiFiManager::getInstance().getSavedNetworksForResponse();
    
    // Send response via SerialManager
    SerialManager::getInstance().sendSavedNetworksResponse(savedNetworks);
}

void MessageProcessor::handleScanWiFiNetworks() {
    SerialQueueManager::getInstance().queueMessage("Starting async WiFi network scan...");
    
    // Start async scan instead of blocking scan
    bool success = WiFiManager::getInstance().startAsyncScan();
    
    if (!success) {
        SerialQueueManager::getInstance().queueMessage("Failed to start WiFi scan");
        // Send empty scan results to indicate failure
        std::vector<WiFiNetworkInfo> emptyNetworks;
        SerialManager::getInstance().sendScanResultsResponse(emptyNetworks);
    }
    // Note: Results will be sent asynchronously when scan completes
}

void MessageProcessor::processBinaryMessage(const uint8_t* data, uint16_t length) {
    TimeoutManager::getInstance().resetActivity();
    if (length < 1) {
        SerialQueueManager::getInstance().queueMessage("Binary message too short");
        return;
    }
    // Extract the message type from the first byte
    DataMessageType messageType = static_cast<DataMessageType>(data[0]);

    switch (messageType) {
        case DataMessageType::UPDATE_AVAILABLE: {
            if (length != 3) {
                SerialQueueManager::getInstance().queueMessage("Invalid update available message length");
            } else {
                // Extract the firmware version from bytes 1-2
                uint16_t newVersion = data[1] | (data[2] << 8); // Little-endian conversion

                // SerialQueueManager::getInstance().queueMessage("New firmware version available: %d\n", newVersion);
                FirmwareVersionTracker::getInstance().retrieveLatestFirmwareFromServer(newVersion);
            }
            break;
        }
        case DataMessageType::MOTOR_CONTROL: {
            if (length != 5) {
                SerialQueueManager::getInstance().queueMessage("Invalid motor control message length");
            } else {
                handleMotorControl(data);
            }
            break;
        }
        case DataMessageType::SOUND_COMMAND: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid sound command message length");
            } else {
                SoundType soundType = static_cast<SoundType>(data[1]);
                handleSoundCommand(soundType);
            }
            break;
        }
        case DataMessageType::SPEAKER_MUTE: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid speaker mute message length");
            } else {
                SpeakerStatus status = static_cast<SpeakerStatus>(data[1]);
                handleSpeakerMute(status);
            }
            break;
        }
        case DataMessageType::BALANCE_CONTROL: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid balance control message length");
            } else {
                BalanceStatus status = static_cast<BalanceStatus>(data[1]);
                char logMessage[64];
                snprintf(logMessage, sizeof(logMessage), "Balance Status: %s", 
                        status == BalanceStatus::BALANCED ? "BALANCED" : "UNBALANCED");
                SerialQueueManager::getInstance().queueMessage(logMessage);
                handleBalanceCommand(status);
            }
            break;
        }
        case DataMessageType::UPDATE_LIGHT_ANIMATION: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid light animation message length");
            } else {
                LightAnimationStatus lightAnimationStatus = static_cast<LightAnimationStatus>(data[1]);
                handleLightCommand(lightAnimationStatus);
            }
            break;
        }
        case DataMessageType::UPDATE_LED_COLORS: {
            if (length != 25) {
                // SerialQueueManager::getInstance().queueMessage("Invalid update led colors message length%d", length);
            } else {
                NewLightColors newLightColors;
                memcpy(&newLightColors, &data[1], sizeof(NewLightColors));
                handleNewLightColors(newLightColors);
            }
            break;
        }
        case DataMessageType::UPDATE_BALANCE_PIDS: {
            if (length != 41) { // 1 byte for type + 40 bytes for the struct (10 floats × 4 bytes)
                SerialQueueManager::getInstance().queueMessage("Invalid update balance pids message length");
            } else {
                NewBalancePids newBalancePids;
                memcpy(&newBalancePids, &data[1], sizeof(NewBalancePids));
                handleChangePidsCommand(newBalancePids);
            }
            break;
        }
        case DataMessageType::BYTECODE_PROGRAM: {
            // First byte is the message type, the rest is bytecode
            const uint8_t* bytecodeData = data + 1;
            uint16_t bytecodeLength = length - 1;

            // Execute the bytecode
            bool success = BytecodeVM::getInstance().loadProgram(bytecodeData, bytecodeLength);

            // Send response
            if (success) {
                SendDataToServer::getInstance().sendBytecodeMessage("Bytecode successfully loaded");
            } else {
                SendDataToServer::getInstance().sendBytecodeMessage("Error loading bytecode: invalid format!");
            }
            break;
        }
        case DataMessageType::STOP_SANDBOX_CODE: {
            if (length != 1) {
                SerialQueueManager::getInstance().queueMessage("Invalid stop sandbox code message length");
            } else {
                BytecodeVM::getInstance().stopProgram();
            }
            break;
        }
        case DataMessageType::OBSTACLE_AVOIDANCE: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid obstacle avoidance command");
            } else {
                ObstacleAvoidanceStatus status = static_cast<ObstacleAvoidanceStatus>(data[1]);
                char logMessage[64];
                snprintf(logMessage, sizeof(logMessage), "Avoidance Status: %s", 
                        status == ObstacleAvoidanceStatus::AVOID ? "AVOID" : "STOP Avoiding");
                SerialQueueManager::getInstance().queueMessage(logMessage);
                handleObstacleAvoidanceCommand(status);
            }
            break;
        }
        case DataMessageType::SERIAL_HANDSHAKE: {
            SerialQueueManager::getInstance().queueMessage("Handshake received from browser!");  // Use safe print
            rgbLed.set_led_blue();
            SerialManager::getInstance().isConnected = true;
            SerialManager::getInstance().lastActivityTime = millis();
            SerialManager::getInstance().sendHandshakeConfirmation();
            SensorPollingManager::getInstance().startPolling();
            break;
        }
        case DataMessageType::SERIAL_KEEPALIVE: {
            SerialManager::getInstance().lastActivityTime = millis();
            break;
        }
        case DataMessageType::SERIAL_END: {
            rgbLed.turn_led_off();
            SerialManager::getInstance().isConnected = false;
            // Don't stop polling here. Users will frequently connect and disconnect
            break;
        }
        case DataMessageType::UPDATE_HEADLIGHTS: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid update headlights message length");
            } else {
                HeadlightStatus status = static_cast<HeadlightStatus>(data[1]);
                if (status == HeadlightStatus::ON) {
                    rgbLed.set_headlights_on();
                } else {
                    rgbLed.reset_headlights_to_default();
                }
            }
            break;
        }
        case DataMessageType::START_SENSOR_POLLING: {
            SensorPollingManager::getInstance().startPolling();
            break;
        }
        // Add this new case in processBinaryMessage()
        case DataMessageType::WIFI_CREDENTIALS: {
            if (length < 3) { // At least 1 byte for each length field + message type
                SerialQueueManager::getInstance().queueMessage("Invalid WiFi credentials message length");
                break;
            }
            
            uint8_t ssidLength = data[1];
            if (ssidLength == 0 || 2 + ssidLength >= length) {
                SerialQueueManager::getInstance().queueMessage("Invalid SSID length");
                break;
            }
            
            String ssid = "";
            for (uint8_t i = 0; i < ssidLength; i++) {
                ssid += (char)data[2 + i];
            }
            
            uint8_t passwordLength = data[2 + ssidLength];
            if (2 + ssidLength + 1 + passwordLength != length) {
                SerialQueueManager::getInstance().queueMessage("Invalid password length");
                break;
            }
            
            String password = "";
            for (uint8_t i = 0; i < passwordLength; i++) {
                password += (char)data[3 + ssidLength + i];
            }
            
            // NEW: Enter ADD_PIP_MODE and store credentials for testing
            NetworkStateManager::getInstance().setAddPipMode(true);
            WiFiManager::getInstance().startAddPipWiFiTest(ssid, password);
            
            break;
        }

        case DataMessageType::WIFI_CONNECTION_RESULT: {
            // This is sent FROM ESP32, not handled by ESP32
            break;
        }

        case DataMessageType::GET_SAVED_WIFI_NETWORKS: {
            if (length != 1) {
                SerialQueueManager::getInstance().queueMessage("Invalid get saved wifi networks message length");
            } else {
                handleGetSavedWiFiNetworks();
            }
            break;
        }

        case DataMessageType::SCAN_WIFI_NETWORKS: {
            if (length != 1) {
                SerialQueueManager::getInstance().queueMessage("Invalid scan wifi networks message length");
            } else {
                handleScanWiFiNetworks();
            }
            break;
        }
        default:
            // SerialQueueManager::getInstance().queueMessage("Unknown message type: %d\n", static_cast<int>(messageType));
            break;
    }
}

void MessageProcessor::resetCommandState() {
    isExecutingCommand = false;
    hasNextCommand = false;
    currentLeftSpeed = 0;
    currentRightSpeed = 0;
    nextLeftSpeed = 0;
    nextRightSpeed = 0;
    startLeftCount = 0;
    startRightCount = 0;
    commandStartTime = 0;
    
    SerialQueueManager::getInstance().queueMessage("MessageProcessor command state reset");
}
