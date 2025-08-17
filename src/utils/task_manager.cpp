#include "task_manager.h"

TaskHandle_t TaskManager::buttonTaskHandle = NULL;
TaskHandle_t TaskManager::serialInputTaskHandle = NULL;
TaskHandle_t TaskManager::ledTaskHandle = NULL;
TaskHandle_t TaskManager::messageProcessorTaskHandle = NULL;
TaskHandle_t TaskManager::bytecodeVMTaskHandle = NULL;
TaskHandle_t TaskManager::stackMonitorTaskHandle = NULL;
TaskHandle_t TaskManager::sensorInitTaskHandle = NULL;
TaskHandle_t TaskManager::sensorPollingTaskHandle = NULL;
TaskHandle_t TaskManager::displayTaskHandle = NULL;
TaskHandle_t TaskManager::networkManagementTaskHandle = NULL;
TaskHandle_t TaskManager::networkCommunicationTaskHandle = NULL;
TaskHandle_t TaskManager::serialQueueTaskHandle = NULL;
TaskHandle_t TaskManager::batteryMonitorTaskHandle = NULL;
TaskHandle_t TaskManager::speakerTaskHandle = NULL;
TaskHandle_t TaskManager::displayInitTaskHandle = NULL;

void TaskManager::buttonTask(void* parameter) {
    setupButtonLoggers();

    for(;;) {
        Buttons::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(1));
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

void TaskManager::messageProcessorTask(void* parameter) {
    for(;;) {
        MessageProcessor::getInstance().processPendingCommands();
        vTaskDelay(pdMS_TO_TICKS(2)); // Fast motor command processing
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
        vTaskDelay(pdMS_TO_TICKS(20));  // 50Hz update rate, smooth for animations
    }
}

void TaskManager::sensorInitTask(void* parameter) {
    disableCore0WDT();
    vTaskDelay(pdMS_TO_TICKS(10));
    
    SerialQueueManager::getInstance().queueMessage("Starting sensor initialization on Core 0...");

    // Get the sensor initializer
    SensorInitializer& initializer = SensorInitializer::getInstance();

    // Keep trying until ALL sensors are initialized
    while (!initializer.areAllSensorsInitialized()) {
        if (!initializer.isSensorInitialized(SensorInitializer::IMU)) {
            SerialQueueManager::getInstance().queueMessage("Trying to init IMU...");
            initializer.tryInitializeIMU();
        }
        if (!initializer.isSensorInitialized(SensorInitializer::MULTIZONE_TOF)) {
            SerialQueueManager::getInstance().queueMessage("Trying to init Multizone TOF...");
            initializer.tryInitializeMultizoneTof();
        }
        if (!initializer.isSensorInitialized(SensorInitializer::SIDE_TOFS)) {
            SerialQueueManager::getInstance().queueMessage("Trying to init Multizone TOF...");
            initializer.tryInitializeSideTofs();
        }
        // if (!initializer.isSensorInitialized(SensorInitializer::COLOR_SENSOR)) {
        //     SerialQueueManager::getInstance().queueMessage("Trying to init IR sensors...");
        //     initializer.tryInitializeColorSensor();
        // }
        // if (!initializer.isSensorInitialized(SensorInitializer::IR_SENSORS)) {
        //     SerialQueueManager::getInstance().queueMessage("Trying to init IR sensors...");
        //     initializer.tryInitializeIrSensors();
        // }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    SerialQueueManager::getInstance().queueMessage("All sensors initialized successfully!");
    enableCore0WDT();
    
    // Create the sensor polling task now that init is complete
    bool pollingTaskCreated = createSensorPollingTask();

    if (pollingTaskCreated) {
        SerialQueueManager::getInstance().queueMessage("All tasks created - sensor polling ready");
    } else {
        SerialQueueManager::getInstance().queueMessage("ERROR: Failed to create required tasks!");
    }
    
    // Self-delete - our job is done
    SerialQueueManager::getInstance().queueMessage("SensorInit task self-deleting");
    sensorInitTaskHandle = NULL;
    vTaskDelete(NULL);
}

// SensorPolling Task - handles ongoing sensor data collection
void TaskManager::sensorPollingTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Sensor polling task started");
    
    for(;;) {
        if (ImuSensor::getInstance().shouldBePolling()) {
            ImuSensor::getInstance().updateSensorData();
        }
        // if (MultizoneTofSensor::getInstance().shouldBePolling()) {
        //     MultizoneTofSensor::getInstance().updateSensorData();
        // }
        if (SideTofManager::getInstance().shouldBePolling()) {
            SideTofManager::getInstance().updateSensorData();
        }
        // if (ColorSensor::getInstance().shouldBePolling()) {
        //     ColorSensor::getInstance().updateSensorData();
        // }
        // if (IrSensor::getInstance().shouldBePolling()) {
        //     IrSensor::getInstance().updateSensorData();
        // }
        vTaskDelay(pdMS_TO_TICKS(5));  // Same timing as before
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
    BatteryMonitor::getInstance().initialize();
    
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

void TaskManager::displayInitTask(void* parameter) {
    disableCore0WDT();
    vTaskDelay(pdMS_TO_TICKS(10));
    
    SerialQueueManager::getInstance().queueMessage("Starting display initialization...");
    
    if (!DisplayScreen::getInstance().init()) {
        SerialQueueManager::getInstance().queueMessage("Display initialization failed");
    } else {
        SerialQueueManager::getInstance().queueMessage("Display initialized successfully");
        
        // Create the display task now that init is complete
        bool displayTaskCreated = createDisplayTask();
        if (displayTaskCreated) {
            SerialQueueManager::getInstance().queueMessage("Display task created successfully");
        } else {
            SerialQueueManager::getInstance().queueMessage("ERROR: Failed to create Display task!");
        }
    }
    
    enableCore0WDT();
    
    // Self-delete - our job is done
    SerialQueueManager::getInstance().queueMessage("DisplayInit task self-deleting");
    displayInitTaskHandle = NULL;
    vTaskDelete(NULL);
}

bool TaskManager::createButtonTask() {
    return createTask("Buttons", buttonTask, BUTTON_STACK_SIZE, 
                     Priority::CRITICAL, Core::CORE_0, &buttonTaskHandle);
}

bool TaskManager::createSerialInputTask() {
    return createTask("SerialInput", serialInputTask, SERIAL_INPUT_STACK_SIZE,
                     Priority::COMMUNICATION, Core::CORE_1, &serialInputTaskHandle);
}

bool TaskManager::createLedTask() {
    return createTask("LED", ledTask, LED_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &ledTaskHandle);
}

bool TaskManager::createMessageProcessorTask() {
    return createTask("MessageProcessor", messageProcessorTask, MESSAGE_PROCESSOR_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &messageProcessorTaskHandle);
}

bool TaskManager::createBytecodeVMTask() {
    return createTask("BytecodeVM", bytecodeVMTask, BYTECODE_VM_STACK_SIZE,
                     Priority::USER_PROGRAMS, Core::CORE_0, &bytecodeVMTaskHandle);
}

bool TaskManager::createStackMonitorTask() {
    return createTask("StackMonitor", stackMonitorTask, STACK_MONITOR_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &stackMonitorTaskHandle);
}

bool TaskManager::createDisplayTask() {
    return createTask("Display", displayTask, DISPLAY_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &displayTaskHandle);
}

bool TaskManager::createSensorInitTask() {
    return createTask("SensorInit", sensorInitTask, SENSOR_INIT_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &sensorInitTaskHandle);
}

bool TaskManager::createSensorPollingTask() {
    return createTask("SensorPolling", sensorPollingTask, SENSOR_POLLING_STACK_SIZE,
                     Priority::CRITICAL, Core::CORE_0, &sensorPollingTaskHandle);
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

bool TaskManager::createDisplayInitTask() {
    return createTask("DisplayInit", displayInitTask, DISPLAY_INIT_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &displayInitTaskHandle);
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
    TaskHandle_t* taskHandle,  // <-- ADD THIS PARAMETER
    void* parameters
) {
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        name,
        stackSize,
        parameters,
        static_cast<uint8_t>(priority),
        taskHandle,  // <-- USE THE PASSED HANDLE INSTEAD OF NULL
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
        {messageProcessorTaskHandle, "MessageProcessor", MESSAGE_PROCESSOR_STACK_SIZE},
        {bytecodeVMTaskHandle, "BytecodeVM", BYTECODE_VM_STACK_SIZE},
        {sensorInitTaskHandle, "SensorInit", SENSOR_INIT_STACK_SIZE},        // May be NULL after self-delete
        {sensorPollingTaskHandle, "SensorPolling", SENSOR_POLLING_STACK_SIZE}, // May be NULL initially
        {displayTaskHandle, "Display", DISPLAY_STACK_SIZE},  // Add this line
        {networkManagementTaskHandle, "NetworkMgmt", NETWORK_MANAGEMENT_STACK_SIZE},
        {networkCommunicationTaskHandle, "NetworkComm", NETWORK_COMMUNICATION_STACK_SIZE},
        {serialQueueTaskHandle, "SerialQueue", SERIAL_QUEUE_STACK_SIZE}, // ADD THIS LINE
        {batteryMonitorTaskHandle, "BatteryMonitor", BATTERY_MONITOR_STACK_SIZE},
        {speakerTaskHandle, "Speaker", SPEAKER_STACK_SIZE},
        {displayInitTaskHandle, "DisplayInit", DISPLAY_INIT_STACK_SIZE}
    };
    
    for (const auto& task : tasks) {
        if (task.handle != NULL) {
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
