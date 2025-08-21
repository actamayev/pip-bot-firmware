#include "sensor_initializer.h"
#include "utils/utils.h"

SensorInitializer::SensorInitializer() {
    // Initialize the status array
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensorInitialized[i] = false;
    }
    
    SerialQueueManager::getInstance().queueMessage("Starting centralized sensor initialization...");
    
    // Initialize sensors sequentially to avoid I2C conflicts
    // Start with the heaviest I2C user first (multizone requires ~84KB data transfer)
    // initializeMultizoneTof();
    initializeSideTofs();  
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

bool SensorInitializer::areAllSensorsInitialized() const {
    // Check if all sensors are initialized
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (!sensorInitialized[i]) {
            return false;
        }
    }
    return true;
}

// void SensorInitializer::initializeMultizoneTof() {
//     SerialQueueManager::getInstance().queueMessage("Initializing Multizone sensor...");

//     if (!MultizoneTofSensor::getInstance().initialize()) {
//         SerialQueueManager::getInstance().queueMessage("Multizone sensor initialization failed");
//         return;
//     }
//     SerialQueueManager::getInstance().queueMessage("Multizone sensor setup complete");
//     sensorInitialized[MULTIZONE_TOF] = true;
// }

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

void SensorInitializer::initializeSideTofs() {
    SerialQueueManager::getInstance().queueMessage("Initializing Side TOF sensors...");

    if (!SideTofManager::getInstance().initialize()) {
        SerialQueueManager::getInstance().queueMessage("Side TOF sensors initialization failed");
        return;
    }
    SerialQueueManager::getInstance().queueMessage("Side TOF sensors setup complete");
    sensorInitialized[SIDE_TOFS] = true;
}


// bool SensorInitializer::tryInitializeMultizoneTof() {
//     MultizoneTofSensor& mZoneSensor = MultizoneTofSensor::getInstance();
//     if (!mZoneSensor.needsInitialization()) {
//         sensorInitialized[MULTIZONE_TOF] = true;
//         return true; // Already initialized
//     }
    
//     if (!mZoneSensor.canRetryInitialization()) {
//         // Check if we've reached max retries
//         if (mZoneSensor.getInitRetryCount() >= mZoneSensor.getMaxInitRetries()) {
//             SerialQueueManager::getInstance().queueMessage("TOF sensor initialization failed after maximum retries. Sensor disabled.");
//         }
//         return false; // Can't retry yet
//     }
    
//     SerialQueueManager::getInstance().queueMessage("Retrying TOF sensor initialization...");
//     bool success = mZoneSensor.initialize();
    
//     if (success) {
//         sensorInitialized[MULTIZONE_TOF] = true;
//         SerialQueueManager::getInstance().queueMessage("TOF sensor retry initialization successful!");
//     }
    
//     return success;
// }

bool SensorInitializer::tryInitializeIMU() {
    ImuSensor& imu = ImuSensor::getInstance();

    if (!imu.needsInitialization()) {
        sensorInitialized[IMU] = true;
        return true; // Already initialized
    }
    
    if (!imu.canRetryInitialization()) {
        // Check if we've reached max retries
        if (imu.getInitRetryCount() >= imu.getMaxInitRetries()) {
            SerialQueueManager::getInstance().queueMessage("IMU initialization failed after maximum retries. Sensor disabled.");
        }
        return false;
    }
    
    SerialQueueManager::getInstance().queueMessage("Retrying IMU initialization...");
    bool success = imu.initialize();
    
    if (success) {
        SerialQueueManager::getInstance().queueMessage("IMU retry initialization successful!");
        sensorInitialized[IMU] = true;
    }
    
    return success;
}

bool SensorInitializer::tryInitializeSideTofs() {
    SideTofManager& sideTofManager = SideTofManager::getInstance();

    if (!sideTofManager.needsInitialization()) {
        sensorInitialized[SIDE_TOFS] = true;
        return true; // Already initialized
    }
    
    if (!sideTofManager.canRetryInitialization()) {
        return false; // Can't retry yet
    }
    
    SerialQueueManager::getInstance().queueMessage("Retrying Side TOF sensors initialization...");
    bool success = sideTofManager.initialize();
    
    if (success) {
        SerialQueueManager::getInstance().queueMessage("Side TOF sensors retry initialization successful!");
        sensorInitialized[SIDE_TOFS] = true;
    }
    
    return success;
}

bool SensorInitializer::tryInitializeColorSensor() {
    ColorSensor& colorSensor = ColorSensor::getInstance();

    if (!colorSensor.needsInitialization()) {
        sensorInitialized[COLOR_SENSOR] = true;
        return true; // Already initialized
    }
    
    if (!colorSensor.canRetryInitialization()) {
        return false; // Can't retry yet
    }
    
    SerialQueueManager::getInstance().queueMessage("Retrying Color sensor initialization...");
    bool success = colorSensor.initialize();
    
    if (success) {
        SerialQueueManager::getInstance().queueMessage("Color sensor retry initialization successful!");
        sensorInitialized[COLOR_SENSOR] = true;
    }
    
    return success;
}
