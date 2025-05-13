#pragma once

#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include "protocol.h"
#include "utils/singleton.h"
#include "message_processor.h"
#include "send_data_to_server.h"
#include "firmware_version_tracker.h"
#include "custom_interpreter/bytecode_vm.h"
#include "sensors/sensor_polling_manager.h"
#include "utils/preferences_manager.h"

using namespace websockets;

class WebSocketManager : public Singleton<WebSocketManager> {
    friend class Singleton<WebSocketManager>;

    public:
        void connectToWebSocket();
        void pollWebSocket();

        websockets::WebsocketsClient wsClient;
    private:
        // Make constructor private for singleton
        WebSocketManager();

        void handleBinaryMessage(WebsocketsMessage message);
        void sendInitialData();

        unsigned long lastPollTime = 0;
        const unsigned long POLL_INTERVAL = 50; // Poll every 50ms

        bool wsConnected = false;
        unsigned long lastConnectionAttempt = 0;
        const unsigned long CONNECTION_INTERVAL = 3000; // 3 seconds between connection attempts    

        void killWiFiProcesses();
        unsigned long lastPingTime = 0;
        const unsigned long WS_TIMEOUT = 5000; // 5 seconds timeout
        bool hasKilledWiFiProcesses = false;
};
