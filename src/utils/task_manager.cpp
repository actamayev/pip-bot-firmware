#include "task_manager.h"

// Task function declarations (put these in main.cpp)
extern void SensorAndBytecodeTask(void* parameter);
extern void NetworkTask(void* parameter);

bool TaskManager::createButtonTask() {
    return createTask("Buttons", buttonTask, 6144, 4, 0);
}

bool TaskManager::createSerialInputTask() {
    return createTask("SerialInput", serialInputTask, 4096, 3, 1);
}

bool TaskManager::createLedTask() {
    return createTask("LED", ledTask, 2048, 0, 1);
}

bool TaskManager::createSensorTask() {
    return createTask("SensorAndBytecode", SensorAndBytecodeTask, SENSOR_STACK_SIZE, 1, 0);
}

bool TaskManager::createNetworkTask() {
    return createTask("Network", NetworkTask, NETWORK_STACK_SIZE, 1, 1);
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

bool TaskManager::createTask(
    const char* name,
    TaskFunction_t taskFunction, 
    uint32_t stackSize,
    uint8_t priority,
    BaseType_t coreId,
    void* parameters
) {
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        name,
        stackSize,
        parameters,
        priority,
        NULL,           // Don't need task handle for now
        coreId
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
