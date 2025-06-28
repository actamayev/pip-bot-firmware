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

bool TaskManager::createButtonTask() {
    return createTask("Buttons", buttonTask, BUTTON_STACK_SIZE, 
                     Priority::CRITICAL, Core::CORE_0, &buttonTaskHandle);
}

bool TaskManager::createSerialInputTask() {
    return createTask("SerialInput", serialInputTask, SERIAL_INPUT_STACK_SIZE,
                     Priority::COMMUNICATION, Core::CORE_0, &serialInputTaskHandle);
}

bool TaskManager::createLedTask() {
    return createTask("LED", ledTask, LED_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_0, &ledTaskHandle);
}

bool TaskManager::createMessageProcessorTask() {
    return createTask("MessageProcessor", messageProcessorTask, MESSAGE_PROCESSOR_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &messageProcessorTaskHandle);
}

bool TaskManager::createBytecodeVMTask() {
    return createTask("BytecodeVM", bytecodeVMTask, BYTECODE_VM_STACK_SIZE,
                     Priority::USER_PROGRAMS, Core::CORE_0, &bytecodeVMTaskHandle);
}

bool TaskManager::createSensorTask() {
    return createTask("SensorAndBytecode", SensorAndBytecodeTask, SENSOR_STACK_SIZE,
                     Priority::SYSTEM_CONTROL, Core::CORE_0, &sensorTaskHandle);
}

bool TaskManager::createNetworkTask() {
    return createTask("Network", NetworkTask, NETWORK_STACK_SIZE,
                     Priority::COMMUNICATION, Core::CORE_1, &networkTaskHandle);
}

bool TaskManager::createStackMonitorTask() {
    return createTask("StackMonitor", stackMonitorTask, STACK_MONITOR_STACK_SIZE,
                     Priority::BACKGROUND, Core::CORE_1, &stackMonitorTaskHandle);
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
