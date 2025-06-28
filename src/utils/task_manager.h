#pragma once
#include <freertos/FreeRTOS.h> // MUST BE BEFORE TASK.h
#include <freertos/task.h>
#include "actuators/buttons.h"
#include "networking/serial_manager.h"
#include "actuators/led/led_animations.h"
#include "networking/serial_queue_manager.h"
#include "custom_interpreter/bytecode_vm.h"

class TaskManager {
    public:
        // Specific task creators with sensible defaults
        static bool createButtonTask();
        static bool createSerialInputTask(); 
        static bool createLedTask();
        static bool createMessageProcessorTask();
        static bool createBytecodeVMTask();
        static bool createSensorTask();
        static bool createNetworkTask();
        static void printStackUsage();

    private:
        static bool logTaskCreation(const char* name, bool success);
        static void buttonTask(void* parameter);
        static void serialInputTask(void* parameter);
        static void ledTask(void* parameter);
        static void messageProcessorTask(void* parameter);
        static void bytecodeVMTask(void* parameter);

        static constexpr uint32_t BUTTON_STACK_SIZE = 4096;
        static constexpr uint32_t SERIAL_INPUT_STACK_SIZE = 8192;
        static constexpr uint32_t LED_STACK_SIZE = 6144;
        static constexpr uint32_t MESSAGE_PROCESSOR_STACK_SIZE = 8192; // Increase - motor + encoder logic
        static constexpr uint32_t BYTECODE_VM_STACK_SIZE = 16384;
        static constexpr uint32_t SENSOR_STACK_SIZE = 20480;
        static constexpr uint32_t NETWORK_STACK_SIZE = 8192;
        
        // Task priorities (higher number = higher priority)
        enum class Priority : uint8_t {
            LOWEST = 0,      // Background tasks
            LOW_MEDIUM = 1,         // User programs, sensors
            MEDIUM = 2,      // Motor control, system functions  
            MEDIUM_HIGH = 3,        // Communication, serial
            HIGHEST = 4      // User interaction (buttons)
        };

        // Core assignments
        enum class Core : BaseType_t {
            CORE_0 = 0,      // Hardware/Real-time
            CORE_1 = 1       // Application/Communication
        };

        // Generic task creator for custom tasks
        static bool createTask(
            const char* name,
            TaskFunction_t taskFunction, 
            uint32_t stackSize,
            Priority priority,
            Core coreId,
            void* parameters = NULL
        );

        static TaskHandle_t buttonTaskHandle;
        static TaskHandle_t serialInputTaskHandle;
        static TaskHandle_t ledTaskHandle;
        static TaskHandle_t messageProcessorTaskHandle;
        static TaskHandle_t bytecodeVMTaskHandle;
        static TaskHandle_t sensorTaskHandle;
        static TaskHandle_t networkTaskHandle;
};
