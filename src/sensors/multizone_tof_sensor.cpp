#include "multizone_tof_sensor.h"

bool MultizoneTofSensor::initialize() {
    // Add a delay before trying to initialize
    vTaskDelay(pdMS_TO_TICKS(50));

    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        if (sensor.begin() == 0) {
            // If begin successful, continue with initialization
            if (sensor.init_sensor() == 0) {
                // Configure the sensor
                if (configure_sensor()) {
                    // Initialize point histories
                    initialize_point_histories();

                    SerialQueueManager::get_instance().queue_message("MZ TOF sensor initialization complete");
                    _isInitialized = true;
                    return true;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Longer delay between attempts
    }

    SerialQueueManager::get_instance().queue_message("MZ TOF sensor initialization failed");
    // scan_i2_c();  // Scan I2C bus to help debug
    return false;
}

bool MultizoneTofSensor::should_be_polling() {
    if (!_isInitialized) {
        return false;
    }

    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    // Continue polling if we should be enabled OR if sensor is currently enabled
    // (to allow proper cleanup when timeout expires)
    return ReportTimeouts::should_enable_tof() || sensorEnabled;
}

void MultizoneTofSensor::update_sensor_data() {
    if (!_isInitialized) {
        return;
    }

    // Throttle VL53L7CX checks to 50Hz max (every 20ms) to reduce I2C load
    static uint32_t last_check_time = 0;
    uint32_t current_time = millis();

    if (current_time - last_check_time < CHECK_SENSOR_TIME) {
        return;
    }

    last_check_time = current_time;

    // Check if we should enable/disable the sensor based on timeouts
    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    bool should_enable = ReportTimeouts::should_enable_tof();

    if (should_enable && !_sensorEnabled) {
        enable_tof_sensor();
    } else if (!should_enable && sensorEnabled) {
        disable_tof_sensor();
        return; // Don't try to read data if sensor is disabled
    }

    if (!_sensorEnabled) {
        return; // Skip if sensor not enabled
    }

    // Check watchdog first
    if (!check_watchdog() && _sensorActive) {
        reset_sensor();
        return;
    }

    uint8_t is_data_ready = 0;

    // Check if new data is ready
    if (_sensor.vl53l7cx_check_data_ready(&is_data_ready) != 0 || is_data_ready == 0) {
        return;
    }

    // Get the ranging data
    VL53L7CX_ResultsData raw_data;
    if (_sensor.vl53l7cx_get_ranging_data(&raw_data) != 0) {
        return; // Failed to get data
    }

    // Update watchdog timer on successful data reception
    _lastValidDataTime = millis();

    // Process obstacle detection with the raw data
    bool obstacle_detected = process_obstacle_detection(raw_data);

    // Calculate front distance from ROI zones
    float front_distance = calculate_front_distance(raw_data);

    // Create TofData structure and write to buffer
    TofData tof_data;
    tof_data.raw_data = raw_data;
    tof_data.is_object_detected = obstacle_detected;
    tof_data.front_distance = front_distance;
    tof_data.is_valid = true;
    tof_data.timestamp = millis();

    // Write to buffer
    SensorDataBuffer::get_instance().update_tof_data(tof_data);
}

void MultizoneTofSensor::enable_tof_sensor() {
    if (!isInitialized || sensorEnabled) {
        return;
    }

    start_ranging();
    sensorActive = true;
    sensorEnabled = true;
    lastValidDataTime = millis();

    SerialQueueManager::get_instance().queue_message("MZ TOF sensor enabled");
}

void MultizoneTofSensor::disable_tof_sensor() {
    if (!sensorEnabled) {
        return;
    }

    stop_ranging();
    sensorActive = false;
    sensorEnabled = false;

    SerialQueueManager::get_instance().queue_message("MZ TOF sensor disabled due to timeout");
}

bool MultizoneTofSensor::process_obstacle_detection(const VL53L7CX_ResultsData& raw_data) const {
    // First update all point histories with current readings
    for (int row_index = 0; row_index < ROI_ROWS; row_index++) {
        int row = row_index + 3; // Convert to physical row (3-4)

        for (int col_index = 0; col_index < ROI_COLS; col_index++) {
            int col = col_index + 1; // Convert to physical column (1-6)

            // Calculate the actual index in the sensor data array
            int index = (row * 8) + col;

            // Check if we have valid data for this point
            if (raw_data.nb_target_detected[index] > 0) {
                uint16_t distance = raw_data.distance_mm[index];
                uint8_t status = raw_data.target_status[index];

                // Apply filtering parameters
                if (distance <= MAX_DISTANCE && distance >= MIN_DISTANCE && status >= SIGNAL_THRESHOLD) {
                    // Update the history for this valid point
                    update_point_history(row_index, col_index, static_cast<float>(distance));
                } else {
                    // For invalid readings, update with -1 (not usable)
                    update_point_history(row_index, col_index, -1.0f);
                }
            } else {
                // No reading for this point, update with -1 (not usable)
                update_point_history(row_index, col_index, -1.0f);
            }
        }
    }

    // Now check each point to see if any has consistently detected an obstacle
    for (int row_index = 0; row_index < ROI_ROWS; row_index++) {
        for (int col_index = 0; col_index < ROI_COLS; col_index++) {
            if (is_point_obstacle_consistent(row_index, col_index)) {
                return true; // Found a point with consistent obstacle detection
            }
        }
    }

    return false; // No object detected
}

// Initialize the point histories
void MultizoneTofSensor::initialize_point_histories() {
    for (auto& pointHistorie : pointHistories) {
        for (int c = 0; c < ROI_COLS; c++) {
            pointHistorie[c].index = 0;
            pointHistorie[c].validReadings = 0;

            // Initialize all distance values to 0
            for (int i = 0; i < HISTORY_SIZE; i++) {
                pointHistorie[c].distances[i] = 0;
            }
        }
    }
}

// Update the history for a specific point
void MultizoneTofSensor::update_point_history(int row_index, int col_index, float distance) {
    // Update the history for this point
    pointHistories[row_index][col_index].distances[pointHistories[row_index][col_index].index] = distance;
    pointHistories[row_index][col_index].index = (pointHistories[row_index][col_index].index + 1) % HISTORY_SIZE;

    // Increment valid readings counter (up to HISTORY_SIZE)
    if (pointHistories[row_index][col_index].validReadings < HISTORY_SIZE) {
        pointHistories[row_index][col_index].validReadings++;
    }
}

// Check if a point has consistently detected an obstacle
bool MultizoneTofSensor::is_point_obstacle_consistent(int row_index, int col_index) {
    // If we don't have enough readings yet, return false
    if (pointHistories[row_index][col_index].validReadings < HISTORY_SIZE) {
        return false;
    }

    // Check if all readings in the history are below the threshold
    for (float distance : pointHistories[row_index][col_index].distances) {
        if (distance >= OBSTACLE_DISTANCE_THRESHOLD || distance <= 0) { // Skip invalid readings
            return false;
        }
    }

    // All readings are valid and below threshold
    return true;
}

bool MultizoneTofSensor::configure_sensor() {
    // Configure sensor settings from configuration constants
    sensor.vl53l7cx_set_resolution(TOF_RESOLUTION); // Use 8x8 resolution

    sensor.vl53l7cx_set_ranging_frequency_hz(RANGING_FREQUENCY);
    // Set target order to closest (better for obstacle avoidance)
    // sensor.vl53l7cx_set_target_order(VL53L7CX_TARGET_ORDER_CLOSEST);

    // Apply the optimized filtering parameters
    sensor.vl53l7cx_set_xtalk_margin(X_TALK_MARGIN);
    sensor.vl53l7cx_set_sharpener_percent(SHARPENER_PERCENT);
    sensor.vl53l7cx_set_integration_time_ms(INTEGRATION_TIME_MS);
    sensor.vl53l7cx_enable_non_blocking_mode();

    return true;
}

bool MultizoneTofSensor::reset_sensor() {
    SerialQueueManager::get_instance().queue_message("MZ SENSOR RESET: Data stopped - performing recovery...");

    // Set sensor as inactive during reset
    sensorActive = false;

    // Stop ranging
    stop_ranging();
    vTaskDelay(pdMS_TO_TICKS(100));

    // Reset just the sensor without touching I2C bus initialization
    sensor.begin();

    if (sensor.init_sensor() != 0) {
        SerialQueueManager::get_instance().queue_message("Failed to reinitialize sensor!");
        return false;
    }

    // Reconfigure sensor
    configure_sensor();

    // Reset history
    initialize_point_histories();

    // Start ranging again
    start_ranging();

    // Reset watchdog timer
    lastValidDataTime = millis();

    // Set sensor as active again
    sensorActive = true;

    SerialQueueManager::get_instance().queue_message("Sensor reset complete");
    return true;
}

bool MultizoneTofSensor::check_watchdog() {
    if (millis() - _lastValidDataTime > _WATCHDOG_TIMEOUT) {
        return false; // Watchdog timeout
    }
    return true; // Watchdog OK
}

void MultizoneTofSensor::start_ranging() {
    _sensor.vl53l7cx_start_ranging();
}

void MultizoneTofSensor::stop_ranging() {
    _sensor.vl53l7cx_stop_ranging();
}

void MultizoneTofSensor::turn_off_sensor() {
    stop_ranging();
    sensor.vl53l7cx_set_power_mode(VL53L7CX_POWER_MODE_SLEEP);
    _sensorActive = false;
    _sensorEnabled = false;
    _isInitialized = false;

    initialize_point_histories();
}

float MultizoneTofSensor::calculate_front_distance(const VL53L7CX_ResultsData& raw_data) {
    float min_distance = 9999.0f; // Start with very large value
    bool found_valid_reading = false;

    // Scan through the front-center zones (row 5, columns 3-4)
    int row = 5; // Only use row 5 (close to the top)

    for (int col_index = 0; col_index < 2; col_index++) { // 2 columns (3-4)
        int col = col_index + 3;                          // Convert to physical column (3-4)

        // Calculate the actual index in the sensor data array
        int index = (row * 8) + col;

        // Check if we have valid data for this point
        if (raw_data.nb_target_detected[index] > 0) {
            uint16_t distance = raw_data.distance_mm[index];
            uint8_t status = raw_data.target_status[index];

            // Apply same filtering as obstacle detection
            if (distance <= MAX_DISTANCE && distance >= MIN_DISTANCE && status >= SIGNAL_THRESHOLD) {
                if (distance < min_distance) {
                    min_distance = distance;
                    found_valid_reading = true;
                }
            }
        }
    }

    return found_valid_reading ? (min_distance / 25.4f) : -1.0f; // Convert mm to inches, return -1 if no valid readings
}
