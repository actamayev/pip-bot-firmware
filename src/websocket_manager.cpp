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
    const bool connectedToWS = wsClient.connect(ws_server_url);
    if (connectedToWS) {
        wsClient.send("Hello from ESP32!");
    } else {
        Serial.println("Failed to connect to WebSocket server.");
    }
}

void WebSocketManager::pollWebSocket() {
    if (wsClient.available()) {
        wsClient.poll();
    }
}

WebSocketManager websocketManager;  // Create global instance
