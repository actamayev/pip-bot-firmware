#include "multizone_tof_sensor.h"

bool MultizoneTofSensor::canRetryInitialization() const {
    if (isInitialized) return false;

    unsigned long currentTime = millis();
    if (currentTime - lastInitAttempt < INIT_RETRY_INTERVAL) {
        return false; // Too soon to retry
    }

    if (initRetryCount >= MAX_INIT_RETRIES) {
        return false; // Too many retries
    }

    return true;
}

bool MultizoneTofSensor::initialize() {
    lastInitAttempt = millis();
    initRetryCount++;
    
    // Add a delay before trying to initialize
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        if (sensor.begin() == 0) {
            // If begin successful, continue with initialization
            if (!sensor.init_sensor()) {
                // Configure the sensor
                if (configureSensor()) {
                    // Initialize point histories
                    initializePointHistories();
                    
                    SerialQueueManager::getInstance().queueMessage("TOF sensor initialization complete");
                    isInitialized = true;
                    return true;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));  // Longer delay between attempts
    }
    
    SerialQueueManager::getInstance().queueMessage("TOF sensor initialization failed");
    // scanI2C();  // Scan I2C bus to help debug
    return false;
}

bool MultizoneTofSensor::shouldBePolling() const {
    if (!isInitialized) return false;
    
    ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
    return timeouts.shouldEnableTof();
}

void MultizoneTofSensor::updateSensorData() {
    if (!isInitialized) return;

    // Check if we should enable/disable the sensor based on timeouts
    ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
    bool shouldEnable = timeouts.shouldEnableTof();
    
    if (shouldEnable && !sensorEnabled) {
        enableTofSensor();
    } else if (!shouldEnable && sensorEnabled) {
        disableTofSensor();
        return; // Don't try to read data if sensor is disabled
    }
    
    if (!sensorEnabled) return; // Skip if sensor not enabled
    
    // Check watchdog first
    if (!checkWatchdog() && sensorActive) {
        resetSensor();
        return;
    }
    
    uint8_t isDataReady = 0;
    
    // Check if new data is ready
    if (sensor.vl53l7cx_check_data_ready(&isDataReady) != 0 || isDataReady == 0) {
        return;
    }
    
    // Get the ranging data
    VL53L7CX_ResultsData rawData;
    if (sensor.vl53l7cx_get_ranging_data(&rawData) != 0) {
        return; // Failed to get data
    }
    
    // Update watchdog timer on successful data reception
    lastValidDataTime = millis();
    
    // Process obstacle detection with the raw data
    bool obstacleDetected = processObstacleDetection(rawData);
    
    // Create TofData structure and write to buffer
    TofData tofData;
    tofData.rawData = rawData;
    tofData.isObjectDetected = obstacleDetected;
    tofData.isValid = true;
    tofData.timestamp = millis();
    
    // Write to buffer
    SensorDataBuffer::getInstance().updateTofData(tofData);
}

void MultizoneTofSensor::enableTofSensor() {
    if (!isInitialized || sensorEnabled) return;
    
    startRanging();
    sensorActive = true;
    sensorEnabled = true;
    lastValidDataTime = millis();
    
    SerialQueueManager::getInstance().queueMessage("TOF sensor enabled");
}

void MultizoneTofSensor::disableTofSensor() {
    if (!sensorEnabled) return;
    
    stopRanging();
    sensorActive = false;
    sensorEnabled = false;
    
    SerialQueueManager::getInstance().queueMessage("TOF sensor disabled due to timeout");
}

bool MultizoneTofSensor::processObstacleDetection(const VL53L7CX_ResultsData& rawData) {
    // First update all point histories with current readings
    for (int rowIdx = 0; rowIdx < ROI_ROWS; rowIdx++) {
        int row = rowIdx + 3;  // Convert to physical row (3-4)
        
        for (int colIdx = 0; colIdx < ROI_COLS; colIdx++) {
            int col = colIdx + 1;  // Convert to physical column (1-6)
            
            // Calculate the actual index in the sensor data array
            int index = row * 8 + col;
            
            // Check if we have valid data for this point
            if (rawData.nb_target_detected[index] > 0) {
                uint16_t distance = rawData.distance_mm[index];
                uint8_t status = rawData.target_status[index];
                
                // Apply filtering parameters
                if (distance <= MAX_DISTANCE && 
                    distance >= MIN_DISTANCE && 
                    status >= SIGNAL_THRESHOLD) {
                    
                    // Update the history for this valid point
                    updatePointHistory(rowIdx, colIdx, (float)distance);
                } else {
                    // For invalid readings, update with -1 (not usable)
                    updatePointHistory(rowIdx, colIdx, -1.0f);
                }
            } else {
                // No reading for this point, update with -1 (not usable)
                updatePointHistory(rowIdx, colIdx, -1.0f);
            }
        }
    }
    
    // Now check each point to see if any has consistently detected an obstacle
    for (int rowIdx = 0; rowIdx < ROI_ROWS; rowIdx++) {
        for (int colIdx = 0; colIdx < ROI_COLS; colIdx++) {
            if (isPointObstacleConsistent(rowIdx, colIdx)) {
                return true;  // Found a point with consistent obstacle detection
            }
        }
    }
    
    return false; // No object detected
}

// Initialize the point histories
void MultizoneTofSensor::initializePointHistories() {
    for (int r = 0; r < ROI_ROWS; r++) {
        for (int c = 0; c < ROI_COLS; c++) {
            pointHistories[r][c].index = 0;
            pointHistories[r][c].validReadings = 0;
            
            // Initialize all distance values to 0
            for (int i = 0; i < HISTORY_SIZE; i++) {
                pointHistories[r][c].distances[i] = 0;
            }
        }
    }
}

// Update the history for a specific point
void MultizoneTofSensor::updatePointHistory(int rowIdx, int colIdx, float distance) {
    // Update the history for this point
    pointHistories[rowIdx][colIdx].distances[pointHistories[rowIdx][colIdx].index] = distance;
    pointHistories[rowIdx][colIdx].index = (pointHistories[rowIdx][colIdx].index + 1) % HISTORY_SIZE;
    
    // Increment valid readings counter (up to HISTORY_SIZE)
    if (pointHistories[rowIdx][colIdx].validReadings < HISTORY_SIZE) {
        pointHistories[rowIdx][colIdx].validReadings++;
    }
}

// Check if a point has consistently detected an obstacle
bool MultizoneTofSensor::isPointObstacleConsistent(int rowIdx, int colIdx) {
    // If we don't have enough readings yet, return false
    if (pointHistories[rowIdx][colIdx].validReadings < HISTORY_SIZE) {
        return false;
    }
    
    // Check if all readings in the history are below the threshold
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (pointHistories[rowIdx][colIdx].distances[i] >= OBSTACLE_DISTANCE_THRESHOLD || 
            pointHistories[rowIdx][colIdx].distances[i] <= 0) { // Skip invalid readings
            return false;
        }
    }
    
    // All readings are valid and below threshold
    return true;
}

bool MultizoneTofSensor::configureSensor() {
    // Configure sensor settings from configuration constants
    sensor.vl53l7cx_set_resolution(TOF_RESOLUTION); // Use 8x8 resolution
    
    sensor.vl53l7cx_set_ranging_frequency_hz(RANGING_FREQUENCY);
    // Set target order to closest (better for obstacle avoidance)
    sensor.vl53l7cx_set_target_order(VL53L7CX_TARGET_ORDER_CLOSEST);
    
    // Apply the optimized filtering parameters
    sensor.vl53l7cx_set_xtalk_margin(X_TALK_MARGIN);
    sensor.vl53l7cx_set_sharpener_percent(SHARPENER_PERCENT);
    sensor.vl53l7cx_set_integration_time_ms(INTEGRATION_TIME_MS);
    sensor.vl53l7cx_set_ranging_mode(VL53L7CX_RANGING_MODE_CONTINUOUS);

    return true;
}

bool MultizoneTofSensor::resetSensor() {
    SerialQueueManager::getInstance().queueMessage("MZ SENSOR RESET: Data stopped - performing recovery...");
    
    // Set sensor as inactive during reset
    sensorActive = false;
    
    // Stop ranging
    stopRanging();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Reset just the sensor without touching I2C bus initialization
    sensor.begin();
    
    if (sensor.init_sensor()) {
        SerialQueueManager::getInstance().queueMessage("Failed to reinitialize sensor!");
        return false;
    }
    
    // Reconfigure sensor
    configureSensor();
    
    // Reset history
    initializePointHistories();
    
    // Start ranging again
    startRanging();
    
    // Reset watchdog timer
    lastValidDataTime = millis();
    
    // Set sensor as active again
    sensorActive = true;
    
    SerialQueueManager::getInstance().queueMessage("Sensor reset complete");
    return true;
}

bool MultizoneTofSensor::checkWatchdog() {
    if (millis() - lastValidDataTime > WATCHDOG_TIMEOUT) {
        return false; // Watchdog timeout
    }
    return true; // Watchdog OK
}

void MultizoneTofSensor::startRanging() {
    sensor.vl53l7cx_start_ranging();
}

void MultizoneTofSensor::stopRanging() {
    sensor.vl53l7cx_stop_ranging();
}

void MultizoneTofSensor::turnOffSensor() {
    stopRanging();
    sensor.vl53l7cx_set_power_mode(VL53L7CX_POWER_MODE_SLEEP);
    sensorActive = false;
    sensorEnabled = false;
    isInitialized = false;
    initRetryCount = 0;
    lastInitAttempt = 0;
    
    initializePointHistories();
}
