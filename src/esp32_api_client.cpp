#include "esp32_api_client.h"

ESP32ApiClient apiClient;

// Constructor implementation
ESP32ApiClient::ESP32ApiClient()
    : httpClient(),   // Use global cert from config.h
      wsManager(),                     // Use default constructor for WebSocketManager
      authService(httpClient) {
        Serial.println("here");
    }

void ESP32ApiClient::connectWebSocket() {
    wsManager.connectToWebSocket();
}

// WebSocket polling function
void ESP32ApiClient::pollWebSocket() {
    wsManager.pollWebSocket();
}
