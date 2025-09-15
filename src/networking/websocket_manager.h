#pragma once

#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include "protocol.h"
#include "utils/singleton.h"
#include "message_processor.h"
#include "send_sensor_data.h"
#include "sensors/battery_monitor.h"
#include "firmware_version_tracker.h"
#include "utils/preferences_manager.h"
#include "sensors/sensor_data_buffer.h"
#include "custom_interpreter/bytecode_vm.h"

using namespace websockets;

class WebSocketManager : public Singleton<WebSocketManager> {
    friend class Singleton<WebSocketManager>;
    friend class SendSensorData;

    public:
        void connectToWebSocket();
        void pollWebSocket();

        bool isWsConnected() const { return wsConnected; }
        void sendBatteryMonitorData();
        void sendPipTurningOff();
        void sendDinoScore(int score);
        bool isUserConnectedToThisPip() const { return userConnectedToThisPip; }
        void setIsUserConnectedToThisPip(bool newIsUserConnectedToThisPip);

    private:
        // Make constructor private for singleton
        WebSocketManager();

        websockets::WebsocketsClient wsClient;
        void handleBinaryMessage(WebsocketsMessage message);
        void sendInitialData();

        unsigned long lastPollTime = 0;
        const unsigned long POLL_INTERVAL = 40; // Poll every 40ms

        bool wsConnected = false;
        unsigned long lastConnectionAttempt = 0;
        const unsigned long CONNECTION_INTERVAL = 3000; // 3 seconds between connection attempts    

        void killWiFiProcesses();
        unsigned long lastPingTime = 0;
        // NOTE: The WS_TIMEOUT must be greater than the PING_INTERVAL in SingleESP32Connection.
        const unsigned long WS_TIMEOUT = 2000; // 2 seconds timeout
        bool hasKilledWiFiProcesses = false;
        bool userConnectedToThisPip = false;
};
