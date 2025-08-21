#include "message_processor.h"

void MessageProcessor::handleMotorControl(const uint8_t* data) {
    // Extract 16-bit signed integers (little-endian)
    int16_t leftSpeed = static_cast<int16_t>(data[1] | (data[2] << 8));
    int16_t rightSpeed = static_cast<int16_t>(data[3] | (data[4] << 8));
    
    motorDriver.updateMotorSpeeds(leftSpeed, rightSpeed);
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

void MessageProcessor::handleGetSavedWiFiNetworks() {
    // Get saved networks from WiFiManager
    std::vector<WiFiCredentials> savedNetworks = WiFiManager::getInstance().getSavedNetworksForResponse();
    
    // Send response via SerialManager
    SerialManager::getInstance().sendSavedNetworksResponse(savedNetworks);
}

void MessageProcessor::handleSoftScanWiFiNetworks() {
    // Check if we have recent scan results (within 1 minute)
    WiFiManager& wifiManager = WiFiManager::getInstance();
    if (wifiManager.hasAvailableNetworks()) {
        SerialManager::getInstance().sendScanResultsResponse(wifiManager.getAvailableNetworks());
        return;
    }
    unsigned long now = millis();
    if (now - wifiManager.getLastScanCompleteTime() < 60000) return;
    // Start async scan instead of blocking scan
    bool success = wifiManager.startAsyncScan();
    
    if (success) return; // Note: Results will be sent asynchronously when scan completes
    SerialQueueManager::getInstance().queueMessage("Failed to start WiFi scan");
    // Send empty scan results to indicate failure
    std::vector<WiFiNetworkInfo> emptyNetworks;
    SerialManager::getInstance().sendScanResultsResponse(emptyNetworks);
}

void MessageProcessor::handleHardScanWiFiNetworks() {
    bool success = WiFiManager::getInstance().startAsyncScan();
    if (success) return;
    SerialQueueManager::getInstance().queueMessage("Failed to start hard WiFi scan");
    std::vector<WiFiNetworkInfo> emptyNetworks;
    SerialManager::getInstance().sendScanResultsResponse(emptyNetworks);
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
                Speaker::getInstance().playFile(soundType);
            }
            break;
        }
        case DataMessageType::SPEAKER_MUTE: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid speaker mute message length");
            } else {
                SpeakerStatus status = static_cast<SpeakerStatus>(data[1]);
                Speaker::getInstance().setMuted(status == SpeakerStatus::MUTED);
            }
            break;
        }
        case DataMessageType::BALANCE_CONTROL: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid balance control message length");
            } else {
                BalanceStatus status = static_cast<BalanceStatus>(data[1]);
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
            if (length != 19) {
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
                BalanceController::getInstance().updateBalancePids(newBalancePids);
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
                handleObstacleAvoidanceCommand(status);
            }
            break;
        }
        case DataMessageType::SERIAL_HANDSHAKE: {
            if (!Buttons::getInstance().isEitherButtonPressed()) {
                // When waking up from deep sleep mode, and connected to USB, we don't want the flash of blue
                rgbLed.set_led_blue();
            }
            SerialManager::getInstance().isConnected = true;
            SerialManager::getInstance().lastActivityTime = millis();
            SerialManager::getInstance().sendHandshakeConfirmation();

            // Send initial battery data on handshake
            const BatteryState& batteryState = BatteryMonitor::getInstance().getBatteryState();
            if (batteryState.isInitialized) {
                SerialManager::getInstance().sendBatteryMonitorData();
                BatteryMonitor::getInstance().lastBatteryLogTime = millis();
            }
            break;
        }
        case DataMessageType::SERIAL_KEEPALIVE: {
            SerialManager::getInstance().lastActivityTime = millis();
            break;
        }
        case DataMessageType::SERIAL_END: {
            rgbLed.turn_all_leds_off();
            SerialManager::getInstance().isConnected = false;
            SensorDataBuffer::getInstance().stopPollingAllSensors();
            break;
        }
        case DataMessageType::UPDATE_HEADLIGHTS: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid update headlights message length");
            } else {
                HeadlightStatus status = static_cast<HeadlightStatus>(data[1]);
                if (status == HeadlightStatus::ON) {
                    rgbLed.turn_headlights_on();
                } else {
                    rgbLed.turn_headlights_off();
                }
            }
            break;
        }
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

        case DataMessageType::SOFT_SCAN_WIFI_NETWORKS: {
            if (length != 1) {
                SerialQueueManager::getInstance().queueMessage("Invalid soft scan wifi networks message length");
            } else {
                handleSoftScanWiFiNetworks();
            }
            break;
        }
        case DataMessageType::HARD_SCAN_WIFI_NETWORKS: {
            if (length != 1) {
                SerialQueueManager::getInstance().queueMessage("Invalid hard scan wifi networks message length");
            } else {
                handleHardScanWiFiNetworks();
            }
            break;
        }
        case DataMessageType::UPDATE_HORN_SOUND: {
            if (length != 2) {
                SerialQueueManager::getInstance().queueMessage("Invalid update horn message length");
            } else {
                HornSoundStatus status = static_cast<HornSoundStatus>(data[1]);
                if (status == HornSoundStatus::ON) {
                    // TODO: Fix this
                    rgbLed.turn_headlights_on();
                } else {
                    rgbLed.turn_headlights_off();
                }
            }
            break;
        }
        case DataMessageType::SPEAKER_VOLUME: {
            if (length != 5) { // 1 byte for message type + 4 bytes for float32
                SerialQueueManager::getInstance().queueMessage("Invalid speaker volume message length");
            } else {
                // Extract the 4-byte float32 value (little-endian)
                float volume;
                memcpy(&volume, &data[1], sizeof(float));
                Speaker::getInstance().setVolume(volume);
            }
            break;
        }
        case DataMessageType::STOP_SOUND: {
            // 7/21/25 TODO: Implement this
            break;
        }
        case DataMessageType::REQUEST_BATTERY_MONITOR_DATA: { // This comes from the server when the user reloads the page (user requests battery data for each pip)
            if (length != 1) {
                SerialQueueManager::getInstance().queueMessage("Invalid request battery monitor data message length");
            } else {
                BatteryMonitor::getInstance().sendBatteryMonitorDataOverWebSocket();
            }
        }
        case DataMessageType::UPDATE_DISPLAY: {
            if (length != 1025) { // 1 byte for type + 1024 bytes for buffer
                SerialQueueManager::getInstance().queueMessage("Invalid display buffer message length");
            } else {
                DisplayScreen::getInstance().showCustomBuffer(&data[1]);
            }
            break;
        }
        default:
            break;
    }
}
