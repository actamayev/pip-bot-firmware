#include "task_manager.h"

#include "career_quest/career_quest_triggers.h"
#include "sensors/sensor_initializer.h"

TaskHandle_t TaskManager::button_task_handle = nullptr;
TaskHandle_t TaskManager::serial_input_task_handle = nullptr;
TaskHandle_t TaskManager::led_task_handle = nullptr;
TaskHandle_t TaskManager::bytecode_vm_task_handle = nullptr;
TaskHandle_t TaskManager::stack_monitor_task_handle = nullptr;
TaskHandle_t TaskManager::sensor_polling_task_handle = nullptr;

// Individual sensor task handles
TaskHandle_t TaskManager::imu_sensor_task_handle = nullptr;
TaskHandle_t TaskManager::encoder_sensor_task_handle = nullptr;
TaskHandle_t TaskManager::multizone_tof_sensor_task_handle = nullptr;
TaskHandle_t TaskManager::side_tof_sensor_task_handle = nullptr;
TaskHandle_t TaskManager::color_sensor_task_handle = nullptr;
TaskHandle_t TaskManager::sensor_logger_task_handle = nullptr;
TaskHandle_t TaskManager::display_task_handle = nullptr;
TaskHandle_t TaskManager::network_management_task_handle = nullptr;
TaskHandle_t TaskManager::send_sensor_data_task_handle = nullptr;
TaskHandle_t TaskManager::web_socket_polling_task_handle = nullptr;
TaskHandle_t TaskManager::serial_queue_task_handle = nullptr;
TaskHandle_t TaskManager::battery_monitor_task_handle = nullptr;
TaskHandle_t TaskManager::speaker_task_handle = nullptr;
TaskHandle_t TaskManager::motor_task_handle = nullptr;
TaskHandle_t TaskManager::demo_manager_task_handle = nullptr;
TaskHandle_t TaskManager::game_manager_task_handle = nullptr;
TaskHandle_t TaskManager::career_quest_task_handle = nullptr;
TaskHandle_t TaskManager::display_init_task_handle = nullptr;

void TaskManager::button_task(void* parameter) {
    setup_button_loggers();

    for (;;) {
        Buttons::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void TaskManager::serial_input_task(void* parameter) {
    for (;;) {
        SerialManager::get_instance().poll_serial();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void TaskManager::led_task(void* parameter) {
    for (;;) {
        led_animations.update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::bytecode_vm_task(void* parameter) {
    for (;;) {
        BytecodeVM::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::stack_monitor_task(void* parameter) {
    for (;;) {
        print_stack_usage();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskManager::display_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Display task started");

    for (;;) {
        DisplayScreen::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(40)); // 25Hz update rate, smooth for animations
    }
}

// Individual Sensor Polling Tasks
void TaskManager::imu_sensor_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("IMU sensor task started");

    // Wait for centralized initialization to complete
    while (!SensorInitializer::get_instance().is_sensor_initialized(SensorInitializer::IMU)) {
        vTaskDelay(pdMS_TO_TICKS(50)); // Check every 50ms
    }
    SerialQueueManager::get_instance().queue_message("IMU centralized initialization complete.");

    // Main polling loop
    for (;;) {
        if (ImuSensor::get_instance().should_be_polling()) {
            ImuSensor::get_instance().update_sensor_data();
        }
        vTaskDelay(pdMS_TO_TICKS(5)); // 200Hz - critical for motion control
    }
}

void TaskManager::encoder_sensor_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Encoder sensor task started");
    EncoderManager::get_instance().initialize();
    SerialQueueManager::get_instance().queue_message("Encoders initialized successfully");

    // Main polling loop
    for (;;) {
        if (EncoderManager::get_instance().should_be_polling()) {
            EncoderManager::get_instance().update_sensor_data();
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 50Hz - good balance for encoder data
    }
}

void TaskManager::multizone_tof_sensor_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Multizone TOF sensor task started");

    // Wait for centralized initialization to complete
    while (!SensorInitializer::get_instance().is_sensor_initialized(SensorInitializer::MULTIZONE_TOF)) {
        vTaskDelay(pdMS_TO_TICKS(50)); // Check every 50ms
    }
    SerialQueueManager::get_instance().queue_message("Multizone TOF centralized initialization complete.");

    // Main polling loop
    for (;;) {
        if (MultizoneTofSensor::get_instance().should_be_polling()) {
            MultizoneTofSensor::get_instance().update_sensor_data();
        }
        vTaskDelay(pdMS_TO_TICKS(5)); // Allow frequent polling, throttling handled in update_sensor_data()
    }
}

void TaskManager::side_tof_sensor_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Side TOF sensor task started");

    // Initialize Side TOF sensors directly
    while (!SideTofManager::get_instance().initialize()) {
        SerialQueueManager::get_instance().queue_message("Side TOF initialization failed, retrying in 100ms");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    SerialQueueManager::get_instance().queue_message("Side TOF sensors initialized successfully");

    // Main polling loop
    for (;;) {
        if (SideTofManager::get_instance().should_be_polling()) {
            SideTofManager::get_instance().update_sensor_data();
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // 20Hz target rate
    }
}

void TaskManager::color_sensor_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Color sensor task started");

    // Wait for centralized initialization to complete
    while (!SensorInitializer::get_instance().is_sensor_initialized(SensorInitializer::COLOR_SENSOR)) {
        vTaskDelay(pdMS_TO_TICKS(50)); // Check every 50ms
    }
    SerialQueueManager::get_instance().queue_message("Color sensor centralized initialization complete.");

    // Main polling loop
    for (;;) {
        if (ColorSensor::get_instance().should_be_polling()) {
            ColorSensor::get_instance().update_sensor_data();
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // 20Hz target rate
    }
}

void TaskManager::sensor_logger_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Sensor logger task started");

    // Main logging loop
    for (;;) {
        // Call each frequency logger function
        // imu_logger();
        // multizone_tof_logger();
        // side_tof_logger();
        // color_sensor_logger();
        // irSensorLogger();
        // log_motor_rpm();  // Keep this commented for now since it's not frequency-based
        // display_performance_logger();

        // Small delay between logger cycles - loggers have their own internal timing
        vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz - fast polling, loggers handle their own rate limiting
    }
}

void TaskManager::network_management_task(void* parameter) {
    // Initialize WiFi and networking components (heavy setup)
    WiFiManager::get_instance();
    FirmwareVersionTracker::get_instance();

    // Create the sensor data transmission task now that management is initialized
    bool comm_task_created = create_send_sensor_data_task();
    if (!comm_task_created) {
        SerialQueueManager::get_instance().queue_message("ERROR: Failed to create SendSensorData task!");
    }

    // Main management loop - unified approach
    for (;;) {
        // Heavy/infrequent operations that run regardless of connection state
        WiFiManager::get_instance().check_async_scan_progress();
        TimeoutManager::get_instance().update();

        // Always try to maintain WiFi connection if not connected
        if (WiFiClass::status() != WL_CONNECTED) {
            WiFiManager::get_instance().check_and_reconnect_wifi();
        }

        // Process any WiFi credential testing operations
        WiFiManager::get_instance().process_wifi_credential_test();

        // Slower update rate for management operations
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void TaskManager::web_socket_polling_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("WebSocket polling task started");

    // Main WebSocket polling loop
    for (;;) {
        // Only poll WebSocket if WiFi is connected
        if (WiFiClass::status() == WL_CONNECTED) {
            WebSocketManager::get_instance().poll_websocket();
        }

        // Fast update rate for real-time WebSocket communication
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::send_sensor_data_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Send sensor data task started");

    // Main sensor data transmission loop
    for (;;) {
        // Always attempt sensor data transmission (SendSensorData will handle routing)
        SendSensorData::get_instance().send_sensor_data_to_server();
        SendSensorData::get_instance().send_multizone_data();

        // Fast update rate for real-time communication
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::serial_queue_task(void* parameter) {
    // Cast back to SerialQueueManager and call its task method
    auto* instance = static_cast<SerialQueueManager*>(parameter);
    instance->serial_output_task();
}

void TaskManager::battery_monitor_task(void* parameter) {
    for (;;) {
        BatteryMonitor::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskManager::speaker_task(void* parameter) {
    Speaker::get_instance().initialize();
    SerialQueueManager::get_instance().queue_message("Speaker task started");

    for (;;) {
        Speaker::get_instance().update();
        DanceManager::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(10)); // Update every 10ms for smooth audio
    }
}

void TaskManager::motor_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Motor task started");

    for (;;) {
        motor_driver.update();
        motor_driver.process_pending_commands();
        vTaskDelay(pdMS_TO_TICKS(2)); // Fast motor update - 2ms
    }
}

void TaskManager::demo_manager_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("DemoManager task started");

    for (;;) {
        DemoManager::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(5)); // Demo updates every 5ms
    }
}

void TaskManager::game_manager_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("GameManager task started");

    for (;;) {
        GameManager::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(10)); // Game updates ~60fps (16ms)
    }
}

void TaskManager::career_quest_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("CareerQuest task started");

    for (;;) {
        career_quest_triggers.update();
        vTaskDelay(pdMS_TO_TICKS(10)); // Career quest updates every 10ms
    }
}

void TaskManager::display_init_task(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Starting display initialization...");

    if (!DisplayScreen::get_instance().init(true)) {
        SerialQueueManager::get_instance().queue_message("ERROR: Failed to init Display!");
    } else {
        // Yield before creating display task
        vTaskDelay(pdMS_TO_TICKS(5));

        // Create the display task now that init is complete
        bool display_task_created = create_display_task();
        if (display_task_created) {
            SerialQueueManager::get_instance().queue_message("Display task created successfully");
        } else {
            SerialQueueManager::get_instance().queue_message("ERROR: Failed to create Display task!");
        }
    }

    // Self-delete - our job is done
    SerialQueueManager::get_instance().queue_message("DisplayInit task self-deleting");
    display_init_task_handle = nullptr;
    vTaskDelete(nullptr);
}

bool TaskManager::create_button_task() {
    return create_task("Buttons", button_task, BUTTON_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &button_task_handle);
}

bool TaskManager::create_serial_input_task() {
    return create_task("SerialInput", serial_input_task, SERIAL_INPUT_STACK_SIZE, Priority::COMMUNICATION, Core::CORE_1, &serial_input_task_handle);
}

bool TaskManager::create_led_task() {
    return create_task("LED", led_task, LED_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &led_task_handle);
}

bool TaskManager::create_bytecode_vm_task() {
    return create_task("BytecodeVM", bytecode_vm_task, BYTECODE_VM_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_1, &bytecode_vm_task_handle);
}

bool TaskManager::create_stack_monitor_task() {
    return create_task("StackMonitor", stack_monitor_task, STACK_MONITOR_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &stack_monitor_task_handle);
}

bool TaskManager::create_display_task() {
    return create_task("Display", display_task, DISPLAY_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &display_task_handle);
}

bool TaskManager::create_network_management_task() {
    return create_task("NetworkMgmt", network_management_task, NETWORK_MANAGEMENT_STACK_SIZE, Priority::COMMUNICATION, Core::CORE_1,
                       &network_management_task_handle);
}

bool TaskManager::create_send_sensor_data_task() {
    return create_task("SendSensorData", send_sensor_data_task, SEND_SENSOR_DATA_STACK_SIZE, Priority::REALTIME_COMM, Core::CORE_1,
                       &send_sensor_data_task_handle);
}

bool TaskManager::create_web_socket_polling_task() {
    return create_task("WebSocketPoll", web_socket_polling_task, WEBSOCKET_POLLING_STACK_SIZE, Priority::REALTIME_COMM, Core::CORE_1,
                       &web_socket_polling_task_handle);
}

bool TaskManager::create_serial_queue_task() {
    // Pass the SerialQueueManager instance as parameter
    SerialQueueManager::get_instance().initialize();
    void* instance = &SerialQueueManager::get_instance();
    return create_task("SerialQueue", serial_queue_task, SERIAL_QUEUE_STACK_SIZE, Priority::CRITICAL, Core::CORE_1, &serial_queue_task_handle,
                       instance);
}

bool TaskManager::create_battery_monitor_task() {
    return create_task("BatteryMonitor", battery_monitor_task, BATTERY_MONITOR_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1,
                       &battery_monitor_task_handle);
}

bool TaskManager::create_speaker_task() {
    return create_task("Speaker", speaker_task, SPEAKER_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &speaker_task_handle);
}

bool TaskManager::create_motor_task() {
    return create_task("Motor", motor_task, MOTOR_STACK_SIZE, Priority::CRITICAL, Core::CORE_0, &motor_task_handle);
}

bool TaskManager::create_demo_manager_task() {
    return create_task("DemoManager", demo_manager_task, DEMO_MANAGER_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &demo_manager_task_handle);
}

bool TaskManager::create_game_manager_task() {
    return create_task("GameManager", game_manager_task, GAME_MANAGER_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_1, &game_manager_task_handle);
}

bool TaskManager::create_career_quest_task() {
    return create_task("CareerQuest", career_quest_task, CAREER_QUEST_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_1, &career_quest_task_handle);
}

bool TaskManager::create_display_init_task() {
    return create_task("DisplayInit", display_init_task, DISPLAY_INIT_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &display_init_task_handle);
}

// Individual sensor task creation methods
bool TaskManager::create_imu_sensor_task() {
    return create_task("IMUSensor", imu_sensor_task, IMU_SENSOR_STACK_SIZE, Priority::CRITICAL, Core::CORE_0, &imu_sensor_task_handle);
}

bool TaskManager::create_encoder_sensor_task() {
    return create_task("EncoderSensor", encoder_sensor_task, ENCODER_SENSOR_STACK_SIZE, Priority::CRITICAL, Core::CORE_0,
                       &encoder_sensor_task_handle);
}

bool TaskManager::create_multizone_tof_sensor_task() {
    return create_task("MultizoneTOF", multizone_tof_sensor_task, MULTIZONE_TOF_STACK_SIZE, Priority::COMMUNICATION, Core::CORE_0,
                       &multizone_tof_sensor_task_handle);
}

bool TaskManager::create_side_tof_sensor_task() {
    return create_task("SideTOF", side_tof_sensor_task, SIDE_TOF_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &side_tof_sensor_task_handle);
}

bool TaskManager::create_color_sensor_task() {
    return create_task("ColorSensor", color_sensor_task, COLOR_SENSOR_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &color_sensor_task_handle);
}

bool TaskManager::create_sensor_logger_task() {
    return create_task("SensorLogger", sensor_logger_task, SENSOR_LOGGER_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &sensor_logger_task_handle);
}

bool TaskManager::is_display_initialized() {
    // Return true if either init task is running or display task already exists
    return (display_init_task_handle != nullptr) || (display_task_handle != nullptr);
}

bool TaskManager::create_task(const char* name, TaskFunction_t task_function, uint32_t stack_size, Priority priority, Core core_id,
                              TaskHandle_t* task_handle, void* parameters) {
    // Safety check: don't create if task already exists
    if (task_handle != nullptr && *task_handle != nullptr) {
        SerialQueueManager::get_instance().queue_message(String("Task already exists: ") + name + ", skipping creation");
        return true; // Task exists, consider it success
    }

    BaseType_t result = xTaskCreatePinnedToCore(task_function, name, stack_size, parameters, static_cast<uint8_t>(priority), task_handle,
                                                static_cast<BaseType_t>(core_id));

    return log_task_creation(name, result == pdPASS);
}

bool TaskManager::log_task_creation(const char* name, bool success) {
    if (success) {
        SerialQueueManager::get_instance().queue_message(String("✓ Created task: ") + name);
    } else {
        SerialQueueManager::get_instance().queue_message(String("✗ Failed to create task: ") + name, SerialPriority::CRITICAL);
    }
    return success;
}

void TaskManager::print_stack_usage() {
    SerialQueueManager::get_instance().queue_message("", SerialPriority::CRITICAL);
    SerialQueueManager::get_instance().queue_message("╔══════════════════════════════════════╗", SerialPriority::CRITICAL);
    SerialQueueManager::get_instance().queue_message("║           TASK STACK USAGE           ║", SerialPriority::CRITICAL);
    SerialQueueManager::get_instance().queue_message("╠══════════════════════════════════════╣", SerialPriority::CRITICAL);

    struct TaskInfo {
        TaskHandle_t handle;
        const char* name;
        uint32_t allocated_size;
    };

    TaskInfo tasks[] = {{button_task_handle, "Buttons", BUTTON_STACK_SIZE},
                        {serial_input_task_handle, "SerialInput", SERIAL_INPUT_STACK_SIZE},
                        {led_task_handle, "LED", LED_STACK_SIZE},
                        {bytecode_vm_task_handle, "BytecodeVM", BYTECODE_VM_STACK_SIZE},
                        {stack_monitor_task_handle, "StackMonitor", STACK_MONITOR_STACK_SIZE}, // May be NULL after self-delete
                        // Individual sensor tasks
                        {imu_sensor_task_handle, "IMUSensor", IMU_SENSOR_STACK_SIZE},
                        {encoder_sensor_task_handle, "EncoderSensor", ENCODER_SENSOR_STACK_SIZE},
                        {multizone_tof_sensor_task_handle, "MultizoneTOF", MULTIZONE_TOF_STACK_SIZE},
                        {side_tof_sensor_task_handle, "SideTOF", SIDE_TOF_STACK_SIZE},
                        {color_sensor_task_handle, "ColorSensor", COLOR_SENSOR_STACK_SIZE},
                        // Other tasks
                        {display_task_handle, "Display", DISPLAY_STACK_SIZE},
                        {network_management_task_handle, "NetworkMgmt", NETWORK_MANAGEMENT_STACK_SIZE},
                        {send_sensor_data_task_handle, "SendSensorData", SEND_SENSOR_DATA_STACK_SIZE},
                        {web_socket_polling_task_handle, "WebSocketPoll", WEBSOCKET_POLLING_STACK_SIZE},
                        {serial_queue_task_handle, "SerialQueue", SERIAL_QUEUE_STACK_SIZE},
                        {battery_monitor_task_handle, "BatteryMonitor", BATTERY_MONITOR_STACK_SIZE},
                        {speaker_task_handle, "Speaker", SPEAKER_STACK_SIZE},
                        {motor_task_handle, "Motor", MOTOR_STACK_SIZE},
                        {demo_manager_task_handle, "DemoManager", DEMO_MANAGER_STACK_SIZE},
                        {game_manager_task_handle, "GameManager", GAME_MANAGER_STACK_SIZE},
                        {career_quest_task_handle, "CareerQuest", CAREER_QUEST_STACK_SIZE},
                        {display_init_task_handle, "DisplayInit", DISPLAY_INIT_STACK_SIZE}};

    for (const auto& task : tasks) {
        if (task.handle != nullptr && eTaskGetState(task.handle) != eDeleted) {
            UBaseType_t free_stack = uxTaskGetStackHighWaterMark(task.handle);
            uint32_t used_stack = task.allocated_size - (free_stack * sizeof(StackType_t));
            float percent_used = static_cast<float>(used_stack) / task.allocated_size * 100.0f;

            // Format with fixed width for alignment
            char task_name[16];
            snprintf(task_name, sizeof(task_name), "%-15s", task.name);

            String message =
                "║ " + String(task_name) + " " + String(used_stack, DEC) + "/" + String(task.allocated_size) + " (" + String(percent_used, 1) + "%)";

            // Pad to consistent width
            while (message.length() < 37) {
                message += " ";
            }
            message += "║";

            if (percent_used > 90.0f) {
                message += " 🔴";
                SerialQueueManager::get_instance().queue_message(message, SerialPriority::CRITICAL);
            } else if (percent_used > 75.0f) {
                message += " 🟡";
                SerialQueueManager::get_instance().queue_message(message, SerialPriority::CRITICAL);
            } else {
                SerialQueueManager::get_instance().queue_message(message, SerialPriority::CRITICAL);
            }
        }
    }
    SerialQueueManager::get_instance().queue_message("╚══════════════════════════════════════╝", SerialPriority::CRITICAL);
    SerialQueueManager::get_instance().queue_message("", SerialPriority::CRITICAL);
}
