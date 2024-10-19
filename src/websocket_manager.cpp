#include "websocket_manager.h"
#include "config.h"
#include <ArduinoWebsockets.h>
using namespace websockets;

WebsocketsClient wsClient;

void WebSocketManager::connectToWebSocket() {
    wsClient.onMessage([](WebsocketsMessage message) {
        Serial.print("Received message: ");
        Serial.println(message.data());
    });

    Serial.println("Connecting to WebSocket server...");
    if (wsClient.connect(ws_server_url)) {
        wsClient.send("Hello from ESP32!");
    } else {
        Serial.println("Failed to connect to WebSocket server.");
    }
}

void WebSocketManager::reconnectWebSocket() {
    if (!wsClient.available()) {
        Serial.println("Reconnecting WebSocket...");
        connectToWebSocket();
    }
}

void WebSocketManager::pollWebSocket() {
    if (wsClient.available()) {
        wsClient.poll();  // Only poll if connected
    }
}

WebSocketManager websocketManager;  // Create global instance
