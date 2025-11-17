#include "serial_manager.h"

void SerialManager::poll_serial() {
    if (Serial.available() <= 0) {
        // Check for timeout if we're connected but haven't received data for a while
        if (isConnected && (millis() - lastActivityTime > SERIAL_CONNECTION_TIMEOUT)) {
            isConnected = false;
            if (!WebSocketManager::get_instance().is_ws_connected()) {
                careerQuestTriggers.stopAllCareerQuestTriggers(false);
            }
        }
        return;
    }

    lastActivityTime = millis();

    if (!isConnected) {
        isConnected = true;
        // If we were previously trying to connect to wifi (breathing red), we should turn it off when connecting to serial
        ledAnimations.stopAnimation();
    }

    // Read available bytes and process according to the current state
    while (Serial.available() > 0) {
        uint8_t inByte = Serial.read();

        switch (parseState) {
            case ParseState::WAITING_FOR_START:
                if (inByte == START_MARKER) {
                    // Start of a new message
                    bufferPosition = 0;
                    parseState = ParseState::READING_MESSAGE_TYPE;
                }
                break;

            case ParseState::READING_MESSAGE_TYPE:
                receiveBuffer[bufferPosition++] = inByte; // Store message type as first byte
                parseState = ParseState::READING_FORMAT_FLAG;
                break;

            case ParseState::READING_FORMAT_FLAG:
                useLongFormat = (inByte != 0);
                parseState = ParseState::READING_LENGTH_BYTE1;
                break;

            case ParseState::READING_LENGTH_BYTE1:
                if (useLongFormat) {
                    // First byte of 16-bit length (little-endian)
                    expectedPayloadLength = inByte;
                    parseState = ParseState::READING_LENGTH_BYTE2;
                } else {
                    // 8-bit length
                    expectedPayloadLength = inByte;

                    // If payload length is 0, skip to waiting for end marker
                    if (expectedPayloadLength == 0) {
                        parseState = ParseState::WAITING_FOR_END;
                    } else {
                        parseState = ParseState::READING_PAYLOAD;
                    }
                }
                break;

            case ParseState::READING_LENGTH_BYTE2:
                // Second byte of 16-bit length (little-endian)
                expectedPayloadLength |= (inByte << 8);

                // If payload length is 0, skip to waiting for end marker
                if (expectedPayloadLength == 0) {
                    parseState = ParseState::WAITING_FOR_END;
                } else {
                    parseState = ParseState::READING_PAYLOAD;
                }
                break;

            case ParseState::READING_PAYLOAD:
                // Store payload byte
                if (bufferPosition < sizeof(receiveBuffer)) {
                    receiveBuffer[bufferPosition++] = inByte;

                    // Check if we've read the complete payload
                    if (bufferPosition >= expectedPayloadLength + 1) { // +1 for message type
                        parseState = ParseState::WAITING_FOR_END;
                    }
                } else {
                    // Buffer overflow
                    SerialQueueManager::get_instance().queue_message("Serial buffer overflow");
                    parseState = ParseState::WAITING_FOR_START;
                }
                break;

            case ParseState::WAITING_FOR_END:
                if (inByte == END_MARKER) {
                    // Process the message
                    MessageProcessor::get_instance().process_binary_message(receiveBuffer, bufferPosition);

                    // Reset for next message
                    parseState = ParseState::WAITING_FOR_START;
                } else {
                    // Invalid end marker
                    SerialQueueManager::get_instance().queue_message("Invalid end marker");
                    parseState = ParseState::WAITING_FOR_START;
                }
                break;
        }
    }
}

void SerialManager::send_pip_id_message() {
    if (!isConnected) return;

    auto doc = makeBaseMessageSerial<256>(ToSerialMessage::PIP_ID);
    JsonObject payload = doc.createNestedObject("payload");
    payload["pipId"] = PreferencesManager::get_instance().getPipId();

    String jsonString;
    serializeJson(doc, jsonString);

    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_json_message(ToSerialMessage route, const String& status) {
    if (!isConnected) return;

    auto doc = makeBaseMessageSerial<256>(route);
    JsonObject payload = doc.createNestedObject("payload");
    payload["status"] = status;

    String jsonString;
    serializeJson(doc, jsonString);

    // Browser responses are CRITICAL
    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_saved_networks_response(const std::vector<WiFiCredentials>& networks) {
    if (!isConnected) return;

    // Build JSON response
    auto doc = makeBaseMessageSerial<1024>(ToSerialMessage::SAVED_NETWORKS);
    JsonArray payload = doc.createNestedArray("payload");

    for (size_t i = 0; i < networks.size(); i++) {
        JsonObject network = payload.createNestedObject();
        network["ssid"] = networks[i].ssid;
        network["index"] = i;
    }

    String jsonString;
    serializeJson(doc, jsonString);

    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_scan_results_response(const std::vector<WiFiNetworkInfo>& networks) {
    if (!isConnected) return;

    // Send each network as individual message
    for (size_t i = 0; i < networks.size(); i++) {
        auto doc = makeBaseMessageSerial<256>(ToSerialMessage::SCAN_RESULT_ITEM);
        JsonObject payload = doc.createNestedObject("payload");
        payload["ssid"] = networks[i].ssid;
        payload["rssi"] = networks[i].rssi;
        payload["encrypted"] = (networks[i].encryptionType != WIFI_AUTH_OPEN);

        String jsonString;
        serializeJson(doc, jsonString);

        SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
    }

    // Send completion message
    auto completeDoc = makeBaseMessageSerial<256>(ToSerialMessage::SCAN_COMPLETE);
    JsonObject completePayload = completeDoc.createNestedObject("payload");
    completePayload["totalNetworks"] = networks.size();

    String completeJsonString;
    serializeJson(completeDoc, completeJsonString);

    SerialQueueManager::get_instance().queue_message(completeJsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_scan_started_message() {
    if (!isConnected) return;

    auto doc = makeBaseMessageSerial<256>(ToSerialMessage::SCAN_STARTED);
    JsonObject payload = doc.createNestedObject("payload");
    payload["scanning"] = true;

    String jsonString;
    serializeJson(doc, jsonString);

    // Use CRITICAL priority for browser responses
    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_monitor_data() {
    if (!isConnected) return;

    const BatteryState& batteryState = BatteryMonitor::get_instance().get_battery_state();

    // Send each battery data field as individual message
    send_battery_data_item("stateOfCharge", static_cast<int>(round(batteryState.displayedStateOfCharge)));
    send_battery_data_item("voltage", batteryState.voltage);
    send_battery_data_item("current", batteryState.current);
    send_battery_data_item("power", batteryState.power);
    send_battery_data_item("remainingCapacity", batteryState.remainingCapacity);
    send_battery_data_item("fullCapacity", batteryState.fullCapacity);
    send_battery_data_item("health", batteryState.health);
    send_battery_data_item("isCharging", batteryState.isCharging);
    send_battery_data_item("isDischarging", batteryState.isDischarging);
    send_battery_data_item("isLowBattery", batteryState.isLowBattery);
    send_battery_data_item("isCriticalBattery", batteryState.isCriticalBattery);
    send_battery_data_item("estimatedTimeToEmpty", batteryState.estimatedTimeToEmpty);
    send_battery_data_item("estimatedTimeToFull", batteryState.estimatedTimeToFull);

    auto completeDoc = makeBaseMessageSerial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_COMPLETE);
    JsonObject completePayload = completeDoc.createNestedObject("payload");
    completePayload["totalItems"] = 13; // Number of battery data fields sent

    String completeJsonString;
    serializeJson(completeDoc, completeJsonString);

    SerialQueueManager::get_instance().queue_message(completeJsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_data_item(const String& key, int value) {
    if (!isConnected) return;

    auto doc = makeBaseMessageSerial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_ITEM);
    JsonObject payload = doc.createNestedObject("payload");
    payload["key"] = key;
    payload["value"] = value;

    String jsonString;
    serializeJson(doc, jsonString);

    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_data_item(const String& key, unsigned int value) {
    if (!isConnected) return;

    auto doc = makeBaseMessageSerial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_ITEM);
    JsonObject payload = doc.createNestedObject("payload");
    payload["key"] = key;
    payload["value"] = value;

    String jsonString;
    serializeJson(doc, jsonString);

    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_data_item(const String& key, float value) {
    if (!isConnected) return;

    auto doc = makeBaseMessageSerial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_ITEM);
    JsonObject payload = doc.createNestedObject("payload");
    payload["key"] = key;
    payload["value"] = value;

    String jsonString;
    serializeJson(doc, jsonString);

    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_battery_data_item(const String& key, bool value) {
    if (!isConnected) return;

    auto doc = makeBaseMessageSerial<256>(ToSerialMessage::BATTERY_MONITOR_DATA_ITEM);
    JsonObject payload = doc.createNestedObject("payload");
    payload["key"] = key;
    payload["value"] = value;

    String jsonString;
    serializeJson(doc, jsonString);

    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_dino_score(int score) {
    if (!isConnected) return;

    auto doc = makeBaseMessageCommon<256>(ToCommonMessage::DINO_SCORE);
    JsonObject payload = doc.createNestedObject("payload");
    payload["score"] = score;

    String jsonString;
    serializeJson(doc, jsonString);

    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_network_deleted_response(bool success) {
    if (!isConnected) return;

    auto doc = makeBaseMessageSerial<256>(ToSerialMessage::WIFI_DELETED_NETWORK);
    JsonObject payload = doc.createNestedObject("payload");
    payload["status"] = success;

    String jsonString;
    serializeJson(doc, jsonString);

    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::send_pip_turning_off() {
    if (!isConnected) return;

    auto pipTurningOffDoc = makeBaseMessageCommon<256>(ToCommonMessage::PIP_TURNING_OFF);
    JsonObject payload = pipTurningOffDoc.createNestedObject("payload");
    payload["reason"] = "PIP is turning off";

    String jsonString;
    serializeJson(pipTurningOffDoc, jsonString);

    SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
}
