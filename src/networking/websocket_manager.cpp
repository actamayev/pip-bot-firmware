#include "../utils/config.h"
#include "../utils/structs.h"
#include "./websocket_manager.h"

WebSocketManager::WebSocketManager() {
    wsConnected = false;
    lastConnectionAttempt = 0;
    if (DEFAULT_ENVIRONMENT == "local") return;
    wsClient.setCACert(rootCACertificate);
}

void WebSocketManager::handleBinaryMessage(WebsocketsMessage message) {
    const uint8_t* data = (const uint8_t*)message.c_str();
    size_t length = message.length();

    if (length < 1) {
        Serial.println("Binary message too short");
        return;
    }

    // Extract the message type from the first byte
    DataMessageType messageType = static_cast<DataMessageType>(data[0]);

    switch (messageType) {
        case DataMessageType::UPDATE_AVAILABLE:
            if (length != 3) {
                Serial.println("Invalid update available message length");
            } else {
                // Extract the firmware version from bytes 1-2
                uint16_t newVersion = data[1] | (data[2] << 8); // Little-endian conversion

                Serial.printf("New firmware version available: %d\n", newVersion);

                // Store as pending version and start update
                FirmwareVersionTracker::getInstance().setPendingVersion(newVersion);
                FirmwareVersionTracker::getInstance().retrieveLatestFirmwareFromServer();
            }
            break;
        case DataMessageType::MOTOR_CONTROL:
            if (length != 5) {
                Serial.println("Invalid motor control message length");
            } else {
                MessageProcessor::getInstance().handleMotorControl(data);
            }
            break;
        case DataMessageType::SOUND_COMMAND:
            if (length != 2) {
                Serial.println("Invalid sound command message length");
            } else {
                SoundType soundType = static_cast<SoundType>(data[1]);
                MessageProcessor::getInstance().handleSoundCommand(soundType);
            }
            break;
        case DataMessageType::SPEAKER_MUTE:
            if (length != 2) {
                Serial.println("Invalid speaker mute message length");
            } else {
                SpeakerStatus status = static_cast<SpeakerStatus>(data[1]);
                MessageProcessor::getInstance().handleSpeakerMute(status);
            }
            break;
        case DataMessageType::BALANCE_CONTROL:
            if (length != 2) {
                Serial.println("Invalid balance control message length");
            } else {
                BalanceStatus status = static_cast<BalanceStatus>(data[1]);
                Serial.print("Balance Status: ");
                Serial.println(status == BalanceStatus::BALANCED ? "BALANCED" : "UNBALANCED");
                MessageProcessor::getInstance().handleBalanceCommand(status);
            }
            break;
        case DataMessageType::UPDATE_LIGHT_ANIMATION:
            if (length != 2) {
                Serial.println("Invalid balance control message length");
            } else {
                LightAnimationStatus lightAnimationStatus = static_cast<LightAnimationStatus>(data[1]);
                MessageProcessor::getInstance().handleLightCommand(lightAnimationStatus);
            }
            break;
        case DataMessageType::UPDATE_LED_COLORS:
            if (length != 19) {
                Serial.println("Invalid update led colors message length");
            } else {
                NewLightColors newLightColors;
                memcpy(&newLightColors, &data[1], sizeof(NewLightColors));
                MessageProcessor::getInstance().handleNewLightColors(newLightColors);
            }
            break;
        case DataMessageType::UPDATE_BALANCE_PIDS:
            if (length != 41) { // 1 byte for type + 40 bytes for the struct (10 floats × 4 bytes)
                Serial.println("Invalid update balance pids message length");
            } else {
                NewBalancePids newBalancePids;
                memcpy(&newBalancePids, &data[1], sizeof(NewBalancePids));
                MessageProcessor::getInstance().handleChangePidsCommand(newBalancePids);
            }
            break;
        case DataMessageType::BYTECODE_PROGRAM: {
            // First byte is the message type, the rest is bytecode
            const uint8_t* bytecodeData = data + 1;
            size_t bytecodeLength = length - 1;
            
            // Execute the bytecode
            bool success = BytecodeVM::getInstance().loadProgram(bytecodeData, bytecodeLength);

            // Send response
            if (success) {
                SendDataToServer::getInstance().sendBytecodeMessage("Bytecode successfully loaded");
            } else {
                SendDataToServer::getInstance().sendBytecodeMessage("Error loading bytecode: invalid format!");
            }
            break;
        }
        break;
        default:
            Serial.printf("Unknown message type: %d\n", static_cast<int>(messageType));
            break;
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
                break;
            case WebsocketsEvent::ConnectionClosed:
                Serial.println("WebSocket disconnected");
                this->wsConnected = false;
                break;
            case WebsocketsEvent::GotPing:
                Serial.println("Got ping");
                break;
            case WebsocketsEvent::GotPong:
                Serial.println("Got pong");
                break;
        }
    });

    // Initial connection attempt - but actual connection will be managed by pollWebSocket
    Serial.println("WebSocket connection will be managed by the polling mechanism");
    lastConnectionAttempt = 0; // Force an immediate connection attempt in the next poll
}

void WebSocketManager::sendInitialData() {
    Serial.println("WebSocket connected. Sending initial data...");
    StaticJsonDocument<SMALL_DOC_SIZE> initDoc;
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
