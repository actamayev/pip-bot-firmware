#include "sensor_initializer.h"
#include "utils/utils.h"

// TODO 8/21/25: Consider moving all sensor initialization to the individual sensor level.
// Last time we tried this, it didn't work well, but I think it was because the IMU hogged the line bc of the 84KB firmware it need to flash from the ESP to the MZ.
// We already moved the side tof init to the sensor level
SensorInitializer::SensorInitializer() {
    // Initialize the status array
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensorInitialized[i] = false;
    }

    SerialQueueManager::getInstance().queueMessage("Starting centralized sensor initialization...");
    
    // Initialize sensors sequentially to avoid I2C conflicts
    // Start with the heaviest I2C user first (multizone requires ~84KB data transfer)
    initializeMultizoneTof();
    initializeIMU();
    initializeColorSensor();       
    
    SerialQueueManager::getInstance().queueMessage("Centralized sensor initialization complete");
}

bool SensorInitializer::isSensorInitialized(SensorType sensor) const {
    if (sensor >= 0 && sensor < SENSOR_COUNT) {
        return sensorInitialized[sensor];
    }
    return false;
}

void SensorInitializer::initializeMultizoneTof() {
    SerialQueueManager::getInstance().queueMessage("Initializing Multizone sensor...");

    if (!MultizoneTofSensor::getInstance().initialize()) {
        SerialQueueManager::getInstance().queueMessage("Multizone sensor initialization failed");
        return;
    }
    SerialQueueManager::getInstance().queueMessage("Multizone sensor setup complete");
    sensorInitialized[MULTIZONE_TOF] = true;
}

void SensorInitializer::initializeIMU() {
    SerialQueueManager::getInstance().queueMessage("Initializing IMU...");

    if (!ImuSensor::getInstance().initialize()) {
        SerialQueueManager::getInstance().queueMessage("IMU initialization failed");
        return;
    }
    SerialQueueManager::getInstance().queueMessage("IMU setup complete");
    sensorInitialized[IMU] = true;
}


void SensorInitializer::initializeColorSensor() {
    SerialQueueManager::getInstance().queueMessage("Initializing Color Sensor...");

    if (!ColorSensor::getInstance().initialize()) {
        SerialQueueManager::getInstance().queueMessage("Color Sensor initialization failed");
        return;
    }
    SerialQueueManager::getInstance().queueMessage("Color Sensor setup complete");
    sensorInitialized[COLOR_SENSOR] = true;
}
