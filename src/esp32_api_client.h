#ifndef ESP32_API_CLIENT_H
#define ESP32_API_CLIENT_H

#include "http_client.h"
#include "websocket_manager.h"
#include "auth_service.h"  // Assuming you have an AuthService class

class ESP32ApiClient {
    private:
        HttpClient httpClient;
        WebSocketManager wsManager;
        AuthService authService;

    public:
        // Constructor
        ESP32ApiClient();

        void connectWebSocket();  // Initiates WebSocket connection
        void pollWebSocket();     // Polls WebSocket for activity
};

extern ESP32ApiClient apiClient;

#endif
