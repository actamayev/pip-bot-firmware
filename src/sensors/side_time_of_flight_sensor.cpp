#include "side_time_of_flight_sensor.h"

#include <algorithm>

bool SideTimeOfFlightSensor::initialize(const uint8_t TOF_ADDRESS) {
    // Save the sensor address to the class member variable
    _sensorAddress = TOF_ADDRESS;

    // Add a delay before trying to initialize
    vTaskDelay(pdMS_TO_TICKS(50));

    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        // Reset and initialize sensor
        reset_specific_sensor();

        // Initialize the sensor in auto mode
        basic_initialization_auto_mode();

        // Try to read data to verify initialization (use raw reading since calibration not loaded yet)
        uint16_t test_value = VCNL36828P_GET_PS_DATA(_sensorAddress);

        // Check if the reading seems valid (this may need to be adjusted based on your sensor)
        if (test_value != 0xFFFF && test_value != 0) { // Adjust these values based on what indicates failure
            _isInitialized = true;

            // Load existing calibration or perform new calibration
            load_calibration_from_preferences();
            if (!_isCalibrated) {
                SerialQueueManager::get_instance().queue_message("No calibration found - performing auto-calibration");
                perform_calibration();
            }

            return true;
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Delay between attempts
    }

    return false;
}

// NEW: Buffer-based sensor data update method
void SideTimeOfFlightSensor::update_sensor_data() {
    // Only try to read if sensor is initialized
    if (!_isInitialized) {
        return;
    }

    // Skip rate limiting entirely for maximum performance like performance test
    uint32_t current_time = millis();
    uint32_t elapsed_time = current_time - _lastUpdateTime;
    if (elapsed_time < DELAY_BETWEEN_READINGS) {
        return;
    }

    // Read current sensor data with no throttling
    uint16_t counts = read_proximity_data();
    _lastUpdateTime = current_time;

    // Note: This method is called by SideTofManager which will combine
    // both left and right readings and write to buffer
    // We don't write to buffer directly here since we need both sensors
}

uint16_t SideTimeOfFlightSensor::get_current_counts() {
    // Only try to read if sensor is initialized
    if (!_isInitialized) {
        return 0;
    }

    uint32_t current_time = millis();
    uint32_t elapsed_time = current_time - _lastUpdateTime;

    // Only update if enough time has passed
    if (elapsed_time >= DELAY_BETWEEN_READINGS) {
        _lastUpdateTime = current_time;
    }

    return read_proximity_data();
}

void SideTimeOfFlightSensor::basic_initialization_auto_mode() {
    // 1.) Initialization
    // Switch the sensor off
    VCNL36828P_SET_PS_ON(_sensorAddress, VCNL36828P_PS_ON_DIS);

    // 2.) Setting up PS
    // PS_CONF1_H
    // Enable/disable high dynamic range (12 bit/16 bit) ADC output setting
    VCNL36828P_SET_PS_HD(_sensorAddress, VCNL36828P_PS_HD_16Bits);

    // PS_CONF2_L
    // Set the integration time for one measurement; the pulse length "T" is determined by PS_ITB
    VCNL36828P_SET_PS_IT(_sensorAddress, VCNL36828P_PS_IT_1T); // Fastest - matches performance test
    // Set the pulse length "T" for PS_IT
    VCNL36828P_SET_PS_ITB(_sensorAddress, VCNL36828P_PS_ITB_25us); // Shortest - matches performance test
    // PS_CONF2_H
    // Set the VCSEL driving current
    VCNL36828P_SET_I_VCSEL(_sensorAddress, VCNL36828P_I_VCSEL_20mA);

    // PS_CONF3_L
    // Set the measurement mode of the sensor
    VCNL36828P_SET_PS_MODE(_sensorAddress, VCNL36828P_PS_MODE_AUTO_MODE);

    // 3.) Switch On the sensor
    // Enable the internal calibration
    VCNL36828P_SET_PS_CAL(_sensorAddress, VCNL36828P_PS_CAL_EN);
    // Switch the sensor on
    VCNL36828P_SET_PS_ON(_sensorAddress, VCNL36828P_PS_ON_EN);

    // Delay needs to be changed depending on the API of the µ-controller
    vTaskDelay(pdMS_TO_TICKS(100));
}

void SideTimeOfFlightSensor::load_calibration_from_preferences() {
    PreferencesManager& prefs = PreferencesManager::get_instance();

    if (!prefs.has_side_tof_calibration(_sensorAddress)) {
        _isCalibrated = false;
        char log_message[128];
        snprintf(log_message, sizeof(log_message), "No stored calibration found for sensor 0x%02X", _sensorAddress);
        SerialQueueManager::get_instance().queue_message(log_message);
        return;
    }
    _baselineValue = prefs.get_side_tof_baseline(_sensorAddress);
    _useHardwareCalibration = prefs.get_side_tof_use_hardware_calibration(_sensorAddress);
    _isCalibrated = true;

    char log_message[128];
    snprintf(log_message, sizeof(log_message), "Loaded calibration for sensor 0x%02X: baseline=%u, hw_calib=%s", _sensorAddress, _baselineValue,
             _useHardwareCalibration ? "true" : "false");
    SerialQueueManager::get_instance().queue_message(log_message);

    if (_useHardwareCalibration) {
        apply_hardware_calibration(_baselineValue);
    }
}

bool SideTimeOfFlightSensor::perform_calibration() {
    char log_message[128];
    snprintf(log_message, sizeof(log_message), "Calibrating sensor 0x%02X...", _sensorAddress);
    SerialQueueManager::get_instance().queue_message(log_message);

    SerialQueueManager::get_instance().queue_message("Make sure no obstacles are in front of the sensors!");
    vTaskDelay(pdMS_TO_TICKS(3000)); // Give time to clear obstacles

    uint16_t baseline = capture_baseline_reading();

    snprintf(log_message, sizeof(log_message), "Baseline reading: %u", baseline);
    SerialQueueManager::get_instance().queue_message(log_message);

    _baselineValue = baseline;

    // Use hardware cancellation if baseline is within valid range (0-4095)
    if (baseline <= 4095) {
        _useHardwareCalibration = true;
        apply_hardware_calibration(baseline);
        SerialQueueManager::get_instance().queue_message("Using hardware calibration");
    } else {
        _useHardwareCalibration = false;
        VCNL36828P_SET_PS_CANC(_sensorAddress, 0); // Clear hardware cancellation
        SerialQueueManager::get_instance().queue_message("Using software calibration (baseline too high for hardware)");
    }

    // Store calibration in NVS
    PreferencesManager::get_instance().store_side_tof_calibration(_sensorAddress, _baselineValue, _useHardwareCalibration);

    _isCalibrated = true;

    snprintf(log_message, sizeof(log_message), "Sensor 0x%02X calibrated successfully - baseline: %u", _sensorAddress, _baselineValue);
    SerialQueueManager::get_instance().queue_message(log_message);

    return true;
}

uint16_t SideTimeOfFlightSensor::capture_baseline_reading() {
    const int NUM_SAMPLES = 10;
    uint32_t sum = 0;

    // Take multiple readings and average them for better accuracy
    for (int i = 0; i < NUM_SAMPLES; i++) {
        uint16_t reading = VCNL36828P_GET_PS_DATA(_sensorAddress);
        sum += reading;
        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay between readings
    }

    return static_cast<uint16_t>(sum / NUM_SAMPLES);
}

void SideTimeOfFlightSensor::apply_hardware_calibration(uint16_t baseline) {
    VCNL36828P_SET_PS_CANC(_sensorAddress, baseline);
}

uint16_t SideTimeOfFlightSensor::apply_calibration(uint16_t raw_reading) {
    if (!_isCalibrated) {
        return raw_reading; // Return raw reading if not calibrated
    }

    // If using hardware calibration, the reading is already calibrated
    if (_useHardwareCalibration) {
        return raw_reading;
    }

    // Apply software calibration (subtract baseline)
    int32_t calibrated_reading = static_cast<int32_t>(raw_reading) - static_cast<int32_t>(_baselineValue);
    calibrated_reading = std::max(calibratedReading, 0);

    return static_cast<uint16_t>(calibrated_reading);
}
