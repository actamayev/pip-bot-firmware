#include "./include/config.h"
#include "./include/websocket_manager.h"

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
    if (!message.isText()) {
        Serial.println("Message is not text!");
        return;
    }

    const String& data = message.data();
    const char* json = data.c_str();
    
    Serial.printf("\nReceived message stats:\n");
    Serial.printf("- Message length: %u bytes\n", data.length());
    Serial.printf("- Free heap: %u bytes\n", ESP.getFreeHeap());

    // Initialize parser
    jsmn_init(&parser);
    
    // Parse JSON
    int tokenCount = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    if (tokenCount < 0) {
        Serial.printf("Failed to parse JSON: %d\n", tokenCount);
        sendErrorMessage("JSON parse error");
        return;
    }

    Serial.printf("Successfully parsed JSON. Token count: %d\n", tokenCount);
    
    // The first token should be the object
    if (tokenCount < 1 || tokens[0].type != JSMN_OBJECT) {
        Serial.println("Expected JSON object");
        return;
    }

    // Since it's a flat object, we can process key-value pairs directly
    for (int i = 1; i < tokenCount; i += 2) {
        // Get the current key
        String key = String(json + tokens[i].start, tokens[i].end - tokens[i].start);
        Serial.printf("Found key: %s\n", key.c_str());
        
        if (key == "event") {
            String eventType = String(json + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
            Serial.printf("Found event type: %s\n", eventType.c_str());
            
            if (eventType == "new-user-code") {
                Serial.println("Processing firmware update message");

                // Parse remaining fields
                size_t chunkIndex = 0;
                size_t totalChunks = 0;
                size_t totalSize = 0;
                bool isLast = false;
                const char* chunkData = nullptr;
                int chunkDataLength = 0;
                
                // Continue parsing from the next token
                for (int j = i + 2; j < tokenCount; j += 2) {
                    String fieldKey = String(json + tokens[j].start, tokens[j].end - tokens[j].start);
                    Serial.printf("Processing field: %s\n", fieldKey.c_str());

                    if (fieldKey == "chunkIndex") {
                        chunkIndex = extractInt(json, &tokens[j + 1]);
                        Serial.printf("ChunkIndex: %u\n", chunkIndex);
                    }
                    else if (fieldKey == "totalChunks") {
                        totalChunks = extractInt(json, &tokens[j + 1]);
                        Serial.printf("TotalChunks: %u\n", totalChunks);
                    }
                    else if (fieldKey == "totalSize") {
                        totalSize = extractInt(json, &tokens[j + 1]);
                        Serial.printf("TotalSize: %u\n", totalSize);
                    }
                    else if (fieldKey == "isLast") {
                        isLast = extractBool(json, &tokens[j + 1]);
                        Serial.printf("IsLast: %s\n", isLast ? "true" : "false");
                    }
                    else if (fieldKey == "data") {
                        chunkData = json + tokens[j + 1].start;
                        chunkDataLength = tokens[j + 1].end - tokens[j + 1].start;
                        Serial.printf("Found data chunk of length: %d\n", chunkDataLength);
                    }
                }

                // Process chunk if we have all required data
                if (chunkData && chunkDataLength > 0) {
                    // Handle first chunk
                    if (chunkIndex == 0) {
                        Serial.printf("Attempting to begin update with size: %u\n", totalSize);
                        if (!updater.begin(totalSize)) {
                            Serial.println("Failed to begin update!");
                            return;
                        }
                        updater.setTotalChunks(totalChunks);
                        Serial.println("Update successfully started");
                        sendJsonMessage("update_status", "ready");
                    }

                    // Process chunk if update is in progress
                    if (updater.isUpdateInProgress()) {
                        Serial.printf("Processing chunk %u of %u\n", chunkIndex + 1, totalChunks);
                        // Create temporary null-terminated string for chunk data
                        char* tempChunk = (char*)ps_malloc(chunkDataLength + 1);
                        if (tempChunk) {
                            memcpy(tempChunk, chunkData, chunkDataLength);
                            tempChunk[chunkDataLength] = '\0';
                            updater.processChunk(tempChunk, chunkIndex, isLast);
                            free(tempChunk);
                        } else {
                            Serial.println("Failed to allocate memory for chunk!");
                        }
                    } else {
                        Serial.println("Update not in progress!");
                    }
                }
                break;  // We found and processed the firmware update event
            }
        }
    }
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

