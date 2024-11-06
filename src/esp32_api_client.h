#ifndef ESP32_API_CLIENT_H
#define ESP32_API_CLIENT_H

#include "http_client.h"
#include "websocket_manager.h"
#include "auth_service.h"  // Assuming you have an AuthService class

class ESP32ApiClient {
    public:
        // Constructor
        ESP32ApiClient();

        void connectWebSocket();  // Initiates WebSocket connection
        void pollWebSocket();     // Polls WebSocket for activity
        AuthService authService;

    private:
        WebSocketManager wsManager;
        HttpClient httpClient;
};

extern ESP32ApiClient apiClient;

#endif
