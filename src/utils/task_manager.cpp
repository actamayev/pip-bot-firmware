#include "task_manager.h"

// Task function declarations (put these in main.cpp)
extern void SensorAndBytecodeTask(void* parameter);
extern void NetworkTask(void* parameter);

bool TaskManager::createButtonTask() {
    return createTask("Buttons", buttonTask, BUTTON_STACK_SIZE, Priority::HIGHEST, Core::CORE_0);
}

bool TaskManager::createSerialInputTask() {
    return createTask("SerialInput", serialInputTask, SERIAL_INPUT_STACK_SIZE, Priority::MEDIUM_HIGH, Core::CORE_0);
}

bool TaskManager::createLedTask() {
    return createTask("LED", ledTask, LED_STACK_SIZE, Priority::LOWEST, Core::CORE_0);
}

bool TaskManager::createMessageProcessorTask() {
    return createTask("MessageProcessor", messageProcessorTask, MESSAGE_PROCESSOR_STACK_SIZE, Priority::MEDIUM, Core::CORE_0);
}

bool TaskManager::createBytecodeVMTask() {
    return createTask("BytecodeVM", bytecodeVMTask, BYTECODE_VM_STACK_SIZE, Priority::LOW_MEDIUM, Core::CORE_0);
}

bool TaskManager::createSensorTask() {
    return createTask("SensorAndBytecode", SensorAndBytecodeTask, SENSOR_STACK_SIZE, Priority::LOW_MEDIUM, Core::CORE_0);
}

bool TaskManager::createNetworkTask() {
    return createTask("Network", NetworkTask, NETWORK_STACK_SIZE, Priority::LOW_MEDIUM, Core::CORE_1);
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

void TaskManager::bytecodeVMTask(void* parameter) {
    for(;;) {
        BytecodeVM::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(5)); // User programs don't need super fast updates
    }
}

bool TaskManager::createTask(
    const char* name,
    TaskFunction_t taskFunction, 
    uint32_t stackSize,
    Priority priority,
    Core coreId,
    void* parameters
) {
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        name,
        stackSize,
        parameters,
        static_cast<uint8_t>(priority),    // Convert enum to uint8_t
        NULL,
        static_cast<BaseType_t>(coreId)    // Convert enum to BaseType_t
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

// Add this function to TaskManager
// void TaskManager::printStackUsage() {
//     SerialQueueManager::getInstance().queueMessage("=== STACK USAGE ===", SerialPriority::HIGH_PRIO);
    
//     // Check each task's remaining stack
//     TaskHandle_t tasks[] = {
//         buttonTaskHandle,
//         serialInputTaskHandle, 
//         ledTaskHandle,
//         messageProcessorTaskHandle,
//         bytecodeVMTaskHandle,
//         sensorTaskHandle,
//         networkTaskHandle
//     };
    
//     const char* names[] = {
//         "Buttons", "SerialInput", "LED", "MessageProcessor", 
//         "BytecodeVM", "Sensor", "Network"
//     };
    
//     for (int i = 0; i < 7; i++) {
//         if (tasks[i] != NULL) {
//             UBaseType_t freeStack = uxTaskGetStackHighWaterMark(tasks[i]);
//             SerialQueueManager::getInstance().queueMessage(
//                 String(names[i]) + ": " + String(freeStack) + " bytes free",
//                 SerialPriority::HIGH_PRIO
//             );
//         }
//     }
// }