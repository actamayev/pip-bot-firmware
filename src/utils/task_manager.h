#pragma once
#include <freertos/FreeRTOS.h> // MUST BE BEFORE TASK.h
#include <freertos/task.h>
#include "actuators/buttons.h"
#include "actuators/led/led_animations.h"
#include "networking/serial_queue_manager.h"

class TaskManager {
    public:
        // Specific task creators with sensible defaults
        static bool createButtonTask();
        static bool createSerialInputTask(); 
        static bool createLedTask();
        static bool createSensorTask();
        static bool createNetworkTask();
        
        // Generic task creator for custom tasks
        static bool createTask(
            const char* name,
            TaskFunction_t taskFunction, 
            uint32_t stackSize,
            uint8_t priority,
            BaseType_t coreId,
            void* parameters = NULL
        );

    private:
        static bool logTaskCreation(const char* name, bool success);
        static void buttonTask(void* parameter);
        static void serialInputTask(void* parameter);
        static void ledTask(void* parameter);
};
