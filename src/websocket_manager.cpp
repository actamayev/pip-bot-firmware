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

    wsClient.onEvent([](WebsocketsEvent event, String data) {
        switch (event) {
            case WebsocketsEvent::ConnectionOpened:
                Serial.println("WebSocket connected.");
                break;
            case WebsocketsEvent::ConnectionClosed:
                Serial.println("WebSocket disconnected.");
                break;
            case WebsocketsEvent::GotPing:
                Serial.println("WebSocket Ping received.");
                break;
            case WebsocketsEvent::GotPong:
                Serial.println("WebSocket Pong received.");
                break;
        }
    });

    Serial.println("Connecting to WebSocket server...");
    const bool connectedToWS = wsClient.connect(ws_server_url);

    if (connectedToWS) {
        wsClient.send("Hello from ESP32!");
        Serial.println("WebSocket connected.");
    } else {
        Serial.println("Failed to connect to WebSocket server.");
    }
}

void WebSocketManager::pollWebSocket() {
    if (wsClient.available()) {
        wsClient.poll();
    }
}

void WebSocketManager::reconnectWebSocket() {
    // Reconnect only if WiFi is still connected
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnecting to WebSocket...");
        connectToWebSocket();  // Call the connect method again
    } else {
        Serial.println("Cannot reconnect WebSocket, WiFi is not connected.");
    }
}

WebSocketManager websocketManager;  // Create global instance
