#include "serial_manager.h"
#include <freertos/FreeRTOS.h>  // Must be first!
#include <freertos/semphr.h>

// Static mutex declaration
static SemaphoreHandle_t serialMutex = nullptr;

void SerialManager::pollSerial() {
    if (Serial.available() <= 0) {
        // Check for timeout if we're connected but haven't received data for a while
        if (isConnected && (millis() - lastActivityTime > SERIAL_CONNECTION_TIMEOUT)) {
            SerialQueueManager::getInstance().queueMessage("Serial connection timed out!");
            isConnected = false;
        }
        return;
    }

    lastActivityTime = millis();

    if (!isConnected) {
        isConnected = true;
        SerialQueueManager::getInstance().queueMessage("Serial connection detected!");
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
                    SerialQueueManager::getInstance().queueMessage("Serial buffer overflow");
                    parseState = ParseState::WAITING_FOR_START;
                }
                break;

            case ParseState::WAITING_FOR_END:
                // log inbyte
                // SerialQueueManager::getInstance().queueMessage("Received byte: %02X\n", inByte);
                if (inByte == END_MARKER) {
                    // Complete message received
                    // SerialQueueManager::getInstance().queueMessage("Complete framed message received. Type: %d, Length: %d\n", 
                    //             currentMessageType, expectedPayloadLength);
                    
                    // Process the message
                    MessageProcessor::getInstance().processBinaryMessage(receiveBuffer, bufferPosition);
                    
                    // Reset for next message
                    parseState = ParseState::WAITING_FOR_START;
                } else {
                    // Invalid end marker
                    SerialQueueManager::getInstance().queueMessage("Invalid end marker");
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
                SerialQueueManager::getInstance().queueMessage("Handshake received from browser!");
                isConnected = true;
                lastActivityTime = millis();
                sendHandshakeConfirmation();
                return;
            } else if (message.indexOf("KEEPALIVE") >= 0) {
                // Just update the last activity time
                SerialQueueManager::getInstance().queueMessage("Keepalive received from browser!");
                lastActivityTime = millis();
                return;
            }
        }
    }
    
    // SerialQueueManager::getInstance().queueMessage("Processing complete message of length %zu\n", bufferPosition);
    // Process binary message as before
    MessageProcessor::getInstance().processBinaryMessage(receiveBuffer, bufferPosition);
}

// Add this method to sendHandshakeConfirmation()
void SerialManager::sendHandshakeConfirmation() {
    // Use CRITICAL priority for browser responses
    SerialQueueManager::getInstance().queueMessage(
        "{\"status\":\"connected\",\"message\":\"ESP32 Web Serial connection established\"}", 
        SerialPriority::CRITICAL
    );
    vTaskDelay(pdMS_TO_TICKS(50));
    sendPipIdMessage();
}

void SerialManager::sendPipIdMessage() {
    if (!isConnected) return;
    
    String pipId = PreferencesManager::getInstance().getPipId();
    
    StaticJsonDocument<256> doc;
    doc["route"] = "/pip-id";
    JsonObject payload = doc.createNestedObject("payload");
    payload["pipId"] = pipId;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Use CRITICAL priority for browser communication
    SerialQueueManager::getInstance().queueMessage(jsonString, SerialPriority::CRITICAL);
    SerialQueueManager::getInstance().queueMessage("Sent PipID: " + pipId, SerialPriority::HIGH_PRIO);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void SerialManager::sendJsonMessage(const String& route, const String& status) {
    if (!isConnected) return;
    
    StaticJsonDocument<256> doc;
    doc["route"] = route;
    JsonObject payload = doc.createNestedObject("payload");
    payload["status"] = status;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Browser responses are CRITICAL
    SerialQueueManager::getInstance().queueMessage(jsonString, SerialPriority::CRITICAL);
}

// void SerialQueueManager::getInstance().queueMessage(const String& message, SerialPriority priority) {
    // SerialQueueManager::getInstance().queueMessage(message, priority);

    // static SemaphoreHandle_t serialMutex = nullptr;  // Function-local static
    
    // // Initialize once, when first called (after FreeRTOS is running)
    // if (serialMutex == nullptr) {
    //     serialMutex = xSemaphoreCreateMutex();
    //     if (serialMutex == nullptr) {
    //         SerialQueueManager::getInstance().queueMessage("ERROR: Failed to create serial mutex!");
    //         SerialQueueManager::getInstance().queueMessage(message);  // Fallback
    //         return;
    //     }
    // }
    
    // if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    //     SerialQueueManager::getInstance().queueMessage(message);
    //     Serial.flush();
    //     xSemaphoreGive(serialMutex);
    // } else {
    //     SerialQueueManager::getInstance().queueMessage(message);  // Timeout fallback
    // }
// }
