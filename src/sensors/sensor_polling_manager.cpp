#include "sensor_polling_manager.h"

void SensorPollingManager::startPolling() {
    unsigned long currentTime = millis();
    // Set or extend polling end time to 1 minute from now
    pollingEndTime = currentTime + POLLING_DURATION_MS;

    // Already polling or initializing, just extend time
    if (isStartingInitializingPolling || isFinishedInitializingPolling) return;

    // Just set the flags - don't do initialization here
    isStartingInitializingPolling = true;
    isFinishedInitializingPolling = false;
    SerialQueueManager::getInstance().queueMessage("Sensor polling requested for 1 minute");
    lastPollTime = currentTime;
    TimeoutManager::getInstance().resetActivity();
}

void SensorPollingManager::stopPolling() {
    if (!isStartingInitializingPolling) return;
    
    isStartingInitializingPolling = false;
    isFinishedInitializingPolling = false;
    SerialQueueManager::getInstance().queueMessage("Sensor polling stopped");
    
    // Turn off all sensors to save power
    ImuSensor::getInstance().turnOff();
    // SideTofManager::getInstance().turnOffSideTofs();
    // MultizoneTofSensor::getInstance().turnOffSensor();
}

void SensorPollingManager::update() {
    unsigned long currentTime = millis();

    // Still need to check for polling timeout even during initialization
    if (isStartingInitializingPolling && currentTime >= pollingEndTime) {
        SerialQueueManager::getInstance().queueMessage("Sensor polling timeout reached");
        stopPolling();
        return;
    }
    
    // Handle initialization phase (only on Core 0)
    if (isStartingInitializingPolling && !isFinishedInitializingPolling) {
        initializeAllSensors();
        return; // Skip polling on this cycle
    }
    
    // Normal polling after initialization is complete
    if (isFinishedInitializingPolling && currentTime - lastPollTime >= POLL_INTERVAL_MS) {
        lastPollTime = currentTime;
        pollSensors();
    }
}

void SensorPollingManager::initializeAllSensors() {
    SerialQueueManager::getInstance().queueMessage("Initializing sensors...");
    
    // Initialize all sensors if needed
    if (ImuSensor::getInstance().needsInitialization()) {
        ImuSensor::getInstance().initialize();
    }
    
    // if (MultizoneTofSensor::getInstance().needsInitialization()) {
    //     MultizoneTofSensor::getInstance().initialize();
    // }
    
    // if (SideTofManager::getInstance().leftSideTofSensor.needsInitialization()) {
    //     SideTofManager::getInstance().leftSideTofSensor.initialize(LEFT_TOF_ADDRESS);
    // }
    
    // if (SideTofManager::getInstance().rightSideTofSensor.needsInitialization()) {
    //     SideTofManager::getInstance().rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS);
    // }
    
    // Now we're done initializing
    isFinishedInitializingPolling = true;
    SerialQueueManager::getInstance().queueMessage("Sensor initialization complete, now polling");
}

void SensorPollingManager::pollSensors() {
    // Skip polling if not fully initialized
    if (!isFinishedInitializingPolling) return;

    // Poll IMU sensor
    if (!ImuSensor::getInstance().needsInitialization()) {
        ImuSensor::getInstance().updateAllSensorData();
    } else if (ImuSensor::getInstance().canRetryInitialization()) {
        ImuSensor::getInstance().initialize();
    }
    
    // Poll Multizone TOF sensor
    // if (!MultizoneTofSensor::getInstance().needsInitialization()) {
    //     MultizoneTofSensor::getInstance().getTofData();
    // } else if (MultizoneTofSensor::getInstance().canRetryInitialization()) {
    //     MultizoneTofSensor::getInstance().initialize();
    // }
    
    // // // Poll Side TOF sensors
    // if (!SideTofManager::getInstance().leftSideTofSensor.needsInitialization()) {
    //     SideTofManager::getInstance().leftSideTofSensor.getCounts();
    // } else if (SideTofManager::getInstance().leftSideTofSensor.canRetryInitialization()) {
    //     SideTofManager::getInstance().leftSideTofSensor.initialize(LEFT_TOF_ADDRESS);
    // }
    
    // if (!SideTofManager::getInstance().rightSideTofSensor.needsInitialization()) {
    //     SideTofManager::getInstance().rightSideTofSensor.getCounts();
    // } else if (SideTofManager::getInstance().rightSideTofSensor.canRetryInitialization()) {
    //     SideTofManager::getInstance().rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS);
    // }
    TimeoutManager::getInstance().resetActivity();
}
