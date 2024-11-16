#include <Arduino.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
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
        Serial.println("message not text");
        return;
    }

    // Use StaticJsonDocument to avoid heap fragmentation
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message.data());

    if (error) {
        Serial.printf("JSON parsing failed: %s\n", error.c_str());
        return;
    }

    // Validate message
    if (!doc.containsKey("event") || strcmp(doc["event"], "new-user-code") != 0) {
        Serial.println("doc doesn't contain event");
        return;
    }

    static bool updateInProgress = false;
    static size_t totalSize = 0;
    static size_t receivedSize = 0;
    static uint32_t lastChunkTime = 0;

    // Get chunk information
    size_t chunkIndex = doc["chunkIndex"] | 0;
    size_t totalChunks = doc["totalChunks"] | 0;
    bool isLast = doc["isLast"] | false;
    const char* data = doc["data"];

    if (!data) {
        Serial.println("No data in chunk");
        return;
    }

    // First chunk initializes the update
    if (chunkIndex == 0) {
        totalSize = doc["totalSize"] | 0;
        if (totalSize == 0) {
            Serial.println("Invalid total size");
            return;
        }

        Serial.printf("Starting update of %u bytes\n", totalSize);
        if (!Update.begin(totalSize)) {
            Serial.printf("Not enough space: %s\n", Update.errorString());
            return;
        }
        updateInProgress = true;
        receivedSize = 0;
    }

    // Process chunk
    if (updateInProgress) {
        // Decode base64 chunk
        size_t decodedLen = strlen(data) * 3 / 4;
        uint8_t* decodedData = (uint8_t*)malloc(decodedLen);
        
        if (!decodedData) {
            Serial.println("Failed to allocate decode buffer");
            Update.abort();
            updateInProgress = false;
            return;
        }

        size_t actualLen = decodedLen;
        if (decodeBase64(data, decodedData, &actualLen)) {
            if (Update.write(decodedData, actualLen) != actualLen) {
                Serial.printf("Write failed at size: %u\n", receivedSize);
                free(decodedData);
                Update.abort();
                updateInProgress = false;
                return;
            }
            receivedSize += actualLen;
            Serial.printf("Progress: %d%%\n", (receivedSize * 100) / totalSize);
        }

        free(decodedData);
        lastChunkTime = millis();

        // Handle last chunk
        if (isLast) {
            if (Update.end(true)) {
                Serial.println("Update complete!");
                delay(1000);
                ESP.restart();
            } else {
                Serial.printf("Update failed: %s\n", Update.errorString());
                updateInProgress = false;
            }
        }
    }
}

// void WebSocketManager::loop() {
//     static uint32_t lastCheck = 0;
//     const uint32_t TIMEOUT = 10000; // 10 second timeout

//     if (updateInProgress && (millis() - lastChunkTime > TIMEOUT)) {
//         Serial.println("Update timeout");
//         Update.abort();
//         updateInProgress = false;
//     }
// }

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
