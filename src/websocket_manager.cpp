#include <string>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "./include/config.h"
#include "./include/websocket_manager.h"

WebSocketManager::WebSocketManager() {
    if (environment == Environment::LocalDev) return;
    wsClient.setCACert(rootCACertificate);
}

bool WebSocketManager::decodeBase64(const char* input, uint8_t* output, size_t* outputLength) {
    size_t inputLength = strlen(input);
    size_t written = 0;
    
    int result = mbedtls_base64_decode(
        output,
        *outputLength,
        &written,
        (const unsigned char*)input,
        inputLength
    );
    
    if (result == 0) {
        *outputLength = written;
        return true;
    }
    return false;
}

void WebSocketManager::handleBinaryUpdate(const uint8_t* data, size_t len) {
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Serial.println("Not enough space to begin OTA");
        return;
    }

    // Convert const uint8_t* to uint8_t* as required by Update.write
    uint8_t* writeData = const_cast<uint8_t*>(data);
    
    // Write data in chunks
    size_t written = Update.write(writeData, len);
    if (written != len) {
        Serial.println("Error during OTA update");
        Serial.printf("Written %d bytes out of %d\n", written, len);
        Update.abort();
        return;
    }

    if (!Update.end()) {
        Serial.printf("Error finishing update: %s\n", Update.errorString());
        return;
    }

    Serial.println("OTA update successful, restarting...");
    ESP.restart();
}

void WebSocketManager::handleMessage(WebsocketsMessage message) {
    if (message.isText()) {
        Serial.print("Received message: ");
        Serial.println(message.data());

        // Parse JSON message
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message.data());

        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.f_str());
            return;
        }

        // Check if this is a firmware update message
        if (doc["event"] == "new-user-code") {
            const char* base64Data = doc["data"];
            
            // Calculate maximum decoded length (approximate)
            size_t maxDecodedLength = strlen(base64Data) * 3 / 4;
            uint8_t* decodedData = new uint8_t[maxDecodedLength];
            
            // Actual decoded length will be stored here
            size_t decodedLength = maxDecodedLength;
            
            // Decode base64 data
            if (decodeBase64(base64Data, decodedData, &decodedLength)) {
                // Handle the update
                handleBinaryUpdate(decodedData, decodedLength);
            } else {
                Serial.println("Base64 decoding failed");
            }
            
            // Clean up
            delete[] decodedData;
        }
    }
}

void WebSocketManager::connectToWebSocket() {
    wsClient.onMessage([this](WebsocketsMessage message) {
        this->handleMessage(message);
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

    Serial.println("WebSocket connected. Sending initial data...");

    JsonDocument jsonDoc;
    jsonDoc["pipUUID"] = pip_id;

    char jsonBuffer[200];
    serializeJson(jsonDoc, jsonBuffer);

    wsClient.send(jsonBuffer);
    wsClient.ping();
}

void WebSocketManager::pollWebSocket() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (wsClient.available()) {
        wsClient.poll();
    }
}

void WebSocketManager::reconnectWebSocket() {
    wsClient.close();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnecting to WebSocket...");
        connectToWebSocket();
    } else {
        Serial.println("Cannot reconnect WebSocket, WiFi is not connected.");
    }
}
