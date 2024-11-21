#include "./include/config.h"
#include "./include/websocket_manager.h"

struct MessageTokens {
    int eventIndex = -1;
    int chunkIndexIndex = -1;
    int totalChunksIndex = -1;
    int totalSizeIndex = -1;
    int isLastIndex = -1;
    int dataIndex = -1;
};

// Add this to WebSocketManager class
MessageTokens tokenPositions;

WebSocketManager::WebSocketManager() {
    if (environment == Environment::LocalDev) return;
    wsClient.setCACert(rootCACertificate);
}

int64_t WebSocketManager::extractInt(const char* json, const jsmntok_t* tok) {
    char numStr[32];
    int len = min(31, tok->end - tok->start);
    strncpy(numStr, json + tok->start, len);
    numStr[len] = '\0';
    return atoll(numStr);
}

bool WebSocketManager::extractBool(const char* json, const jsmntok_t* tok) {
    return (tok->end - tok->start == 4 && 
            strncmp(json + tok->start, "true", 4) == 0);
}

void WebSocketManager::handleMessage(WebsocketsMessage message) {
    if (message.isBinary()) {
        handleBinaryMessage(message);
    } else {
        handleJsonMessage(message);
    }
}

void WebSocketManager::handleJsonMessage(WebsocketsMessage message) {
    const String& data = message.data();
    const char* json = data.c_str();
    
    Serial.println("Received JSON message:");
    Serial.println(json);

    jsmn_init(&parser);
    int tokenCount = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    Serial.printf("Token count: %d\n", tokenCount);

    if (tokenCount < 1 || tokens[0].type != JSMN_OBJECT) {
        sendErrorMessage("Invalid JSON");
        return;
    }

    // Find the "event" field in the JSON
    for (int i = 1; i < tokenCount; i += 2) {
        String key = String(json + tokens[i].start, tokens[i].end - tokens[i].start);
        if (key == "event") {
            // Get the value of the "event" field
            String eventType = String(json + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
            Serial.printf("Found event type: '%s'\n", eventType.c_str());
            
            if (eventType == "new-user-code-meta") {
                Serial.println("in new user code meta");
                currentChunk.chunkIndex = 0;
                currentChunk.totalChunks = 0;
                currentChunk.totalSize = 0;
                currentChunk.chunkSize = 0;  // Initialize chunk size
                currentChunk.isLast = false;
                currentChunk.expectingBinary = true;
                
                // Now extract the other fields
                for (int j = 1; j < tokenCount; j += 2) {
                    String fieldKey = String(json + tokens[j].start, tokens[j].end - tokens[j].start);
                    if (fieldKey == "chunkIndex") {
                        currentChunk.chunkIndex = extractInt(json, &tokens[j + 1]);
                    } else if (fieldKey == "totalChunks") {
                        currentChunk.totalChunks = extractInt(json, &tokens[j + 1]);
                    } else if (fieldKey == "totalSize") {
                        currentChunk.totalSize = extractInt(json, &tokens[j + 1]);
                    } else if (fieldKey == "isLast") {
                        currentChunk.isLast = extractBool(json, &tokens[j + 1]);
                    } else if (fieldKey == "chunkSize") {
                        currentChunk.chunkSize = extractInt(json, &tokens[j + 1]);
                    }
                }

                Serial.printf("Expecting binary chunk %d/%d of size: %d bytes\n", 
                    currentChunk.chunkIndex + 1, 
                    currentChunk.totalChunks,
                    currentChunk.chunkSize);
                
                // Initialize update if this is the first chunk
                if (currentChunk.chunkIndex == 0) {
                    if (!updater.begin(currentChunk.totalSize)) {
                        sendErrorMessage("Failed to initialize update");
                        return;
                    }
                    updater.setTotalChunks(currentChunk.totalChunks);
                    sendJsonMessage("update_status", "ready");
                }
                break;
            }
        }
    }
}

void WebSocketManager::handleBinaryMessage(WebsocketsMessage message) {
    if (!currentChunk.expectingBinary) {
        sendErrorMessage("Unexpected binary message");
        return;
    }

    // Get the raw binary data
    const char* rawData = message.c_str();  // Get raw data pointer
    size_t dataLen = message.length();      // Get actual length

    Serial.printf("Received binary chunk %d/%d, size: %d bytes\n", 
        currentChunk.chunkIndex + 1, currentChunk.totalChunks, dataLen);

    if (dataLen == 0) {
        Serial.println("Error: Received empty chunk");
        return;
    }

    // Verify the chunk size matches what was announced in metadata
    if (dataLen != currentChunk.chunkSize) {
        Serial.printf("Warning: Chunk size mismatch. Expected %d, got %d bytes\n", 
            currentChunk.chunkSize, dataLen);
    }

    if (updater.isUpdateInProgress()) {
        // Create a properly sized buffer
        uint8_t* buffer = (uint8_t*)ps_malloc(dataLen);
        if (!buffer) {
            Serial.println("Failed to allocate buffer for binary chunk");
            return;
        }

        // Copy the data
        memcpy(buffer, rawData, dataLen);

        // Debug: Print first few bytes
        Serial.print("First 8 bytes: ");
        for(int i = 0; i < min(8, (int)dataLen); i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        Serial.println();

        // Process the chunk
        updater.processChunk(buffer, dataLen, currentChunk.chunkIndex, currentChunk.isLast);

        // Clean up
        free(buffer);
    }

    currentChunk.expectingBinary = false;
}

void WebSocketManager::sendJsonMessage(const char* event, const char* status, const char* extra) {
    StaticJsonDocument<SMALL_DOC_SIZE> doc;
    doc["event"] = event;
    doc["status"] = status;
    if (extra) {
        doc["error"] = extra;  // or other field depending on context
    }

    String jsonString;
    serializeJson(doc, jsonString);
    wsClient.send(jsonString);
}

void WebSocketManager::sendErrorMessage(const char* error) {
    sendJsonMessage("update_status", "error", error);
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
        // Use ArduinoJson for this small message
        StaticJsonDocument<SMALL_DOC_SIZE> initDoc;
        initDoc["pipUUID"] = getPipID();
        String jsonString;
        serializeJson(initDoc, jsonString);
        wsClient.send(jsonString);
    } else {
        Serial.println("WebSocket connection failed");
    }
}

void WebSocketManager::processChunk(uint8_t* chunkData, size_t chunkDataLength,
    size_t chunkIndex, size_t totalChunks,
    size_t totalSize, bool isLast) {
    if (chunkIndex == 0) {
        if (!updater.begin(totalSize)) return;
        updater.setTotalChunks(totalChunks);
        sendJsonMessage("update_status", "ready");
    }

    if (updater.isUpdateInProgress()) {
        updater.processChunk(chunkData, chunkDataLength, chunkIndex, isLast);
    }
}

void WebSocketManager::pollWebSocket() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, cannot poll WebSocket");
        return;
    }

    updater.checkTimeout();

    if (wsClient.available()) {
        try {
            wsClient.poll();
        } catch (const std::exception& e) {
            Serial.printf("Error during WebSocket poll: %s\n", e.what());
        }
    }
}
