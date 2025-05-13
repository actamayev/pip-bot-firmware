#include "sensor_initializer.h"
#include "utils/utils.h"

SensorInitializer::SensorInitializer() {
    // Initialize the status array
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensorInitialized[i] = false;
    }
    initializeMultizoneTof();
    initializeIMU();
    initializeSideTimeOfFlights();
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

void SensorInitializer::initializeMultizoneTof() {
    Serial.println("Initializing Multizone sensor...");

    if (!MultizoneTofSensor::getInstance().initialize()) {
        Serial.println("Multizone sensor initialization failed");
        return;
    }
    Serial.println("Multizone sensor setup complete");
    sensorInitialized[MULTIZONE_TOF] = true;
}

void SensorInitializer::initializeIMU() {
    Serial.println("Initializing IMU...");

    if (!ImuSensor::getInstance().initialize()) {
        Serial.println("IMU initialization failed");
        return;
    }
    Serial.println("IMU setup complete");
    sensorInitialized[IMU] = true;
}

void SensorInitializer::initializeColorSensor() {
    Serial.println("Initializing Color Sensor...");

    if (!ColorSensor::getInstance().initialize()) {
        Serial.println("Color Sensor initialization failed");
        return;
    }
    Serial.println("Color Sensor setup complete");
    // sensorInitialized[COLOR_SENSOR] = true;
}

void SensorInitializer::initializeSideTimeOfFlights() {
    Serial.println("Initializing left side TOF...");
    if (!SideTofManager::getInstance().leftSideTofSensor.initialize(LEFT_TOF_ADDRESS)) {
        Serial.println("Left TOF initialization failed");
        return;
    }
    sensorInitialized[LEFT_SIDE_TOF] = true;
    
    Serial.println("Initializing right side TOF...");
    if (!SideTofManager::getInstance().rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS)) {
        Serial.println("Right TOF initialization failed");
        return;
    }
    sensorInitialized[RIGHT_SIDE_TOF] = true;
    
    Serial.println("Side TOF setup complete");
}

void SensorInitializer::initializeIRSensors() {
    Serial.println("Initializing IR sensors...");
    IrSensor::getInstance();
    // sensorInitialized[IR_SENSORS] = true;
}

bool SensorInitializer::tryInitializeMultizoneTof() {
    MultizoneTofSensor& mZoneSensor = MultizoneTofSensor::getInstance();
    if (!mZoneSensor.needsInitialization()) {
        sensorInitialized[MULTIZONE_TOF] = true;
        return true; // Already initialized
    }
    
    if (!mZoneSensor.canRetryInitialization()) {
        // Check if we've reached max retries and should restart
        if (mZoneSensor.getInitRetryCount() >= mZoneSensor.getMaxInitRetries()) {
            Serial.println("TOF sensor initialization failed after maximum retries. Restarting ESP...");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Give serial time to send
            ESP.restart(); // Restart the ESP
        }
        return false; // Can't retry yet
    }
    
    Serial.println("Retrying TOF sensor initialization...");
    bool success = mZoneSensor.initialize();
    
    if (success) {
        sensorInitialized[MULTIZONE_TOF] = true;
        Serial.println("TOF sensor retry initialization successful!");
    }
    
    return success;
}

bool SensorInitializer::tryInitializeIMU() {
    ImuSensor& imu = ImuSensor::getInstance();

    if (!imu.needsInitialization()) {
        sensorInitialized[IMU] = true;
        return true; // Already initialized
    }
    
    if (!imu.canRetryInitialization()) {
        // Check if we've reached max retries and should restart
        if (imu.getInitRetryCount() >= imu.getMaxInitRetries()) {
            Serial.println("IMU initialization failed after maximum retries. Restarting ESP...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP.restart();
        }
        return false;
    }
    
    Serial.println("Retrying IMU initialization...");
    bool success = imu.initialize();
    
    if (success) {
        Serial.println("IMU retry initialization successful!");
        sensorInitialized[IMU] = true;
    }
    
    return success;
}

bool SensorInitializer::tryInitializeLeftSideTof() {
    SideTimeOfFlightSensor& leftTof = SideTofManager::getInstance().leftSideTofSensor;

    if (!leftTof.needsInitialization()) {
        sensorInitialized[LEFT_SIDE_TOF] = true;
        return true; // Already initialized
    }
    
    if (!leftTof.canRetryInitialization()) {
        // Check if we've reached max retries
        if (leftTof.getInitRetryCount() >= leftTof.getMaxInitRetries()) {
            Serial.println("Left side TOF initialization failed after maximum retries. Restarting ESP...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP.restart();
        }
        return false; // Can't retry yet
    }
    
    Serial.println("Retrying left side TOF initialization...");
    bool success = leftTof.initialize(LEFT_TOF_ADDRESS);
    
    if (success) {
        sensorInitialized[LEFT_SIDE_TOF] = true;
        Serial.println("Left side TOF retry initialization successful!");
    }
    
    return success;
}

bool SensorInitializer::tryInitializeRightSideTof() {
    SideTimeOfFlightSensor& rightTof = SideTofManager::getInstance().rightSideTofSensor;

    if (!rightTof.needsInitialization()) {
        sensorInitialized[RIGHT_SIDE_TOF] = true;
        return true; // Already initialized
    }
    
    if (!rightTof.canRetryInitialization()) {
        // Check if we've reached max retries
        if (rightTof.getInitRetryCount() >= rightTof.getMaxInitRetries()) {
            Serial.println("Right side TOF initialization failed after maximum retries. Restarting ESP...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP.restart();
        }
        return false; // Can't retry yet
    }
    
    Serial.println("Retrying right side TOF initialization...");
    bool success = rightTof.initialize(RIGHT_TOF_ADDRESS);
    
    if (success) {
        sensorInitialized[RIGHT_SIDE_TOF] = true;
        Serial.println("Right side TOF retry initialization successful!");
    }
    
    return success;
}
