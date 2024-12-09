#include "./include/esp32_api_client.h"
#include "./include/websocket_manager.h"  // Include this after the forward declaration

ESP32ApiClient* ESP32ApiClient::instance = nullptr;

// Constructor implementation
ESP32ApiClient::ESP32ApiClient() {
    Serial.println("here");
}

void ESP32ApiClient::connectWebSocket() {
    WebSocketManager::getInstance().connectToWebSocket();
}

// WebSocket polling function
void ESP32ApiClient::pollWebSocket() {
    WebSocketManager::getInstance().pollWebSocket();
}
