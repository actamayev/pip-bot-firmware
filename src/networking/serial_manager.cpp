#include "serial_manager.h"

void SerialManager::poll_serial() {
    if (Serial.available() <= 0) {
        // Check for timeout if we're connected but haven't received data for a while
        if (_isConnected && (millis() - last_activity_time > SERIAL_CONNECTION_TIMEOUT)) {
            _isConnected = false;
            if (!WebSocketManager::get_instance().is_ws_connected()) {
                career_quest_triggers.stop_all_career_quest_triggers(false);
            }
        }
        return;
    }

    last_activity_time = millis();

    if (!is_serial_connected()) {
        _isConnected = true;
        // If we were previously trying to connect to wifi (breathing red), we should turn it off when connecting to serial
        led_animations.stop_animation();
    }

    // Read available bytes and process according to the current state
    while (Serial.available() > 0) {
        uint8_t in_byte = Serial.read();

        switch (_parseState) {
            case ParseState::WAITING_FOR_START:
                if (in_byte == START_MARKER) {
                    // Start of a new message
                    _bufferPosition = 0;
                    _parseState = ParseState::READING_MESSAGE_TYPE;
                }
                break;

            case ParseState::READING_MESSAGE_TYPE:
                _receiveBuffer[_bufferPosition++] = in_byte; // Store message type as first byte
                _parseState = ParseState::READING_FORMAT_FLAG;
                break;

            case ParseState::READING_FORMAT_FLAG:
                _useLongFormat = (in_byte != 0);
                _parseState = ParseState::READING_LENGTH_BYTE1;
                break;

            case ParseState::READING_LENGTH_BYTE1:
                if (_useLongFormat) {
                    // First byte of 16-bit length (little-endian)
                    _expectedPayloadLength = in_byte;
                    _parseState = ParseState::READING_LENGTH_BYTE2;
                } else {
                    // 8-bit length
                    _expectedPayloadLength = in_byte;

                    // If payload length is 0, skip to waiting for end marker
                    if (_expectedPayloadLength == 0) {
                        _parseState = ParseState::WAITING_FOR_END;
                    } else {
                        _parseState = ParseState::READING_PAYLOAD;
                    }
                }
                break;

            case ParseState::READING_LENGTH_BYTE2:
                // Second byte of 16-bit length (little-endian)
                _expectedPayloadLength |= (in_byte << 8);

                // If payload length is 0, skip to waiting for end marker
                if (_expectedPayloadLength == 0) {
                    _parseState = ParseState::WAITING_FOR_END;
                } else {
                    _parseState = ParseState::READING_PAYLOAD;
                }
                break;

            case ParseState::READING_PAYLOAD:
                // Store payload byte
                if (_bufferPosition < sizeof(_receiveBuffer)) {
                    _receiveBuffer[_bufferPosition++] = in_byte;

                    // Check if we've read the complete payload
                    if (_bufferPosition >= _expectedPayloadLength + 1) { // +1 for message type
                        _parseState = ParseState::WAITING_FOR_END;
                    }
                } else {
                    // Buffer overflow
                    SerialQueueManager::get_instance().queue_message("Serial buffer overflow");
                    _parseState = ParseState::WAITING_FOR_START;
                }
                break;

            case ParseState::WAITING_FOR_END:
                if (in_byte == END_MARKER) {
                    // Process the message
                    MessageProcessor::get_instance().process_binary_message(_receiveBuffer, _bufferPosition);

                    // Reset for next message
                    _parseState = ParseState::WAITING_FOR_START;
                } else {
                    // Invalid end marker
                    SerialQueueManager::get_instance().queue_message("Invalid end marker");
                    _parseState = ParseState::WAITING_FOR_START;
                }
                break;
        }
    }
}

void SerialManager::send_pip_id_message() {
    if (!is_serial_connected()) {
        return;
    }

    auto doc = make_base_message_serial<256>(ToSerialMessage::PIP_ID);
    JsonObject payload = doc.createNestedObject("payload");
    payload["pipId"] = PreferencesManager::get_instance().get_pip_id();

    String json_string;
    serializeJson(doc, json_string);

    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_json_message(ToSerialMessage route, const String& status) {
    if (!is_serial_connected()) {
        return;
    }

    auto doc = make_base_message_serial<256>(route);
    JsonObject payload = doc.createNestedObject("payload");
    payload["status"] = status;

    String json_string;
    serializeJson(doc, json_string);

    // Browser responses are CRITICAL
    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_saved_networks_response(const std::vector<WiFiCredentials>& networks) {
    if (!is_serial_connected()) {
        return;
    }

    // Build JSON response
    auto doc = make_base_message_serial<1024>(ToSerialMessage::SAVED_NETWORKS);
    JsonArray payload = doc.createNestedArray("payload");

    for (size_t i = 0; i < networks.size(); i++) {
        JsonObject network = payload.createNestedObject();
        network["ssid"] = networks[i].ssid;
        network["index"] = i;
    }

    String json_string;
    serializeJson(doc, json_string);

    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_scan_results_response(const std::vector<WiFiNetworkInfo>& networks) {
    if (!is_serial_connected()) {
        return;
    }

    // Send each network as individual message
    for (size_t i = 0; i < networks.size(); i++) {
        auto doc = make_base_message_serial<256>(ToSerialMessage::SCAN_RESULT_ITEM);
        JsonObject payload = doc.createNestedObject("payload");
        payload["ssid"] = networks[i].ssid;
        payload["rssi"] = networks[i].rssi;
        payload["encrypted"] = (networks[i].encryptionType != WIFI_AUTH_OPEN);

        String json_string;
        serializeJson(doc, json_string);

        SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
    }

    // Send completion message
    auto complete_doc = make_base_message_serial<256>(ToSerialMessage::SCAN_COMPLETE);
    JsonObject complete_payload = complete_doc.createNestedObject("payload");
    complete_payload["totalNetworks"] = networks.size();

    String complete_json_string;
    serializeJson(complete_doc, complete_json_string);

    SerialQueueManager::get_instance().queue_message(complete_json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_scan_started_message() {
    if (!is_serial_connected()) {
        return;
    }

    auto doc = make_base_message_serial<256>(ToSerialMessage::SCAN_STARTED);
    JsonObject payload = doc.createNestedObject("payload");
    payload["scanning"] = true;

    String json_string;
    serializeJson(doc, json_string);

    // Use CRITICAL priority for browser responses
    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_monitor_data() {
    if (!is_serial_connected()) {
        return;
    }

    const BatteryState& battery_state = BatteryMonitor::get_instance().get_battery_state();

    // Send each battery data field as individual message
    send_battery_data_item("stateOfCharge", static_cast<int>(round(battery_state.displayedStateOfCharge)));
    send_battery_data_item("voltage", battery_state.voltage);
    send_battery_data_item("current", battery_state.current);
    send_battery_data_item("power", battery_state.power);
    send_battery_data_item("remainingCapacity", battery_state.remainingCapacity);
    send_battery_data_item("fullCapacity", battery_state.fullCapacity);
    send_battery_data_item("health", battery_state.health);
    send_battery_data_item("isCharging", battery_state.isCharging);
    send_battery_data_item("isDischarging", battery_state.isDischarging);
    send_battery_data_item("isLowBattery", battery_state.isLowBattery);
    send_battery_data_item("isCriticalBattery", battery_state.isCriticalBattery);
    send_battery_data_item("estimatedTimeToEmpty", battery_state.estimatedTimeToEmpty);
    send_battery_data_item("estimatedTimeToFull", battery_state.estimatedTimeToFull);

    auto complete_doc = make_base_message_serial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_COMPLETE);
    JsonObject complete_payload = complete_doc.createNestedObject("payload");
    complete_payload["totalItems"] = 13; // Number of battery data fields sent

    String complete_json_string;
    serializeJson(complete_doc, complete_json_string);

    SerialQueueManager::get_instance().queue_message(complete_json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_data_item(const String& key, int value) {
    if (!is_serial_connected()) {
        return;
    }

    auto doc = make_base_message_serial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_ITEM);
    JsonObject payload = doc.createNestedObject("payload");
    payload["key"] = key;
    payload["value"] = value;

    String json_string;
    serializeJson(doc, json_string);

    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_data_item(const String& key, uint32_t value) {
    if (!is_serial_connected()) {
        return;
    }

    auto doc = make_base_message_serial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_ITEM);
    JsonObject payload = doc.createNestedObject("payload");
    payload["key"] = key;
    payload["value"] = value;

    String json_string;
    serializeJson(doc, json_string);

    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_data_item(const String& key, float value) {
    if (!is_serial_connected()) {
        return;
    }

    auto doc = make_base_message_serial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_ITEM);
    JsonObject payload = doc.createNestedObject("payload");
    payload["key"] = key;
    payload["value"] = value;

    String json_string;
    serializeJson(doc, json_string);

    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_data_item(const String& key, bool value) {
    if (!is_serial_connected()) {
        return;
    }

    auto doc = make_base_message_serial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_ITEM);
    JsonObject payload = doc.createNestedObject("payload");
    payload["key"] = key;
    payload["value"] = value;

    String json_string;
    serializeJson(doc, json_string);

    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_dino_score(int score) {
    if (!is_serial_connected()) {
        return;
    }

    auto doc = make_base_message_common<256>(ToCommonMessage::DINO_SCORE);
    JsonObject payload = doc.createNestedObject("payload");
    payload["score"] = score;

    String json_string;
    serializeJson(doc, json_string);

    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_network_deleted_response(bool success) {
    if (!is_serial_connected()) {
        return;
    }

    auto doc = make_base_message_serial<256>(ToSerialMessage::WIFI_DELETED_NETWORK);
    JsonObject payload = doc.createNestedObject("payload");
    payload["status"] = success;

    String json_string;
    serializeJson(doc, json_string);

    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}

void SerialManager::send_pip_turning_off() {
    if (!is_serial_connected()) {
        return;
    }

    auto pip_turning_off_doc = make_base_message_common<256>(ToCommonMessage::PIP_TURNING_OFF);
    JsonObject payload = pip_turning_off_doc.createNestedObject("payload");
    payload["reason"] = "PIP is turning off";

    String json_string;
    serializeJson(pip_turning_off_doc, json_string);

    SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
}
