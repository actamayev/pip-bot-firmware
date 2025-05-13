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
    
    Serial.printf("Initializing MZ-TOF sensor (attempt %d of %d)...\n", 
                 initRetryCount, MAX_INIT_RETRIES);
    
    // Add a delay before trying to initialize
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        if (sensor.begin() == 0) {
            // If begin successful, continue with initialization
            if (!sensor.init_sensor()) {
                // Configure the sensor
                if (configureSensor()) {
                    // Start ranging
                    startRanging();
                    
                    // Set sensor as active
                    sensorActive = true;
                    
                    // Initialize watchdog timer
                    lastValidDataTime = millis();
                    
                    // Initialize point histories
                    initializePointHistories();
                    
                    Serial.println("TOF sensor initialization complete");
                    isInitialized = true;
                    return true;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));  // Longer delay between attempts
    }
    
    Serial.printf("TOF sensor initialization failed (retry %d of %d)\n", 
                 initRetryCount, MAX_INIT_RETRIES);
    scanI2C();  // Scan I2C bus to help debug
    return false;
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
    Serial.println("Point histories initialized");
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
        if (pointHistories[rowIdx][colIdx].distances[i] >= obstacleDistanceThreshold || 
            pointHistories[rowIdx][colIdx].distances[i] <= 0) { // Skip invalid readings
            return false;
        }
    }
    
    // All readings are valid and below threshold
    return true;
}

bool MultizoneTofSensor::configureSensor() {
    // Configure sensor settings from configuration constants
    sensor.vl53l7cx_set_resolution(tofResolution); // Use 8x8 resolution
    
    sensor.vl53l7cx_set_ranging_frequency_hz(rangingFrequency);
    // Set target order to closest (better for obstacle avoidance)
    sensor.vl53l7cx_set_target_order(VL53L7CX_TARGET_ORDER_CLOSEST);
    
    // Apply the optimized filtering parameters
    sensor.vl53l7cx_set_xtalk_margin(xtalkMargin);
    sensor.vl53l7cx_set_sharpener_percent(sharpenerPercent);
    sensor.vl53l7cx_set_integration_time_ms(integrationTimeMs);
    sensor.vl53l7cx_set_ranging_mode(VL53L7CX_RANGING_MODE_CONTINUOUS);

    Serial.println("Sensor configured with optimized parameters");
    return true;
}

bool MultizoneTofSensor::resetSensor() {
    Serial.println("SENSOR RESET: Data stopped - performing recovery...");
    
    // Set sensor as inactive during reset
    sensorActive = false;
    
    // Stop ranging
    stopRanging();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Reset just the sensor without touching I2C bus initialization
    sensor.begin();
    
    if (sensor.init_sensor()) {
        Serial.println("Failed to reinitialize sensor!");
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
    
    Serial.println("Sensor reset complete");
    return true;
}

void MultizoneTofSensor::measureDistance() {
    uint8_t isDataReady = 0;
    
    // Check if watchdog has timed out first, if so reset the sensor
    if (!checkWatchdog() && sensorActive) {
        resetSensor();
    }
    
    // Check if new data is ready (passing the required parameter)
    if (sensor.vl53l7cx_check_data_ready(&isDataReady) != 0 || isDataReady == 0) {
        return;
    }
    
    // Get the ranging data
    sensor.vl53l7cx_get_ranging_data(&sensorData);
    
    // Update watchdog timer on successful data reception
    lastValidDataTime = millis();
}

VL53L7CX_ResultsData MultizoneTofSensor::getTofData() {
    measureDistance();
    return sensorData;
}

// Modified isObjectDetected function to use point histories
bool MultizoneTofSensor::isObjectDetected() {
    measureDistance();
    // Get the latest sensor data
    VL53L7CX_ResultsData *Result = &sensorData;
    
    // First update all point histories with current readings
    for (int rowIdx = 0; rowIdx < ROI_ROWS; rowIdx++) {
        int row = rowIdx + 3;  // Convert to physical row (3-4)
        
        for (int colIdx = 0; colIdx < ROI_COLS; colIdx++) {
            int col = colIdx + 1;  // Convert to physical column (1-6)
            
            // Calculate the actual index in the sensor data array
            int index = row * 8 + col;
            
            // Check if we have valid data for this point
            if (Result->nb_target_detected[index] > 0) {
                uint16_t distance = Result->distance_mm[index];
                uint8_t status = Result->target_status[index];
                
                // Apply filtering parameters
                if (distance <= maxDistance && 
                    distance >= minDistance && 
                    status >= signalThreshold) {
                    
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

bool MultizoneTofSensor::checkWatchdog() {
    if (millis() - lastValidDataTime > watchdogTimeout) {
        return false; // Watchdog timeout
    }
    return true; // Watchdog OK
}

void MultizoneTofSensor::startRanging() {
    Serial.println("Starting to range");
    sensor.vl53l7cx_start_ranging();
}

void MultizoneTofSensor::stopRanging() {
    sensor.vl53l7cx_stop_ranging();
}

void MultizoneTofSensor::printResult(VL53L7CX_ResultsData *Result) {
    Serial.println("VL53L7CX 8x8 Grid Distance Measurement");
    Serial.println("--------------------------------------\n");
    
    // Print 8x8 grid
    // Traverse rows from bottom to top (7 to 0) to fix vertical flip
    for (int row = 7; row >= 0; row--) {
        // Print separator line
        for (int i = 0; i < 8; i++) {
        Serial.print(" --------");
        }
        Serial.println("-");
        
        // Print distance values
        Serial.print("|");
        // Traverse columns from right to left (7 to 0) to fix horizontal flip
        for (int col = 7; col >= 0; col--) {
        // Calculate proper index in the data array
        int index = row * 8 + col;
        
        if (Result->nb_target_detected[index] > 0) {
            // Apply distance thresholds and signal quality filtering
            uint16_t distance = Result->distance_mm[index];
            uint8_t status = Result->target_status[index];
            
            // Filter out readings below MIN_DISTANCE, above MAX_DISTANCE or with poor signal quality
            if (distance > maxDistance || distance < minDistance || status < signalThreshold) {
            Serial.print("    X   ");
            } else {
            Serial.printf(" %4d mm", distance);
            }
        } else {
            Serial.print("    X   ");
        }
        Serial.print("|");
        }
        Serial.println();
    }
    
    // Print final separator line
    for (int i = 0; i < 8; i++) {
        Serial.print(" --------");
    }
    Serial.println("-\n");
    
    // Print point histories status
    // printPointHistoriesStatus(Result);
}

void MultizoneTofSensor::turnOffSensor() {
    stopRanging();
    sensor.vl53l7cx_set_power_mode(VL53L7CX_POWER_MODE_SLEEP);
    sensorActive = false;
    isInitialized = false;
    initRetryCount = 0;
    lastInitAttempt = 0;
    
    initializePointHistories();
    
    Serial.println("MZ sensor turned off");
}
