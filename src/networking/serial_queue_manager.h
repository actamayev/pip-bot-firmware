#pragma once

#include "utils/config.h"
#include "utils/singleton.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

enum class SerialPriority {
    CRITICAL = 0,  // Browser command responses, critical errors
    HIGH_PRIO = 1,      // Sensor data, connection status, important events
    NORMAL = 2,    // General debug info, state changes
    LOW_PRIO = 3        // Verbose logs, frequent updates
};

struct SerialMessage {
    char message[256];
    SerialPriority priority;
    uint32_t timestamp;
    uint16_t length;
    
    SerialMessage() : priority(SerialPriority::NORMAL), timestamp(0), length(0) {
        message[0] = '\0';
    }
};

class SerialQueueManager : public Singleton<SerialQueueManager> {
    friend class Singleton<SerialQueueManager>;

    public:
        void initialize();
        bool queueMessage(const String& msg, SerialPriority priority = SerialPriority::LOW_PRIO);
        bool queueMessage(const char* msg, SerialPriority priority = SerialPriority::LOW_PRIO);
        void shutdown();
        
        // Statistics for debugging
        uint32_t getDroppedMessageCount() const { return droppedMessages; }
        uint32_t getTotalMessageCount() const { return totalMessages; }
        
    private:
        SerialQueueManager() = default;
        
        // Queue configuration
        static constexpr uint16_t SERIAL_QUEUE_SIZE = 50;
        static constexpr UBaseType_t SERIAL_TASK_PRIORITY = 2; // Medium priority
        
        // FreeRTOS objects
        QueueHandle_t messageQueue = nullptr;
        TaskHandle_t serialTaskHandle = nullptr;
        
        // Statistics
        uint32_t droppedMessages = 0;
        uint32_t totalMessages = 0;
        
        // Task function (must be static for FreeRTOS)
        static void serialOutputTaskWrapper(void* parameter);
        void serialOutputTask();
        
        // Helper functions
        bool addMessageToQueue(const SerialMessage& msg);
        void processMessage(const SerialMessage& msg);
};
