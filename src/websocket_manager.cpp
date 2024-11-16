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

void WebSocketManager::handleBinaryUpdate(const uint8_t* data, size_t len, bool isLastChunk) {
    if (!updateState.updateStarted) {
        // First chunk, start the update
        if (!Update.begin(updateState.totalSize)) {
            Serial.println("Not enough space to begin OTA");
            resetUpdateState();
            return;
        }
        updateState.updateStarted = true;
        Serial.printf("Starting OTA update of %d bytes\n", updateState.totalSize);
    }

    // Write chunk
    size_t written = Update.write(const_cast<uint8_t*>(data), len);
    if (written != len) {
        Serial.printf("Error writing chunk: %d/%d bytes written\n", written, len);
        Update.abort();
        resetUpdateState();
        return;
    }

    updateState.receivedSize += written;
    Serial.printf("OTA Progress: %d%%\n", (updateState.receivedSize * 100) / updateState.totalSize);

    if (isLastChunk) {
        if (!Update.end(true)) {
            Serial.printf("Error finishing update: %s\n", Update.errorString());
            resetUpdateState();
            return;
        }

        Serial.println("OTA update successful");
        delay(1000);  // Give time for the success message to be sent
        ESP.restart();
    }
}

void WebSocketManager::handleMessage(WebsocketsMessage message) {
    if (!message.isText()) { 
        Serial.println("Received non-text message, ignoring");
        return; 
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message.data());

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    const char* eventType = doc["event"];
    if (!eventType || strcmp(eventType, "new-user-code") != 0) {
        return;
    }

    // Get chunk information
    const char* base64Data = doc["data"];
    bool isLastChunk = doc["isLastChunk"] | false;
    size_t totalChunks = doc["totalChunks"] | 0;
    size_t chunkIndex = doc["chunkIndex"] | 0;
    
    if (chunkIndex == 0) {
        // First chunk, initialize update state
        updateState.totalSize = doc["totalSize"] | 0;
        updateState.totalChunks = totalChunks;
        updateState.receivedSize = 0;
        updateState.receivedChunks = 0;
        updateState.updateStarted = false;
    }

    if (!base64Data) {
        Serial.println("No data field in message");
        return;
    }

    // Decode and process chunk
    size_t maxDecodedLength = strlen(base64Data) * 3 / 4;
    std::vector<uint8_t> decodedData(maxDecodedLength);
    size_t decodedLength = maxDecodedLength;

    if (decodeBase64(base64Data, decodedData.data(), &decodedLength)) {
        updateState.receivedChunks++;
        Serial.printf("Processing chunk %d/%d\n", updateState.receivedChunks, updateState.totalChunks);
        handleBinaryUpdate(decodedData.data(), decodedLength, isLastChunk);
    } else {
        Serial.println("Base64 decoding failed");
        resetUpdateState();
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
