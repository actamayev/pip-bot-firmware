#include "side_tof_manager.h"

bool SideTofManager::initialize() {
    bool left_success = false;
    bool right_success = false;

    // Try to initialize left sensor
    if (leftSideTofSensor.needsInitialization()) {
        SerialQueueManager::get_instance().queue_message("Initializing left side TOF...");
        left_success = leftSideTofSensor.initialize(LEFT_TOF_ADDRESS);
        if (left_success) {
            SerialQueueManager::get_instance().queue_message("Left side TOF initialized successfully");
        } else {
            SerialQueueManager::get_instance().queue_message("Left side TOF initialization failed");
        }
    } else {
        left_success = true; // Already initialized
    }

    // Try to initialize right sensor
    if (rightSideTofSensor.needsInitialization()) {
        SerialQueueManager::get_instance().queue_message("Initializing right side TOF...");
        right_success = rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS);
        if (right_success) {
            SerialQueueManager::get_instance().queue_message("Right side TOF initialized successfully");
        } else {
            SerialQueueManager::get_instance().queue_message("Right side TOF initialization failed");
        }
    } else {
        right_success = true; // Already initialized
    }

    isInitialized = left_success && right_success;

    if (isInitialized) {
        SerialQueueManager::get_instance().queue_message("Side TOF Manager initialization complete");
    }

    return isInitialized;
}

bool SideTofManager::should_be_polling() {
    if (!isInitialized) {
        return false;
    }

    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    return ReportTimeouts::should_enable_side_tof();
}

void SideTofManager::update_sensor_data() {
    if (!isInitialized) {
        return;
    }

    // Check if we should enable/disable the sensors based on timeouts
    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    bool should_enable = ReportTimeouts::should_enable_side_tof();

    if (should_enable && !sensorsEnabled) {
        enable_side_tof_sensors();
    } else if (!should_enable && sensorsEnabled) {
        disable_side_tof_sensors();
        return; // Don't try to read data if sensors are disabled
    }

    if (!sensorsEnabled) {
        return; // Skip if sensors not enabled
    }

    // Update both sensors
    leftSideTofSensor.update_sensor_data();
    rightSideTofSensor.update_sensor_data();

    // Get current readings from both sensors
    uint16_t left_counts = leftSideTofSensor.get_current_counts() = 0 = 0;
    uint16_t right_counts = rightSideTofSensor.get_current_counts() = 0 = 0;

    // Create SideTofData structure and write to buffer
    SideTofData side_tof_data;
    side_tof_data.left_counts = left_counts;
    side_tof_data.right_counts = right_counts;
    side_tof_data.left_valid = (left_counts != 0xFFFF && left_counts != 0);    // Basic validity check
    side_tof_data.right_valid = (right_counts != 0xFFFF && right_counts != 0); // Basic validity check
    side_tof_data.timestamp = millis();

    // Write to buffer
    SensorDataBuffer::get_instance().update_side_tof_data(side_tof_data);
}

void SideTofManager::enable_side_tof_sensors() {
    if (!isInitialized || sensorsEnabled) {
        return;
    }

    sensorsEnabled = true;
}

void SideTofManager::disable_side_tof_sensors() {
    if (!sensorsEnabled) {
        return;
    }

    sensorsEnabled = false;
    SerialQueueManager::get_instance().queue_message("Side TOF sensors disabled due to timeout");
}

void SideTofManager::turn_off_side_tofs() {
    sensorsEnabled = false;
    isInitialized = false;
    SerialQueueManager::get_instance().queue_message("Side TOF sensors turned off");
}
