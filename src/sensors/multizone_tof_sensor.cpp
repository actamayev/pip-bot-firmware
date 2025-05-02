#include "./multizone_tof_sensor.h"

bool MultizoneTofSensor::initialize() {
    Serial.println("Initializing TOF sensor...");

    if (sensor.begin() != 0) {
        Serial.println("Failed to begin sensor communication");
        return false;
    }

    // Initialize the sensor
    if (sensor.init_sensor()) {
        Serial.println(F("Sensor initialization failed"));
        return false;
    }
    
    // Configure sensor settings from configuration constants
    sensor.vl53l7cx_set_resolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);
    sensor.vl53l7cx_set_ranging_frequency_hz(TOF_RANGING_FREQUENCY);
    
    // Apply the optimized filtering parameters
    sensor.vl53l7cx_set_xtalk_margin(xtalkMargin);
    sensor.vl53l7cx_set_sharpener_percent(sharpenerPercent);
    sensor.vl53l7cx_set_integration_time_ms(integrationTimeMs);

    // Start ranging
    startRanging();

    Serial.println("TOF sensor initialization complete");
    return true;
}

void MultizoneTofSensor::measureDistance() {
    uint8_t isDataReady = 0;
    
    // Check if new data is ready (passing the required parameter)
    if (sensor.vl53l7cx_check_data_ready(&isDataReady) != 0 || isDataReady == 0) {
        return;
    }
    
    // Get the ranging data
    sensor.vl53l7cx_get_ranging_data(&sensorData);
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

void MultizoneTofSensor::startRanging() {
    Serial.println("Starting to range");
    sensor.vl53l7cx_start_ranging();
}

void MultizoneTofSensor::stopRanging() {
    sensor.vl53l7cx_stop_ranging();
}
