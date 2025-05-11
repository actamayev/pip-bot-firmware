#include "../utils/config.h"
#include "../utils/structs.h"
#include "websocket_manager.h"

WebSocketManager::WebSocketManager() {
    wsConnected = false;
    lastConnectionAttempt = 0;
    if (DEFAULT_ENVIRONMENT == "local") return;
    wsClient.setCACert(rootCACertificate);
}

void WebSocketManager::handleBinaryMessage(WebsocketsMessage message) {
    const uint8_t* data = (const uint8_t*)message.c_str();
    uint16_t length = message.length();
    
    // Check if this is a framed message (starts with START_MARKER)
    if (length >= 4 && data[0] == START_MARKER) {
        // This is a framed message. Parse it.
        uint8_t messageType = data[1];
        bool useLongFormat = (data[2] != 0);
        
        uint16_t payloadLength;
        uint16_t headerSize;
        
        if (useLongFormat) {
            // 16-bit length
            payloadLength = data[3] | (data[4] << 8);  // Little-endian
            headerSize = 5;  // START + TYPE + FORMAT + LENGTH(2)
        } else {
            // 8-bit length
            payloadLength = data[3];
            headerSize = 4;  // START + TYPE + FORMAT + LENGTH(1)
        }
        
        // Verify end marker and total length
        if (length == headerSize + payloadLength + 1 && data[headerSize + payloadLength] == END_MARKER) {
            // Extract just the message type and payload
            uint8_t* processedData = new uint8_t[payloadLength + 1];
            processedData[0] = messageType;  // Message type
            
            // Copy the actual payload (if any)
            if (payloadLength > 0) {
                memcpy(processedData + 1, data + headerSize, payloadLength);
            }
            
            // Process the extracted message
            MessageProcessor::getInstance().processBinaryMessage(processedData, payloadLength + 1);
            
            delete[] processedData;
        } else {
            Serial.println("Invalid framed message (bad end marker or length)");
        }
    } else {
        // For backward compatibility: handle legacy non-framed messages
        MessageProcessor::getInstance().processBinaryMessage(data, length);
    }
}

void WebSocketManager::connectToWebSocket() {
    wsClient.onMessage([this](WebsocketsMessage message) {
        this->handleBinaryMessage(message);
    });

    wsClient.onEvent([this](WebsocketsEvent event, String data) {
        switch (event) {
            case WebsocketsEvent::ConnectionOpened:
                Serial.println("WebSocket connected");
                this->wsConnected = true;
                this->hasKilledWiFiProcesses = false; // Reset the flag
                this->lastPingTime = millis(); // Initialize ping time
                rgbLed.set_led_blue();
                ledAnimations.stopAnimation();
                SensorPollingManager::getInstance().startPolling();
                break;
            case WebsocketsEvent::ConnectionClosed:
                Serial.println("WebSocket disconnected");
                killWiFiProcesses();
                this->wsConnected = false;
                break;
            case WebsocketsEvent::GotPing:
                Serial.println("Got ping");
                this->lastPingTime = millis(); // Update ping time
                break;
            case WebsocketsEvent::GotPong:
                Serial.println("Got pong");
                this->lastPingTime = millis(); // Update ping time
                break;
        }
    });

    lastConnectionAttempt = 0; // Force an immediate connection attempt in the next poll
}

void WebSocketManager::sendInitialData() {
    Serial.println("WebSocket connected. Sending initial data...");
    StaticJsonDocument<256> initDoc;
    initDoc["route"] = "/register";
    JsonObject payload = initDoc.createNestedObject("payload");
    payload["pipUUID"] = getPipID();
    payload["firmwareVersion"] = FirmwareVersionTracker::getInstance().getFirmwareVersion();
    String jsonString;
    serializeJson(initDoc, jsonString);

    WiFi.mode(WIFI_STA);
    wsClient.send(jsonString);
}

void WebSocketManager::pollWebSocket() {
    unsigned long currentTime = millis();
    if (currentTime - lastPollTime < POLL_INTERVAL) return;
    
    lastPollTime = currentTime;
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, cannot connect to WebSocket");
        return;
    }

    if (wsConnected && (currentTime - lastPingTime >= WS_TIMEOUT)) {
        Serial.println("WebSocket ping timeout - connection lost");
        wsConnected = false;
        killWiFiProcesses();
    }

    // Connection management - try to connect if not connected
    if (!wsConnected && (currentTime - lastConnectionAttempt >= CONNECTION_INTERVAL)) {
        lastConnectionAttempt = currentTime;
        
        Serial.println("Attempting to connect to WebSocket...");
        
        if (wsClient.connect(getWsServerUrl())) {
            Serial.println("WebSocket connected successfully");
            wsConnected = true;
            sendInitialData();
        } else {
            Serial.println("WebSocket connection failed. Will try again in 3 seconds");
        }
        return;
    }

    // Only poll if connected
    if (wsConnected) {
        try {
            wsClient.poll();
        } catch (const std::exception& e) {
            Serial.printf("Error during WebSocket poll: %s\n", e.what());
            wsConnected = false;  // Mark as disconnected to trigger reconnect
        }
    }
}

void WebSocketManager::killWiFiProcesses() {
    // This method activates when the ESP has been disconnected from WS.
    // Should only run once.
    if (hasKilledWiFiProcesses) return;
    Serial.println("Killing WiFi processes...");
    motorDriver.brake_if_moving();
    rgbLed.set_led_red();
    ledAnimations.startBreathing();
    SensorPollingManager::getInstance().stopPolling();
    hasKilledWiFiProcesses = true;
}
