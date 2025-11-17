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

bool SerialQueueManager::queue_message(const String& msg, SerialPriority priority) {
    return queue_message(msg.c_str(), priority);
}

bool SerialQueueManager::queue_message(const char* msg, SerialPriority priority) {
    if (_messageQueue == nullptr || msg == nullptr) {
        return false;
    }

    // Create message structure
    SerialMessage message;
    message.priority = priority;
    message.timestamp = millis();

    // Copy message, ensuring null termination and not exceeding buffer
    size_t msg_len = strlen(msg);
    if (msg_len >= sizeof(message.message)) {
        msg_len = sizeof(message.message) - 1;
    }

    strncpy(message.message, msg, msg_len);
    message.message[msg_len] = '\0';
    message.length = msg_len;

    return add_message_to_queue(message);
}

bool SerialQueueManager::add_message_to_queue(const SerialMessage& msg) {
    // Try to send to queue without blocking
    BaseType_t result = xQueueSend(_messageQueue, &msg, 0);

    if (result == pdPASS) {
        return true; // Successfully queued
    }

    // Queue is full - remove oldest message to make room
    SerialMessage old_message;
    if (xQueueReceive(_messageQueue, &old_message, 0) == pdPASS) {
        // Now try to add the new message
        result = xQueueSend(_messageQueue, &msg, 0);
        if (result == pdPASS) {
            return true;
        }
    }

    return false;
}

void SerialQueueManager::serial_output_task() {
    SerialMessage message;

    // Array to hold messages for priority sorting
    SerialMessage priority_buffer[10]; // Process up to 10 messages at once for priority
    int buffer_count = 0;

    while (true) {
        // Wait for at least one message (blocking)
        if (xQueueReceive(_messageQueue, &message, portMAX_DELAY) == pdPASS) {
            priority_buffer[0] = message;
            buffer_count = 1;

            // Check if there are more messages available for batch processing
            while (buffer_count < 10 && xQueueReceive(_messageQueue, &message, 0) == pdPASS) {
                priority_buffer[buffer_count] = message;
                buffer_count++;
            }

            // Sort by priority (simple bubble sort - good enough for small arrays)
            for (int i = 0; i < buffer_count - 1; i++) {
                for (int j = 0; j < buffer_count - i - 1; j++) {
                    if (static_cast<int>(priority_buffer[j].priority) > static_cast<int>(priority_buffer[j + 1].priority)) {
                        SerialMessage temp = priority_buffer[j];
                        priority_buffer[j] = priority_buffer[j + 1];
                        priority_buffer[j + 1] = temp;
                    }
                }
            }

            // Process all messages in priority order
            for (int i = 0; i < buffer_count; i++) {
                process_message(priority_buffer[i]);
            }
        }
    }
}

void SerialQueueManager::process_message(const SerialMessage& msg) {
    const char* priority_str = "";
    switch (msg.priority) {
        case SerialPriority::CRITICAL:
            priority_str = "[CRIT] ";
            break;
        case SerialPriority::HIGH_PRIO:
            priority_str = "[HIGH] ";
            break;
        case SerialPriority::NORMAL:
            priority_str = "";
            break;
        case SerialPriority::LOW_PRIO:
            priority_str = "[LOW] ";
            break;
    }

    Serial.print(priority_str);
    Serial.println(msg.message);
    Serial.flush();
}
