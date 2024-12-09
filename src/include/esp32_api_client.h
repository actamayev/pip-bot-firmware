#pragma once

#include "websocket_manager.h"

class WebSocketManager;

class ESP32ApiClient {
    public:
        static ESP32ApiClient& getInstance() {
            if (instance == nullptr) {
                instance = new ESP32ApiClient();
            }
            return *instance;
        }

        void connectWebSocket();
        void pollWebSocket();
    private:
        static ESP32ApiClient* instance;

        // Private constructor
        ESP32ApiClient();

        // Delete copy constructor and assignment operator
        ESP32ApiClient(const ESP32ApiClient&) = delete;
        ESP32ApiClient& operator=(const ESP32ApiClient&) = delete;
};
