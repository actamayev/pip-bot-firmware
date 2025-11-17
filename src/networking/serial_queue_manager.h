#pragma once
#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "utils/config.h"
#include "utils/singleton.h"

enum class SerialPriority {
    CRITICAL = 0,  // Browser command responses, critical errors
    HIGH_PRIO = 1, // Sensor data, connection status, important events
    NORMAL = 2,    // General debug info, state changes
    LOW_PRIO = 3   // Verbose logs, frequent updates
};

struct SerialMessage {
    char message[256]{};
    SerialPriority priority{SerialPriority::NORMAL};
    uint32_t timestamp{0};
    uint16_t length{0};

    SerialMessage() {
        message[0] = '\0';
    }
};

class SerialQueueManager : public Singleton<SerialQueueManager> {
    friend class Singleton<SerialQueueManager>;

  public:
    void initialize();
    bool queueMessage(const String& msg, SerialPriority priority = SerialPriority::LOW_PRIO);
    bool queueMessage(const char* msg, SerialPriority priority = SerialPriority::LOW_PRIO);
    void serialOutputTask();

  private:
    SerialQueueManager() = default;

    // Queue configuration
    static constexpr uint16_t SERIAL_QUEUE_SIZE = 50;

    // FreeRTOS objects
    QueueHandle_t _messageQueue = nullptr;

    // Helper functions
    bool addMessageToQueue(const SerialMessage& msg);
    void processMessage(const SerialMessage& msg);
};
