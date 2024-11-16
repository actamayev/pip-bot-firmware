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
    if (buffer == nullptr && checkHeapSpace()) {
            buffer = (uint8_t*)malloc(BUFFER_SIZE);
            if (buffer == nullptr) {
                Serial.println("Failed to allocate WebSocket buffer");
            } else {
                Serial.printf("Successfully allocated %u byte buffer\n", BUFFER_SIZE);
            }
        }
}

WebSocketManager::~WebSocketManager() {
    if (buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }
}

bool WebSocketManager::checkHeapSpace() {
    const size_t REQUIRED_HEAP = BUFFER_SIZE + (32 * 1024); // Buffer size plus 32KB overhead
    size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < REQUIRED_HEAP) {
        Serial.printf("Insufficient heap space. Required: %u, Available: %u\n", 
            REQUIRED_HEAP, freeHeap);
        return false;
    }
    return true;
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

bool WebSocketManager::checkMemoryRequirements(size_t updateSize) const {
    const size_t requiredHeap = BUFFER_SIZE + HEAP_OVERHEAD;
    const size_t freeHeap = ESP.getFreeHeap();
    
    Serial.printf("Memory Check:\n");
    Serial.printf("- Required heap: %u bytes\n", requiredHeap);
    Serial.printf("- Free heap: %u bytes\n", freeHeap);
    Serial.printf("- Update size: %u bytes\n", updateSize);
    
    if (freeHeap < requiredHeap) {
        Serial.printf("Insufficient heap. Need %u more bytes\n", 
            requiredHeap - freeHeap);
        return false;
    }
    
    return true;
}

bool WebSocketManager::initializeBuffer() {
    if (buffer != nullptr) return true;
    
    buffer = (uint8_t*)malloc(BUFFER_SIZE);
    if (buffer == nullptr) {
        Serial.println("Failed to allocate buffer");
        return false;
    }
    
    Serial.printf("Buffer allocated: %u bytes\n", BUFFER_SIZE);
    return true;
}

bool WebSocketManager::startUpdate(size_t size) {
    if (size == 0) return false;

    if (!checkMemoryRequirements(size)) return false;

    // Initialize buffer if needed
    if (!initializeBuffer()) return false;

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

    const String& data = message.data();
    
    // Debug prints
    Serial.printf("Received message length: %u\n", data.length());
    Serial.printf("First 100 chars: %.100s\n", data.c_str());
    Serial.printf("Free heap before JSON: %u\n", ESP.getFreeHeap());

    // Use DynamicJsonDocument with precise sizing
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
        Serial.printf("JSON Error: %s\n", error.c_str());
        Serial.printf("Message length: %u\n", data.length());
        
        // Send error back to server
        DynamicJsonDocument errorDoc(256);
        errorDoc["event"] = "update_status";
        errorDoc["status"] = "error";
        errorDoc["error"] = String("JSON parse error: ") + error.c_str();
        
        String errorJson;
        serializeJson(errorDoc, errorJson);
        wsClient.send(errorJson);
        return;
    }

    // Get event type
    const char* eventType = doc["event"];
    if (!eventType) {
        Serial.println("No event type in message");
        return;
    }
    Serial.printf("Event type: %s\n", eventType);

    if (strcmp(eventType, "new-user-code") == 0) {
        // Handle new user code
        size_t chunkIndex = doc["chunkIndex"] | 0;
        size_t totalChunks = doc["totalChunks"] | 0;
        const char* data = doc["data"];
        
        if (!data) {
            Serial.println("No data in chunk");
            return;
        }

        Serial.printf("Processing chunk %u/%u\n", chunkIndex + 1, totalChunks);
        
        // First chunk initialization
        if (chunkIndex == 0) {
            size_t totalSize = doc["totalSize"] | 0;
            if (!startUpdate(totalSize)) {
                return;
            }
            updateState.totalChunks = totalChunks;
            
            // Send ready status
            DynamicJsonDocument readyDoc(256);
            readyDoc["event"] = "update_status";
            readyDoc["status"] = "ready";
            String readyJson;
            serializeJson(readyDoc, readyJson);
            wsClient.send(readyJson);
        }

        // Process chunk
        if (updateState.updateStarted) {
            processChunk(data, chunkIndex, doc["isLast"] | false);
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
        JsonDocument jsonDoc;  // Small document for initial message
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
