#include "task_manager.h"

TaskHandle_t TaskManager::buttonTaskHandle = NULL;
TaskHandle_t TaskManager::serialInputTaskHandle = NULL;
TaskHandle_t TaskManager::ledTaskHandle = NULL;
TaskHandle_t TaskManager::messageProcessorTaskHandle = NULL;
TaskHandle_t TaskManager::bytecodeVMTaskHandle = NULL;
TaskHandle_t TaskManager::sensorTaskHandle = NULL;
TaskHandle_t TaskManager::networkTaskHandle = NULL;
TaskHandle_t TaskManager::stackMonitorTaskHandle = NULL;

// Task function declarations (put these in main.cpp)
extern void SensorAndBytecodeTask(void* parameter);
extern void NetworkTask(void* parameter);

bool TaskManager::createButtonTask() {
    BaseType_t result = xTaskCreatePinnedToCore(
        buttonTask,
        "Buttons",
        BUTTON_STACK_SIZE,
        NULL,
        static_cast<uint8_t>(Priority::CRITICAL),
        &buttonTaskHandle,  // <-- Store handle instead of NULL
        static_cast<BaseType_t>(Core::CORE_0)
    );
    return logTaskCreation("Buttons", result == pdPASS);
}

bool TaskManager::createSerialInputTask() {
    BaseType_t result = xTaskCreatePinnedToCore(
        serialInputTask,
        "SerialInput",
        SERIAL_INPUT_STACK_SIZE,
        NULL,
        static_cast<uint8_t>(Priority::COMMUNICATION),
        &serialInputTaskHandle,  // <-- Store handle
        static_cast<BaseType_t>(Core::CORE_0)
    );
    return logTaskCreation("SerialInput", result == pdPASS);
    // return createTask("SerialInput", serialInputTask, SERIAL_INPUT_STACK_SIZE, Priority::MEDIUM_HIGH, Core::CORE_0);
}

bool TaskManager::createLedTask() {
    BaseType_t result = xTaskCreatePinnedToCore(
        ledTask,
        "LED",
        LED_STACK_SIZE,
        NULL,
        static_cast<uint8_t>(Priority::BACKGROUND),
        &ledTaskHandle,  // <-- Store handle
        static_cast<BaseType_t>(Core::CORE_0)
    );
    return logTaskCreation("LED", result == pdPASS);
    // return createTask("LED", ledTask, LED_STACK_SIZE, Priority::LOWEST, Core::CORE_0);
}

bool TaskManager::createMessageProcessorTask() {
    BaseType_t result = xTaskCreatePinnedToCore(
        messageProcessorTask,
        "MessageProcessor",
        MESSAGE_PROCESSOR_STACK_SIZE,
        NULL,
        static_cast<uint8_t>(Priority::SYSTEM_CONTROL),
        &messageProcessorTaskHandle,  // <-- Store handle
        static_cast<BaseType_t>(Core::CORE_0)
    );
    return logTaskCreation("MessageProcessor", result == pdPASS);
}

bool TaskManager::createBytecodeVMTask() {
    BaseType_t result = xTaskCreatePinnedToCore(
        bytecodeVMTask,
        "BytecodeVM",
        BYTECODE_VM_STACK_SIZE,
        NULL,
        static_cast<uint8_t>(Priority::USER_PROGRAMS),
        &bytecodeVMTaskHandle,  // <-- Store handle
        static_cast<BaseType_t>(Core::CORE_0)
    );
    return logTaskCreation("BytecodeVM", result == pdPASS);
}

bool TaskManager::createSensorTask() {
    BaseType_t result = xTaskCreatePinnedToCore(
        SensorAndBytecodeTask,
        "SensorAndBytecode",
        SENSOR_STACK_SIZE,
        NULL,
        static_cast<uint8_t>(Priority::USER_PROGRAMS),
        &sensorTaskHandle,  // <-- Store handle
        static_cast<BaseType_t>(Core::CORE_0)
    );
    return logTaskCreation("SensorAndBytecode", result == pdPASS);
    // return createTask("SensorAndBytecode", SensorAndBytecodeTask, SENSOR_STACK_SIZE, Priority::LOW_MEDIUM, Core::CORE_0);
}

bool TaskManager::createNetworkTask() {
    BaseType_t result = xTaskCreatePinnedToCore(
        NetworkTask,
        "Network",
        NETWORK_STACK_SIZE,
        NULL,
        static_cast<uint8_t>(Priority::COMMUNICATION),
        &networkTaskHandle,  // <-- Store handle
        static_cast<BaseType_t>(Core::CORE_0)
    );
    return logTaskCreation("Network", result == pdPASS);
    // return createTask("Network", NetworkTask, NETWORK_STACK_SIZE, Priority::LOW_MEDIUM, Core::CORE_1);
}

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

void TaskManager::createStackMonitorTask() {
    BaseType_t result = xTaskCreatePinnedToCore(
        stackMonitorTask,
        "StackMonitor",
        STACK_MONITOR_STACK_SIZE,
        NULL,
        static_cast<uint8_t>(Priority::BACKGROUND),  // Background monitoring
        &stackMonitorTaskHandle,
        static_cast<BaseType_t>(Core::CORE_1)    // Use Core 1 - not time critical
    );
    logTaskCreation("StackMonitor", result == pdPASS);
    return;
}

void TaskManager::stackMonitorTask(void* parameter) {
    for(;;) {
        printStackUsage();
        vTaskDelay(pdMS_TO_TICKS(1000));  // Every 10 seconds, not 1 second
    }
}

void TaskManager::bytecodeVMTask(void* parameter) {
    for(;;) {
        BytecodeVM::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(5));  // No monitoring code here!
    }
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
        {sensorTaskHandle, "Sensor", SENSOR_STACK_SIZE},
        {networkTaskHandle, "Network", NETWORK_STACK_SIZE}
    };
    
    for (int i = 0; i < 7; i++) {
        if (tasks[i].handle != NULL) {
            UBaseType_t freeStack = uxTaskGetStackHighWaterMark(tasks[i].handle);
            uint32_t usedStack = tasks[i].allocatedSize - (freeStack * sizeof(StackType_t));
            float percentUsed = (float)usedStack / tasks[i].allocatedSize * 100.0f;
            
            // Format with fixed width for alignment
            char taskName[16];
            snprintf(taskName, sizeof(taskName), "%-15s", tasks[i].name);
            
            String message = "║ " + String(taskName) + " " + 
                           String(usedStack, DEC) + "/" + String(tasks[i].allocatedSize) + 
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
