#include "websocket_manager.h"

#include "actuators/display_screen.h"
#include "networking/send_sensor_data.h"
#include "utils/config.h"
#include "utils/structs.h"

WebSocketManager::WebSocketManager() {
    wsConnected = false;
    lastConnectionAttempt = 0;
    String pipId = PreferencesManager::get_instance().getPipId();
    wsClient.addHeader("X-Pip-Id", pipId);
    if (DEFAULT_ENVIRONMENT == "local") return;
    wsClient.setCACert(ROOT_CA_CERTIFICATE);
}

void WebSocketManager::handle_binary_message(WebsocketsMessage message) {
    const uint8_t* data = (const uint8_t*)message.c_str();
    uint16_t length = message.length();

    // Check if this is a framed message (starts with START_MARKER)
    if (length >= 4 && data[0] == START_MARKER) {
        // This is a framed message. Parse it.
        uint8_t messageType = data[1];
        bool useLongFormat = (data[2] != 0);

        uint16_t payloadLength;
        uint16_t headerSize;

        if (useLongFormat) {
            // 16-bit length
            payloadLength = data[3] | (data[4] << 8); // Little-endian
            headerSize = 5;                           // START + TYPE + FORMAT + LENGTH(2)
        } else {
            // 8-bit length
            payloadLength = data[3];
            headerSize = 4; // START + TYPE + FORMAT + LENGTH(1)
        }

        // Verify end marker and total length
        if (length == headerSize + payloadLength + 1 && data[headerSize + payloadLength] == END_MARKER) {
            // Extract just the message type and payload
            uint8_t* processedData = new uint8_t[payloadLength + 1];
            processedData[0] = messageType; // Message type

            // Copy the actual payload (if any)
            if (payloadLength > 0) {
                memcpy(processedData + 1, data + headerSize, payloadLength);
            }

            // Process the extracted message
            MessageProcessor::get_instance().process_binary_message(processedData, payloadLength + 1);

            delete[] processedData;
        } else {
            SerialQueueManager::get_instance().queue_message("Invalid framed message (bad end marker or length)");
        }
    }
}

void WebSocketManager::connect_to_websocket() {
    wsClient.onMessage([this](WebsocketsMessage message) { this->handle_binary_message(message); });

    wsClient.onEvent([this](WebsocketsEvent event, String data) {
        switch (event) {
            case WebsocketsEvent::ConnectionOpened:
                this->wsConnected = true;
                this->hasKilledWiFiProcesses = false; // Reset the flag
                this->lastPingTime = millis();        // Initialize ping time
                this->send_initial_data();
                break;
            case WebsocketsEvent::ConnectionClosed:
                SerialQueueManager::get_instance().queue_message("WebSocket disconnected");
                kill_wifi_processes();
                this->wsConnected = false;
                break;
            case WebsocketsEvent::GotPing:
                SerialQueueManager::get_instance().queue_message("Got ping");
                this->lastPingTime = millis(); // Update ping time
                break;
            case WebsocketsEvent::GotPong:
                SerialQueueManager::get_instance().queue_message("Got pong");
                // TODO 11/13/25: Should this be lastPingTime?
                this->lastPingTime = millis(); // Update ping time
                break;
        }
    });

    lastConnectionAttempt = 0; // Force an immediate connection attempt in the next poll
}

void WebSocketManager::add_battery_data_to_payload(JsonObject& payload) {
    const BatteryState& batteryState = BatteryMonitor::get_instance().get_battery_state();
    if (!batteryState.isInitialized) return;

    JsonObject batteryData = payload.createNestedObject("batteryData");
    batteryData["stateOfCharge"] = static_cast<int>(round(batteryState.displayedStateOfCharge));
    batteryData["voltage"] = batteryState.voltage;
    batteryData["current"] = batteryState.current;
    batteryData["power"] = batteryState.power;
    batteryData["remainingCapacity"] = batteryState.remainingCapacity;
    batteryData["fullCapacity"] = batteryState.fullCapacity;
    batteryData["health"] = batteryState.health;
    batteryData["isCharging"] = batteryState.isCharging;
    batteryData["isDischarging"] = batteryState.isDischarging;
    batteryData["isLowBattery"] = batteryState.isLowBattery;
    batteryData["isCriticalBattery"] = batteryState.isCriticalBattery;
    batteryData["estimatedTimeToEmpty"] = batteryState.estimatedTimeToEmpty;
    batteryData["estimatedTimeToFull"] = batteryState.estimatedTimeToFull;
}

void WebSocketManager::send_initial_data() {
    SerialQueueManager::get_instance().queue_message("WebSocket connected. Sending initial data...");
    auto initDoc = makeBaseMessageServer<256>(ToServerMessage::DEVICE_INITIAL_DATA);
    JsonObject payload = initDoc.createNestedObject("payload");
    payload["firmwareVersion"] = FirmwareVersionTracker::get_instance().get_firmware_version();

    // Add battery data to the same request
    add_battery_data_to_payload(payload);

    String jsonString;
    serializeJson(initDoc, jsonString);
    WiFi.mode(WIFI_STA);
    wsClient.send(jsonString);
}

void WebSocketManager::send_battery_monitor_data() {
    if (!wsConnected) return;

    auto batteryDoc = makeBaseMessageServer<256>(ToServerMessage::BATTERY_MONITOR_DATA_FULL);
    JsonObject payload = batteryDoc.createNestedObject("payload");
    add_battery_data_to_payload(payload);

    String jsonString;
    serializeJson(batteryDoc, jsonString);
    wsClient.send(jsonString);
}

void WebSocketManager::poll_websocket() {
    unsigned long currentTime = millis();
    if (currentTime - lastPollTime < POLL_INTERVAL) return;

    lastPollTime = currentTime;

    if (WiFi.status() != WL_CONNECTED) {
        if (wsConnected) {
            SerialQueueManager::get_instance().queue_message("WiFi lost during WebSocket session", SerialPriority::HIGH_PRIO);
            wsConnected = false;
            kill_wifi_processes();
        }
        return;
    }

    if (wsConnected && (currentTime - lastPingTime >= WS_TIMEOUT)) {
        SerialQueueManager::get_instance().queue_message("WebSocket ping timeout - connection lost", SerialPriority::HIGH_PRIO);
        wsConnected = false;
        kill_wifi_processes();
    }

    // Connection management - try to connect if not connected
    if (!wsConnected && (currentTime - lastConnectionAttempt >= CONNECTION_INTERVAL)) {
        lastConnectionAttempt = currentTime;

        SerialQueueManager::get_instance().queue_message("Attempting to connect to WebSocket...");

        if (!wsClient.connect(getWsServerUrl())) {
            SerialQueueManager::get_instance().queue_message("WebSocket connection failed. Will try again in 3 seconds");
        } else {
            SerialQueueManager::get_instance().queue_message("WebSocket connected successfully");
            wsConnected = true;
        }
        return;
    }

    // Only poll if connected
    if (!wsConnected) return;
    try {
        wsClient.poll();
    } catch (const std::exception& e) {
        // SerialQueueManager::get_instance().queueMessage("Error during WebSocket poll: %s\n", e.what());
        wsConnected = false; // Mark as disconnected to trigger reconnect
    }
}

void WebSocketManager::kill_wifi_processes() {
    // This method activates when the ESP has been disconnected from WS.
    // Should only run once.
    if (hasKilledWiFiProcesses) return;
    careerQuestTriggers.stop_all_career_quest_triggers(false);
    motorDriver.reset_command_state(false);

    // Stop all sensor data transmission to reduce network load
    SendSensorData::get_instance().set_send_sensor_data(false);
    SendSensorData::get_instance().set_send_multizone_data(false);
    SendSensorData::get_instance().set_euler_data_enabled(false);
    SendSensorData::get_instance().set_side_tof_data_enabled(false);
    SendSensorData::get_instance().set_accel_data_enabled(false);
    SendSensorData::get_instance().set_color_sensor_data_enabled(false);
    SendSensorData::get_instance().set_encoder_data_enabled(false);

    hasKilledWiFiProcesses = true;
    userConnectedToThisPip = false;
}

void WebSocketManager::send_pip_turning_off() {
    if (!wsConnected) return;
    auto pipTurningOffDoc = makeBaseMessageCommon<256>(ToCommonMessage::PIP_TURNING_OFF);
    JsonObject payload = pipTurningOffDoc.createNestedObject("payload");
    payload["reason"] = "Pip is turning off";
    String jsonString;
    serializeJson(pipTurningOffDoc, jsonString);
    wsClient.send(jsonString);
}

void WebSocketManager::send_dino_score(int score) {
    if (!wsConnected) return;

    auto doc = makeBaseMessageCommon<256>(ToCommonMessage::DINO_SCORE);
    JsonObject payload = doc.createNestedObject("payload");
    payload["score"] = score;

    String jsonString;
    serializeJson(doc, jsonString);
    wsClient.send(jsonString);
}

void WebSocketManager::set_is_user_connected_to_this_pip(bool newIsUserConnectedToThisPip) {
    userConnectedToThisPip = newIsUserConnectedToThisPip;
    if (newIsUserConnectedToThisPip) return;
    motorDriver.reset_command_state(true);
    careerQuestTriggers.stop_all_career_quest_triggers(false);
}
