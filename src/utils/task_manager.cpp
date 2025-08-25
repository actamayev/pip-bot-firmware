#include "task_manager.h"
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
TaskHandle_t TaskManager::irSensorTaskHandle = NULL;
TaskHandle_t TaskManager::sensorLoggerTaskHandle = NULL;
TaskHandle_t TaskManager::displayTaskHandle = NULL;
TaskHandle_t TaskManager::networkManagementTaskHandle = NULL;
TaskHandle_t TaskManager::networkCommunicationTaskHandle = NULL;
TaskHandle_t TaskManager::serialQueueTaskHandle = NULL;
TaskHandle_t TaskManager::batteryMonitorTaskHandle = NULL;
TaskHandle_t TaskManager::speakerTaskHandle = NULL;
TaskHandle_t TaskManager::motorTaskHandle = NULL;
TaskHandle_t TaskManager::demoManagerTaskHandle = NULL;
TaskHandle_t TaskManager::displayInitTaskHandle = NULL;

void TaskManager::buttonTask(void* parameter) {
    setupButtonLoggers();

    for(;;) {
        Buttons::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void TaskManager::serialInputTask(void* parameter) {
    for(;;) {
        SerialManager::getInstance().pollSerial();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void TaskManager::ledTask(void* parameter) {
    for(;;) {
        ledAnimations.update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::bytecodeVMTask(void* parameter) {
    for(;;) {
        BytecodeVM::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::stackMonitorTask(void* parameter) {
    for(;;) {
        printStackUsage();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskManager::displayTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Display task started");
    
    for(;;) {
        DisplayScreen::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(50));  // 20Hz update rate, smooth for animations
    }
}

// Individual Sensor Polling Tasks
void TaskManager::imuSensorTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("IMU sensor task started");
    
    // Wait for centralized initialization to complete
    while (!SensorInitializer::getInstance().isSensorInitialized(SensorInitializer::IMU)) {
        vTaskDelay(pdMS_TO_TICKS(50));  // Check every 50ms
    }
    SerialQueueManager::getInstance().queueMessage("IMU centralized initialization complete, starting polling");
    
    // Main polling loop
    for(;;) {
        if (ImuSensor::getInstance().shouldBePolling()) {
            ImuSensor::getInstance().updateSensorData();
        }
        vTaskDelay(pdMS_TO_TICKS(5));  // 200Hz - critical for motion control
    }
}

void TaskManager::encoderSensorTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Encoder sensor task started");
    EncoderManager::getInstance().initialize();
    SerialQueueManager::getInstance().queueMessage("Encoders initialized successfully");
    
    // Main polling loop
    for(;;) {
        if (EncoderManager::getInstance().shouldBePolling()) {
            EncoderManager::getInstance().updateSensorData();
        }
        vTaskDelay(pdMS_TO_TICKS(20));  // 50Hz - good balance for encoder data
    }
}

void TaskManager::multizoneTofSensorTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Multizone TOF sensor task started");
    
    // Wait for centralized initialization to complete
    while (!SensorInitializer::getInstance().isSensorInitialized(SensorInitializer::MULTIZONE_TOF)) {
        vTaskDelay(pdMS_TO_TICKS(50));  // Check every 50ms
    }
    SerialQueueManager::getInstance().queueMessage("Multizone TOF centralized initialization complete, starting polling");
    
    // Main polling loop
    for(;;) {
        if (MultizoneTofSensor::getInstance().shouldBePolling()) {
            MultizoneTofSensor::getInstance().updateSensorData();
        }
        vTaskDelay(pdMS_TO_TICKS(5));  // Allow frequent polling, throttling handled in updateSensorData()
    }
}

void TaskManager::sideTofSensorTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Side TOF sensor task started");
    
    // Initialize Side TOF sensors directly
    while (!SideTofManager::getInstance().initialize()) {
        SerialQueueManager::getInstance().queueMessage("Side TOF initialization failed, retrying in 100ms");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    SerialQueueManager::getInstance().queueMessage("Side TOF sensors initialized successfully");
    
    // Main polling loop
    for(;;) {
        if (SideTofManager::getInstance().shouldBePolling()) {
            SideTofManager::getInstance().updateSensorData();
        }
        vTaskDelay(pdMS_TO_TICKS(1));  // Allow frequent polling like performance test
    }
}

void TaskManager::colorSensorTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Color sensor task started");
    
    // Wait for centralized initialization to complete
    while (!SensorInitializer::getInstance().isSensorInitialized(SensorInitializer::COLOR_SENSOR)) {
        vTaskDelay(pdMS_TO_TICKS(50));  // Check every 50ms
    }
    SerialQueueManager::getInstance().queueMessage("Color sensor centralized initialization complete, starting polling");
    
    // Main polling loop
    for(;;) {
        if (ColorSensor::getInstance().shouldBePolling()) {
            ColorSensor::getInstance().updateSensorData();
        }
        vTaskDelay(pdMS_TO_TICKS(1));  // Allow frequent polling like performance test
    }
}

void TaskManager::irSensorTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("IR sensor task started");
    
    // Initialize IR sensors directly (not I2C, no conflicts)
    IrSensor::getInstance();  // Simple initialization - just creates instance
    SerialQueueManager::getInstance().queueMessage("IR sensors initialized successfully");
    
    // Main polling loop
    for(;;) {
        if (IrSensor::getInstance().shouldBePolling()) {
            IrSensor::getInstance().updateSensorData();
        }
        vTaskDelay(pdMS_TO_TICKS(25));  // 40Hz - moderate processing (5 sensors)
    }
}

void TaskManager::sensorLoggerTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Sensor logger task started");
    
    // Main logging loop
    for(;;) {
        // Call each frequency logger function
        // imuLogger();
        // multizoneTofLogger();
        // sideTofLogger();
        colorSensorLogger();
        // log_motor_rpm();  // Keep this commented for now since it's not frequency-based
        
        // Small delay between logger cycles - loggers have their own internal timing
        vTaskDelay(pdMS_TO_TICKS(10));  // 100Hz - fast polling, loggers handle their own rate limiting
    }
}

void TaskManager::networkManagementTask(void* parameter) {
    // Initialize WiFi and networking components (heavy setup)
    WiFiManager::getInstance();
    FirmwareVersionTracker::getInstance();
    
    // Create the communication task now that management is initialized
    bool commTaskCreated = createNetworkCommunicationTask();
    if (!commTaskCreated) {
        SerialQueueManager::getInstance().queueMessage("ERROR: Failed to create NetworkCommunication task!");
    }

    // Main management loop - slower operations
    for(;;) {
        NetworkMode mode = NetworkStateManager::getInstance().getCurrentMode();
        
        // Heavy/infrequent operations
        WiFiManager::getInstance().checkAsyncScanProgress();
        TimeoutManager::getInstance().update();

        switch (mode) {
            case NetworkMode::SERIAL_MODE:
                // When in serial mode, we don't do any WiFi operations
                break;

            case NetworkMode::ADD_PIP_MODE:
                // Process ADD_PIP_MODE WiFi testing
                WiFiManager::getInstance().processAddPipMode();
                break;

            case NetworkMode::WIFI_MODE:
                // WiFi connected mode - management tasks only
                // (Communication task handles WebSocket polling)
                break;

            case NetworkMode::NONE:
                // No connectivity - try to establish WiFi
                WiFiManager::getInstance().checkAndReconnectWiFi();
                // HapticFeedbackManager::getInstance().update();
                // WifiSelectionManager::getInstance().processNetworkSelection();
                break;
        }

        // Slower update rate for management operations
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void TaskManager::networkCommunicationTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Network communication task started");
    
    // Main communication loop - fast operations
    for(;;) {
        NetworkMode mode = NetworkStateManager::getInstance().getCurrentMode();
        
        // Only do communication tasks when in WiFi mode
        if (mode == NetworkMode::WIFI_MODE) {
            // Lightweight, frequent operations
            WebSocketManager::getInstance().pollWebSocket();
            SendDataToServer::getInstance().sendSensorDataToServer();
        }

        // Fast update rate for real-time communication
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::serialQueueTask(void* parameter) {
    // Cast back to SerialQueueManager and call its task method
    SerialQueueManager* instance = static_cast<SerialQueueManager*>(parameter);
    instance->serialOutputTask();
}

void TaskManager::batteryMonitorTask(void* parameter) {
    for(;;) {
        BatteryMonitor::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskManager::speakerTask(void* parameter) {
    Speaker::getInstance().initialize();
    SerialQueueManager::getInstance().queueMessage("Speaker task started");

    for(;;) {
        Speaker::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(10)); // Update every 10ms for smooth audio
    }
}

void TaskManager::motorTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Motor task started");
    
    for(;;) {
        motorDriver.update();
        motorDriver.processPendingCommands();
        vTaskDelay(pdMS_TO_TICKS(2)); // Fast motor update - 2ms
    }
}

void TaskManager::demoManagerTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("DemoManager task started");

    for(;;) {
        DemoManager::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(5)); // Demo updates every 5ms
    }
}

void TaskManager::displayInitTask(void* parameter) {    
    SerialQueueManager::getInstance().queueMessage("Starting display initialization...");
    
    if (!DisplayScreen::getInstance().init(true)) {
        SerialQueueManager::getInstance().queueMessage("ERROR: Failed to init Display!");
    } else {
        // Yield before creating display task
        vTaskDelay(pdMS_TO_TICKS(5));
        
        // Create the display task now that init is complete
        bool displayTaskCreated = createDisplayTask();
        if (displayTaskCreated) {
            SerialQueueManager::getInstance().queueMessage("Display task created successfully");
        } else {
            SerialQueueManager::getInstance().queueMessage("ERROR: Failed to create Display task!");
        }
    }
    
    // Self-delete - our job is done
    SerialQueueManager::getInstance().queueMessage("DisplayInit task self-deleting");
    displayInitTaskHandle = NULL;
    vTaskDelete(NULL);
}

bool TaskManager::createButtonTask() {
    return createTask("Buttons", buttonTask, BUTTON_STACK_SIZE, 
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &buttonTaskHandle);
}

bool TaskManager::createSerialInputTask() {
    return createTask("SerialInput", serialInputTask, SERIAL_INPUT_STACK_SIZE,
                     Priority::COMMUNICATION, Core::CORE_1, &serialInputTaskHandle);
}

bool TaskManager::createLedTask() {
    return createTask("LED", ledTask, LED_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &ledTaskHandle);
}

bool TaskManager::createBytecodeVMTask() {
    return createTask("BytecodeVM", bytecodeVMTask, BYTECODE_VM_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_1, &bytecodeVMTaskHandle);
}

bool TaskManager::createStackMonitorTask() {
    return createTask("StackMonitor", stackMonitorTask, STACK_MONITOR_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &stackMonitorTaskHandle);
}

bool TaskManager::createDisplayTask() {
    return createTask("Display", displayTask, DISPLAY_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &displayTaskHandle);
}

bool TaskManager::createNetworkManagementTask() {
    return createTask("NetworkMgmt", networkManagementTask, NETWORK_MANAGEMENT_STACK_SIZE,
                     Priority::COMMUNICATION, Core::CORE_1, &networkManagementTaskHandle);
}

bool TaskManager::createNetworkCommunicationTask() {
    return createTask("NetworkComm", networkCommunicationTask, NETWORK_COMMUNICATION_STACK_SIZE,
                     Priority::REALTIME_COMM, Core::CORE_1, &networkCommunicationTaskHandle);
}

bool TaskManager::createSerialQueueTask() {
    // Pass the SerialQueueManager instance as parameter
    SerialQueueManager::getInstance().initialize();
    void* instance = &SerialQueueManager::getInstance();
    return createTask("SerialQueue", serialQueueTask, SERIAL_QUEUE_STACK_SIZE,
                     Priority::CRITICAL, Core::CORE_1, &serialQueueTaskHandle, instance);
}

bool TaskManager::createBatteryMonitorTask() {
    return createTask("BatteryMonitor", batteryMonitorTask, BATTERY_MONITOR_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &batteryMonitorTaskHandle);
}

bool TaskManager::createSpeakerTask() {
    return createTask("Speaker", speakerTask, SPEAKER_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &speakerTaskHandle);
}

bool TaskManager::createMotorTask() {
    return createTask("Motor", motorTask, MOTOR_STACK_SIZE,
                     Priority::CRITICAL, Core::CORE_0, &motorTaskHandle);
}

bool TaskManager::createDemoManagerTask() {
    return createTask("DemoManager", demoManagerTask, DEMO_MANAGER_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &demoManagerTaskHandle);
}

bool TaskManager::createDisplayInitTask() {
    return createTask("DisplayInit", displayInitTask, DISPLAY_INIT_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &displayInitTaskHandle);
}

// Individual sensor task creation methods
bool TaskManager::createImuSensorTask() {
    return createTask("IMUSensor", imuSensorTask, IMU_SENSOR_STACK_SIZE,
                     Priority::CRITICAL, Core::CORE_0, &imuSensorTaskHandle);
}

bool TaskManager::createEncoderSensorTask() {
    return createTask("EncoderSensor", encoderSensorTask, ENCODER_SENSOR_STACK_SIZE,
                     Priority::CRITICAL, Core::CORE_0, &encoderSensorTaskHandle);
}

bool TaskManager::createMultizoneTofSensorTask() {
    return createTask("MultizoneTOF", multizoneTofSensorTask, MULTIZONE_TOF_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &multizoneTofSensorTaskHandle);
}

bool TaskManager::createSideTofSensorTask() {
    return createTask("SideTOF", sideTofSensorTask, SIDE_TOF_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &sideTofSensorTaskHandle);
}

bool TaskManager::createColorSensorTask() {
    return createTask("ColorSensor", colorSensorTask, COLOR_SENSOR_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_0, &colorSensorTaskHandle);
}

bool TaskManager::createIrSensorTask() {
    return createTask("IRSensor", irSensorTask, IR_SENSOR_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_0, &irSensorTaskHandle);
}

bool TaskManager::createSensorLoggerTask() {
    return createTask("SensorLogger", sensorLoggerTask, SENSOR_LOGGER_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &sensorLoggerTaskHandle);
}

bool TaskManager::isDisplayInitialized() {
    // Return true if either init task is running or display task already exists
    return (displayInitTaskHandle != NULL) || (displayTaskHandle != NULL);
}

bool TaskManager::createTask(
    const char* name,
    TaskFunction_t taskFunction, 
    uint32_t stackSize,
    Priority priority,
    Core coreId,
    TaskHandle_t* taskHandle,
    void* parameters
) {
    // Safety check: don't create if task already exists
    if (taskHandle != nullptr && *taskHandle != NULL) {
        SerialQueueManager::getInstance().queueMessage(
            String("Task already exists: ") + name + ", skipping creation"
        );
        return true;  // Task exists, consider it success
    }
    
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        name,
        stackSize,
        parameters,
        static_cast<uint8_t>(priority),
        taskHandle,
        static_cast<BaseType_t>(coreId)
    );
    
    return logTaskCreation(name, result == pdPASS);
}

bool TaskManager::logTaskCreation(const char* name, bool success) {
    if (success) {
        SerialQueueManager::getInstance().queueMessage(String("✓ Created task: ") + name);
    } else {
        SerialQueueManager::getInstance().queueMessage(
            String("✗ Failed to create task: ") + name, 
            SerialPriority::CRITICAL
        );
    }
    return success;
}

void TaskManager::printStackUsage() {
    SerialQueueManager::getInstance().queueMessage("", SerialPriority::CRITICAL);
    SerialQueueManager::getInstance().queueMessage("╔══════════════════════════════════════╗", SerialPriority::CRITICAL);
    SerialQueueManager::getInstance().queueMessage("║           TASK STACK USAGE           ║", SerialPriority::CRITICAL);
    SerialQueueManager::getInstance().queueMessage("╠══════════════════════════════════════╣", SerialPriority::CRITICAL);
    
    struct TaskInfo {
        TaskHandle_t handle;
        const char* name;
        uint32_t allocatedSize;
    };
    
    TaskInfo tasks[] = {
        {buttonTaskHandle, "Buttons", BUTTON_STACK_SIZE},
        {serialInputTaskHandle, "SerialInput", SERIAL_INPUT_STACK_SIZE},
        {ledTaskHandle, "LED", LED_STACK_SIZE},
        {bytecodeVMTaskHandle, "BytecodeVM", BYTECODE_VM_STACK_SIZE},
        {stackMonitorTaskHandle, "StackMonitor", STACK_MONITOR_STACK_SIZE},        // May be NULL after self-delete
        // Individual sensor tasks
        {imuSensorTaskHandle, "IMUSensor", IMU_SENSOR_STACK_SIZE},
        {encoderSensorTaskHandle, "EncoderSensor", ENCODER_SENSOR_STACK_SIZE},
        {multizoneTofSensorTaskHandle, "MultizoneTOF", MULTIZONE_TOF_STACK_SIZE},
        {sideTofSensorTaskHandle, "SideTOF", SIDE_TOF_STACK_SIZE},
        {colorSensorTaskHandle, "ColorSensor", COLOR_SENSOR_STACK_SIZE},
        {irSensorTaskHandle, "IRSensor", IR_SENSOR_STACK_SIZE},
        // Other tasks
        {displayTaskHandle, "Display", DISPLAY_STACK_SIZE},
        {networkManagementTaskHandle, "NetworkMgmt", NETWORK_MANAGEMENT_STACK_SIZE},
        {networkCommunicationTaskHandle, "NetworkComm", NETWORK_COMMUNICATION_STACK_SIZE},
        {serialQueueTaskHandle, "SerialQueue", SERIAL_QUEUE_STACK_SIZE},
        {batteryMonitorTaskHandle, "BatteryMonitor", BATTERY_MONITOR_STACK_SIZE},
        {speakerTaskHandle, "Speaker", SPEAKER_STACK_SIZE},
        {motorTaskHandle, "Motor", MOTOR_STACK_SIZE},
        {demoManagerTaskHandle, "DemoManager", DEMO_MANAGER_STACK_SIZE},
        {displayInitTaskHandle, "DisplayInit", DISPLAY_INIT_STACK_SIZE}
    };
    
    for (const auto& task : tasks) {
        if (task.handle != NULL && eTaskGetState(task.handle) != eDeleted) {
            UBaseType_t freeStack = uxTaskGetStackHighWaterMark(task.handle);
            uint32_t usedStack = task.allocatedSize - (freeStack * sizeof(StackType_t));
            float percentUsed = (float)usedStack / task.allocatedSize * 100.0f;
            
            // Format with fixed width for alignment
            char taskName[16];
            snprintf(taskName, sizeof(taskName), "%-15s", task.name);
            
            String message = "║ " + String(taskName) + " " + 
                        String(usedStack, DEC) + "/" + String(task.allocatedSize) + 
                        " (" + String(percentUsed, 1) + "%)";
                        
            // Pad to consistent width
            while (message.length() < 37) {
                message += " ";
            }
            message += "║";
            
            if (percentUsed > 90.0f) {
                message += " 🔴";
                SerialQueueManager::getInstance().queueMessage(message, SerialPriority::CRITICAL);
            } else if (percentUsed > 75.0f) {
                message += " 🟡";
                SerialQueueManager::getInstance().queueMessage(message, SerialPriority::CRITICAL);
            } else {
                SerialQueueManager::getInstance().queueMessage(message, SerialPriority::CRITICAL);
            }
        }
    }
    SerialQueueManager::getInstance().queueMessage("╚══════════════════════════════════════╝", SerialPriority::CRITICAL);
    SerialQueueManager::getInstance().queueMessage("", SerialPriority::CRITICAL);
}
