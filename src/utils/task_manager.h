#pragma once
#include <freertos/FreeRTOS.h> // MUST BE BEFORE TASK.h
#include <freertos/task.h>
#include "actuators/buttons.h"
#include "actuators/speaker.h"
#include "utils/sensor_loggers.h"
#include "sensors/battery_monitor.h"
#include "networking/serial_manager.h"
#include "actuators/led/led_animations.h"
#include "custom_interpreter/bytecode_vm.h"
#include "networking/serial_queue_manager.h"
#include "actuators/motor_driver.h"
#include "demos/demo_manager.h"

class TaskManager {
    public:
        static bool createButtonTask();
        static bool createSerialInputTask(); 
        static bool createLedTask();
        static bool createBytecodeVMTask();
        static bool createStackMonitorTask();
        static bool createDisplayTask();
        static bool createDisplayInitTask();    // NEW: Separate display initialization
        static bool isDisplayInitialized();    // NEW: Check if display init is done or in progress
        static bool createNetworkManagementTask();
        static bool createNetworkCommunicationTask();
        static bool createSerialQueueTask();
        static bool createBatteryMonitorTask();
        static bool createSpeakerTask();
        static bool createMotorTask();
        static bool createDemoManagerTask();
        static bool createCareerQuestTask();
        
        // Individual sensor task creation methods
        static bool createImuSensorTask();
        static bool createEncoderSensorTask();
        static bool createMultizoneTofSensorTask();
        static bool createSideTofSensorTask();
        static bool createColorSensorTask();
        static bool createIrSensorTask();
        static bool createSensorLoggerTask();

    private:
        static bool logTaskCreation(const char* name, bool success);
        static void buttonTask(void* parameter);
        static void serialInputTask(void* parameter);
        static void ledTask(void* parameter);
        static void bytecodeVMTask(void* parameter);
        static void stackMonitorTask(void* parameter);
        
        // Individual sensor polling tasks
        static void imuSensorTask(void* parameter);
        static void encoderSensorTask(void* parameter);
        static void multizoneTofSensorTask(void* parameter);
        static void sideTofSensorTask(void* parameter);
        static void colorSensorTask(void* parameter);
        static void irSensorTask(void* parameter);
        static void sensorLoggerTask(void* parameter);
        
        static void displayTask(void* parameter);
        static void displayInitTask(void* parameter);        // NEW: Display init task
        static void networkManagementTask(void* parameter);
        static void networkCommunicationTask(void* parameter);
        static void serialQueueTask(void* parameter);
        static void batteryMonitorTask(void* parameter);
        static void speakerTask(void* parameter);
        static void motorTask(void* parameter);
        static void demoManagerTask(void* parameter);
        static void careerQuestTask(void* parameter);

        static constexpr uint32_t BUTTON_STACK_SIZE = 4096;
        static constexpr uint32_t SERIAL_INPUT_STACK_SIZE = 10240;
        static constexpr uint32_t LED_STACK_SIZE = 6144;
        static constexpr uint32_t BYTECODE_VM_STACK_SIZE = 16384;
        static constexpr uint32_t STACK_MONITOR_STACK_SIZE = 4096;  // Increased - printStackUsage needs more space
        static constexpr uint32_t SENSOR_POLLING_STACK_SIZE = 10240; // Just polling (deprecated)
        
        // Individual sensor stack sizes
        static constexpr uint32_t IMU_SENSOR_STACK_SIZE = 4096;      // Fast, lightweight
        static constexpr uint32_t ENCODER_SENSOR_STACK_SIZE = 4096;  // Fast, lightweight  
        static constexpr uint32_t MULTIZONE_TOF_STACK_SIZE = 8192;   // Heavy processing, 64 zones
        static constexpr uint32_t SIDE_TOF_STACK_SIZE = 6144;        // Moderate processing
        static constexpr uint32_t COLOR_SENSOR_STACK_SIZE = 4096;    // Light processing
        static constexpr uint32_t IR_SENSOR_STACK_SIZE = 6144;       // Moderate processing (5 sensors)
        static constexpr uint32_t SENSOR_LOGGER_STACK_SIZE = 4096;   // Light processing - just calling logger functions
        
        static constexpr uint32_t DISPLAY_STACK_SIZE = 4096;  // I2C + display buffer operations
        static constexpr uint32_t DISPLAY_INIT_STACK_SIZE = 8192;     // Reduced from 30KB - should be sufficient with optimizations
        static constexpr uint32_t NETWORK_MANAGEMENT_STACK_SIZE = 8192;    // Heavy WiFi operations
        static constexpr uint32_t NETWORK_COMMUNICATION_STACK_SIZE = 8192; // Lightweight WebSocket polling
        static constexpr uint32_t SERIAL_QUEUE_STACK_SIZE = 10240;
        static constexpr uint32_t BATTERY_MONITOR_STACK_SIZE = 6144;
        static constexpr uint32_t SPEAKER_STACK_SIZE = 12288;
        static constexpr uint32_t MOTOR_STACK_SIZE = 4096;
        static constexpr uint32_t DEMO_MANAGER_STACK_SIZE = 6144;
        static constexpr uint32_t CAREER_QUEST_STACK_SIZE = 8192;

        // Task priorities (higher number = higher priority)
        enum class Priority : uint8_t {
            BACKGROUND = 0,     // StackMonitor, LED
            SYSTEM_CONTROL = 1, // SensorPolling, BytecodeVM 
            COMMUNICATION = 2,  // SerialInput, NetworkMgmt
            REALTIME_COMM = 3,  // NetworkComm (WebSocket needs low latency)
            CRITICAL = 4        // Buttons, SerialQueue (immediate response)
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
        static TaskHandle_t bytecodeVMTaskHandle;
        static TaskHandle_t stackMonitorTaskHandle;
        static TaskHandle_t sensorPollingTaskHandle;
        
        // Individual sensor task handles
        static TaskHandle_t imuSensorTaskHandle;
        static TaskHandle_t encoderSensorTaskHandle;
        static TaskHandle_t multizoneTofSensorTaskHandle;
        static TaskHandle_t sideTofSensorTaskHandle;
        static TaskHandle_t colorSensorTaskHandle;
        static TaskHandle_t irSensorTaskHandle;
        static TaskHandle_t sensorLoggerTaskHandle;
        
        static TaskHandle_t displayTaskHandle;
        static TaskHandle_t displayInitTaskHandle;           // NEW: Display init task handle
        static TaskHandle_t networkManagementTaskHandle;
        static TaskHandle_t networkCommunicationTaskHandle;
        static TaskHandle_t serialQueueTaskHandle;
        static TaskHandle_t batteryMonitorTaskHandle;
        static TaskHandle_t speakerTaskHandle;
        static TaskHandle_t motorTaskHandle;
        static TaskHandle_t demoManagerTaskHandle;
        static TaskHandle_t careerQuestTaskHandle;

        static void printStackUsage();
};
