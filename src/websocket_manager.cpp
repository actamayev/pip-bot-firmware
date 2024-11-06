#include <string>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "config.h"
#include "websocket_manager.h"

using namespace websockets;

// TODO: This isn't correctly 
WebSocketManager::WebSocketManager() {
    // Serial.println(
    //     environment == Environment::LocalDev ? "LocalDev" :
    //     environment == Environment::Staging ? "Staging" :
    //     environment == Environment::Production ? "Production" :
    //     "Unknown"
    // );
    Serial.println("WebSocketManager constructor called");
    if (environment == Environment::LocalDev) return;
    Serial.println("Setting up secure WebSocket");
    wsClient.setCACert(rootCACertificate);
};

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

    Serial.println("Attempting to connect to WebSocket Secure (WSS)...");
    Serial.println(getWsServerUrl());
    const bool connectedToWS = wsClient.connect(getWsServerUrl());

    if (!connectedToWS) {
        Serial.println("WebSocket connection failed.");
        return;
    }
    // Send initial message after successful connection
    Serial.println("WebSocket connected. Sending initial data...");

    JsonDocument jsonDoc;

    jsonDoc["pipUUID"] = pip_uuid;

    char jsonBuffer[200];
    serializeJson(jsonDoc, jsonBuffer);

    wsClient.send(jsonBuffer);  // Send custom JSON message
    wsClient.send("Hello from ESP32!");  // Send additional welcome message
    wsClient.ping();  // Optionally, send a ping
}

void WebSocketManager::pollWebSocket() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (wsClient.available()) {
        wsClient.poll();
    }
}

void WebSocketManager::reconnectWebSocket() {
    wsClient.close();  // Properly close existing connection

    // Reconnect only if WiFi is still connected
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnecting to WebSocket...");
        connectToWebSocket();  // Call the connect method again
    } else {
        Serial.println("Cannot reconnect WebSocket, WiFi is not connected.");
    }
}
