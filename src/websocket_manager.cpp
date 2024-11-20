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
    const String& data = message.data();
    const char* json = data.c_str();
    
    jsmn_init(&parser);
    int tokenCount = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    if (tokenCount < 1 || tokens[0].type != JSMN_OBJECT) {
        sendErrorMessage("Invalid JSON");
        return;
    }

    // First pass: map token positions (only needed for first message)
    if (tokenPositions.eventIndex == -1) {
        for (int i = 1; i < tokenCount; i += 2) {
            String key = String(json + tokens[i].start, tokens[i].end - tokens[i].start);
            if (key == "event") tokenPositions.eventIndex = i;
            else if (key == "chunkIndex") tokenPositions.chunkIndexIndex = i;
            else if (key == "totalChunks") tokenPositions.totalChunksIndex = i;
            else if (key == "totalSize") tokenPositions.totalSizeIndex = i;
            else if (key == "isLast") tokenPositions.isLastIndex = i;
            else if (key == "data") tokenPositions.dataIndex = i;
        }
    }

    // Fast path: direct token access
    if (tokenPositions.eventIndex != -1) {
        String eventType = String(json + tokens[tokenPositions.eventIndex + 1].start, 
                                tokens[tokenPositions.eventIndex + 1].end - tokens[tokenPositions.eventIndex + 1].start);
        
        if (eventType == "new-user-code") {
            // Direct access to known token positions
            size_t chunkIndex = extractInt(json, &tokens[tokenPositions.chunkIndexIndex + 1]);
            size_t totalChunks = extractInt(json, &tokens[tokenPositions.totalChunksIndex + 1]);
            size_t totalSize = extractInt(json, &tokens[tokenPositions.totalSizeIndex + 1]);
            bool isLast = extractBool(json, &tokens[tokenPositions.isLastIndex + 1]);
            
            const char* chunkData = json + tokens[tokenPositions.dataIndex + 1].start;
            int chunkDataLength = tokens[tokenPositions.dataIndex + 1].end - tokens[tokenPositions.dataIndex + 1].start;
            
            // Process chunk with optimized data access
            processChunk(chunkData, chunkDataLength, chunkIndex, totalChunks, totalSize, isLast);
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

void WebSocketManager::processChunk(const char* chunkData, int chunkDataLength,
    size_t chunkIndex, size_t totalChunks,
    size_t totalSize, bool isLast) {
    if (chunkIndex == 0) {
        if (!updater.begin(totalSize)) return;
        updater.setTotalChunks(totalChunks);
        sendJsonMessage("update_status", "ready");
    }

    if (updater.isUpdateInProgress()) {
        // Use stack allocation for small chunks, heap for large ones
        if (chunkDataLength < 1024) {
            char stackChunk[1024];
            memcpy(stackChunk, chunkData, chunkDataLength);
            stackChunk[chunkDataLength] = '\0';
            updater.processChunk(stackChunk, chunkIndex, isLast);
        } else {
            char* heapChunk = (char*)ps_malloc(chunkDataLength + 1);
            if (heapChunk) {
                memcpy(heapChunk, chunkData, chunkDataLength);
                heapChunk[chunkDataLength] = '\0';
                updater.processChunk(heapChunk, chunkIndex, isLast);
                free(heapChunk);
            }
        }
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

