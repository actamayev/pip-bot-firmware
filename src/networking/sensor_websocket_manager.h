#pragma once

#include <Arduino.h>

#include <ArduinoWebsockets.h>

#include "networking/serial_queue_manager.h"
#include "utils/config.h"
#include "utils/preferences_manager.h"
#include "utils/singleton.h"

using namespace websockets;

class SensorWebSocketManager : public Singleton<SensorWebSocketManager> {
    friend class Singleton<SensorWebSocketManager>;
    friend class SendSensorData;

  public:
    void connect_to_websocket();
    void poll_websocket();

    bool is_ws_connected() const {
        return _wsConnected;
    }

  private:
    SensorWebSocketManager();

    websockets::WebsocketsClient _wsClient;

    uint32_t _lastPollTime = 0;
    const uint32_t POLL_INTERVAL = 5; // Poll every 5ms for fast sends

    bool _wsConnected = false;
    uint32_t _lastConnectionAttempt = 0;
    const uint32_t CONNECTION_INTERVAL = 3000; // 3 seconds between connection attempts

    // No ping/pong tracking - this connection is for sending only
};
