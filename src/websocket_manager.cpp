#include "./include/config.h"
#include "./include/websocket_manager.h"

WebSocketManager::WebSocketManager() {
    if (environment == Environment::LocalDev) return;
    wsClient.setCACert(rootCACertificate);
}

bool WebSocketManager::jsoneq(const char* json, const jsmntok_t* tok, const char* s) {
    if (tok->type == JSMN_STRING && 
        (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return true;
    }
    return false;
}

String WebSocketManager::extractString(const char* json, const jsmntok_t* tok) {
    return String(json + tok->start, tok->end - tok->start);
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
    if (!message.isText()) return;

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

    // Find event type
    for (int i = 1; i < tokenCount - 1; i++) {
        if (jsoneq(json, &tokens[i], "event") && tokens[i + 1].type == JSMN_STRING) {
            // Get event value
            String eventType = String(json + tokens[i + 1].start, 
                tokens[i + 1].end - tokens[i + 1].start);

            if (eventType == "new-user-code") {
                // Process firmware update message
                size_t chunkIndex = 0;
                size_t totalChunks = 0;
                size_t totalSize = 0;
                bool isLast = false;
                const char* chunkData = nullptr;
                int chunkDataLength = 0;

                // Extract values
                for (int j = i + 2; j < tokenCount - 1; j++) {
                    if (jsoneq(json, &tokens[j], "chunkIndex")) {
                        char numStr[16];
                        int len = tokens[j + 1].end - tokens[j + 1].start;
                        strncpy(numStr, json + tokens[j + 1].start, len);
                        numStr[len] = '\0';
                        chunkIndex = atoi(numStr);
                        j++;
                    }
                    else if (jsoneq(json, &tokens[j], "totalChunks")) {
                        char numStr[16];
                        int len = tokens[j + 1].end - tokens[j + 1].start;
                        strncpy(numStr, json + tokens[j + 1].start, len);
                        numStr[len] = '\0';
                        totalChunks = atoi(numStr);
                        j++;
                    }
                    else if (jsoneq(json, &tokens[j], "totalSize")) {
                        char numStr[16];
                        int len = tokens[j + 1].end - tokens[j + 1].start;
                        strncpy(numStr, json + tokens[j + 1].start, len);
                        numStr[len] = '\0';
                        totalSize = atoi(numStr);
                        j++;
                    }
                    else if (jsoneq(json, &tokens[j], "isLast")) {
                        isLast = (tokens[j + 1].end - tokens[j + 1].start == 4 &&
                                strncmp(json + tokens[j + 1].start, "true", 4) == 0);
                        j++;
                    }
                    else if (jsoneq(json, &tokens[j], "data")) {
                        chunkData = json + tokens[j + 1].start;
                        chunkDataLength = tokens[j + 1].end - tokens[j + 1].start;
                        j++;
                    }
                }

                // Process chunk
                if (chunkData && chunkDataLength > 0) {
                    // Handle first chunk
                    if (chunkIndex == 0) {
                        if (!updater.begin(totalSize)) {
                            return;
                        }
                        updater.setTotalChunks(totalChunks);
                        sendJsonMessage("update_status", "ready");
                    }

                    // Process chunk if update is in progress
                    if (updater.isUpdateInProgress()) {
                        // Create temporary null-terminated string for chunk data
                        char* tempChunk = (char*)ps_malloc(chunkDataLength + 1);
                        if (tempChunk) {
                            memcpy(tempChunk, chunkData, chunkDataLength);
                            tempChunk[chunkDataLength] = '\0';
                            updater.processChunk(tempChunk, chunkIndex, isLast);
                            free(tempChunk);
                        }
                    }
                }
            }
            break;
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

