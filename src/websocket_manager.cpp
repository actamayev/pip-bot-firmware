#include "./include/config.h"
#include "./include/websocket_manager.h"

WebSocketManager::WebSocketManager() {
    if (environment == Environment::LocalDev) return;
    wsClient.setCACert(rootCACertificate);
}

void WebSocketManager::handleMessage(WebsocketsMessage message) {
    if (!message.isText()) return;

    const String& data = message.data();

    Serial.println("\n");
    Serial.printf("Received message length: %u\n", data.length());
    Serial.printf("First 150 chars: %.150s\n", data.c_str());
    Serial.printf("Free heap before JSON: %u\n", ESP.getFreeHeap());

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
        Serial.printf("JSON Error: %s\n", error.c_str());
        Serial.printf("Message length: %u\n", data.length());

        DynamicJsonDocument errorDoc(256);
        errorDoc["event"] = "update_status";
        errorDoc["status"] = "error";
        errorDoc["error"] = String("JSON parse error: ") + error.c_str();

        String errorJson;
        serializeJson(errorDoc, errorJson);
        wsClient.send(errorJson);
        return;
    }

    const char* eventType = doc["event"];
    if (!eventType) {
        Serial.println("No event type in message");
        return;
    }
    Serial.printf("Event type: %s\n", eventType);

    if (strcmp(eventType, "new-user-code") == 0) {
        size_t chunkIndex = doc["chunkIndex"] | 0;
        size_t totalChunks = doc["totalChunks"] | 0;
        const char* chunkData = doc["data"];

        if (!chunkData) {
            Serial.println("No data in chunk");
            return;
        }

        Serial.printf("Processing chunk %u/%u\n", chunkIndex + 1, totalChunks);

        // First chunk initialization
        if (chunkIndex == 0) {
            size_t totalSize = doc["totalSize"] | 0;
            if (!updater.begin(totalSize)) {
                return;
            }
            updater.setTotalChunks(totalChunks);

            DynamicJsonDocument readyDoc(256);
            readyDoc["event"] = "update_status";
            readyDoc["status"] = "ready";
            String readyJson;
            serializeJson(readyDoc, readyJson);
            wsClient.send(readyJson);
        }

        if (updater.isUpdateInProgress()) {
            updater.processChunk(chunkData, chunkIndex, doc["isLast"] | false);
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
        jsonDoc["pipUUID"] = getPipID();
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

    updater.checkTimeout();

    if (wsClient.available()) {
        try {
            wsClient.poll();
        } catch (const std::exception& e) {
            Serial.printf("Error during WebSocket poll: %s\n", e.what());
        }
    }
}

