#include "serial_manager.h"

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

    // Read available bytes and process according to the current state
    while (Serial.available() > 0) {
        uint8_t inByte = Serial.read();
        
        switch (parseState) {
            case ParseState::WAITING_FOR_START:
                if (inByte == START_MARKER) {
                    // Start of a new message
                    bufferPosition = 0;
                    parseState = ParseState::READING_MESSAGE_TYPE;
                }
                break;
                
            case ParseState::READING_MESSAGE_TYPE:
                currentMessageType = inByte;
                receiveBuffer[bufferPosition++] = inByte;  // Store message type as first byte
                parseState = ParseState::READING_FORMAT_FLAG;
                break;
                
            case ParseState::READING_FORMAT_FLAG:
                useLongFormat = (inByte != 0);
                parseState = ParseState::READING_LENGTH_BYTE1;
                break;
                
            case ParseState::READING_LENGTH_BYTE1:
                if (useLongFormat) {
                    // First byte of 16-bit length (little-endian)
                    expectedPayloadLength = inByte;
                    parseState = ParseState::READING_LENGTH_BYTE2;
                } else {
                    // 8-bit length
                    expectedPayloadLength = inByte;
                    
                    // If payload length is 0, skip to waiting for end marker
                    if (expectedPayloadLength == 0) {
                        parseState = ParseState::WAITING_FOR_END;
                    } else {
                        parseState = ParseState::READING_PAYLOAD;
                    }
                }
                break;
                
            case ParseState::READING_LENGTH_BYTE2:
                // Second byte of 16-bit length (little-endian)
                expectedPayloadLength |= (inByte << 8);
                
                // If payload length is 0, skip to waiting for end marker
                if (expectedPayloadLength == 0) {
                    parseState = ParseState::WAITING_FOR_END;
                } else {
                    parseState = ParseState::READING_PAYLOAD;
                }
                break;

            case ParseState::READING_PAYLOAD:
                // Store payload byte
                if (bufferPosition < sizeof(receiveBuffer)) {
                    receiveBuffer[bufferPosition++] = inByte;
                    
                    // Check if we've read the complete payload
                    if (bufferPosition >= expectedPayloadLength + 1) {  // +1 for message type
                        parseState = ParseState::WAITING_FOR_END;
                    }
                } else {
                    // Buffer overflow
                    Serial.println("Serial buffer overflow");
                    parseState = ParseState::WAITING_FOR_START;
                }
                break;

            case ParseState::WAITING_FOR_END:
                // log inbyte
                // Serial.printf("Received byte: %02X\n", inByte);
                if (inByte == END_MARKER) {
                    // Complete message received
                    // Serial.printf("Complete framed message received. Type: %d, Length: %d\n", 
                    //             currentMessageType, expectedPayloadLength);
                    
                    // Process the message
                    MessageProcessor::getInstance().processBinaryMessage(receiveBuffer, bufferPosition);
                    
                    // Reset for next message
                    parseState = ParseState::WAITING_FOR_START;
                } else {
                    // Invalid end marker
                    Serial.println("Invalid end marker");
                    parseState = ParseState::WAITING_FOR_START;
                }
                break;
        }
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
                Serial.println("Keepalive received from browser!");
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
