#include "command_websocket_manager.h"

#include "actuators/display_screen.h"
#include "networking/send_sensor_data.h"
#include "utils/config.h"
#include "utils/structs.h"

CommandWebSocketManager::CommandWebSocketManager() {
    _wsConnected = false;
    _lastConnectionAttempt = 0;
    String pip_id = PreferencesManager::get_instance().get_pip_id();
    _wsClient.addHeader("X-Pip-Id", pip_id);
    if (DEFAULT_ENVIRONMENT == "local") {
        return;
    }
    _wsClient.setCACert(ROOT_CA_CERTIFICATE);
}

void CommandWebSocketManager::handle_binary_message(WebsocketsMessage message) {
    const auto* data = reinterpret_cast<const uint8_t*>(message.c_str());
    const uint16_t LENGTH = message.length();

    // Check if this is a framed message (starts with START_MARKER)
    if (LENGTH >= 4 && data[0] == START_MARKER) {
        const uint8_t MESSAGE_TYPE = data[1];
        const bool USE_LONG_FORMAT = (data[2] != 0);

        uint16_t payload_length = 0;
        uint16_t header_size = 0;

        if (USE_LONG_FORMAT) {
            payload_length = data[3] | (data[4] << 8);
            header_size = 5;
        } else {
            payload_length = data[3];
            header_size = 4;
        }

        if (LENGTH == header_size + payload_length + 1 && data[header_size + payload_length] == END_MARKER) {
            auto* processed_data = new uint8_t[payload_length + 1];
            processed_data[0] = MESSAGE_TYPE;

            if (payload_length > 0) {
                memcpy(processed_data + 1, data + header_size, payload_length);
            }

            MessageProcessor::get_instance().process_binary_message(processed_data, payload_length + 1);

            delete[] processed_data;
        } else {
            SerialQueueManager::get_instance().queue_message("[WS_CMD] Invalid framed message");
        }
    }
}

void CommandWebSocketManager::connect_to_websocket() {
    _wsClient.onMessage([this](WebsocketsMessage message) { this->handle_binary_message(message); });

    _wsClient.onEvent([this](WebsocketsEvent event, String data) {
        switch (event) {
            case WebsocketsEvent::ConnectionOpened:
                SerialQueueManager::get_instance().queue_message("[WS_CMD] Connection opened");
                this->_wsConnected = true;
                this->_hasKilledWiFiProcesses = false;
                this->_lastPingTime = millis();
                this->send_initial_data();
                break;
            case WebsocketsEvent::ConnectionClosed:
                SerialQueueManager::get_instance().queue_message("[WS_CMD] Connection closed");
                kill_wifi_processes();
                this->_wsConnected = false;
                break;
            case WebsocketsEvent::GotPing:
                SerialQueueManager::get_instance().queue_message("[WS_CMD] Got ping");
                this->_lastPingTime = millis();
                break;
            case WebsocketsEvent::GotPong:
                SerialQueueManager::get_instance().queue_message("[WS_CMD] Got pong");
                this->_lastPingTime = millis();
                break;
        }
    });

    _lastConnectionAttempt = 0;
}

void CommandWebSocketManager::add_battery_data_to_payload(JsonObject& payload) {
    const BatteryState& battery_state = BatteryMonitor::get_instance().get_battery_state();
    if (!battery_state.isInitialized) {
        return;
    }

    JsonObject battery_data = payload.createNestedObject("batteryData");
    battery_data["stateOfCharge"] = static_cast<int>(round(battery_state.displayedStateOfCharge));
    battery_data["voltage"] = battery_state.voltage;
    battery_data["current"] = battery_state.current;
    battery_data["power"] = battery_state.power;
    battery_data["remainingCapacity"] = battery_state.remainingCapacity;
    battery_data["fullCapacity"] = battery_state.fullCapacity;
    battery_data["health"] = battery_state.health;
    battery_data["isCharging"] = battery_state.isCharging;
    battery_data["isDischarging"] = battery_state.isDischarging;
    battery_data["isLowBattery"] = battery_state.isLowBattery;
    battery_data["isCriticalBattery"] = battery_state.isCriticalBattery;
    battery_data["estimatedTimeToEmpty"] = battery_state.estimatedTimeToEmpty;
    battery_data["estimatedTimeToFull"] = battery_state.estimatedTimeToFull;
}

void CommandWebSocketManager::send_initial_data() {
    SerialQueueManager::get_instance().queue_message("[WS_CMD] Sending initial data...");
    auto init_doc = make_base_message_server<256>(ToServerMessage::DEVICE_INITIAL_DATA);
    JsonObject payload = init_doc.createNestedObject("payload");
    payload["firmwareVersion"] = FirmwareVersionTracker::get_instance().get_firmware_version();

    add_battery_data_to_payload(payload);

    String json_string;
    serializeJson(init_doc, json_string);
    WiFiClass::mode(WIFI_STA);
    CommandWebSocketManager::get_instance()._wsClient.send(json_string);
}

void CommandWebSocketManager::send_battery_monitor_data() {
    CommandWebSocketManager& instance = CommandWebSocketManager::get_instance();
    if (!instance._wsConnected) {
        return;
    }

    auto battery_doc = make_base_message_server<256>(ToServerMessage::BATTERY_MONITOR_DATA_FULL);
    JsonObject payload = battery_doc.createNestedObject("payload");
    add_battery_data_to_payload(payload);

    String json_string;
    serializeJson(battery_doc, json_string);
    instance._wsClient.send(json_string);
}

void CommandWebSocketManager::poll_websocket() {
    CommandWebSocketManager& instance = CommandWebSocketManager::get_instance();
    const uint32_t CURRENT_TIME = millis();
    if (CURRENT_TIME - instance._lastPollTime < POLL_INTERVAL) {
        return;
    }

    instance._lastPollTime = CURRENT_TIME;

    if (WiFiClass::status() != WL_CONNECTED) {
        if (instance._wsConnected) {
            SerialQueueManager::get_instance().queue_message("[WS_CMD] WiFi lost", SerialPriority::HIGH_PRIO);
            instance._wsConnected = false;
            kill_wifi_processes();
        }
        return;
    }

    if (instance._wsConnected && (CURRENT_TIME - instance._lastPingTime >= WS_TIMEOUT)) {
        SerialQueueManager::get_instance().queue_message("[WS_CMD] Ping timeout", SerialPriority::HIGH_PRIO);
        instance._wsConnected = false;
        kill_wifi_processes();
    }

    if (!instance._wsConnected && (CURRENT_TIME - instance._lastConnectionAttempt >= CONNECTION_INTERVAL)) {
        instance._lastConnectionAttempt = CURRENT_TIME;

        SerialQueueManager::get_instance().queue_message("[WS_CMD] Attempting to connect...");

        if (!instance._wsClient.connect(get_ws_server_url())) {
            SerialQueueManager::get_instance().queue_message("[WS_CMD] Connection failed");
        } else {
            SerialQueueManager::get_instance().queue_message("[WS_CMD] Connected successfully");
            instance._wsConnected = true;
        }
        return;
    }

    if (!instance._wsConnected) {
        return;
    }

    try {
        instance._wsClient.poll();
    } catch (const std::exception& e) {
        instance._wsConnected = false;
    }
}

void CommandWebSocketManager::kill_wifi_processes() {
    CommandWebSocketManager& instance = CommandWebSocketManager::get_instance();
    if (instance._hasKilledWiFiProcesses) {
        return;
    }

    career_quest_triggers.stop_all_career_quest_triggers(false);
    motor_driver.reset_command_state(false);

    SendSensorData::get_instance().set_send_sensor_data(false);
    SendSensorData::get_instance().set_send_multizone_data(false);
    SendSensorData::get_instance().set_euler_data_enabled(false);
    SendSensorData::get_instance().set_side_tof_data_enabled(false);
    SendSensorData::get_instance().set_accel_data_enabled(false);
    SendSensorData::get_instance().set_color_sensor_data_enabled(false);
    SendSensorData::get_instance().set_encoder_data_enabled(false);

    instance._hasKilledWiFiProcesses = true;
    instance._userConnectedToThisPip = false;
}

void CommandWebSocketManager::send_pip_turning_off() {
    CommandWebSocketManager& instance = CommandWebSocketManager::get_instance();
    if (!instance._wsConnected) {
        return;
    }

    auto pip_turning_off_doc = make_base_message_common<256>(ToCommonMessage::PIP_TURNING_OFF);
    JsonObject payload = pip_turning_off_doc.createNestedObject("payload");
    payload["reason"] = "Pip is turning off";
    String json_string;
    serializeJson(pip_turning_off_doc, json_string);
    instance._wsClient.send(json_string);
}

void CommandWebSocketManager::send_dino_score(int score) {
    CommandWebSocketManager& instance = CommandWebSocketManager::get_instance();
    if (!instance._wsConnected) {
        return;
    }

    auto doc = make_base_message_common<256>(ToCommonMessage::DINO_SCORE);
    JsonObject payload = doc.createNestedObject("payload");
    payload["score"] = score;

    String json_string;
    serializeJson(doc, json_string);
    instance._wsClient.send(json_string);
}

void CommandWebSocketManager::set_is_user_connected_to_this_pip(bool new_is_user_connected_to_this_pip) {
    CommandWebSocketManager& instance = CommandWebSocketManager::get_instance();
    instance._userConnectedToThisPip = new_is_user_connected_to_this_pip;
    if (new_is_user_connected_to_this_pip) {
        return;
    }
    motor_driver.reset_command_state(true);
    career_quest_triggers.stop_all_career_quest_triggers(false);
}
