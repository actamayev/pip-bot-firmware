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
    if (!message.isText()) return;

    // Print memory diagnostics
    Serial.println("\n=== Memory Diagnostics ===");
    Serial.printf("Total heap: %u bytes\n", ESP.getHeapSize());
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Minimum free heap: %u bytes\n", ESP.getMinFreeHeap());
    Serial.printf("Maximum allocatable heap: %u bytes\n", ESP.getMaxAllocHeap());

    // Check for PSRAM
    if (psramFound()) {
        Serial.printf("Total PSRAM: %u bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("No PSRAM found");
    }

    // Print flash size
    Serial.println("\n=== Flash Info ===");
    Serial.printf("Flash chip size: %u bytes\n", ESP.getFlashChipSize());
    Serial.printf("Available sketch space: %u bytes\n", ESP.getFreeSketchSpace());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message.data());
    if (error) {
        Serial.printf("JSON parsing failed: %s\n", error.c_str());
        return;
    }

    const char* eventType = doc["event"];
    if (!eventType || strcmp(eventType, "new-user-code") != 0) return;

    const char* base64Data = doc["data"];
    if (!base64Data) {
        Serial.println("No base64 data found");
        return;
    }

    // Calculate sizes
    size_t inputLen = strlen(base64Data);
    size_t maxDecodedLen = (inputLen * 3) / 4;
    
    Serial.println("\n=== Update Size Info ===");
    Serial.printf("Base64 input length: %u bytes\n", inputLen);
    Serial.printf("Expected decoded length: %u bytes\n", maxDecodedLen);
    Serial.printf("Available update space: %u bytes\n", ESP.getFreeSketchSpace());

    // Try to allocate memory
    Serial.println("\nAttempting memory allocation...");
    uint8_t* decodedData = nullptr;
    
    if (psramFound()) {
        Serial.println("Trying PSRAM allocation");
        decodedData = (uint8_t*)ps_malloc(maxDecodedLen);
    }
    
    if (!decodedData) {
        Serial.println("Trying heap allocation");
        decodedData = (uint8_t*)malloc(maxDecodedLen);
    }

    if (!decodedData) {
        Serial.println("Failed to allocate memory");
        return;
    }

    Serial.println("Memory allocation successful");

    size_t actualLen = maxDecodedLen;
    if (decodeBase64(base64Data, decodedData, &actualLen)) {
        Serial.printf("\nActual decoded length: %u bytes\n", actualLen);
        
        Serial.println("Attempting to begin update...");
        if (!Update.begin(actualLen)) {
            Serial.printf("Update begin failed. Error: %s\n", Update.errorString());
            free(decodedData);
            return;
        }

        Serial.println("Writing update...");
        if (Update.write(decodedData, actualLen) != actualLen) {
            Serial.printf("Update write failed. Error: %s\n", Update.errorString());
            Update.abort();
        } else if (!Update.end()) {
            Serial.printf("Update end failed. Error: %s\n", Update.errorString());
        } else {
            Serial.println("Update successful, restarting...");
            delay(1000);
            ESP.restart();
        }
    } else {
        Serial.println("Base64 decoding failed");
    }

    free(decodedData);
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
