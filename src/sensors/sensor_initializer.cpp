#include "./sensor_initializer.h"
#include "../utils/utils.h"

bool SensorInitializer::initializeAllSensors() {
    // This method would be called by Sensors class to initialize all sensors
    return areAllSensorsInitialized();
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

void SensorInitializer::initializeMultizoneTof(MultizoneTofSensor& sensor) {
    Serial.println("Initializing Multizone sensor...");

    if (sensor.initialize()) {
        Serial.println("Multizone sensor setup complete");
        sensorInitialized[MULTIZONE_TOF] = true;
    } else {
        Serial.println("Multizone sensor initialization failed");
    }
}

void SensorInitializer::initializeIMU(ImuSensor& sensor) {
    Serial.println("Initializing IMU...");

    if (!sensor.initialize()) {
        Serial.println("IMU initialization failed");
        return;
    }
    Serial.println("IMU setup complete");
    sensorInitialized[IMU] = true;
}

void SensorInitializer::initializeColorSensor(ColorSensor& sensor) {
    Serial.println("Initializing Color Sensor...");

    if (!sensor.initialize()) {
        Serial.println("Color Sensor initialization failed");
        return;
    }
    Serial.println("Color Sensor setup complete");
    // sensorInitialized[COLOR_SENSOR] = true;
}

void SensorInitializer::initializeSideTimeOfFlights(SideTimeOfFlightSensor& leftSensor, SideTimeOfFlightSensor& rightSensor) {
    Serial.println("Initializing left side TOF...");
    if (!leftSensor.initialize(LEFT_TOF_ADDRESS)) {
        Serial.println("Left TOF initialization failed");
        return;
    }
    sensorInitialized[LEFT_SIDE_TOF] = true;
    
    Serial.println("Initializing right side TOF...");
    if (!rightSensor.initialize(RIGHT_TOF_ADDRESS)) {
        Serial.println("Right TOF initialization failed");
        return;
    }
    sensorInitialized[RIGHT_SIDE_TOF] = true;
    
    Serial.println("Side TOF setup complete");
}

bool SensorInitializer::isImuInitialized(const ImuSensor& sensor) const {
    return !sensor.needsInitialization();
}

bool SensorInitializer::tryInitializeMultizoneTof(MultizoneTofSensor& sensor) {
    if (!sensor.needsInitialization()) {
        sensorInitialized[MULTIZONE_TOF] = true;
        return true; // Already initialized
    }
    
    if (!sensor.canRetryInitialization()) {
        // Check if we've reached max retries and should restart
        if (sensor.getInitRetryCount() >= sensor.getMaxInitRetries()) {
            Serial.println("TOF sensor initialization failed after maximum retries. Restarting ESP...");
            delay(1000); // Give serial time to send
            ESP.restart(); // Restart the ESP
        }
        return false; // Can't retry yet
    }
    
    Serial.println("Retrying TOF sensor initialization...");
    bool success = sensor.initialize();
    
    if (success) {
        sensorInitialized[MULTIZONE_TOF] = true;
        Serial.println("TOF sensor retry initialization successful!");
    }
    
    return success;
}

bool SensorInitializer::tryInitializeIMU(ImuSensor& sensor) {
    if (!sensor.needsInitialization()) {
        sensorInitialized[IMU] = true;
        return true; // Already initialized
    }
    
    if (!sensor.canRetryInitialization()) {
        // Check if we've reached max retries and should restart
        if (sensor.getInitRetryCount() >= sensor.getMaxInitRetries()) {
            Serial.println("IMU initialization failed after maximum retries. Restarting ESP...");
            delay(1000);
            ESP.restart();
        }
        return false;
    }
    
    Serial.println("Retrying IMU initialization...");
    bool success = sensor.initialize();
    
    if (success) {
        Serial.println("IMU retry initialization successful!");
        sensorInitialized[IMU] = true;
    }
    
    return success;
}

bool SensorInitializer::tryInitializeLeftSideTof(SideTimeOfFlightSensor& sensor) {
    if (!sensor.needsInitialization()) {
        sensorInitialized[LEFT_SIDE_TOF] = true;
        return true; // Already initialized
    }
    
    if (!sensor.canRetryInitialization()) {
        // Check if we've reached max retries
        if (sensor.getInitRetryCount() >= sensor.getMaxInitRetries()) {
            Serial.println("Left side TOF initialization failed after maximum retries. Restarting ESP...");
            delay(1000);
            ESP.restart();
        }
        return false; // Can't retry yet
    }
    
    Serial.println("Retrying left side TOF initialization...");
    bool success = sensor.initialize(LEFT_TOF_ADDRESS);
    
    if (success) {
        sensorInitialized[LEFT_SIDE_TOF] = true;
        Serial.println("Left side TOF retry initialization successful!");
    }
    
    return success;
}

bool SensorInitializer::tryInitializeRightSideTof(SideTimeOfFlightSensor& sensor) {
    if (!sensor.needsInitialization()) {
        sensorInitialized[RIGHT_SIDE_TOF] = true;
        return true; // Already initialized
    }
    
    if (!sensor.canRetryInitialization()) {
        // Check if we've reached max retries
        if (sensor.getInitRetryCount() >= sensor.getMaxInitRetries()) {
            Serial.println("Right side TOF initialization failed after maximum retries. Restarting ESP...");
            delay(1000);
            ESP.restart();
        }
        return false; // Can't retry yet
    }
    
    Serial.println("Retrying right side TOF initialization...");
    bool success = sensor.initialize(RIGHT_TOF_ADDRESS);
    
    if (success) {
        sensorInitialized[RIGHT_SIDE_TOF] = true;
        Serial.println("Right side TOF retry initialization successful!");
    }
    
    return success;
}
