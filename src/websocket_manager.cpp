#include <Arduino.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "./include/config.h"
#include "./include/websocket_manager.h"

WebSocketManager::WebSocketManager() {
    if (environment == Environment::LocalDev) return;
    wsClient.setCACert(rootCACertificate);
    wsClient.setInsecure(); // Add fallback for development
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
    static size_t currentLength = 0;
    static bool updateStarted = false;

    if (!updateStarted) {
        // Calculate the required size before starting
        if (!Update.begin(len)) {
            Serial.println("Not enough space to begin OTA");
            return;
        }
        updateStarted = true;
        currentLength = 0;
    }

    // Write data in chunks
    size_t written = Update.write(const_cast<uint8_t*>(data), len);
    if (written != len) {
        Serial.println("Error during OTA update");
        Serial.printf("Written %d bytes out of %d\n", written, len);
        Update.abort();
        updateStarted = false;
        return;
    }

    currentLength += written;
    Serial.printf("OTA Progress: %d%%\n", (currentLength * 100) / len);

    if (currentLength == len) {
        if (!Update.end(true)) {
            Serial.printf("Error finishing update: %s\n", Update.errorString());
            updateStarted = false;
            return;
        }

        Serial.println("OTA update successful");
        // Add a delay before restart to ensure client receives success message
        delay(1000);
        ESP.restart();
    }
}

void WebSocketManager::handleMessage(WebsocketsMessage message) {
    if (!message.isText()) { 
        Serial.println("Received non-text message, ignoring");
        return; 
    }

    // Parse JSON message
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message.data());

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    const char* eventType = doc["event"];
    if (!eventType) {
        Serial.println("No event type in message");
        return;
    }

    if (strcmp(eventType, "new-user-code") != 0) {
        Serial.printf("Unknown event type: %s\n", eventType);
        return;
    }

    const char* base64Data = doc["data"];
    if (!base64Data) {
        Serial.println("No data field in message");
        return;
    }

    // Calculate maximum decoded length
    size_t maxDecodedLength = strlen(base64Data) * 3 / 4;
    
    // Use std::vector for automatic memory management
    std::vector<uint8_t> decodedData(maxDecodedLength);
    size_t decodedLength = maxDecodedLength;

    // Decode base64 data
    if (decodeBase64(base64Data, decodedData.data(), &decodedLength)) {
        Serial.printf("Starting update with %d bytes\n", decodedLength);
        handleBinaryUpdate(decodedData.data(), decodedLength);
    } else {
        Serial.println("Base64 decoding failed");
    }
}

void WebSocketManager::connectToWebSocket() {
    wsClient.onMessage([this](WebsocketsMessage message) {
        this->handleMessage(message);
    });

    wsClient.onEvent([](WebsocketsEvent event, String data) {
        switch (event) {
            case WebsocketsEvent::ConnectionOpened:
                Serial.println("WebSocket connected");
                break;
            case WebsocketsEvent::ConnectionClosed:
                Serial.println("WebSocket disconnected");
                break;
            case WebsocketsEvent::GotPing:
                Serial.println("Got ping");
                break;
            case WebsocketsEvent::GotPong:
                Serial.println("Got pong");
                break;
        }
    });

    Serial.println("Attempting to connect to WebSocket...");
    bool connected = false;
    int retries = 0;
    const int maxRetries = 3;

    while (!connected && retries < maxRetries) {
        connected = wsClient.connect(getWsServerUrl());
        if (!connected) {
            Serial.printf("Connection attempt %d failed\n", retries + 1);
            delay(1000);
            retries++;
        }
    }

    if (!connected) {
        Serial.println("Failed to connect to WebSocket");
        return;
    }

    Serial.println("WebSocket connected. Sending initial data...");

    JsonDocument jsonDoc;
    jsonDoc["pipUUID"] = pip_id;

    String jsonString;
    serializeJson(jsonDoc, jsonString);
    wsClient.send(jsonString);
}

void WebSocketManager::pollWebSocket() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, cannot poll WebSocket");
        return;
    }
    
    if (wsClient.available()) {
        try {
            wsClient.poll();
        } catch (const std::exception& e) {
            Serial.printf("Error during WebSocket poll: %s\n", e.what());
        }
    }
}

void WebSocketManager::reconnectWebSocket() {
    wsClient.close();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnecting to WebSocket...");
        connectToWebSocket();
    } else {
        Serial.println("Cannot reconnect WebSocket, WiFi is not connected");
    }
}
