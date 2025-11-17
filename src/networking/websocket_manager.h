#pragma once

#include <Arduino.h>

#include <ArduinoWebsockets.h>

#include "custom_interpreter/bytecode_vm.h"
#include "firmware_version_tracker.h"
#include "message_processor.h"
#include "protocol.h"
#include "send_sensor_data.h"
#include "sensors/battery_monitor.h"
#include "sensors/sensor_data_buffer.h"
#include "utils/preferences_manager.h"
#include "utils/singleton.h"

using namespace websockets;

class WebSocketManager : public Singleton<WebSocketManager> {
    friend class Singleton<WebSocketManager>;
    friend class SendSensorData;

  public:
    void connect_to_websocket();
    void poll_websocket();

    bool is_ws_connected() const {
        return wsConnected;
    }
    void send_battery_monitor_data();
    void send_pip_turning_off();
    void send_dino_score(int score);
    bool is_user_connected_to_this_pip() const {
        return userConnectedToThisPip;
    }
    void set_is_user_connected_to_this_pip(bool newIsUserConnectedToThisPip);

  private:
    // Make constructor private for singleton
    WebSocketManager();

    websockets::WebsocketsClient wsClient;
    void handle_binary_message(WebsocketsMessage message);
    void send_initial_data();
    void add_battery_data_to_payload(JsonObject& payload);

    uint32_t lastPollTime = 0;
    const uint32_t POLL_INTERVAL = 40; // Poll every 40ms

    bool wsConnected = false;
    uint32_t lastConnectionAttempt = 0;
    const uint32_t CONNECTION_INTERVAL = 3000; // 3 seconds between connection attempts

    void kill_wifi_processes();
    uint32_t lastPingTime = 0;
    // NOTE: The WS_TIMEOUT must be greater than the PING_INTERVAL in SingleESP32Connection.
    const uint32_t WS_TIMEOUT = 3000; // 3 seconds timeout
    bool hasKilledWiFiProcesses = false;
    bool userConnectedToThisPip = false;
};
