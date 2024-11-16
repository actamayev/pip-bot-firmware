#include <Arduino.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "./include/config.h"
#include "./include/websocket_manager.h"

WebSocketManager::WebSocketManager() {
    if (environment == Environment::LocalDev) return;
    wsClient.setCACert(rootCACertificate);
    wsClient.setInsecure();

    // Pre-allocate buffer
    if (buffer == nullptr) {
        buffer = (uint8_t*)malloc(BUFFER_SIZE);
        if (buffer == nullptr) {
            Serial.println("Failed to allocate buffer");
        }
    }
}

WebSocketManager::~WebSocketManager() {
    if (buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }
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

bool WebSocketManager::startUpdate(size_t size) {
    if (size == 0) return false;

    const size_t REQUIRED_HEAP = BUFFER_SIZE + 32768; // Buffer size plus 32KB overhead
    if (ESP.getFreeHeap() < REQUIRED_HEAP) {
        Serial.printf("Not enough heap space. Required: %u, Available: %u\n", 
            REQUIRED_HEAP, ESP.getFreeHeap());
        return false;
    }

    Serial.printf("Starting update of %u bytes\n", size);
    if (!Update.begin(size)) {
        Serial.printf("Not enough space: %s\n", Update.errorString());
        return false;
    }

    updateState.updateStarted = true;
    updateState.totalSize = size;
    updateState.receivedSize = 0;
    updateState.lastChunkTime = millis();

    Serial.printf("Update started. Size: %u bytes\n", size);
    return true;
}

void WebSocketManager::endUpdate(bool success) {
    if (!updateState.updateStarted) return;

    if (success && Update.end()) {
        Serial.println("Update successful!");
        delay(1000);
        ESP.restart();
    } else {
        Serial.printf("Update failed: %s\n", Update.errorString());
        Update.abort();
    }
    resetUpdateState();
}

void WebSocketManager::processChunk(const char* data, size_t chunkIndex, bool isLast) {
    if (!buffer || !updateState.updateStarted) {
        Serial.println("Update not properly initialized");
        return;
    }

    size_t decodedLen = BUFFER_SIZE;
    if (decodeBase64(data, buffer, &decodedLen)) {
        if (Update.write(buffer, decodedLen) != decodedLen) {
            Serial.printf("Write failed at chunk: %u\n", chunkIndex);
            endUpdate(false);
            return;
        }
        
        updateState.receivedSize += decodedLen;
        updateState.receivedChunks++;
        updateState.lastChunkTime = millis();
        
        Serial.printf("Chunk %u/%u - Progress: %d%%\n", 
            chunkIndex + 1, 
            updateState.totalChunks,
            (updateState.receivedSize * 100) / updateState.totalSize
        );

        if (isLast) {
            Serial.println("Processing final chunk");
            endUpdate(true);
        }
    } else {
        Serial.println("Base64 decode failed");
        endUpdate(false);
    }
}

void WebSocketManager::handleMessage(WebsocketsMessage message) {
    if (!message.isText()) return;

    // Log memory status at start
    Serial.printf("Free heap before processing: %u\n", ESP.getFreeHeap());

    // Parse JSON message
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, message.data());

    if (error) {
        Serial.printf("JSON Error: %s\n", error.c_str());
        StaticJsonDocument<256> response;
        response["event"] = "update_status";
        response["status"] = "error";
        response["error"] = "JSON parse error";
        String jsonString;
        serializeJson(response, jsonString);
        wsClient.send(jsonString);
        return;
    }

    // Check for event type
    const char* eventType = doc["event"];
    if (!eventType || strcmp(eventType, "new-user-code") != 0) {
        return;
    }

    // Get all necessary data
    size_t chunkIndex = doc["chunkIndex"] | 0;
    size_t totalChunks = doc["totalChunks"] | 0;
    bool isLast = doc["isLast"] | false;
    const char* data = doc["data"];

    if (!data) {
        Serial.println("No data in chunk");
        StaticJsonDocument<256> response;
        response["event"] = "update_status";
        response["status"] = "error";
        response["error"] = "Missing data in chunk";
        String jsonString;
        serializeJson(response, jsonString);
        wsClient.send(jsonString);
        return;
    }

    // Handle first chunk and initialization
    if (chunkIndex == 0) {
        size_t totalSize = doc["totalSize"] | 0;
        Serial.printf("Starting new update of size: %u\n", totalSize);
        
        // Check heap space
        const size_t REQUIRED_HEAP = BUFFER_SIZE + 32768;
        if (ESP.getFreeHeap() < REQUIRED_HEAP) {
            StaticJsonDocument<256> response;
            response["event"] = "update_status";
            response["status"] = "error";
            response["error"] = "Insufficient heap space";
            String jsonString;
            serializeJson(response, jsonString);
            wsClient.send(jsonString);
            return;
        }

        if (!startUpdate(totalSize)) {
            StaticJsonDocument<256> response;
            response["event"] = "update_status";
            response["status"] = "error";
            response["error"] = "Failed to initialize update";
            String jsonString;
            serializeJson(response, jsonString);
            wsClient.send(jsonString);
            return;
        }

        updateState.totalChunks = totalChunks;

        // Send success status
        StaticJsonDocument<256> response;
        response["event"] = "update_status";
        response["status"] = "ready";
        String jsonString;
        serializeJson(response, jsonString);
        wsClient.send(jsonString);
    }

    // Process chunk only if update is properly initialized
    if (updateState.updateStarted) {
        processChunk(data, chunkIndex, isLast);
    } else if (chunkIndex != 0) {
        // If we get a non-zero chunk without initialization
        StaticJsonDocument<256> response;
        response["event"] = "update_status";
        response["status"] = "error";
        response["error"] = "Update not initialized";
        String jsonString;
        serializeJson(response, jsonString);
        wsClient.send(jsonString);
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
    if (wsClient.connect(getWsServerUrl())) {
        Serial.println("WebSocket connected. Sending initial data...");
        StaticJsonDocument<256> jsonDoc;  // Small document for initial message
        jsonDoc["pipUUID"] = pip_id;
        String jsonString;
        serializeJson(jsonDoc, jsonString);
        wsClient.send(jsonString);
    } else {
        Serial.println("WebSocket connection failed");
    }
}

void WebSocketManager::pollWebSocket() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, cannot poll WebSocket");
        return;
    }
    
    // Check for update timeout
    if (updateState.updateStarted && 
        (millis() - updateState.lastChunkTime > 10000)) { // 10 second timeout
        Serial.println("Update timeout");
        endUpdate(false);
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
