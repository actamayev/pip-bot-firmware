#include "./serial_manager.h"

void SerialManager::pollSerial() {
    if (Serial.available() <= 0) {
        // Check for timeout if we're connected but haven't received data for a while
        if (isConnected && (millis() - lastActivityTime > SERIAL_CONNECTION_TIMEOUT)) {
            Serial.println("Serial connection timed out!");
            isConnected = false;
        }
        return;
    }

    lastActivityTime = millis();

    if (!isConnected) {
        isConnected = true;
        Serial.println("Serial connection detected!");
        sendHandshakeConfirmation();
    }

    // print sizeof receiveBuffer
    Serial.printf("bufferPosition: %zu\n", bufferPosition);

    while (Serial.available() > 0 && bufferPosition < sizeof(receiveBuffer)) {
        receiveBuffer[bufferPosition++] = Serial.read();
        
        if (bufferPosition == 1) {
            messageStarted = true;
        }
        
        if (messageStarted && bufferPosition > 0) {
            DataMessageType messageType = static_cast<DataMessageType>(receiveBuffer[0]);
            uint16_t expectedLength = 0;
            
            // Determine expected message length based on message type
            switch (messageType) {
                case DataMessageType::UPDATE_AVAILABLE:
                    expectedLength = 3;
                    break;
                case DataMessageType::MOTOR_CONTROL:
                    expectedLength = 5;
                    break;
                case DataMessageType::SOUND_COMMAND:
                case DataMessageType::SPEAKER_MUTE:
                case DataMessageType::BALANCE_CONTROL:
                case DataMessageType::UPDATE_LIGHT_ANIMATION:
                case DataMessageType::OBSTACLE_AVOIDANCE:
                    expectedLength = 2;
                    break;
                case DataMessageType::UPDATE_LED_COLORS:
                    expectedLength = 25;
                    break;
                case DataMessageType::UPDATE_BALANCE_PIDS:
                    expectedLength = 41;
                    break;
                case DataMessageType::STOP_SANDBOX_CODE:
                case DataMessageType::SERIAL_HANDSHAKE:  // Add handshake
                case DataMessageType::SERIAL_KEEPALIVE:  // Add keepalive
                case DataMessageType::SERIAL_END:  // Add keepalive
                    expectedLength = 1;  // These only need 1 byte (the message type)
                    break;
                case DataMessageType::BYTECODE_PROGRAM:
                    // Variable length - process when no more data available
                    break;
                case DataMessageType::UPDATE_HEADLIGHTS:
                    expectedLength = 2;
                    break;
                default:
                    Serial.printf("Unknown message type in serial manager: %d\n", static_cast<int>(messageType));
                    bufferPosition = 0;
                    messageStarted = false;
                    break;
            }

            // Process if we have complete message
            if (expectedLength > 0 && bufferPosition >= expectedLength) {
                processCompleteMessage();
                bufferPosition = 0;
                messageStarted = false;
            }

            // Special handling for bytecode which has variable length
            if (messageType == DataMessageType::BYTECODE_PROGRAM && Serial.available() == 0) {
                Serial.println("Bytecode program");
                processCompleteMessage();
                bufferPosition = 0;
                messageStarted = false;
            }
        }
    }
    
    if (bufferPosition >= sizeof(receiveBuffer)) {
        Serial.println("Serial buffer overflow");
        bufferPosition = 0;
        messageStarted = false;
    }
}

void SerialManager::processCompleteMessage() {  
    // Check if the message is a text message (handshake or keepalive)
    if (messageStarted && bufferPosition > 0) {
        // Check if this is a text message by checking first byte isn't a valid DataMessageType
        if (receiveBuffer[0] >= static_cast<uint8_t>(DataMessageType::SERIAL_HANDSHAKE)) {
            // Convert buffer to string for text-based commands
            String message = "";
            for (uint16_t i = 0; i < bufferPosition; i++) {
                message += (char)receiveBuffer[i];
            }
            
            if (message.indexOf("HANDSHAKE") >= 0) {
                Serial.println("Handshake received from browser!");
                isConnected = true;
                lastActivityTime = millis();
                sendHandshakeConfirmation();
                return;
            } else if (message.indexOf("KEEPALIVE") >= 0) {
                // Just update the last activity time
                lastActivityTime = millis();
                return;
            }
        }
    }
    
    Serial.printf("Processing complete message of length %zu\n", bufferPosition);
    // Process binary message as before
    MessageProcessor::getInstance().processBinaryMessage(receiveBuffer, bufferPosition);
}

void SerialManager::sendHandshakeConfirmation() {
    Serial.println("{\"status\":\"connected\",\"message\":\"ESP32 Web Serial connection established\"}");
}

void SerialManager::sendJsonToSerial(const String& jsonData) {
    if (!isConnected) return;
    Serial.println(jsonData);
}
