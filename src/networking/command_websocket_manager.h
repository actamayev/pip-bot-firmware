#pragma once

#include <Arduino.h>

#include <ArduinoWebsockets.h>

#include "custom_interpreter/bytecode_vm.h"
#include "firmware_version_tracker.h"
#include "message_processor.h"
#include "protocol.h"
#include "sensors/battery_monitor.h"
#include "utils/preferences_manager.h"
#include "utils/singleton.h"

using namespace websockets;

class CommandWebSocketManager : public Singleton<CommandWebSocketManager> {
    friend class Singleton<CommandWebSocketManager>;

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

    websockets::WebsocketsClient _wsClient; // Made public for binary message sending

  private:
    CommandWebSocketManager();

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
    const uint32_t WS_TIMEOUT = 3000; // 3 seconds timeout
    bool _hasKilledWiFiProcesses = false;
    bool _userConnectedToThisPip = false;
};
