#include "serial_manager.h"

void SerialManager::pollSerial() {
    if (Serial.available() <= 0) return;
    lastActivityTime = millis();
    
    if (!isConnected) {
        isConnected = true;
        sendHandshakeConfirmation();
    }
    
    while (Serial.available() > 0 && bufferPosition < sizeof(receiveBuffer)) {
        receiveBuffer[bufferPosition++] = Serial.read();
        
        if (bufferPosition == 1) {
            messageStarted = true;
        }
        
        if (messageStarted && bufferPosition > 0) {
            DataMessageType messageType = static_cast<DataMessageType>(receiveBuffer[0]);
            size_t expectedLength = 0;
            
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
                    expectedLength = 19;
                    break;
                case DataMessageType::UPDATE_BALANCE_PIDS:
                    expectedLength = 41;
                    break;
                case DataMessageType::STOP_SANDBOX_CODE:
                    expectedLength = 1;
                    break;
                case DataMessageType::BYTECODE_PROGRAM:
                    // Variable length - process when no more data available
                    break;
                default:
                    Serial.printf("Unknown message type: %d\n", static_cast<int>(messageType));
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
    MessageProcessor::getInstance().processBinaryMessage(receiveBuffer, bufferPosition);
}

void SerialManager::sendHandshakeConfirmation() {
    Serial.println("{\"status\":\"connected\",\"message\":\"ESP32 Web Serial connection established\"}");
}

void SerialManager::sendJsonToSerial(const String& jsonData) {
    if (!isConnected) return;
    Serial.println(jsonData);
}
