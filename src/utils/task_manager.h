#pragma once
#include <freertos/FreeRTOS.h> // MUST BE BEFORE TASK.h
#include <freertos/task.h>

#include "actuators/buttons.h"
#include "actuators/dance_manager.h"
#include "actuators/led/led_animations.h"
#include "actuators/motor_driver.h"
#include "actuators/speaker.h"
#include "custom_interpreter/bytecode_vm.h"
#include "demos/demo_manager.h"
#include "games/game_manager.h"
#include "networking/serial_manager.h"
#include "networking/serial_queue_manager.h"
#include "sensors/battery_monitor.h"
#include "utils/sensor_loggers.h"

class TaskManager {
  public:
    static bool create_button_task();
    static bool create_serial_input_task();
    static bool create_led_task();
    static bool create_bytecode_vm_task();
    static bool create_stack_monitor_task();
    static bool create_display_task();
    static bool create_display_init_task(); // NEW: Separate display initialization
    static bool is_display_initialized();   // NEW: Check if display init is done or in progress
    static bool create_network_management_task();
    static bool create_send_sensor_data_task();
    static bool create_web_socket_polling_task();
    static bool create_serial_queue_task();
    static bool create_battery_monitor_task();
    static bool create_speaker_task();
    static bool create_motor_task();
    static bool create_demo_manager_task();
    static bool create_game_manager_task();
    static bool create_career_quest_task();

    // Individual sensor task creation methods
    static bool create_imu_sensor_task();
    static bool create_encoder_sensor_task();
    static bool create_multizone_tof_sensor_task();
    static bool create_side_tof_sensor_task();
    static bool create_color_sensor_task();
    static bool create_sensor_logger_task();

    static bool create_heartbeat_task();

  private:
    static bool log_task_creation(const char* name, bool success);
    static void button_task(void* parameter);
    static void serial_input_task(void* parameter);
    static void led_task(void* parameter);
    static void bytecode_vm_task(void* parameter);
    static void stack_monitor_task(void* parameter);

    // Individual sensor polling tasks
    static void imu_sensor_task(void* parameter);
    static void encoder_sensor_task(void* parameter);
    static void multizone_tof_sensor_task(void* parameter);
    static void side_tof_sensor_task(void* parameter);
    static void color_sensor_task(void* parameter);
    static void sensor_logger_task(void* parameter);

    static void display_task(void* parameter);
    static void display_init_task(void* parameter); // NEW: Display init task
    static void network_management_task(void* parameter);
    static void send_sensor_data_task(void* parameter);
    static void web_socket_polling_task(void* parameter);
    static void serial_queue_task(void* parameter);
    static void battery_monitor_task(void* parameter);
    static void speaker_task(void* parameter);
    static void motor_task(void* parameter);
    static void demo_manager_task(void* parameter);
    static void game_manager_task(void* parameter);
    static void career_quest_task(void* parameter);

    static constexpr uint32_t BUTTON_STACK_SIZE = 4096;
    static constexpr uint32_t SERIAL_INPUT_STACK_SIZE = 10240;
    static constexpr uint32_t LED_STACK_SIZE = 6144;
    static constexpr uint32_t BYTECODE_VM_STACK_SIZE = 16384;
    static constexpr uint32_t STACK_MONITOR_STACK_SIZE = 4096;   // Increased - print_stack_usage needs more space
    static constexpr uint32_t SENSOR_POLLING_STACK_SIZE = 10240; // Just polling (deprecated)

    // Individual sensor stack sizes
    static constexpr uint32_t IMU_SENSOR_STACK_SIZE = 4096;     // Fast, lightweight
    static constexpr uint32_t ENCODER_SENSOR_STACK_SIZE = 4096; // Fast, lightweight
    static constexpr uint32_t MULTIZONE_TOF_STACK_SIZE = 8192;  // Heavy processing, 64 zones
    static constexpr uint32_t SIDE_TOF_STACK_SIZE = 6144;       // Moderate processing
    static constexpr uint32_t COLOR_SENSOR_STACK_SIZE = 4096;   // Light processing
    static constexpr uint32_t SENSOR_LOGGER_STACK_SIZE = 4096;  // Light processing - just calling logger functions

    static constexpr uint32_t DISPLAY_STACK_SIZE = 4096;            // I2C + display buffer operations
    static constexpr uint32_t DISPLAY_INIT_STACK_SIZE = 8192;       // Reduced from 30KB - should be sufficient with optimizations
    static constexpr uint32_t NETWORK_MANAGEMENT_STACK_SIZE = 8192; // Heavy WiFi operations
    static constexpr uint32_t SEND_SENSOR_DATA_STACK_SIZE = 8192;   // Sensor data transmission
    static constexpr uint32_t WEBSOCKET_POLLING_STACK_SIZE = 6144;  // Lightweight WebSocket polling
    static constexpr uint32_t SERIAL_QUEUE_STACK_SIZE = 10240;
    static constexpr uint32_t BATTERY_MONITOR_STACK_SIZE = 6144;
    static constexpr uint32_t SPEAKER_STACK_SIZE = 12288;
    static constexpr uint32_t MOTOR_STACK_SIZE = 4096;
    static constexpr uint32_t DEMO_MANAGER_STACK_SIZE = 6144;
    static constexpr uint32_t GAME_MANAGER_STACK_SIZE = 8192;
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
        CORE_0 = 0, // Hardware/Real-time
        CORE_1 = 1  // Application/Communication
    };

    // Generic task creator for custom tasks
    static bool create_task(const char* name, TaskFunction_t task_function, uint32_t stack_size, Priority priority, Core core_id,
                            TaskHandle_t* task_handle, // <-- ADD THIS PARAMETER
                            void* parameters = nullptr);

    static TaskHandle_t button_task_handle;
    static TaskHandle_t serial_input_task_handle;
    static TaskHandle_t led_task_handle;
    static TaskHandle_t bytecode_vm_task_handle;
    static TaskHandle_t stack_monitor_task_handle;
    static TaskHandle_t sensor_polling_task_handle;

    // Individual sensor task handles
    static TaskHandle_t imu_sensor_task_handle;
    static TaskHandle_t encoder_sensor_task_handle;
    static TaskHandle_t multizone_tof_sensor_task_handle;
    static TaskHandle_t side_tof_sensor_task_handle;
    static TaskHandle_t color_sensor_task_handle;
    static TaskHandle_t sensor_logger_task_handle;

    static TaskHandle_t display_task_handle;
    static TaskHandle_t display_init_task_handle; // NEW: Display init task handle
    static TaskHandle_t network_management_task_handle;
    static TaskHandle_t send_sensor_data_task_handle;
    static TaskHandle_t web_socket_polling_task_handle;
    static TaskHandle_t serial_queue_task_handle;
    static TaskHandle_t battery_monitor_task_handle;
    static TaskHandle_t speaker_task_handle;
    static TaskHandle_t motor_task_handle;
    static TaskHandle_t demo_manager_task_handle;
    static TaskHandle_t game_manager_task_handle;
    static TaskHandle_t career_quest_task_handle;

    static void print_stack_usage();

    static void heartbeat_task(void* parameter);

    // ... existing stack sizes ...
    static constexpr uint32_t HEARTBEAT_STACK_SIZE = 4096;

    // ... existing task handles ...
    static TaskHandle_t heartbeat_task_handle;
};
