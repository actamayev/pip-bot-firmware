#include "serial_queue_manager.h"

void SerialQueueManager::initialize() {
    // Create the message queue
    _messageQueue = xQueueCreate(SERIAL_QUEUE_SIZE, sizeof(SerialMessage));
    if (_messageQueue == nullptr) {
        Serial.println("ERROR: Failed to create serial message queue!");
        return;
    }

    Serial.println("SerialQueueManager initialized - task will be created by TaskManager");
}

bool SerialQueueManager::queueMessage(const String& msg, SerialPriority priority) {
    return queueMessage(msg.c_str(), priority);
}

bool SerialQueueManager::queueMessage(const char* msg, SerialPriority priority) {
    if (_messageQueue == nullptr || msg == nullptr) {
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

    return addMessageToQueue(message);
}

bool SerialQueueManager::addMessageToQueue(const SerialMessage& msg) {
    // Try to send to queue without blocking
    BaseType_t result = xQueueSend(_messageQueue, &msg, 0);

    if (result == pdPASS) {
        return true; // Successfully queued
    }

    // Queue is full - remove oldest message to make room
    SerialMessage oldMessage;
    if (xQueueReceive(_messageQueue, &oldMessage, 0) == pdPASS) {
        // Now try to add the new message
        result = xQueueSend(_messageQueue, &msg, 0);
        if (result == pdPASS) {
            return true;
        }
    }

    return false;
}

void SerialQueueManager::serialOutputTask() {
    SerialMessage message;

    // Array to hold messages for priority sorting
    SerialMessage priorityBuffer[10]; // Process up to 10 messages at once for priority
    int bufferCount = 0;

    while (true) {
        // Wait for at least one message (blocking)
        if (xQueueReceive(_messageQueue, &message, portMAX_DELAY) == pdPASS) {
            priorityBuffer[0] = message;
            bufferCount = 1;

            // Check if there are more messages available for batch processing
            while (bufferCount < 10 && xQueueReceive(_messageQueue, &message, 0) == pdPASS) {
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
    const char* priorityStr = "";
    switch (msg.priority) {
        case SerialPriority::CRITICAL:
            priorityStr = "[CRIT] ";
            break;
        case SerialPriority::HIGH_PRIO:
            priorityStr = "[HIGH] ";
            break;
        case SerialPriority::NORMAL:
            priorityStr = "";
            break;
        case SerialPriority::LOW_PRIO:
            priorityStr = "[LOW] ";
            break;
    }

    Serial.print(priorityStr);
    Serial.println(msg.message);
    Serial.flush();
}
