#include "task_manager.h"

#include "career_quest/career_quest_triggers.h"
#include "sensors/sensor_initializer.h"

TaskHandle_t TaskManager::buttonTaskHandle = NULL;
TaskHandle_t TaskManager::serialInputTaskHandle = NULL;
TaskHandle_t TaskManager::ledTaskHandle = NULL;
TaskHandle_t TaskManager::bytecodeVMTaskHandle = NULL;
TaskHandle_t TaskManager::stackMonitorTaskHandle = NULL;
TaskHandle_t TaskManager::sensorPollingTaskHandle = NULL;

// Individual sensor task handles
TaskHandle_t TaskManager::imuSensorTaskHandle = NULL;
TaskHandle_t TaskManager::encoderSensorTaskHandle = NULL;
TaskHandle_t TaskManager::multizoneTofSensorTaskHandle = NULL;
TaskHandle_t TaskManager::sideTofSensorTaskHandle = NULL;
TaskHandle_t TaskManager::colorSensorTaskHandle = NULL;
TaskHandle_t TaskManager::sensorLoggerTaskHandle = NULL;
TaskHandle_t TaskManager::displayTaskHandle = NULL;
TaskHandle_t TaskManager::networkManagementTaskHandle = NULL;
TaskHandle_t TaskManager::sendSensorDataTaskHandle = NULL;
TaskHandle_t TaskManager::webSocketPollingTaskHandle = NULL;
TaskHandle_t TaskManager::serialQueueTaskHandle = NULL;
TaskHandle_t TaskManager::batteryMonitorTaskHandle = NULL;
TaskHandle_t TaskManager::speakerTaskHandle = NULL;
TaskHandle_t TaskManager::motorTaskHandle = NULL;
TaskHandle_t TaskManager::demoManagerTaskHandle = NULL;
TaskHandle_t TaskManager::gameManagerTaskHandle = NULL;
TaskHandle_t TaskManager::careerQuestTaskHandle = NULL;
TaskHandle_t TaskManager::displayInitTaskHandle = NULL;

void TaskManager::buttonTask(void* parameter) {
    setup_button_loggers();

    for (;;) {
        Buttons::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void TaskManager::serialInputTask(void* parameter) {
    for (;;) {
        SerialManager::get_instance().poll_serial();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void TaskManager::ledTask(void* parameter) {
    for (;;) {
        ledAnimations.update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::bytecodeVMTask(void* parameter) {
    for (;;) {
        BytecodeVM::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::stackMonitorTask(void* parameter) {
    for (;;) {
        print_stack_usage();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskManager::displayTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Display task started");

    for (;;) {
        DisplayScreen::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(40)); // 25Hz update rate, smooth for animations
    }
}

// Individual Sensor Polling Tasks
void TaskManager::imuSensorTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("IMU sensor task started");

    // Wait for centralized initialization to complete
    while (!SensorInitializer::get_instance().isSensorInitialized(SensorInitializer::IMU)) {
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

void TaskManager::encoderSensorTask(void* parameter) {
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

void TaskManager::multizoneTofSensorTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Multizone TOF sensor task started");

    // Wait for centralized initialization to complete
    while (!SensorInitializer::get_instance().isSensorInitialized(SensorInitializer::MULTIZONE_TOF)) {
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

void TaskManager::sideTofSensorTask(void* parameter) {
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

void TaskManager::colorSensorTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Color sensor task started");

    // Wait for centralized initialization to complete
    while (!SensorInitializer::get_instance().isSensorInitialized(SensorInitializer::COLOR_SENSOR)) {
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

void TaskManager::sensorLoggerTask(void* parameter) {
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

void TaskManager::networkManagementTask(void* parameter) {
    // Initialize WiFi and networking components (heavy setup)
    WiFiManager::get_instance();
    FirmwareVersionTracker::get_instance();

    // Create the sensor data transmission task now that management is initialized
    bool commTaskCreated = create_send_sensor_data_task();
    if (!commTaskCreated) {
        SerialQueueManager::get_instance().queue_message("ERROR: Failed to create SendSensorData task!");
    }

    // Main management loop - unified approach
    for (;;) {
        // Heavy/infrequent operations that run regardless of connection state
        WiFiManager::get_instance().check_async_scan_progress();
        TimeoutManager::get_instance().update();

        // Always try to maintain WiFi connection if not connected
        if (WiFi.status() != WL_CONNECTED) {
            WiFiManager::get_instance().check_and_reconnect_wifi();
        }

        // Process any WiFi credential testing operations
        WiFiManager::get_instance().process_wifi_credential_test();

        // Slower update rate for management operations
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void TaskManager::webSocketPollingTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("WebSocket polling task started");

    // Main WebSocket polling loop
    for (;;) {
        // Only poll WebSocket if WiFi is connected
        if (WiFi.status() == WL_CONNECTED) {
            WebSocketManager::get_instance().poll_web_socket();
        }

        // Fast update rate for real-time WebSocket communication
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::sendSensorDataTask(void* parameter) {
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

void TaskManager::serialQueueTask(void* parameter) {
    // Cast back to SerialQueueManager and call its task method
    SerialQueueManager* instance = static_cast<SerialQueueManager*>(parameter);
    instance->serial_output_task();
}

void TaskManager::batteryMonitorTask(void* parameter) {
    for (;;) {
        BatteryMonitor::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskManager::speakerTask(void* parameter) {
    Speaker::get_instance().initialize();
    SerialQueueManager::get_instance().queue_message("Speaker task started");

    for (;;) {
        Speaker::get_instance().update();
        DanceManager::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(10)); // Update every 10ms for smooth audio
    }
}

void TaskManager::motorTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Motor task started");

    for (;;) {
        motorDriver.update();
        motorDriver.process_pending_commands();
        vTaskDelay(pdMS_TO_TICKS(2)); // Fast motor update - 2ms
    }
}

void TaskManager::demoManagerTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("DemoManager task started");

    for (;;) {
        DemoManager::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(5)); // Demo updates every 5ms
    }
}

void TaskManager::gameManagerTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("GameManager task started");

    for (;;) {
        GameManager::get_instance().update();
        vTaskDelay(pdMS_TO_TICKS(10)); // Game updates ~60fps (16ms)
    }
}

void TaskManager::careerQuestTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("CareerQuest task started");

    for (;;) {
        careerQuestTriggers.update();
        vTaskDelay(pdMS_TO_TICKS(10)); // Career quest updates every 10ms
    }
}

void TaskManager::displayInitTask(void* parameter) {
    SerialQueueManager::get_instance().queue_message("Starting display initialization...");

    if (!DisplayScreen::get_instance().init(true)) {
        SerialQueueManager::get_instance().queue_message("ERROR: Failed to init Display!");
    } else {
        // Yield before creating display task
        vTaskDelay(pdMS_TO_TICKS(5));

        // Create the display task now that init is complete
        bool displayTaskCreated = create_display_task();
        if (displayTaskCreated) {
            SerialQueueManager::get_instance().queue_message("Display task created successfully");
        } else {
            SerialQueueManager::get_instance().queue_message("ERROR: Failed to create Display task!");
        }
    }

    // Self-delete - our job is done
    SerialQueueManager::get_instance().queue_message("DisplayInit task self-deleting");
    displayInitTaskHandle = NULL;
    vTaskDelete(NULL);
}

bool TaskManager::create_button_task() {
    return create_task("Buttons", buttonTask, BUTTON_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &buttonTaskHandle);
}

bool TaskManager::create_serial_input_task() {
    return create_task("SerialInput", serialInputTask, SERIAL_INPUT_STACK_SIZE, Priority::COMMUNICATION, Core::CORE_1, &serialInputTaskHandle);
}

bool TaskManager::create_led_task() {
    return create_task("LED", ledTask, LED_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &ledTaskHandle);
}

bool TaskManager::create_bytecode_vm_task() {
    return create_task("BytecodeVM", bytecodeVMTask, BYTECODE_VM_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_1, &bytecodeVMTaskHandle);
}

bool TaskManager::create_stack_monitor_task() {
    return create_task("StackMonitor", stackMonitorTask, STACK_MONITOR_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &stackMonitorTaskHandle);
}

bool TaskManager::create_display_task() {
    return create_task("Display", displayTask, DISPLAY_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &displayTaskHandle);
}

bool TaskManager::create_network_management_task() {
    return create_task("NetworkMgmt", networkManagementTask, NETWORK_MANAGEMENT_STACK_SIZE, Priority::COMMUNICATION, Core::CORE_1,
                      &networkManagementTaskHandle);
}

bool TaskManager::create_send_sensor_data_task() {
    return create_task("SendSensorData", sendSensorDataTask, SEND_SENSOR_DATA_STACK_SIZE, Priority::REALTIME_COMM, Core::CORE_1,
                      &sendSensorDataTaskHandle);
}

bool TaskManager::create_web_socket_polling_task() {
    return create_task("WebSocketPoll", webSocketPollingTask, WEBSOCKET_POLLING_STACK_SIZE, Priority::REALTIME_COMM, Core::CORE_1,
                      &webSocketPollingTaskHandle);
}

bool TaskManager::create_serial_queue_task() {
    // Pass the SerialQueueManager instance as parameter
    SerialQueueManager::get_instance().initialize();
    void* instance = &SerialQueueManager::get_instance();
    return create_task("SerialQueue", serialQueueTask, SERIAL_QUEUE_STACK_SIZE, Priority::CRITICAL, Core::CORE_1, &serialQueueTaskHandle, instance);
}

bool TaskManager::create_battery_monitor_task() {
    return create_task("BatteryMonitor", batteryMonitorTask, BATTERY_MONITOR_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1,
                      &batteryMonitorTaskHandle);
}

bool TaskManager::create_speaker_task() {
    return create_task("Speaker", speakerTask, SPEAKER_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &speakerTaskHandle);
}

bool TaskManager::create_motor_task() {
    return create_task("Motor", motorTask, MOTOR_STACK_SIZE, Priority::CRITICAL, Core::CORE_0, &motorTaskHandle);
}

bool TaskManager::create_demo_manager_task() {
    return create_task("DemoManager", demoManagerTask, DEMO_MANAGER_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &demoManagerTaskHandle);
}

bool TaskManager::create_game_manager_task() {
    return create_task("GameManager", gameManagerTask, GAME_MANAGER_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_1, &gameManagerTaskHandle);
}

bool TaskManager::create_career_quest_task() {
    return create_task("CareerQuest", careerQuestTask, CAREER_QUEST_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_1, &careerQuestTaskHandle);
}

bool TaskManager::create_display_init_task() {
    return create_task("DisplayInit", displayInitTask, DISPLAY_INIT_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &displayInitTaskHandle);
}

// Individual sensor task creation methods
bool TaskManager::create_imu_sensor_task() {
    return create_task("IMUSensor", imuSensorTask, IMU_SENSOR_STACK_SIZE, Priority::CRITICAL, Core::CORE_0, &imuSensorTaskHandle);
}

bool TaskManager::create_encoder_sensor_task() {
    return create_task("EncoderSensor", encoderSensorTask, ENCODER_SENSOR_STACK_SIZE, Priority::CRITICAL, Core::CORE_0, &encoderSensorTaskHandle);
}

bool TaskManager::create_multizone_tof_sensor_task() {
    return create_task("MultizoneTOF", multizoneTofSensorTask, MULTIZONE_TOF_STACK_SIZE, Priority::COMMUNICATION, Core::CORE_0,
                      &multizoneTofSensorTaskHandle);
}

bool TaskManager::create_side_tof_sensor_task() {
    return create_task("SideTOF", sideTofSensorTask, SIDE_TOF_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &sideTofSensorTaskHandle);
}

bool TaskManager::create_color_sensor_task() {
    return create_task("ColorSensor", colorSensorTask, COLOR_SENSOR_STACK_SIZE, Priority::SYSTEM_CONTROL, Core::CORE_0, &colorSensorTaskHandle);
}

bool TaskManager::create_sensor_logger_task() {
    return create_task("SensorLogger", sensorLoggerTask, SENSOR_LOGGER_STACK_SIZE, Priority::BACKGROUND, Core::CORE_1, &sensorLoggerTaskHandle);
}

bool TaskManager::is_display_initialized() {
    // Return true if either init task is running or display task already exists
    return (displayInitTaskHandle != NULL) || (displayTaskHandle != NULL);
}

bool TaskManager::create_task(const char* name, TaskFunction_t taskFunction, uint32_t stackSize, Priority priority, Core coreId,
                             TaskHandle_t* taskHandle, void* parameters) {
    // Safety check: don't create if task already exists
    if (taskHandle != nullptr && *taskHandle != NULL) {
        SerialQueueManager::get_instance().queue_message(String("Task already exists: ") + name + ", skipping creation");
        return true; // Task exists, consider it success
    }

    BaseType_t result = xTaskCreatePinnedToCore(taskFunction, name, stackSize, parameters, static_cast<uint8_t>(priority), taskHandle,
                                                static_cast<BaseType_t>(coreId));

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
        uint32_t allocatedSize;
    };

    TaskInfo tasks[] = {{buttonTaskHandle, "Buttons", BUTTON_STACK_SIZE},
                        {serialInputTaskHandle, "SerialInput", SERIAL_INPUT_STACK_SIZE},
                        {ledTaskHandle, "LED", LED_STACK_SIZE},
                        {bytecodeVMTaskHandle, "BytecodeVM", BYTECODE_VM_STACK_SIZE},
                        {stackMonitorTaskHandle, "StackMonitor", STACK_MONITOR_STACK_SIZE}, // May be NULL after self-delete
                        // Individual sensor tasks
                        {imuSensorTaskHandle, "IMUSensor", IMU_SENSOR_STACK_SIZE},
                        {encoderSensorTaskHandle, "EncoderSensor", ENCODER_SENSOR_STACK_SIZE},
                        {multizoneTofSensorTaskHandle, "MultizoneTOF", MULTIZONE_TOF_STACK_SIZE},
                        {sideTofSensorTaskHandle, "SideTOF", SIDE_TOF_STACK_SIZE},
                        {colorSensorTaskHandle, "ColorSensor", COLOR_SENSOR_STACK_SIZE},
                        // Other tasks
                        {displayTaskHandle, "Display", DISPLAY_STACK_SIZE},
                        {networkManagementTaskHandle, "NetworkMgmt", NETWORK_MANAGEMENT_STACK_SIZE},
                        {sendSensorDataTaskHandle, "SendSensorData", SEND_SENSOR_DATA_STACK_SIZE},
                        {webSocketPollingTaskHandle, "WebSocketPoll", WEBSOCKET_POLLING_STACK_SIZE},
                        {serialQueueTaskHandle, "SerialQueue", SERIAL_QUEUE_STACK_SIZE},
                        {batteryMonitorTaskHandle, "BatteryMonitor", BATTERY_MONITOR_STACK_SIZE},
                        {speakerTaskHandle, "Speaker", SPEAKER_STACK_SIZE},
                        {motorTaskHandle, "Motor", MOTOR_STACK_SIZE},
                        {demoManagerTaskHandle, "DemoManager", DEMO_MANAGER_STACK_SIZE},
                        {gameManagerTaskHandle, "GameManager", GAME_MANAGER_STACK_SIZE},
                        {careerQuestTaskHandle, "CareerQuest", CAREER_QUEST_STACK_SIZE},
                        {displayInitTaskHandle, "DisplayInit", DISPLAY_INIT_STACK_SIZE}};

    for (const auto& task : tasks) {
        if (task.handle != NULL && eTaskGetState(task.handle) != eDeleted) {
            UBaseType_t freeStack = uxTaskGetStackHighWaterMark(task.handle);
            uint32_t usedStack = task.allocatedSize - (freeStack * sizeof(StackType_t));
            float percentUsed = (float)usedStack / task.allocatedSize * 100.0f;

            // Format with fixed width for alignment
            char taskName[16];
            snprintf(taskName, sizeof(taskName), "%-15s", task.name);

            String message =
                "║ " + String(taskName) + " " + String(usedStack, DEC) + "/" + String(task.allocatedSize) + " (" + String(percentUsed, 1) + "%)";

            // Pad to consistent width
            while (message.length() < 37) {
                message += " ";
            }
            message += "║";

            if (percentUsed > 90.0f) {
                message += " 🔴";
                SerialQueueManager::get_instance().queue_message(message, SerialPriority::CRITICAL);
            } else if (percentUsed > 75.0f) {
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
