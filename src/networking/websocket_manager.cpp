#include "../utils/config.h"
#include "../utils/structs.h"
#include "./websocket_manager.h"

MessageTokens tokenPositions;

WebSocketManager::WebSocketManager() {
    if (DEFAULT_ENVIRONMENT == "local") return;
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

    jsmn_init(&parser);
    int tokenCount = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);

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
                handleFirmwareMetadata(json, tokenCount);
                break;
            }
            // else if (eventType == "start-sending-sensor-data") {
            //     SendDataToServer::getInstance().sendSensorData = true;
            //     break;
            // } else if (eventType == "stop-sending-sensor-data") {
            //     SendDataToServer::getInstance().sendSensorData = false;
            //     break;
            // } 
        }
    }
}

void WebSocketManager::handleFirmwareMetadata(const char* json, int tokenCount) {
    Serial.println("in new user code meta");
    currentChunk.chunkIndex = 0;
    currentChunk.totalChunks = 0;
    currentChunk.totalSize = 0;
    currentChunk.chunkSize = 0;
    currentChunk.isLast = false;
    currentChunk.expectingBinary = true;

    // Extract the firmware metadata fields
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
}

void WebSocketManager::handleBinaryMessage(WebsocketsMessage message) {
    const uint8_t* data = (const uint8_t*)message.c_str();
    size_t length = message.length();
    
    // If expecting a firmware chunk, handle it
    if (currentChunk.expectingBinary) {
        if (!updater.isUpdateInProgress()) {
            sendErrorMessage("No update in progress");
            return;
        }

        // Process firmware chunk
        unsigned long processStart = millis();
        uint8_t* buffer = (uint8_t*)ps_malloc(length);
        if (!buffer) {
            Serial.println("Failed to allocate buffer for binary chunk");
            return;
        }

        memcpy(buffer, data, length);
        updater.processChunk(buffer, length, currentChunk.chunkIndex, currentChunk.isLast);
        free(buffer);

        currentChunk.expectingBinary = false;
        Serial.printf("Firmware chunk processing time: %lu ms\n", millis() - processStart);
        return;
    }

    if (length < 1) {
        Serial.println("Binary message too short");
        return;
    }
    
    // Extract the message type from the first byte
    DataMessageType messageType = static_cast<DataMessageType>(data[0]);

    switch (messageType) {
        case DataMessageType::MOTOR_CONTROL:
            if (length == 5) {
                LabDemoManager::getInstance().handleMotorControl(data);
            } else {
                Serial.println("Invalid motor control message length");
            }
            break;

        case DataMessageType::SOUND_COMMAND:
            if (length == 2) {
                SoundType soundType = static_cast<SoundType>(data[1]);
                LabDemoManager::getInstance().handleSoundCommand(soundType);
            } else {
                Serial.println("Invalid sound command message length");
            }
            break;

        case DataMessageType::SPEAKER_MUTE:
            if (length == 2) {
                SpeakerStatus status = static_cast<SpeakerStatus>(data[1]);
                LabDemoManager::getInstance().handleSpeakerMute(status);
            } else {
                Serial.println("Invalid speaker mute message length");
            }
            break;
        case DataMessageType::BALANCE_CONTROL:
            if (length == 2) {
                BalanceStatus status = static_cast<BalanceStatus>(data[1]);
                Serial.print("Balance Status: ");
                Serial.println(status == BalanceStatus::BALANCED ? "BALANCED" : "UNBALANCED");
                LabDemoManager::getInstance().handleBalanceCommand(status);
            }
            break;
        default:
            Serial.printf("Unknown message type: %d\n", static_cast<int>(messageType));
            break;
    }
}

void WebSocketManager::sendJsonMessage(const char* event, const char* status, const char* extra) {
    StaticJsonDocument<SMALL_DOC_SIZE> doc;
    doc["event"] = event;
    doc["status"] = status;
    if (extra) {
        doc["error"] = extra;
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

    // First attempt
    Serial.println("Attempt 1: Connecting to WebSocket...");
    if (wsClient.connect(getWsServerUrl())) {
        sendInitialData();
        return;
    }

    Serial.println("WebSocket connection failed. Starting retry sequence...");
    startAttemptTime = millis();
    retryCount = 1;  // First retry (second attempt overall)
    connected = false;

    // Retry loop
    while (retryCount <= MAX_RETRIES && !connected) {
        if (attemptConnection()) {
            connected = true;
            break;
        }
    }

    if (!connected) {
        Serial.println("All connection attempts failed");
    }
}

void WebSocketManager::sendInitialData() {
    Serial.println("WebSocket connected. Sending initial data...");
    StaticJsonDocument<SMALL_DOC_SIZE> initDoc;
    initDoc["route"] = "/register";
    JsonObject payload = initDoc.createNestedObject("payload");
    payload["pipUUID"] = getPipID();
    String jsonString;
    serializeJson(initDoc, jsonString);

    WiFi.mode(WIFI_STA);
    wsClient.send(jsonString);
}

bool WebSocketManager::attemptConnection() {
    unsigned long currentTime = millis();
    unsigned long delayPeriod = retryCount * 1000UL;  // Increasing delays: 1s, 2s, 3s, 4s

    if (currentTime - startAttemptTime >= delayPeriod) {
        Serial.printf("Attempt %d: Connecting to WebSocket...\n", retryCount + 1);

        if (wsClient.connect(getWsServerUrl())) {
            sendInitialData();
            return true;
        }

        Serial.printf("WebSocket connection failed on attempt %d\n", retryCount + 1);
        if (retryCount < MAX_RETRIES) {
            Serial.printf("Will retry in %d seconds...\n", retryCount + 1);
        }

        startAttemptTime = currentTime;
        retryCount++;
    }

    yield();
    return false;
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
    unsigned long currentTime = millis();
    if (currentTime - lastPollTime < POLL_INTERVAL) return;
    
    lastPollTime = currentTime;
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, cannot poll WebSocket");
        return;
    }

    updater.checkTimeout();

    if (!wsClient.available()) return;
    try {
        wsClient.poll();
    } catch (const std::exception& e) {
        Serial.printf("Error during WebSocket poll: %s\n", e.what());
    }
}
