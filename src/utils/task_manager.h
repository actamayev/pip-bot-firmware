#pragma once
#include <freertos/FreeRTOS.h> // MUST BE BEFORE TASK.h
#include <freertos/task.h>
#include "actuators/buttons.h"
#include "actuators/speaker.h"
#include "utils/sensor_loggers.h"
#include "sensors/battery_monitor.h"
#include "networking/serial_manager.h"
#include "sensors/sensor_initializer.h"
#include "actuators/led/led_animations.h"
#include "custom_interpreter/bytecode_vm.h"
#include "networking/serial_queue_manager.h"

class TaskManager {
    public:
        static bool createButtonTask();
        static bool createSerialInputTask(); 
        static bool createLedTask();
        static bool createMessageProcessorTask();
        static bool createBytecodeVMTask();
        static bool createStackMonitorTask();
        static bool createSensorInitTask();
        static bool createSensorPollingTask();  // Called by SensorInit when ready
        // static bool createDisplayTask();
        static bool createNetworkManagementTask();
        static bool createNetworkCommunicationTask();
        static bool createSerialQueueTask();
        static bool createBatteryMonitorTask();
        static bool createSpeakerTask();

    private:
        static bool logTaskCreation(const char* name, bool success);
        static void buttonTask(void* parameter);
        static void serialInputTask(void* parameter);
        static void ledTask(void* parameter);
        static void messageProcessorTask(void* parameter);
        static void bytecodeVMTask(void* parameter);
        static void stackMonitorTask(void* parameter);
        static void sensorInitTask(void* parameter);
        static void sensorPollingTask(void* parameter);
        // static void displayTask(void* parameter);
        static void networkManagementTask(void* parameter);
        static void networkCommunicationTask(void* parameter);
        static void serialQueueTask(void* parameter);
        static void batteryMonitorTask(void* parameter);
        static void speakerTask(void* parameter);

        static constexpr uint32_t BUTTON_STACK_SIZE = 4096;
        static constexpr uint32_t SERIAL_INPUT_STACK_SIZE = 8192;
        static constexpr uint32_t LED_STACK_SIZE = 6144;
        static constexpr uint32_t MESSAGE_PROCESSOR_STACK_SIZE = 8192; // Increase - motor + encoder logic
        static constexpr uint32_t BYTECODE_VM_STACK_SIZE = 16384;
        static constexpr uint32_t STACK_MONITOR_STACK_SIZE = 2048;  // Small - just logging
        static constexpr uint32_t SENSOR_INIT_STACK_SIZE = 6144;    // For I2C init complexity
        static constexpr uint32_t SENSOR_POLLING_STACK_SIZE = 10240; // Just polling
        // static constexpr uint32_t DISPLAY_STACK_SIZE = 4096;  // I2C + display buffer operations
        static constexpr uint32_t NETWORK_MANAGEMENT_STACK_SIZE = 8192;    // Heavy WiFi operations
        static constexpr uint32_t NETWORK_COMMUNICATION_STACK_SIZE = 8192; // Lightweight WebSocket polling
        static constexpr uint32_t SERIAL_QUEUE_STACK_SIZE = MAX_PROGRAM_SIZE;
        static constexpr uint32_t BATTERY_MONITOR_STACK_SIZE = 4096;
        static constexpr uint32_t SPEAKER_STACK_SIZE = 4096;

        // Task priorities (higher number = higher priority)
        enum class Priority : uint8_t {
            BACKGROUND = 0,     // StackMonitor, LED
            USER_PROGRAMS = 1,  // BytecodeVM 
            SYSTEM_CONTROL = 2, // MessageProcessor, SensorPolling
            COMMUNICATION = 3,  // SerialInput, NetworkMgmt
            REALTIME_COMM = 4,  // NetworkComm (WebSocket needs low latency)
            CRITICAL = 5        // Buttons, SerialQueue (immediate response)
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
            TaskHandle_t* taskHandle,  // <-- ADD THIS PARAMETER
            void* parameters = NULL
        );

        static TaskHandle_t buttonTaskHandle;
        static TaskHandle_t serialInputTaskHandle;
        static TaskHandle_t ledTaskHandle;
        static TaskHandle_t messageProcessorTaskHandle;
        static TaskHandle_t bytecodeVMTaskHandle;
        static TaskHandle_t stackMonitorTaskHandle;
        static TaskHandle_t sensorInitTaskHandle;
        static TaskHandle_t sensorPollingTaskHandle;
        // static TaskHandle_t displayTaskHandle;
        static TaskHandle_t networkManagementTaskHandle;
        static TaskHandle_t networkCommunicationTaskHandle;
        static TaskHandle_t serialQueueTaskHandle;
        static TaskHandle_t batteryMonitorTaskHandle;
        static TaskHandle_t speakerTaskHandle;

        static void printStackUsage();
};
