#include "side_tof_manager.h"

bool SideTofManager::initialize() {
    bool leftSuccess = false;
    bool rightSuccess = false;

    // Try to initialize left sensor
    if (leftSideTofSensor.needsInitialization()) {
        SerialQueueManager::get_instance().queue_message("Initializing left side TOF...");
        leftSuccess = leftSideTofSensor.initialize(LEFT_TOF_ADDRESS);
        if (leftSuccess) {
            SerialQueueManager::get_instance().queue_message("Left side TOF initialized successfully");
        } else {
            SerialQueueManager::get_instance().queue_message("Left side TOF initialization failed");
        }
    } else {
        leftSuccess = true; // Already initialized
    }

    // Try to initialize right sensor
    if (rightSideTofSensor.needsInitialization()) {
        SerialQueueManager::get_instance().queue_message("Initializing right side TOF...");
        rightSuccess = rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS);
        if (rightSuccess) {
            SerialQueueManager::get_instance().queue_message("Right side TOF initialized successfully");
        } else {
            SerialQueueManager::get_instance().queue_message("Right side TOF initialization failed");
        }
    } else {
        rightSuccess = true; // Already initialized
    }

    isInitialized = leftSuccess && rightSuccess;

    if (isInitialized) {
        SerialQueueManager::get_instance().queue_message("Side TOF Manager initialization complete");
    }

    return isInitialized;
}

bool SideTofManager::should_be_polling() const {
    if (!isInitialized) return false;

    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    return timeouts.shouldEnableSideTof();
}

void SideTofManager::update_sensor_data() {
    if (!isInitialized) return;

    // Check if we should enable/disable the sensors based on timeouts
    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    bool shouldEnable = timeouts.shouldEnableSideTof();

    if (shouldEnable && !sensorsEnabled) {
        enable_side_tof_sensors();
    } else if (!shouldEnable && sensorsEnabled) {
        disable_side_tof_sensors();
        return; // Don't try to read data if sensors are disabled
    }

    if (!sensorsEnabled) return; // Skip if sensors not enabled

    // Update both sensors
    leftSideTofSensor.update_sensor_data();
    rightSideTofSensor.update_sensor_data();

    // Get current readings from both sensors
    uint16_t leftCounts = leftSideTofSensor.get_current_counts();
    uint16_t rightCounts = rightSideTofSensor.get_current_counts();

    // Create SideTofData structure and write to buffer
    SideTofData sideTofData;
    sideTofData.leftCounts = leftCounts;
    sideTofData.rightCounts = rightCounts;
    sideTofData.leftValid = (leftCounts != 0xFFFF && leftCounts != 0);    // Basic validity check
    sideTofData.rightValid = (rightCounts != 0xFFFF && rightCounts != 0); // Basic validity check
    sideTofData.timestamp = millis();

    // Write to buffer
    SensorDataBuffer::get_instance().update_side_tof_data(sideTofData);
}

void SideTofManager::enable_side_tof_sensors() {
    if (!isInitialized || sensorsEnabled) return;

    sensorsEnabled = true;
}

void SideTofManager::disable_side_tof_sensors() {
    if (!sensorsEnabled) return;

    sensorsEnabled = false;
    SerialQueueManager::get_instance().queue_message("Side TOF sensors disabled due to timeout");
}

void SideTofManager::turn_off_side_tofs() {
    sensorsEnabled = false;
    isInitialized = false;
    SerialQueueManager::get_instance().queue_message("Side TOF sensors turned off");
}
