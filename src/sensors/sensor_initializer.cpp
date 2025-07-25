#include "sensor_initializer.h"
#include "utils/utils.h"

SensorInitializer::SensorInitializer() {
    // Initialize the status array
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensorInitialized[i] = false;
    }
    // initializeMultizoneTof();
    initializeIMU();
    // initializeSideTimeOfFlights();
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
    // sensorInitialized[COLOR_SENSOR] = true;
}

// void SensorInitializer::initializeSideTimeOfFlights() {
//     SerialQueueManager::getInstance().queueMessage("Initializing left side TOF...");
//     if (!SideTofManager::getInstance().leftSideTofSensor.initialize(LEFT_TOF_ADDRESS)) {
//         SerialQueueManager::getInstance().queueMessage("Left TOF initialization failed");
//         return;
//     }
//     sensorInitialized[LEFT_SIDE_TOF] = true;
    
//     SerialQueueManager::getInstance().queueMessage("Initializing right side TOF...");
//     if (!SideTofManager::getInstance().rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS)) {
//         SerialQueueManager::getInstance().queueMessage("Right TOF initialization failed");
//         return;
//     }
//     sensorInitialized[RIGHT_SIDE_TOF] = true;
    
//     SerialQueueManager::getInstance().queueMessage("Side TOF setup complete");
// }

void SensorInitializer::initializeIRSensors() {
    SerialQueueManager::getInstance().queueMessage("Initializing IR sensors...");
    IrSensor::getInstance();
    // sensorInitialized[IR_SENSORS] = true;
}

// bool SensorInitializer::tryInitializeMultizoneTof() {
//     MultizoneTofSensor& mZoneSensor = MultizoneTofSensor::getInstance();
//     if (!mZoneSensor.needsInitialization()) {
//         sensorInitialized[MULTIZONE_TOF] = true;
//         return true; // Already initialized
//     }
    
//     if (!mZoneSensor.canRetryInitialization()) {
//         // Check if we've reached max retries and should restart
//         if (mZoneSensor.getInitRetryCount() >= mZoneSensor.getMaxInitRetries()) {
//             SerialQueueManager::getInstance().queueMessage("TOF sensor initialization failed after maximum retries. Restarting ESP...");
//             vTaskDelay(pdMS_TO_TICKS(1000)); // Give serial time to send
//             ESP.restart(); // Restart the ESP
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
        // Check if we've reached max retries and should restart
        if (imu.getInitRetryCount() >= imu.getMaxInitRetries()) {
            SerialQueueManager::getInstance().queueMessage("IMU initialization failed after maximum retries. Restarting ESP...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP.restart();
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

// bool SensorInitializer::tryInitializeLeftSideTof() {
//     SideTimeOfFlightSensor& leftTof = SideTofManager::getInstance().leftSideTofSensor;

//     if (!leftTof.needsInitialization()) {
//         sensorInitialized[LEFT_SIDE_TOF] = true;
//         return true; // Already initialized
//     }
    
//     if (!leftTof.canRetryInitialization()) {
//         // Check if we've reached max retries
//         if (leftTof.getInitRetryCount() >= leftTof.getMaxInitRetries()) {
//             SerialQueueManager::getInstance().queueMessage("Left side TOF initialization failed after maximum retries. Restarting ESP...");
//             vTaskDelay(pdMS_TO_TICKS(1000));
//             ESP.restart();
//         }
//         return false; // Can't retry yet
//     }
    
//     SerialQueueManager::getInstance().queueMessage("Retrying left side TOF initialization...");
//     bool success = leftTof.initialize(LEFT_TOF_ADDRESS);
    
//     if (success) {
//         sensorInitialized[LEFT_SIDE_TOF] = true;
//         SerialQueueManager::getInstance().queueMessage("Left side TOF retry initialization successful!");
//     }
    
//     return success;
// }

// bool SensorInitializer::tryInitializeRightSideTof() {
//     SideTimeOfFlightSensor& rightTof = SideTofManager::getInstance().rightSideTofSensor;

//     if (!rightTof.needsInitialization()) {
//         sensorInitialized[RIGHT_SIDE_TOF] = true;
//         return true; // Already initialized
//     }
    
//     if (!rightTof.canRetryInitialization()) {
//         // Check if we've reached max retries
//         if (rightTof.getInitRetryCount() >= rightTof.getMaxInitRetries()) {
//             SerialQueueManager::getInstance().queueMessage("Right side TOF initialization failed after maximum retries. Restarting ESP...");
//             vTaskDelay(pdMS_TO_TICKS(1000));
//             ESP.restart();
//         }
//         return false; // Can't retry yet
//     }
    
//     SerialQueueManager::getInstance().queueMessage("Retrying right side TOF initialization...");
//     bool success = rightTof.initialize(RIGHT_TOF_ADDRESS);
    
//     if (success) {
//         sensorInitialized[RIGHT_SIDE_TOF] = true;
//         SerialQueueManager::getInstance().queueMessage("Right side TOF retry initialization successful!");
//     }
    
//     return success;
// }
