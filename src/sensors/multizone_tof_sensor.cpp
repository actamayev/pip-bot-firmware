#include "./multizone_tof_sensor.h"

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
    
    Serial.printf("Initializing TOF sensor (attempt %d of %d)...\n", 
                 initRetryCount, MAX_INIT_RETRIES);
    
    // Add a delay before trying to initialize
    delay(50);
    
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
        delay(50);  // Longer delay between attempts
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
    
    return true;
}

bool MultizoneTofSensor::resetSensor() {
    Serial.println("SENSOR RESET: Data stopped - performing recovery...");
    
    // Set sensor as inactive during reset
    sensorActive = false;
    
    // Stop ranging
    stopRanging();
    delay(100);
    
    // Reset just the sensor without touching I2C bus initialization
    sensor.begin();
    
    if (sensor.init_sensor()) {
        Serial.println("Failed to reinitialize sensor!");
        return false;
    }
    
    // Reconfigure sensor
    configureSensor();
    
    // Reset history
    resetHistory();
    
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

float MultizoneTofSensor::getAverageDistanceCenterline() {
    VL53L7CX_ResultsData tofData = getTofData();
    
    int validPointCount = 0;
    float totalDistance = 0;
    
    // Process rows 4 and 3 (4th and 5th rows from the top in the displayed grid)
    for (int row = 4; row >= 3; row--) {
        for (int col = 7; col >= 0; col--) {
            int index = row * 8 + col;
            
            // Check if we have valid data for this point
            if (tofData.nb_target_detected[index] > 0) {
                uint16_t distance = tofData.distance_mm[index];
                uint8_t status = tofData.target_status[index];
                
                // Apply filtering parameters
                if (distance <= maxDistance && 
                    distance >= minDistance && 
                    status >= signalThreshold) {
                    totalDistance += distance;
                    validPointCount++;
                }
            }
        }
    }
    
    // Return the average if we have valid points, otherwise return -1
    if (validPointCount > 0) {
        return totalDistance / static_cast<float>(validPointCount);
    } else {
        return -1.0f; // Indicate no valid points
    }
}

float MultizoneTofSensor::getWeightedAverageDistance() {
    // Get the latest sensor data
    VL53L7CX_ResultsData tofData = sensorData;
    
    int validPointCount = 0;
    float totalWeightedDistance = 0;
    float totalWeight = 0;
    
    // Process rows 2 through 5 (expanded detection area)
    for (int row = 5; row >= 2; row--) {
        // Determine row weight - center rows (3-4) get full weight, outer rows (2,5) get reduced weight
        float rowWeight = (row == 3 || row == 4) ? 1.0 : 0.6;
        
        for (int col = 7; col >= 0; col--) {
            int index = row * 8 + col;
            
            // Determine column weight - center columns get higher weight
            float colWeight;
            if (col >= 3 && col <= 4) {
                colWeight = 1.0;  // Center columns
            } else if (col == 2 || col == 5) {
                colWeight = 0.8;  // Near-center columns
            } else {
                colWeight = 0.5;  // Edge columns
            }
            
            // Combine row and column weights
            float pointWeight = rowWeight * colWeight;
            
            // Check if we have valid data for this point
            if (tofData.nb_target_detected[index] > 0) {
                uint16_t distance = tofData.distance_mm[index];
                uint8_t status = tofData.target_status[index];
                
                // Target status filtering - prioritize readings with high confidence
                // Status 5 = 100% valid, Status 6/9 = ~50% valid, others < 50% valid
                float statusMultiplier = 0.5;  // Default multiplier for lower confidence readings
                if (status == TARGET_STATUS_VALID) {
                    statusMultiplier = 1.0;  // Full confidence
                } else if (status == TARGET_STATUS_VALID_LARGE_PULSE || 
                          status == TARGET_STATUS_VALID_WRAPPED) {
                    statusMultiplier = 0.8;  // Medium-high confidence
                }
                
                // Apply confidence multiplier to the point weight
                pointWeight *= statusMultiplier;
                
                // Apply filtering parameters
                if (distance <= maxDistance && 
                    distance >= minDistance && 
                    status >= signalThreshold) {
                    totalWeightedDistance += distance * pointWeight;
                    totalWeight += pointWeight;
                    validPointCount++;
                }
            }
        }
    }
    
    // Return the weighted average if we have enough valid points, otherwise return -1
    if (validPointCount >= minValidPoints && totalWeight > 0) {
        return totalWeightedDistance / totalWeight;
    } else {
        return -1.0f; // Indicate insufficient valid points
    }
}

// Modified isObjectDetected function to use point histories
bool MultizoneTofSensor::isObjectDetected() {
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
    
    // Also still calculate the weighted average for the approaching object logic
    float weightedDistance = getWeightedAverageDistance();
    
    // If we have a valid weighted distance, update the centerline history
    if (weightedDistance > 0) {
        previousCenterlineDistances[historyIndex] = weightedDistance;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    }
    
    // Now check each point to see if any has consistently detected an obstacle
    for (int rowIdx = 0; rowIdx < ROI_ROWS; rowIdx++) {
        for (int colIdx = 0; colIdx < ROI_COLS; colIdx++) {
            if (isPointObstacleConsistent(rowIdx, colIdx)) {
                return true;  // Found a point with consistent obstacle detection
            }
        }
    }
    
    // Still keep the approaching object logic as a backup
    if (weightedDistance > 0) {
        // Check for approaching objects by analyzing history
        float sumDistanceChange = 0;
        int validComparisons = 0;
        
        // Calculate average change in distance over time
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            int nextIdx = (i + 1) % HISTORY_SIZE;
            // Only consider valid readings (non-zero)
            if (previousCenterlineDistances[i] > 0 && previousCenterlineDistances[nextIdx] > 0) {
                sumDistanceChange += (previousCenterlineDistances[i] - previousCenterlineDistances[nextIdx]);
                validComparisons++;
            }
        }
        
        // If we have enough valid comparisons and the average change shows approaching object
        if (validComparisons >= 2 && (sumDistanceChange / validComparisons) > approachingThreshold) {
            return true; // Object is getting closer at significant rate
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

void MultizoneTofSensor::resetHistory() {
    // Reset history array for weighted average
    for (int i = 0; i < HISTORY_SIZE; i++) {
        previousCenterlineDistances[i] = 0;
    }
    historyIndex = 0;
    
    // Reset point histories
    initializePointHistories();
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
