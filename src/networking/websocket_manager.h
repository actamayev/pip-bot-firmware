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
        return _wsConnected;
    }
    static void send_battery_monitor_data();
    static void send_pip_turning_off();
    static void send_dino_score(int score);
    bool is_user_connected_to_this_pip() const {
        return _userConnectedToThisPip;
    }
    static void set_is_user_connected_to_this_pip(bool new_is_user_connected_to_this_pip);

  private:
    // Make constructor private for singleton
    WebSocketManager();

    websockets::WebsocketsClient _wsClient;
    static void handle_binary_message(WebsocketsMessage message);
    static void send_initial_data();
    static void add_battery_data_to_payload(JsonObject& payload);

    uint32_t _lastPollTime = 0;
    const uint32_t POLL_INTERVAL = 40; // Poll every 40ms

    bool _wsConnected = false;
    uint32_t _lastConnectionAttempt = 0;
    const uint32_t CONNECTION_INTERVAL = 3000; // 3 seconds between connection attempts

    static void kill_wifi_processes();
    uint32_t _lastPingTime = 0;
    // NOTE: The WS_TIMEOUT must be greater than the PING_INTERVAL in SingleESP32Connection.
    const uint32_t WS_TIMEOUT = 3000; // 3 seconds timeout
    bool _hasKilledWiFiProcesses = false;
    bool _userConnectedToThisPip = false;
};
