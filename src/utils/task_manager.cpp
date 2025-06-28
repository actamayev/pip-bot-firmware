#include "task_manager.h"

TaskHandle_t TaskManager::buttonTaskHandle = NULL;
TaskHandle_t TaskManager::serialInputTaskHandle = NULL;
TaskHandle_t TaskManager::ledTaskHandle = NULL;
TaskHandle_t TaskManager::messageProcessorTaskHandle = NULL;
TaskHandle_t TaskManager::bytecodeVMTaskHandle = NULL;
TaskHandle_t TaskManager::networkTaskHandle = NULL;
TaskHandle_t TaskManager::stackMonitorTaskHandle = NULL;
TaskHandle_t TaskManager::sensorInitTaskHandle = NULL;
TaskHandle_t TaskManager::sensorPollingTaskHandle = NULL;
// TaskHandle_t TaskManager::displayTaskHandle = NULL;

extern void NetworkTask(void* parameter);

void TaskManager::buttonTask(void* parameter) {
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
        vTaskDelay(pdMS_TO_TICKS(5));  // No monitoring code here!
    }
}

void TaskManager::stackMonitorTask(void* parameter) {
    for(;;) {
        printStackUsage();
        vTaskDelay(pdMS_TO_TICKS(1000));  // Every 10 seconds, not 1 second
    }
}

// void TaskManager::displayTask(void* parameter) {
//     SerialQueueManager::getInstance().queueMessage("Display task started");
    
//     for(;;) {
//         DisplayScreen::getInstance().update();
//         vTaskDelay(pdMS_TO_TICKS(20));  // 50Hz update rate, smooth for animations
//     }
// }

void TaskManager::sensorInitTask(void* parameter) {
    disableCore0WDT();
    vTaskDelay(pdMS_TO_TICKS(10));
    
    SerialQueueManager::getInstance().queueMessage("Starting sensor initialization on Core 0...");
    
    // Setup button loggers (from original sensor task)
    setupButtonLoggers();
    
//    if (!DisplayScreen::getInstance().init()) {
//         SerialQueueManager::getInstance().queueMessage("Display initialization failed");
//     } else {
//         SerialQueueManager::getInstance().queueMessage("Display initialized successfully");
//     }
    // Get the sensor initializer
    SensorInitializer& initializer = SensorInitializer::getInstance();
    
    // Keep trying until ALL sensors are initialized
    // This preserves the existing behavior where we don't give up
    while (!initializer.areAllSensorsInitialized()) {
        // Try each sensor type individually
        if (!initializer.isSensorInitialized(SensorInitializer::IMU)) {
            SerialQueueManager::getInstance().queueMessage("Trying to init IMU...");
            initializer.tryInitializeIMU();
        }
        
        if (!initializer.isSensorInitialized(SensorInitializer::MULTIZONE_TOF)) {
            SerialQueueManager::getInstance().queueMessage("Trying to init Multizone TOF...");
            initializer.tryInitializeMultizoneTof();
        }
        
        if (!initializer.isSensorInitialized(SensorInitializer::LEFT_SIDE_TOF)) {
            SerialQueueManager::getInstance().queueMessage("Trying to init Left TOF...");
            initializer.tryInitializeLeftSideTof();
        }
        
        if (!initializer.isSensorInitialized(SensorInitializer::RIGHT_SIDE_TOF)) {
            SerialQueueManager::getInstance().queueMessage("Trying to init Right TOF...");
            initializer.tryInitializeRightSideTof();
        }
        
        // Small delay between retry cycles
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    SerialQueueManager::getInstance().queueMessage("All sensors initialized successfully!");
    enableCore0WDT();
    
    // Create the sensor polling task now that init is complete
    bool pollingTaskCreated = createSensorPollingTask();
    // bool displayTaskCreated = createDisplayTask();

    if (pollingTaskCreated) {
        SerialQueueManager::getInstance().queueMessage("All tasks created - initialization complete");
        SensorPollingManager::getInstance().startPolling();
    } else {
        SerialQueueManager::getInstance().queueMessage("ERROR: Failed to create required tasks!");
    }
    
    // Self-delete - our job is done
    SerialQueueManager::getInstance().queueMessage("SensorInit task self-deleting");
    sensorInitTaskHandle = NULL;  // Clear handle before deletion
    vTaskDelete(NULL);
}

// SensorPolling Task - handles ongoing sensor data collection
void TaskManager::sensorPollingTask(void* parameter) {
    SerialQueueManager::getInstance().queueMessage("Sensor polling task started");
    
    // Main polling loop - this preserves all existing SensorPollingManager behavior
    for(;;) {
        SensorPollingManager::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(5));  // Same timing as before
    }
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

bool TaskManager::createNetworkTask() {
    return createTask("Network", NetworkTask, NETWORK_STACK_SIZE,
                     Priority::COMMUNICATION, Core::CORE_1, &networkTaskHandle);
}

bool TaskManager::createSensorInitTask() {
    return createTask("SensorInit", sensorInitTask, SENSOR_INIT_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &sensorInitTaskHandle);
}

bool TaskManager::createSensorPollingTask() {
    return createTask("SensorPolling", sensorPollingTask, SENSOR_POLLING_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &sensorPollingTaskHandle);
}

// bool TaskManager::createDisplayTask() {
//     return createTask("Display", displayTask, DISPLAY_STACK_SIZE,
//                      Priority::BACKGROUND, Core::CORE_1, &displayTaskHandle);
// }

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
        SerialQueueManager::getInstance().queueMessage(
            String("✓ Created task: ") + name, 
            SerialPriority::HIGH_PRIO
        );
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
        // {displayTaskHandle, "Display", DISPLAY_STACK_SIZE},  // Add this line
        {networkTaskHandle, "Network", NETWORK_STACK_SIZE}
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
