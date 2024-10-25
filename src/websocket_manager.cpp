#include <WiFiClientSecure.h>
#include <ArduinoWebsockets.h>
#include "websocket_manager.h"
#include "config.h"

using namespace websockets;

WebsocketsClient wsClient;
WiFiClientSecure secureClient;

void WebSocketManager::connectToWebSocket() {
    secureClient.setCACert(rootCACertificate);
    wsClient.setCACert(rootCACertificate);

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
                wsClient.pong();
                break;
            case WebsocketsEvent::GotPong:
                Serial.println("WebSocket Pong received.");
                break;
        }
    });

    Serial.println("Attempting to connect to WebSocket Secure (WSS)...");
    const bool connectedToWS = wsClient.connect(ws_server_url);

    if (connectedToWS) {
        wsClient.send("Hello from ESP32!");
        wsClient.ping();
    } else {
        Serial.println("Failed to connect to WebSocket server.");

        if (!secureClient.connect("dev-api.bluedotrobots.com", 443)) {  // Connect to HTTPS server
            Serial.println("HTTPS connection failed.");
        } else {
            Serial.println("Connected to server");
            secureClient.println("GET /health HTTP/1.1");
            secureClient.println("Host: dev-api.bluedotrobots.com");
            secureClient.println("Connection: close");
            secureClient.println();  // Empty line to end the headers

            // Wait for the response
            while (secureClient.connected()) {
                String line = secureClient.readStringUntil('\n');
                if (line == "\r") {
                    Serial.println("Headers received");
                    break;
                }
            }

            // Read the response body
            String responseBody = secureClient.readString();
            Serial.println("Response: " + responseBody);
        }
        secureClient.stop();  // Close the connection
    }
}

void WebSocketManager::pollWebSocket() {
    if (wsClient.available()) {
        wsClient.poll();
    }
}

void WebSocketManager::reconnectWebSocket() {
    wsClient.close();  // Properly close existing connection
    secureClient.stop();  // Clean up secure client
    
    // Reconnect only if WiFi is still connected
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnecting to WebSocket...");
        connectToWebSocket();  // Call the connect method again
    } else {
        Serial.println("Cannot reconnect WebSocket, WiFi is not connected.");
    }
}

WebSocketManager websocketManager;  // Create global instance
