#include "sensor_polling_manager.h"

void SensorPollingManager::startPolling() {
    unsigned long currentTime = millis();
    // Set or extend polling end time to 1 minute from now
    pollingEndTime = currentTime + POLLING_DURATION_MS;
    
    if (polling) return;
    
    polling = true;
    lastPollTime = currentTime;
    Serial.println("Sensor polling started for 1 minute");
    
    // Initialize all sensors if needed
    if (ImuSensor::getInstance().needsInitialization()) {
        ImuSensor::getInstance().initialize();
    }
    
    if (MultizoneTofSensor::getInstance().needsInitialization()) {
        MultizoneTofSensor::getInstance().initialize();
    }
    
    if (SideTofManager::getInstance().leftSideTofSensor.needsInitialization()) {
        SideTofManager::getInstance().leftSideTofSensor.initialize(LEFT_TOF_ADDRESS);
    }
    
    if (SideTofManager::getInstance().rightSideTofSensor.needsInitialization()) {
        SideTofManager::getInstance().rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS);
    }
}

void SensorPollingManager::stopPolling() {
    if (!polling) return;
    polling = false;
    Serial.println("Sensor polling stopped");
    
    // Turn off all sensors to save power
    ImuSensor::getInstance().turnOff();
    MultizoneTofSensor::getInstance().turnOffSensor();
    SideTofManager::getInstance().turnOffSideTofs();
}

void SensorPollingManager::update() {
    if (!polling) return; // If not polling, do nothing

    unsigned long currentTime = millis();

    // Check if polling timeout has been reached
    if (currentTime >= pollingEndTime) {
        Serial.println("Sensor polling timeout reached");
        stopPolling();
        return;
    }
    
    // Only poll once per second
    if (currentTime - lastPollTime >= POLL_INTERVAL_MS) {
        lastPollTime = currentTime;
        
        // Poll each sensor
        pollSensors();
    }
}

void SensorPollingManager::pollSensors() {
    // Poll IMU sensor
    if (!ImuSensor::getInstance().needsInitialization()) {
        ImuSensor::getInstance().updateAllSensorData();
    } else if (ImuSensor::getInstance().canRetryInitialization()) {
        ImuSensor::getInstance().initialize();
    }
    
    // Poll Multizone TOF sensor
    if (!MultizoneTofSensor::getInstance().needsInitialization()) {
        MultizoneTofSensor::getInstance().getTofData();
    } else if (MultizoneTofSensor::getInstance().canRetryInitialization()) {
        MultizoneTofSensor::getInstance().initialize();
    }
    
    // Poll Side TOF sensors
    if (!SideTofManager::getInstance().leftSideTofSensor.needsInitialization()) {
        SideTofManager::getInstance().leftSideTofSensor.getCounts();
    } else if (SideTofManager::getInstance().leftSideTofSensor.canRetryInitialization()) {
        SideTofManager::getInstance().leftSideTofSensor.initialize(LEFT_TOF_ADDRESS);
    }
    
    if (!SideTofManager::getInstance().rightSideTofSensor.needsInitialization()) {
        SideTofManager::getInstance().rightSideTofSensor.getCounts();
    } else if (SideTofManager::getInstance().rightSideTofSensor.canRetryInitialization()) {
        SideTofManager::getInstance().rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS);
    }
}
