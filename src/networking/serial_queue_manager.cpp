#include "serial_queue_manager.h"
#include <Arduino.h>

void SerialQueueManager::initialize() {
    // Create the message queue
    messageQueue = xQueueCreate(SERIAL_QUEUE_SIZE, sizeof(SerialMessage));
    if (messageQueue == nullptr) {
        Serial.println("ERROR: Failed to create serial message queue!");
        return;
    }
    
    // Create the serial output task pinned to Core 1
    BaseType_t result = xTaskCreatePinnedToCore(
        serialOutputTaskWrapper,    // Task function
        "SerialOutputTask",         // Task name
        MAX_PROGRAM_SIZE,     // Stack size
        this,                       // Task parameter (pass this instance)
        SERIAL_TASK_PRIORITY,       // Priority
        &serialTaskHandle,          // Task handle
        1                           // Core 1 (same as networking)
    );
    
    if (result != pdPASS) {
        Serial.println("ERROR: Failed to create serial output task!");
        return;
    }
    
    Serial.println("SerialQueueManager initialized successfully");
}

void SerialQueueManager::shutdown() {
    if (serialTaskHandle != nullptr) {
        vTaskDelete(serialTaskHandle);
        serialTaskHandle = nullptr;
    }
    
    if (messageQueue != nullptr) {
        vQueueDelete(messageQueue);
        messageQueue = nullptr;
    }
}

bool SerialQueueManager::queueMessage(const String& msg, SerialPriority priority) {
    return queueMessage(msg.c_str(), priority);
}

bool SerialQueueManager::queueMessage(const char* msg, SerialPriority priority) {
    if (messageQueue == nullptr || msg == nullptr) {
        return false;
    }
    
    // Create message structure
    SerialMessage message;
    message.priority = priority;
    message.timestamp = millis();
    
    // Copy message, ensuring null termination and not exceeding buffer
    size_t msgLen = strlen(msg);
    if (msgLen >= sizeof(message.message)) {
        msgLen = sizeof(message.message) - 1;
    }
    
    strncpy(message.message, msg, msgLen);
    message.message[msgLen] = '\0';
    message.length = msgLen;
    
    totalMessages++;
    
    return addMessageToQueue(message);
}

bool SerialQueueManager::addMessageToQueue(const SerialMessage& msg) {
    // Try to send to queue without blocking
    BaseType_t result = xQueueSend(messageQueue, &msg, 0);
    
    if (result == pdPASS) {
        return true; // Successfully queued
    }
    
    // Queue is full - remove oldest message to make room
    SerialMessage oldMessage;
    if (xQueueReceive(messageQueue, &oldMessage, 0) == pdPASS) {
        droppedMessages++;
        
        // Now try to add the new message
        result = xQueueSend(messageQueue, &msg, 0);
        if (result == pdPASS) {
            return true;
        }
    }
    
    // Failed to make room or add message
    droppedMessages++;
    return false;
}

void SerialQueueManager::serialOutputTaskWrapper(void* parameter) {
    SerialQueueManager* instance = static_cast<SerialQueueManager*>(parameter);
    instance->serialOutputTask();
}

void SerialQueueManager::serialOutputTask() {
    SerialMessage message;
    
    // Array to hold messages for priority sorting
    SerialMessage priorityBuffer[10]; // Process up to 10 messages at once for priority
    int bufferCount = 0;
    
    while (true) {
        // Wait for at least one message (blocking)
        if (xQueueReceive(messageQueue, &message, portMAX_DELAY) == pdPASS) {
            priorityBuffer[0] = message;
            bufferCount = 1;
            
            // Check if there are more messages available for batch processing
            while (bufferCount < 10 && xQueueReceive(messageQueue, &message, 0) == pdPASS) {
                priorityBuffer[bufferCount] = message;
                bufferCount++;
            }
            
            // Sort by priority (simple bubble sort - good enough for small arrays)
            for (int i = 0; i < bufferCount - 1; i++) {
                for (int j = 0; j < bufferCount - i - 1; j++) {
                    if (static_cast<int>(priorityBuffer[j].priority) > static_cast<int>(priorityBuffer[j + 1].priority)) {
                        SerialMessage temp = priorityBuffer[j];
                        priorityBuffer[j] = priorityBuffer[j + 1];
                        priorityBuffer[j + 1] = temp;
                    }
                }
            }
            
            // Process all messages in priority order
            for (int i = 0; i < bufferCount; i++) {
                processMessage(priorityBuffer[i]);
            }
        }
    }
}

void SerialQueueManager::processMessage(const SerialMessage& msg) {
    // This is the ONLY place in the entire system that calls Serial.print/println
    // Add priority prefix for debugging (optional)
    const char* priorityStr = "";
    switch (msg.priority) {
        case SerialPriority::CRITICAL: priorityStr = "[CRIT] "; break;
        case SerialPriority::HIGH_PRIO:     priorityStr = "[HIGH] "; break;
        case SerialPriority::NORMAL:   priorityStr = ""; break; // No prefix for normal
        case SerialPriority::LOW_PRIO:      priorityStr = "[LOW] "; break;
    }
    
    // Output the message
    Serial.print(priorityStr);
    Serial.println(msg.message);
    Serial.flush(); // Ensure immediate transmission
}
