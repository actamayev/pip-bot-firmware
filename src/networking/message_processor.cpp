#include "message_processor.h"

void MessageProcessor::handle_motor_control(const uint8_t* data) {
    // Extract 16-bit signed integers (little-endian)
    int16_t leftSpeed = static_cast<int16_t>(data[1] | (data[2] << 8));
    int16_t rightSpeed = static_cast<int16_t>(data[3] | (data[4] << 8));

    motorDriver.update_motor_pwm(leftSpeed, rightSpeed);
}

void MessageProcessor::handle_balance_command(BalanceStatus status) {
    if (status == BalanceStatus::BALANCED) {
        DemoManager::get_instance().start_demo(demo::DemoType::BALANCE_CONTROLLER);
        return;
    }
    // If balance controller is currently running, stop it
    if (DemoManager::get_instance().get_current_demo() == demo::DemoType::BALANCE_CONTROLLER) {
        DemoManager::get_instance().stop_current_demo();
    }
}

void MessageProcessor::handle_obstacle_avoidance_command(ObstacleAvoidanceStatus status) {
    if (status == ObstacleAvoidanceStatus::AVOID) {
        DemoManager::get_instance().start_demo(demo::DemoType::OBSTACLE_AVOIDER);
        return;
    }
    // If obstacle avoider is currently running, stop it
    if (DemoManager::get_instance().get_current_demo() == demo::DemoType::OBSTACLE_AVOIDER) {
        DemoManager::get_instance().stop_current_demo();
    }
}

void MessageProcessor::handle_light_command(LightAnimationStatus lightAnimationStatus) {
    if (lightAnimationStatus == LightAnimationStatus::NO_ANIMATION) {
        ledAnimations.stop_animation();
    } else if (lightAnimationStatus == LightAnimationStatus::BREATHING) {
        ledAnimations.start_breathing();
    } else if (lightAnimationStatus == LightAnimationStatus::RAINBOW) {
        ledAnimations.start_rainbow();
    } else if (lightAnimationStatus == LightAnimationStatus::STROBE) {
        ledAnimations.start_strobing();
    } else if (lightAnimationStatus == LightAnimationStatus::TURN_OFF) {
        ledAnimations.turn_off();
    } else if (lightAnimationStatus == LightAnimationStatus::FADE_OUT) {
        ledAnimations.fade_out();
    }
}

void MessageProcessor::handle_new_light_colors(NewLightColors newLightColors) {
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

void MessageProcessor::handle_get_saved_wifi_networks() {
    // Get saved networks from WiFiManager
    std::vector<WiFiCredentials> savedNetworks = WiFiManager::get_instance().get_saved_networks_for_response();

    // Send response via SerialManager
    SerialManager::get_instance().send_saved_networks_response(savedNetworks);
}

void MessageProcessor::handle_soft_scan_wifi_networks() {
    // Check if we have recent scan results (within 1 minute)
    WiFiManager& wifiManager = WiFiManager::get_instance();
    if (wifiManager.has_available_networks()) {
        SerialManager::get_instance().send_scan_results_response(wifiManager.get_available_networks());
        return;
    }
    unsigned long now = millis();
    if (now - wifiManager.get_last_scan_complete_time() < 60000) return;
    // Start async scan instead of blocking scan
    bool success = wifiManager.start_async_scan();

    if (success) return; // Note: Results will be sent asynchronously when scan completes
    SerialQueueManager::get_instance().queue_message("Failed to start WiFi scan");
    // Send empty scan results to indicate failure
    std::vector<WiFiNetworkInfo> emptyNetworks;
    SerialManager::get_instance().send_scan_results_response(emptyNetworks);
}

void MessageProcessor::handle_hard_scan_wifi_networks() {
    bool success = WiFiManager::get_instance().start_async_scan();
    if (success) return;
    SerialQueueManager::get_instance().queue_message("Failed to start hard WiFi scan");
    std::vector<WiFiNetworkInfo> emptyNetworks;
    SerialManager::get_instance().send_scan_results_response(emptyNetworks);
}

void MessageProcessor::process_binary_message(const uint8_t* data, uint16_t length) {
    TimeoutManager::get_instance().reset_activity();
    if (length < 1) {
        SerialQueueManager::get_instance().queue_message("Binary message too short");
        return;
    }
    // Extract the message type from the first byte
    DataMessageType messageType = static_cast<DataMessageType>(data[0]);

    switch (messageType) {
        case DataMessageType::UPDATE_AVAILABLE: {
            if (length != 3) {
                SerialQueueManager::get_instance().queue_message("Invalid update available message length");
            } else {
                // Extract the firmware version from bytes 1-2
                uint16_t newVersion = data[1] | (data[2] << 8); // Little-endian conversion

                // SerialQueueManager::get_instance().queueMessage("New firmware version available: %d\n", newVersion);
                FirmwareVersionTracker::get_instance().retrieve_latest_firmware_from_server(newVersion);
            }
            break;
        }
        case DataMessageType::MOTOR_CONTROL: {
            if (length != 5) {
                SerialQueueManager::get_instance().queue_message("Invalid motor control message length");
            } else {
                handle_motor_control(data);
            }
            break;
        }
        case DataMessageType::TONE_COMMAND: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid sound command message length");
            } else {
                ToneType toneType = static_cast<ToneType>(data[1]);
                Speaker::get_instance().play_tone(toneType);
            }
            break;
        }
        case DataMessageType::SPEAKER_MUTE: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid speaker mute message length");
            } else {
                SpeakerStatus status = static_cast<SpeakerStatus>(data[1]);
                Speaker::get_instance().set_muted(status == SpeakerStatus::MUTED);
            }
            break;
        }
        case DataMessageType::BALANCE_CONTROL: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid balance control message length");
            } else {
                BalanceStatus status = static_cast<BalanceStatus>(data[1]);
                handle_balance_command(status);
            }
            break;
        }
        case DataMessageType::UPDATE_LIGHT_ANIMATION: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid light animation message length");
            } else {
                LightAnimationStatus lightAnimationStatus = static_cast<LightAnimationStatus>(data[1]);
                handle_light_command(lightAnimationStatus);
            }
            break;
        }
        case DataMessageType::UPDATE_LED_COLORS: {
            if (length != 19) {
                // SerialQueueManager::get_instance().queueMessage("Invalid update led colors message length%d", length);
            } else {
                NewLightColors newLightColors;
                memcpy(&newLightColors, &data[1], sizeof(NewLightColors));
                handle_new_light_colors(newLightColors);
            }
            break;
        }
        case DataMessageType::UPDATE_BALANCE_PIDS: {
            if (length != 41) { // 1 byte for type + 40 bytes for the struct (10 floats × 4 bytes)
                SerialQueueManager::get_instance().queue_message("Invalid update balance pids message length");
            } else {
                NewBalancePids newBalancePids;
                memcpy(&newBalancePids, &data[1], sizeof(NewBalancePids));
                BalanceController::get_instance().update_balance_pids(newBalancePids);
            }
            break;
        }
        case DataMessageType::BYTECODE_PROGRAM: {
            // First byte is the message type, the rest is bytecode
            const uint8_t* bytecodeData = data + 1;
            uint16_t bytecodeLength = length - 1;

            // Execute the bytecode
            BytecodeVM::get_instance().loadProgram(bytecodeData, bytecodeLength);
            break;
        }
        case DataMessageType::STOP_SANDBOX_CODE: {
            if (length != 1) {
                SerialQueueManager::get_instance().queue_message("Invalid stop sandbox code message length");
            } else {
                BytecodeVM::get_instance().stopProgram();
            }
            break;
        }

        case DataMessageType::STOP_SENSOR_POLLING: {
            if (length != 1) {
                SerialQueueManager::get_instance().queue_message("Invalid stop sandbox code message length");
            } else {
                SensorDataBuffer::get_instance().stop_polling_all_sensors();
            }
            break;
        }

        case DataMessageType::OBSTACLE_AVOIDANCE: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid obstacle avoidance command");
            } else {
                ObstacleAvoidanceStatus status = static_cast<ObstacleAvoidanceStatus>(data[1]);
                handle_obstacle_avoidance_command(status);
            }
            break;
        }
        case DataMessageType::SERIAL_HANDSHAKE: {
            SerialManager::get_instance().isConnected = true;
            SerialManager::get_instance().lastActivityTime = millis();
            SerialManager::get_instance().send_pip_id_message();

            // Send initial battery data on handshake
            const BatteryState& batteryState = BatteryMonitor::get_instance().get_battery_state();
            if (batteryState.isInitialized) {
                SerialManager::get_instance().send_battery_monitor_data();
                BatteryMonitor::get_instance().lastBatteryLogTime = millis();
            }
            break;
        }
        case DataMessageType::SERIAL_KEEPALIVE: {
            SerialManager::get_instance().lastActivityTime = millis();
            break;
        }
        case DataMessageType::SERIAL_END: {
            rgbLed.turn_all_leds_off();
            SerialManager::get_instance().isConnected = false;
            SensorDataBuffer::get_instance().stop_polling_all_sensors();
            Speaker::get_instance().stop_all_sounds();
            break;
        }
        case DataMessageType::UPDATE_HEADLIGHTS: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid update headlights message length");
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
                SerialQueueManager::get_instance().queue_message("Invalid WiFi credentials message length");
                break;
            }

            uint8_t ssidLength = data[1];
            if (ssidLength == 0 || 2 + ssidLength >= length) {
                SerialQueueManager::get_instance().queue_message("Invalid SSID length");
                break;
            }

            String ssid = "";
            for (uint8_t i = 0; i < ssidLength; i++) {
                ssid += (char)data[2 + i];
            }

            uint8_t passwordLength = data[2 + ssidLength];
            if (2 + ssidLength + 1 + passwordLength != length) {
                SerialQueueManager::get_instance().queue_message("Invalid password length");
                break;
            }

            String password = "";
            for (uint8_t i = 0; i < passwordLength; i++) {
                password += (char)data[3 + ssidLength + i];
            }

            // Test WiFi credentials before storing permanently
            WiFiManager::get_instance().startWiFiCredentialTest(ssid, password);

            break;
        }

        case DataMessageType::WIFI_CONNECTION_RESULT: {
            // This is sent FROM ESP32, not handled by ESP32
            break;
        }

        case DataMessageType::GET_SAVED_WIFI_NETWORKS: {
            if (length != 1) {
                SerialQueueManager::get_instance().queue_message("Invalid get saved wifi networks message length");
            } else {
                handle_get_saved_wifi_networks();
            }
            break;
        }

        case DataMessageType::SOFT_SCAN_WIFI_NETWORKS: {
            if (length != 1) {
                SerialQueueManager::get_instance().queue_message("Invalid soft scan wifi networks message length");
            } else {
                handle_soft_scan_wifi_networks();
            }
            break;
        }
        case DataMessageType::HARD_SCAN_WIFI_NETWORKS: {
            if (length != 1) {
                SerialQueueManager::get_instance().queue_message("Invalid hard scan wifi networks message length");
            } else {
                handle_hard_scan_wifi_networks();
            }
            break;
        }
        case DataMessageType::SPEAKER_VOLUME: {
            if (length != 5) { // 1 byte for message type + 4 bytes for float32
                SerialQueueManager::get_instance().queue_message("Invalid speaker volume message length");
            } else {
                // Extract the 4-byte float32 value (little-endian)
                float volume;
                memcpy(&volume, &data[1], sizeof(float));
                Speaker::get_instance().set_volume(volume);
            }
            break;
        }
        case DataMessageType::STOP_TONE: {
            Speaker::get_instance().stop_all_sounds();
            break;
        }
        case DataMessageType::UPDATE_DISPLAY: {
            if (length != 1025) { // 1 byte for type + 1024 bytes for buffer
                SerialQueueManager::get_instance().queue_message("Invalid display buffer message length");
            } else {
                DisplayScreen::get_instance().show_custom_buffer(&data[1]);
            }
            break;
        }
        case DataMessageType::TRIGGER_MESSAGE: {
            if (length < 3) { // 1 byte for type + 1 byte for career + 1 byte for trigger
                SerialQueueManager::get_instance().queue_message("Invalid trigger message length");
            } else {
                CareerType careerType = static_cast<CareerType>(data[1]);

                switch (careerType) {
                    case CareerType::MEET_PIP: {
                        MeetPipTriggerType triggerType = static_cast<MeetPipTriggerType>(data[2]);

                        switch (triggerType) {
                            case MeetPipTriggerType::ENTER_CAREER:
                                DisplayScreen::get_instance().turn_display_off();
                                Speaker::get_instance().stop_all_sounds();
                                ledAnimations.fade_out();
                                rgbLed.turn_headlights_off();
                                break;
                            case MeetPipTriggerType::S2_P1_ENTER:
                                careerQuestTriggers.startS2P1Sequence();
                                break;
                            case MeetPipTriggerType::S2_P1_EXIT:
                                careerQuestTriggers.stopS2P1Sequence();
                                break;
                            case MeetPipTriggerType::S2_P4_ENTER:
                                careerQuestTriggers.startS2P4LightShow();
                                break;
                            case MeetPipTriggerType::S2_P4_EXIT:
                                careerQuestTriggers.stopS2P4LightShow();
                                break;
                            case MeetPipTriggerType::S3_P3_ENTER:
                                careerQuestTriggers.startS3P3DisplayDemo();
                                break;
                            case MeetPipTriggerType::S3_P3_EXIT:
                                careerQuestTriggers.stopS3P3DisplayDemo();
                                break;
                            case MeetPipTriggerType::S4_P4_EXIT:
                                Speaker::get_instance().stop_all_sounds();
                                break;
                            case MeetPipTriggerType::S4_P5_ENTER:
                                Speaker::get_instance().start_entertainer_melody();
                                break;
                            case MeetPipTriggerType::S4_P5_EXIT:
                                Speaker::get_instance().stop_all_sounds();
                                break;
                            case MeetPipTriggerType::S5_P4_ENTER:
                                SendSensorData::get_instance().setSendSensorData(true);
                                SendSensorData::get_instance().setEulerDataEnabled(true);
                                SendSensorData::get_instance().setAccelDataEnabled(true);
                                careerQuestTriggers.startS5P4LedVisualization();
                                break;
                            case MeetPipTriggerType::S5_P4_EXIT:
                                SendSensorData::get_instance().setEulerDataEnabled(false);
                                SendSensorData::get_instance().setAccelDataEnabled(false);
                                SendSensorData::get_instance().setSendSensorData(false);
                                careerQuestTriggers.stopS5P4LedVisualization();
                                break;
                            case MeetPipTriggerType::S5_P5_ENTER:
                                SendSensorData::get_instance().setSendSensorData(true);
                                SendSensorData::get_instance().setEulerDataEnabled(true);
                                break;
                            case MeetPipTriggerType::S5_P5_EXIT:
                                SendSensorData::get_instance().setEulerDataEnabled(false);
                                SendSensorData::get_instance().setSendSensorData(false);
                                break;
                            case MeetPipTriggerType::S6_P4_ENTER:
                                SendSensorData::get_instance().setSendMultizoneData(true);
                                rgbLed.turn_headlights_faint_blue();
                                break;
                            case MeetPipTriggerType::S6_P4_EXIT:
                                SendSensorData::get_instance().setSendMultizoneData(false);
                                rgbLed.turn_headlights_off();
                                break;
                            case MeetPipTriggerType::S6_P6_ENTER:
                                SendSensorData::get_instance().setSendSensorData(true);
                                SendSensorData::get_instance().setSideTofDataEnabled(true);
                                rgbLed.turn_front_middle_leds_faint_blue();
                                break;
                            case MeetPipTriggerType::S6_P6_EXIT:
                                SendSensorData::get_instance().setSideTofDataEnabled(false);
                                SendSensorData::get_instance().setSendSensorData(false);
                                rgbLed.turn_front_middle_leds_off();
                                break;
                            case MeetPipTriggerType::S7_P4_ENTER:
                                careerQuestTriggers.startS7P4ButtonDemo();
                                break;
                            case MeetPipTriggerType::S7_P4_EXIT:
                                careerQuestTriggers.stopS7P4ButtonDemo();
                                break;
                            case MeetPipTriggerType::S7_P6_ENTER:
                                GameManager::get_instance().start_game(Games::GameType::DINO_RUNNER);
                                break;
                            case MeetPipTriggerType::S7_P6_EXIT:
                                GameManager::get_instance().stop_current_game();
                                break;
                            case MeetPipTriggerType::S8_P3_ENTER:
                                SendSensorData::get_instance().setSendSensorData(true);
                                SendSensorData::get_instance().setColorSensorDataEnabled(true);
                                break;
                            case MeetPipTriggerType::S8_P3_EXIT:
                                SendSensorData::get_instance().setColorSensorDataEnabled(false);
                                SendSensorData::get_instance().setSendSensorData(false);
                                SensorDataBuffer::get_instance().stopPollingSensor(SensorDataBuffer::SensorType::COLOR);
                                break;
                            case MeetPipTriggerType::S9_P3_ENTER:
                                DanceManager::get_instance().start_dance();
                                break;
                            case MeetPipTriggerType::S9_P3_EXIT:
                                DanceManager::get_instance().stop_dance(true);
                                break;
                            case MeetPipTriggerType::S9_P6_ENTER:
                                motorDriver.stop_both_motors(); // We need this to prevent students from turning against the motors.
                                SendSensorData::get_instance().setSendSensorData(true);
                                SendSensorData::get_instance().setEncoderDataEnabled(true);
                                rgbLed.turn_headlights_faint_blue();
                                break;
                            case MeetPipTriggerType::S9_P6_EXIT:
                                SendSensorData::get_instance().setEncoderDataEnabled(false);
                                SendSensorData::get_instance().setSendSensorData(false);
                                rgbLed.turn_back_leds_off();
                                break;
                            default:
                                SerialQueueManager::get_instance().queue_message("Unknown introduction trigger type");
                                break;
                        }
                        break;
                    }
                    case CareerType::TURRET_ARCADE: {
                        TurretArcadeTriggerType triggerType = static_cast<TurretArcadeTriggerType>(data[2]);

                        switch (triggerType) {
                            case TurretArcadeTriggerType::ENTER_TURRET_ARCADE:
                                SendSensorData::get_instance().setSendSensorData(true);
                                SendSensorData::get_instance().setEulerDataEnabled(true);
                                SendSensorData::get_instance().setSideTofDataEnabled(true);
                                break;
                            case TurretArcadeTriggerType::EXIT_TURRET_ARCADE:
                                SendSensorData::get_instance().setEulerDataEnabled(false);
                                SendSensorData::get_instance().setSideTofDataEnabled(false);
                                SendSensorData::get_instance().setSendSensorData(false);
                                break;
                            default:
                                SerialQueueManager::get_instance().queue_message("Unknown turret arcade trigger type");
                                break;
                        }
                        break;
                    }
                    default:
                        SerialQueueManager::get_instance().queue_message("Unknown career type");
                        break;
                }
            }
            break;
        }
        case DataMessageType::STOP_CAREER_QUEST_TRIGGER: {
            if (length != 1) {
                SerialQueueManager::get_instance().queue_message("Invalid stop career quest trigger message length");
            } else {
                careerQuestTriggers.stopAllCareerQuestTriggers(true);
            }
            break;
        }
        case DataMessageType::SHOW_DISPLAY_START_SCREEN: {
            if (length != 1) {
                SerialQueueManager::get_instance().queue_message("Invalid show display start screen message length");
            } else {
                DisplayScreen::get_instance().show_start_screen();
            }
            break;
        }
        case DataMessageType::IS_USER_CONNECTED_TO_PIP: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid Is user connected to pip length");
            } else {
                UserConnectedStatus status = static_cast<UserConnectedStatus>(data[1]);
                if (status == UserConnectedStatus::NOT_CONNECTED) {
                    WebSocketManager::get_instance().setIsUserConnectedToThisPip(false);
                } else {
                    WebSocketManager::get_instance().setIsUserConnectedToThisPip(true);
                    BatteryMonitor::get_instance().sendBatteryMonitorDataOverWebSocket();
                }
            }
            break;
        }
        case DataMessageType::FORGET_NETWORK: {
            if (length < 2) {
                SerialQueueManager::get_instance().queue_message("Invalid forget network message length");
                SerialManager::get_instance().sendNetworkDeletedResponse(false);
                break;
            }

            uint8_t ssidLength = data[1];
            if (ssidLength == 0 || 2 + ssidLength != length) {
                SerialQueueManager::get_instance().queue_message("Invalid SSID length in forget network message");
                SerialManager::get_instance().sendNetworkDeletedResponse(false);
                break;
            }

            String ssid = "";
            for (uint8_t i = 0; i < ssidLength; i++) {
                ssid += (char)data[2 + i];
            }

            // Check if we're trying to forget the currently connected network
            if (WiFiManager::get_instance().isConnectedToSSID(ssid)) {
                if (!SerialManager::get_instance().is_serial_connected()) {
                    SerialQueueManager::get_instance().queue_message("Cannot forget currently connected network without serial connection");
                    SerialManager::get_instance().sendNetworkDeletedResponse(false);
                    break;
                }
                // Disconnect from WiFi before forgetting
                WiFi.disconnect(true);
            }

            // Attempt to forget the network
            bool success = PreferencesManager::get_instance().forgetWiFiNetwork(ssid);
            SerialManager::get_instance().sendNetworkDeletedResponse(success);
            break;
        }
        default:
            SerialQueueManager::get_instance().queue_message("Received unknown message type: " + String(static_cast<int>(messageType)));
            break;
    }
}
