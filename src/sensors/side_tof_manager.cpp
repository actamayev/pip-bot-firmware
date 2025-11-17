#include "side_tof_manager.h"

bool SideTofManager::initialize() {
    bool left_success = false;
    bool right_success = false;

    // Try to initialize left sensor
    if (!_leftSideTofSensor.needs_initialization()) {
        left_success = true; // Already initialized
    } else {
        SerialQueueManager::get_instance().queue_message("Initializing left side TOF...");
        left_success = _leftSideTofSensor.initialize(LEFT_TOF_ADDRESS);
        if (left_success) {
            SerialQueueManager::get_instance().queue_message("Left side TOF initialized successfully");
        } else {
            SerialQueueManager::get_instance().queue_message("Left side TOF initialization failed");
        }
    }

    // Try to initialize right sensor
    if (!_rightSideTofSensor.needs_initialization()) {
        right_success = true; // Already initialized
    } else {
        SerialQueueManager::get_instance().queue_message("Initializing right side TOF...");
        right_success = _rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS);
        if (right_success) {
            SerialQueueManager::get_instance().queue_message("Right side TOF initialized successfully");
        } else {
            SerialQueueManager::get_instance().queue_message("Right side TOF initialization failed");
        }
    }

    _isInitialized = left_success && right_success;

    if (_isInitialized) {
        SerialQueueManager::get_instance().queue_message("Side TOF Manager initialization complete");
    }

    return _isInitialized;
}

bool SideTofManager::should_be_polling() {
    if (!_isInitialized) {
        return false;
    }

    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    return timeouts.should_enable_side_tof();
}

void SideTofManager::update_sensor_data() {
    if (!_isInitialized) {
        return;
    }

    // Check if we should enable/disable the sensors based on timeouts
    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    bool should_enable = timeouts.should_enable_side_tof();

    if (should_enable && !_sensorsEnabled) {
        enable_side_tof_sensors();
    } else if (!should_enable && _sensorsEnabled) {
        disable_side_tof_sensors();
        return; // Don't try to read data if sensors are disabled
    }

    if (!_sensorsEnabled) {
        return; // Skip if sensors not enabled
    }

    // Update both sensors
    _leftSideTofSensor.update_sensor_data();
    _rightSideTofSensor.update_sensor_data();

    // Get current readings from both sensors
    uint16_t left_counts = _leftSideTofSensor.get_current_counts();
    uint16_t right_counts = _rightSideTofSensor.get_current_counts();

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
    if (!_isInitialized || _sensorsEnabled) {
        return;
    }

    _sensorsEnabled = true;
}

void SideTofManager::disable_side_tof_sensors() {
    if (!_sensorsEnabled) {
        return;
    }

    _sensorsEnabled = false;
    SerialQueueManager::get_instance().queue_message("Side TOF sensors disabled due to timeout");
}

void SideTofManager::turn_off_side_tofs() {
    _sensorsEnabled = false;
    _isInitialized = false;
    SerialQueueManager::get_instance().queue_message("Side TOF sensors turned off");
}
