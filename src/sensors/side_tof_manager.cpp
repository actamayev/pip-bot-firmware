#include "side_tof_manager.h"

bool SideTofManager::canRetryInitialization() const {
    // Check if either sensor needs initialization and can be retried
    return (leftSideTofSensor.needsInitialization() && leftSideTofSensor.canRetryInitialization()) ||
           (rightSideTofSensor.needsInitialization() && rightSideTofSensor.canRetryInitialization());
}

bool SideTofManager::initialize() {
    bool leftSuccess = false;
    bool rightSuccess = false;
    
    // Try to initialize left sensor
    if (leftSideTofSensor.needsInitialization()) {
        SerialQueueManager::getInstance().queueMessage("Initializing left side TOF...");
        leftSuccess = leftSideTofSensor.initialize(LEFT_TOF_ADDRESS);
        if (leftSuccess) {
            SerialQueueManager::getInstance().queueMessage("Left side TOF initialized successfully");
        } else {
            SerialQueueManager::getInstance().queueMessage("Left side TOF initialization failed");
        }
    } else {
        leftSuccess = true; // Already initialized
    }
    
    // Try to initialize right sensor
    if (rightSideTofSensor.needsInitialization()) {
        SerialQueueManager::getInstance().queueMessage("Initializing right side TOF...");
        rightSuccess = rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS);
        if (rightSuccess) {
            SerialQueueManager::getInstance().queueMessage("Right side TOF initialized successfully");
        } else {
            SerialQueueManager::getInstance().queueMessage("Right side TOF initialization failed");
        }
    } else {
        rightSuccess = true; // Already initialized
    }
    
    isInitialized = leftSuccess && rightSuccess;
    
    if (isInitialized) {
        SerialQueueManager::getInstance().queueMessage("Side TOF Manager initialization complete");
    }
    
    return isInitialized;
}

bool SideTofManager::shouldBePolling() const {
    if (!isInitialized) return false;
    
    ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
    return timeouts.shouldEnableSideTof();
}

void SideTofManager::updateSensorData() {
    if (!isInitialized) return;

    // Check if we should enable/disable the sensors based on timeouts
    ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
    bool shouldEnable = timeouts.shouldEnableSideTof();
    
    if (shouldEnable && !sensorsEnabled) {
        enableSideTofSensors();
    } else if (!shouldEnable && sensorsEnabled) {
        disableSideTofSensors();
        return; // Don't try to read data if sensors are disabled
    }
    
    if (!sensorsEnabled) return; // Skip if sensors not enabled
    
    // Update both sensors
    leftSideTofSensor.updateSensorData();
    rightSideTofSensor.updateSensorData();
    
    // Get current readings from both sensors
    uint16_t leftCounts = leftSideTofSensor.getCurrentCounts();
    uint16_t rightCounts = rightSideTofSensor.getCurrentCounts();
    
    // Create SideTofData structure and write to buffer
    SideTofData sideTofData;
    sideTofData.leftCounts = leftCounts;
    sideTofData.rightCounts = rightCounts;
    sideTofData.leftValid = (leftCounts != 0xFFFF && leftCounts != 0);  // Basic validity check
    sideTofData.rightValid = (rightCounts != 0xFFFF && rightCounts != 0); // Basic validity check
    sideTofData.timestamp = millis();
    
    // Write to buffer
    SensorDataBuffer::getInstance().updateSideTofData(sideTofData);
}

void SideTofManager::enableSideTofSensors() {
    if (!isInitialized || sensorsEnabled) return;
    
    sensorsEnabled = true;
    SerialQueueManager::getInstance().queueMessage("Side TOF sensors enabled");
}

void SideTofManager::disableSideTofSensors() {
    if (!sensorsEnabled) return;
    
    sensorsEnabled = false;
    SerialQueueManager::getInstance().queueMessage("Side TOF sensors disabled due to timeout");
}

bool SideTofManager::needsInitialization() const {
    return !isInitialized;
}

void SideTofManager::turnOffSideTofs() {
    sensorsEnabled = false;
    isInitialized = false;
    SerialQueueManager::getInstance().queueMessage("Side TOF sensors turned off");
}
