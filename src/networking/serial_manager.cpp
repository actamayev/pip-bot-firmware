#include "serial_manager.h"
#include <freertos/FreeRTOS.h>  // Must be first!
#include <freertos/semphr.h>

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

// Add this method to sendHandshakeConfirmation()
void SerialManager::sendHandshakeConfirmation() {
    // Use CRITICAL priority for browser responses
    SerialQueueManager::getInstance().queueMessage(
        "{\"status\":\"connected\",\"message\":\"ESP32 Web Serial connection established\"}", 
        SerialPriority::CRITICAL
    );
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
    
    // Auto-start WiFi scan for user convenience
    SerialQueueManager::getInstance().queueMessage("Auto-starting WiFi scan...");
    WiFiManager::getInstance().startAsyncScan();
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

void SerialManager::sendSavedNetworksResponse(const std::vector<WiFiCredentials>& networks) {
    if (!isConnected) return;
    
    // Build JSON response
    StaticJsonDocument<1024> doc;
    doc["route"] = "/saved-networks";
    JsonArray payload = doc.createNestedArray("payload");
    
    for (size_t i = 0; i < networks.size(); i++) {
        JsonObject network = payload.createNestedObject();
        network["ssid"] = networks[i].ssid;
        network["index"] = i;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Use CRITICAL priority for browser responses
    SerialQueueManager::getInstance().queueMessage(jsonString, SerialPriority::CRITICAL);
}

void SerialManager::sendScanResultsResponse(const std::vector<WiFiNetworkInfo>& networks) {
    if (!isConnected) return;
    
    SerialQueueManager::getInstance().queueMessage("Sending " + String(networks.size()) + " networks individually...");
    
    // Send each network as individual message
    for (size_t i = 0; i < networks.size(); i++) {
        StaticJsonDocument<256> doc;
        doc["route"] = "/scan-result-item";
        JsonObject payload = doc.createNestedObject("payload");
        payload["ssid"] = networks[i].ssid;
        payload["rssi"] = networks[i].rssi;
        payload["encrypted"] = (networks[i].encryptionType != WIFI_AUTH_OPEN);
        
        String jsonString;
        serializeJson(doc, jsonString);
        
        SerialQueueManager::getInstance().queueMessage(jsonString, SerialPriority::CRITICAL);
        
        // Small delay between messages to prevent overwhelming the queue
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // Send completion message
    StaticJsonDocument<128> completeDoc;
    completeDoc["route"] = "/scan-complete";
    JsonObject completePayload = completeDoc.createNestedObject("payload");
    completePayload["totalNetworks"] = networks.size();
    
    String completeJsonString;
    serializeJson(completeDoc, completeJsonString);
    
    SerialQueueManager::getInstance().queueMessage(completeJsonString, SerialPriority::CRITICAL);
    SerialQueueManager::getInstance().queueMessage("Scan results transmission complete");
}

void SerialManager::sendScanStartedMessage() {
    if (!isConnected) return;
    
    StaticJsonDocument<128> doc;
    doc["route"] = "/scan-started";
    JsonObject payload = doc.createNestedObject("payload");
    payload["scanning"] = true;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Use CRITICAL priority for browser responses
    SerialQueueManager::getInstance().queueMessage(jsonString, SerialPriority::CRITICAL);
}
