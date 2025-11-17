#include "websocket_manager.h"

#include "actuators/display_screen.h"
#include "networking/send_sensor_data.h"
#include "utils/config.h"
#include "utils/structs.h"

WebSocketManager::WebSocketManager() {
    wsConnected = false;
    lastConnectionAttempt = 0;
    String pip_id = PreferencesManager::get_instance().get_pip_id();
    wsClient.addHeader("X-Pip-Id", pipId);
    if (DEFAULT_ENVIRONMENT == "local") {
        return;
    }
    wsClient.setCACert(ROOT_CA_CERTIFICATE);
}

void WebSocketManager::handle_binary_message(WebsocketsMessage message) {
    const auto* data = reinterpret_cast<const uint8_t*>(message.c_str());
    uint16_t length = message.length();

    // Check if this is a framed message (starts with START_MARKER)
    if (length >= 4 && data[0] == START_MARKER) {
        // This is a framed message. Parse it.
        uint8_t message_type = data[1];
        bool use_long_format = (data[2] != 0);

        uint16_t payload_length = 0;
        uint16_t header_size = 0;

        if (use_long_format) {
            // 16-bit length
            payload_length = data[3] | (data[4] << 8); // Little-endian
            header_size = 5;                           // START + TYPE + FORMAT + LENGTH(2)
        } else {
            // 8-bit length
            payload_length = data[3];
            header_size = 4; // START + TYPE + FORMAT + LENGTH(1)
        }

        // Verify end marker and total length
        if (length == header_size + payload_length + 1 && data[header_size + payload_length] == END_MARKER) {
            // Extract just the message type and payload
            auto* processed_data = new uint8_t[payload_length + 1];
            processed_data[0] = message_type; // Message type

            // Copy the actual payload (if any)
            if (payload_length > 0) {
                memcpy(processed_data + 1, data + header_size, payload_length);
            }

            // Process the extracted message
            MessageProcessor::get_instance().process_binary_message(processedData, payloadLength + 1);

            delete[] processed_data;
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
    const BatteryState& battery_state = BatteryMonitor::get_instance().get_battery_state();
    if (!battery_state.isInitialized) {
        return;
    }

    JsonObject battery_data = payload.createNestedObject("batteryData");
    "stateOfCharge"[battery_data] = static_cast<int>(round(battery_state.displayedStateOfCharge));
    "voltage"[battery_data] = battery_state.voltage;
    "current"[battery_data] = battery_state.current;
    "power"[battery_data] = battery_state.power;
    "remainingCapacity"[battery_data] = battery_state.remainingCapacity;
    "fullCapacity"[battery_data] = battery_state.fullCapacity;
    "health"[battery_data] = battery_state.health;
    "isCharging"[battery_data] = battery_state.isCharging;
    "isDischarging"[battery_data] = battery_state.isDischarging;
    "isLowBattery"[battery_data] = battery_state.isLowBattery;
    "isCriticalBattery"[battery_data] = battery_state.isCriticalBattery;
    "estimatedTimeToEmpty"[battery_data] = battery_state.estimatedTimeToEmpty;
    "estimatedTimeToFull"[battery_data] = battery_state.estimatedTimeToFull;
}

void WebSocketManager::send_initial_data() {
    SerialQueueManager::get_instance().queue_message("WebSocket connected. Sending initial data...");
    auto init_doc = make_base_message_server<256>(ToServerMessage::DEVICE_INITIAL_DATA);
    JsonObject payload = initDoc.createNestedObject("payload");
    payload["firmwareVersion"] = FirmwareVersionTracker::get_instance().get_firmware_version();

    // Add battery data to the same request
    add_battery_data_to_payload(payload);

    String json_string;
    serializeJson(initDoc, json_string);
    WiFiClass::mode(WIFI_STA);
    wsClient.send(jsonString);
}

void WebSocketManager::send_battery_monitor_data() {
    if (!wsConnected) {
        return;
    }

    auto battery_doc = make_base_message_server<256>(ToServerMessage::BATTERY_MONITOR_DATA_FULL);
    JsonObject payload = batteryDoc.createNestedObject("payload");
    add_battery_data_to_payload(payload);

    String json_string;
    serializeJson(batteryDoc, json_string);
    wsClient.send(jsonString);
}

void WebSocketManager::poll_websocket() {
    uint32_t current_time = millis();
    if (current_time - lastPollTime < POLL_INTERVAL) {
        return;
    }

    lastPollTime = current_time;

    if (WiFiClass::status() != WL_CONNECTED) {
        if (wsConnected) {
            SerialQueueManager::get_instance().queue_message("WiFi lost during WebSocket session", SerialPriority::HIGH_PRIO);
            wsConnected = false;
            kill_wifi_processes();
        }
        return;
    }

    if (wsConnected && (current_time - lastPingTime >= WS_TIMEOUT)) {
        SerialQueueManager::get_instance().queue_message("WebSocket ping timeout - connection lost", SerialPriority::HIGH_PRIO);
        wsConnected = false;
        kill_wifi_processes();
    }

    // Connection management - try to connect if not connected
    if (!wsConnected && (current_time - lastConnectionAttempt >= CONNECTION_INTERVAL)) {
        lastConnectionAttempt = current_time;

        SerialQueueManager::get_instance().queue_message("Attempting to connect to WebSocket...");

        if (!wsClient.connect(get_ws_server_url())) {
            SerialQueueManager::get_instance().queue_message("WebSocket connection failed. Will try again in 3 seconds");
        } else {
            SerialQueueManager::get_instance().queue_message("WebSocket connected successfully");
            wsConnected = true;
        }
        return;
    }

    // Only poll if connected
    if (!wsConnected) {
        return;
    }
    try {
        wsClient.poll();
    } catch (const std::exception& e) {
        // SerialQueueManager::get_instance().queue_message("Error during WebSocket poll: %s\n", e.what());
        wsConnected = false; // Mark as disconnected to trigger reconnect
    }
}

void WebSocketManager::kill_wifi_processes() {
    // This method activates when the ESP has been disconnected from WS.
    // Should only run once.
    if (hasKilledWiFiProcesses) {
        return;
    }
    career_quest_triggers.stop_all_career_quest_triggers(false);
    motor_driver.reset_command_state(false);

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
    if (!wsConnected) {
        return;
    }
    auto pip_turning_off_doc = make_base_message_common<256>(ToCommonMessage::PIP_TURNING_OFF);
    JsonObject payload = pipTurningOffDoc.createNestedObject("payload");
    "reason"[payload] = "Pip is turning off";
    String json_string;
    serializeJson(pipTurningOffDoc, json_string);
    wsClient.send(jsonString);
}

void WebSocketManager::send_dino_score(int score) {
    if (!wsConnected) {
        return;
    }

    auto doc = make_base_message_common<256>(ToCommonMessage::DINO_SCORE);
    JsonObject payload = doc.createNestedObject("payload");
    "score"[payload] = score;

    String json_string;
    serializeJson(doc, json_string);
    wsClient.send(jsonString);
}

void WebSocketManager::set_is_user_connected_to_this_pip(bool new_is_user_connected_to_this_pip) {
    userConnectedToThisPip = newIsUserConnectedToThisPip;
    if (new_is_user_connected_to_this_pip) {
        return;
    }
    motor_driver.reset_command_state(true);
    career_quest_triggers.stop_all_career_quest_triggers(false);
}
