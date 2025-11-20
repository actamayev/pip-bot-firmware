#include "message_processor.h"

#include <math.h>

#include <cmath>

void MessageProcessor::handle_motor_control(const uint8_t* data) {
    // Extract 16-bit signed integers (little-endian)
    auto left_speed = static_cast<int16_t>(data[1] | (data[2] << 8));
    auto right_speed = static_cast<int16_t>(data[3] | (data[4] << 8));

    motor_driver.update_motor_pwm(left_speed, right_speed);
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

void MessageProcessor::handle_light_command(LightAnimationStatus light_animation_status) {
    if (light_animation_status == LightAnimationStatus::NO_ANIMATION) {
        led_animations.stop_animation();
    } else if (light_animation_status == LightAnimationStatus::BREATHING) {
        led_animations.start_breathing();
    } else if (light_animation_status == LightAnimationStatus::RAINBOW) {
        led_animations.start_rainbow();
    } else if (light_animation_status == LightAnimationStatus::STROBE) {
        led_animations.start_strobing();
    } else if (light_animation_status == LightAnimationStatus::TURN_OFF) {
        led_animations.turn_off();
    } else if (light_animation_status == LightAnimationStatus::FADE_OUT) {
        led_animations.fade_out();
    }
}

void MessageProcessor::handle_new_light_colors(NewLightColors new_light_colors) {
    // Cast from float to uint8_t, assuming values are already in 0-255 range
    auto top_left_r = new_light_colors.topLeftRed;
    auto top_left_g = new_light_colors.topLeftGreen;
    auto top_left_b = new_light_colors.topLeftBlue;

    auto top_right_r = new_light_colors.topRightRed;
    auto top_right_g = new_light_colors.topRightGreen;
    auto top_right_b = new_light_colors.topRightBlue;

    auto middle_left_r = new_light_colors.middleLeftRed;
    auto middle_left_g = new_light_colors.middleLeftGreen;
    auto middle_left_b = new_light_colors.middleLeftBlue;

    auto middle_right_r = new_light_colors.middleRightRed;
    auto middle_right_g = new_light_colors.middleRightGreen;
    auto middle_right_b = new_light_colors.middleRightBlue;

    auto back_left_r = new_light_colors.backLeftRed;
    auto back_left_g = new_light_colors.backLeftGreen;
    auto back_left_b = new_light_colors.backLeftBlue;

    auto back_right_r = new_light_colors.backRightRed;
    auto back_right_g = new_light_colors.backRightGreen;
    auto back_right_b = new_light_colors.backRightBlue;

    // Set each LED to its corresponding color
    rgb_led.set_top_left_led(top_left_r, top_left_g, top_left_b);
    rgb_led.set_top_right_led(top_right_r, top_right_g, top_right_b);
    rgb_led.set_middle_left_led(middle_left_r, middle_left_g, middle_left_b);
    rgb_led.set_middle_right_led(middle_right_r, middle_right_g, middle_right_b);
    rgb_led.set_back_left_led(back_left_r, back_left_g, back_left_b);
    rgb_led.set_back_right_led(back_right_r, back_right_g, back_right_b);
}

void MessageProcessor::handle_get_saved_wifi_networks() {
    // Get saved networks from WiFiManager
    std::vector<WiFiCredentials> saved_networks = WiFiManager::get_instance().get_saved_networks_for_response();

    // Send response via SerialManager
    SerialManager::get_instance().send_saved_networks_response(saved_networks);
}

void MessageProcessor::handle_soft_scan_wifi_networks() {
    // Check if we have recent scan results (within 1 minute)
    WiFiManager& wifi_manager = WiFiManager::get_instance();
    if (wifi_manager.has_available_networks()) {
        SerialManager::get_instance().send_scan_results_response(wifi_manager.get_available_networks());
        return;
    }
    const uint32_t NOW = millis();
    if (NOW - wifi_manager.get_last_scan_complete_time() < 60000) {
        return;
    }
    // Start async scan instead of blocking scan
    const bool SUCCESS = wifi_manager.start_async_scan();

    if (SUCCESS) {
        return; // Note: Results will be sent asynchronously when scan completes
    }
    SerialQueueManager::get_instance().queue_message("Failed to start WiFi scan");
    // Send empty scan results to indicate failure
    std::vector<WiFiNetworkInfo> empty_networks;
    SerialManager::get_instance().send_scan_results_response(empty_networks);
}

void MessageProcessor::handle_hard_scan_wifi_networks() {
    bool success = WiFiManager::get_instance().start_async_scan();
    if (success) {
        return;
    }
    SerialQueueManager::get_instance().queue_message("Failed to start hard WiFi scan");
    std::vector<WiFiNetworkInfo> empty_networks;
    SerialManager::get_instance().send_scan_results_response(empty_networks);
}

void MessageProcessor::process_binary_message(const uint8_t* data, uint16_t length) {
    TimeoutManager::get_instance().reset_activity();
    if (length < 1) {
        SerialQueueManager::get_instance().queue_message("Binary message too short");
        return;
    }
    // Extract the message type from the first byte
    auto message_type = static_cast<DataMessageType>(data[0]);

    switch (message_type) {
        case DataMessageType::UPDATE_AVAILABLE: {
            if (length != 3) {
                SerialQueueManager::get_instance().queue_message("Invalid update available message length");
            } else {
                // Extract the firmware version from bytes 1-2
                uint16_t new_version = data[1] | (data[2] << 8); // Little-endian conversion

                // SerialQueueManager::get_instance().queue_message("New firmware version available: %d\n", new_version);
                FirmwareVersionTracker::get_instance().retrieve_latest_firmware_from_server(new_version);
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
                auto tone_type = static_cast<ToneType>(data[1]);
                Speaker::get_instance().play_tone(tone_type);
            }
            break;
        }
        case DataMessageType::SPEAKER_MUTE: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid speaker mute message length");
            } else {
                auto status = static_cast<SpeakerStatus>(data[1]);
                Speaker::get_instance().set_muted(status == SpeakerStatus::MUTED);
            }
            break;
        }
        case DataMessageType::BALANCE_CONTROL: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid balance control message length");
            } else {
                auto status = static_cast<BalanceStatus>(data[1]);
                handle_balance_command(status);
            }
            break;
        }
        case DataMessageType::UPDATE_LIGHT_ANIMATION: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid light animation message length");
            } else {
                auto light_animation_status = static_cast<LightAnimationStatus>(data[1]);
                handle_light_command(light_animation_status);
            }
            break;
        }
        case DataMessageType::UPDATE_LED_COLORS: {
            if (length != 19) {
                // SerialQueueManager::get_instance().queue_message("Invalid update led colors message length%d", length);
            } else {
                NewLightColors new_light_colors;
                memcpy(&new_light_colors, &data[1], sizeof(NewLightColors));
                handle_new_light_colors(new_light_colors);
            }
            break;
        }
        case DataMessageType::UPDATE_BALANCE_PIDS: {
            if (length != 41) { // 1 byte for type + 40 bytes for the struct (10 floats × 4 bytes)
                SerialQueueManager::get_instance().queue_message("Invalid update balance pids message length");
            } else {
                NewBalancePids new_balance_pids{};
                memcpy(&new_balance_pids, &data[1], sizeof(NewBalancePids));
                BalanceController::get_instance().update_balance_pids(new_balance_pids);
            }
            break;
        }
        case DataMessageType::BYTECODE_PROGRAM: {
            // First byte is the message type, the rest is bytecode
            const uint8_t* bytecode_data = data + 1;
            uint16_t bytecode_length = length - 1;

            // Execute the bytecode
            BytecodeVM::get_instance().load_program(bytecode_data, bytecode_length);
            break;
        }
        case DataMessageType::STOP_SANDBOX_CODE: {
            if (length != 1) {
                SerialQueueManager::get_instance().queue_message("Invalid stop sandbox code message length");
            } else {
                BytecodeVM::get_instance().stop_program();
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
                auto status = static_cast<ObstacleAvoidanceStatus>(data[1]);
                handle_obstacle_avoidance_command(status);
            }
            break;
        }
        case DataMessageType::SERIAL_HANDSHAKE: {
            SerialManager::get_instance()._isConnected = true;
            SerialManager::get_instance().last_activity_time = millis();
            SerialManager::get_instance().send_pip_id_message();

            // Send initial battery data on handshake
            const BatteryState& battery_state = BatteryMonitor::get_instance().get_battery_state();
            if (battery_state.isInitialized) {
                SerialManager::get_instance().send_battery_monitor_data();
                BatteryMonitor::get_instance()._lastBatteryLogTime = millis();
            }
            break;
        }
        case DataMessageType::SERIAL_KEEPALIVE: {
            SerialManager::get_instance().last_activity_time = millis();
            break;
        }
        case DataMessageType::SERIAL_END: {
            rgb_led.turn_all_leds_off();
            SerialManager::get_instance()._isConnected = false;
            SensorDataBuffer::get_instance().stop_polling_all_sensors();
            Speaker::get_instance().stop_all_sounds();
            break;
        }
        case DataMessageType::UPDATE_HEADLIGHTS: {
            if (length != 2) {
                SerialQueueManager::get_instance().queue_message("Invalid update headlights message length");
            } else {
                auto status = static_cast<HeadlightStatus>(data[1]);
                if (status == HeadlightStatus::ON) {
                    rgb_led.turn_headlights_on();
                } else {
                    rgb_led.turn_headlights_off();
                }
            }
            break;
        }
        case DataMessageType::WIFI_CREDENTIALS: {
            if (length < 3) { // At least 1 byte for each length field + message type
                SerialQueueManager::get_instance().queue_message("Invalid WiFi credentials message length");
                break;
            }

            const uint8_t SSID_LENGTH = data[1];
            if (SSID_LENGTH == 0 || 2 + SSID_LENGTH >= length) {
                SerialQueueManager::get_instance().queue_message("Invalid SSID length");
                break;
            }

            String ssid = "";
            for (uint8_t i = 0; i < SSID_LENGTH; i++) {
                ssid += static_cast<char>(data[2 + i]);
            }

            const uint8_t PASSWORD_LENGTH = data[2 + SSID_LENGTH];
            if (2 + SSID_LENGTH + 1 + PASSWORD_LENGTH != length) {
                SerialQueueManager::get_instance().queue_message("Invalid password length");
                break;
            }

            String password = "";
            for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
                password += static_cast<char>(data[3 + SSID_LENGTH + i]);
            }

            // Test WiFi credentials before storing permanently
            WiFiManager::get_instance().start_wifi_credential_test(ssid, password);

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
                float volume = NAN;
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
                auto career_type = static_cast<CareerType>(data[1]);

                switch (career_type) {
                    case CareerType::MEET_PIP: {
                        auto trigger_type = static_cast<MeetPipTriggerType>(data[2]);

                        switch (trigger_type) {
                            case MeetPipTriggerType::ENTER_CAREER:
                                DisplayScreen::get_instance().turn_display_off();
                                Speaker::get_instance().stop_all_sounds();
                                led_animations.fade_out();
                                rgb_led.turn_headlights_off();
                                break;
                            case MeetPipTriggerType::S2_P1_ENTER:
                                career_quest_triggers.start_s2_p1_sequence();
                                break;
                            case MeetPipTriggerType::S2_P1_EXIT:
                                career_quest_triggers.stop_s2_p1_sequence();
                                break;
                            case MeetPipTriggerType::S2_P4_ENTER:
                                career_quest_triggers.start_s2_p4_light_show();
                                break;
                            case MeetPipTriggerType::S2_P4_EXIT:
                                career_quest_triggers.stop_s2_p4_light_show();
                                break;
                            case MeetPipTriggerType::S3_P3_ENTER:
                                career_quest_triggers.start_s3_p3_display_demo();
                                break;
                            case MeetPipTriggerType::S3_P3_EXIT:
                                career_quest_triggers.stop_s3_p3_display_demo();
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
                                SendSensorData::get_instance().set_send_sensor_data(true);
                                SendSensorData::get_instance().set_euler_data_enabled(true);
                                SendSensorData::get_instance().set_accel_data_enabled(true);
                                career_quest_triggers.start_s5_p4_led_visualization();
                                break;
                            case MeetPipTriggerType::S5_P4_EXIT:
                                SendSensorData::get_instance().set_euler_data_enabled(false);
                                SendSensorData::get_instance().set_accel_data_enabled(false);
                                SendSensorData::get_instance().set_send_sensor_data(false);
                                career_quest_triggers.stop_s5_p4_led_visualization();
                                break;
                            case MeetPipTriggerType::S5_P5_ENTER:
                                SendSensorData::get_instance().set_send_sensor_data(true);
                                SendSensorData::get_instance().set_euler_data_enabled(true);
                                break;
                            case MeetPipTriggerType::S5_P5_EXIT:
                                SendSensorData::get_instance().set_euler_data_enabled(false);
                                SendSensorData::get_instance().set_send_sensor_data(false);
                                break;
                            case MeetPipTriggerType::S6_P4_ENTER:
                                SendSensorData::get_instance().set_send_multizone_data(true);
                                rgb_led.turn_headlights_faint_blue();
                                break;
                            case MeetPipTriggerType::S6_P4_EXIT:
                                SendSensorData::get_instance().set_send_multizone_data(false);
                                rgb_led.turn_headlights_off();
                                break;
                            case MeetPipTriggerType::S6_P6_ENTER:
                                SendSensorData::get_instance().set_send_sensor_data(true);
                                SendSensorData::get_instance().set_side_tof_data_enabled(true);
                                rgb_led.turn_front_middle_leds_faint_blue();
                                break;
                            case MeetPipTriggerType::S6_P6_EXIT:
                                SendSensorData::get_instance().set_side_tof_data_enabled(false);
                                SendSensorData::get_instance().set_send_sensor_data(false);
                                rgb_led.turn_front_middle_leds_off();
                                break;
                            case MeetPipTriggerType::S7_P4_ENTER:
                                career_quest_triggers.start_s7_p4_button_demo();
                                break;
                            case MeetPipTriggerType::S7_P4_EXIT:
                                career_quest_triggers.stop_s7_p4_button_demo();
                                break;
                            case MeetPipTriggerType::S7_P6_ENTER:
                                GameManager::get_instance().start_game(games::GameType::DINO_RUNNER);
                                break;
                            case MeetPipTriggerType::S7_P6_EXIT:
                                GameManager::get_instance().stop_current_game();
                                break;
                            case MeetPipTriggerType::S8_P3_ENTER:
                                SendSensorData::get_instance().set_send_sensor_data(true);
                                SendSensorData::get_instance().set_color_sensor_data_enabled(true);
                                break;
                            case MeetPipTriggerType::S8_P3_EXIT:
                                SendSensorData::get_instance().set_color_sensor_data_enabled(false);
                                SendSensorData::get_instance().set_send_sensor_data(false);
                                SensorDataBuffer::get_instance().stop_polling_sensor(SensorDataBuffer::SensorType::COLOR);
                                break;
                            case MeetPipTriggerType::S9_P3_ENTER:
                                DanceManager::get_instance().start_dance();
                                break;
                            case MeetPipTriggerType::S9_P3_EXIT:
                                DanceManager::get_instance().stop_dance(true);
                                break;
                            case MeetPipTriggerType::S9_P6_ENTER:
                                motor_driver.stop_both_motors(); // We need this to prevent students from turning against the motors.
                                SendSensorData::get_instance().set_send_sensor_data(true);
                                SendSensorData::get_instance().set_encoder_data_enabled(true);
                                rgb_led.turn_headlights_faint_blue();
                                break;
                            case MeetPipTriggerType::S9_P6_EXIT:
                                SendSensorData::get_instance().set_encoder_data_enabled(false);
                                SendSensorData::get_instance().set_send_sensor_data(false);
                                rgb_led.turn_back_leds_off();
                                break;
                            default:
                                SerialQueueManager::get_instance().queue_message("Unknown introduction trigger type");
                                break;
                        }
                        break;
                    }
                    case CareerType::TURRET_ARCADE: {
                        auto trigger_type = static_cast<TurretArcadeTriggerType>(data[2]);

                        switch (trigger_type) {
                            case TurretArcadeTriggerType::ENTER_TURRET_ARCADE:
                                SendSensorData::get_instance().set_send_sensor_data(true);
                                SendSensorData::get_instance().set_euler_data_enabled(true);
                                SendSensorData::get_instance().set_side_tof_data_enabled(true);
                                break;
                            case TurretArcadeTriggerType::EXIT_TURRET_ARCADE:
                                SendSensorData::get_instance().set_euler_data_enabled(false);
                                SendSensorData::get_instance().set_side_tof_data_enabled(false);
                                SendSensorData::get_instance().set_send_sensor_data(false);
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
                career_quest_triggers.stop_all_career_quest_triggers(true);
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
                auto status = static_cast<UserConnectedStatus>(data[1]);
                if (status == UserConnectedStatus::NOT_CONNECTED) {
                    CommandWebSocketManager::get_instance().set_is_user_connected_to_this_pip(false);
                } else {
                    CommandWebSocketManager::get_instance().set_is_user_connected_to_this_pip(true);
                    BatteryMonitor::get_instance().send_battery_monitor_data_over_websocket();
                }
            }
            break;
        }
        case DataMessageType::FORGET_NETWORK: {
            if (length < 2) {
                SerialQueueManager::get_instance().queue_message("Invalid forget network message length");
                SerialManager::get_instance().send_network_deleted_response(false);
                break;
            }

            uint8_t ssid_length = data[1];
            if (ssid_length == 0 || 2 + ssid_length != length) {
                SerialQueueManager::get_instance().queue_message("Invalid SSID length in forget network message");
                SerialManager::get_instance().send_network_deleted_response(false);
                break;
            }

            String ssid = "";
            for (uint8_t i = 0; i < ssid_length; i++) {
                ssid += static_cast<char>(data[2 + i]);
            }

            // Check if we're trying to forget the currently connected network
            if (WiFiManager::get_instance().is_connected_to_ssid(ssid)) {
                if (!SerialManager::get_instance().is_serial_connected()) {
                    SerialQueueManager::get_instance().queue_message("Cannot forget currently connected network without serial connection");
                    SerialManager::get_instance().send_network_deleted_response(false);
                    break;
                }
                // Disconnect from WiFi before forgetting
                WiFi.disconnect(true);
            }

            // Attempt to forget the network
            bool success = PreferencesManager::get_instance().forget_wifi_network(ssid);
            SerialManager::get_instance().send_network_deleted_response(success);
            break;
        }
        default:
            SerialQueueManager::get_instance().queue_message("Received unknown message type: " + String(static_cast<int>(message_type)));
            break;
    }
}
