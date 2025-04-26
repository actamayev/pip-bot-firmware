#include "./multizone_tof_sensor.h"

bool MultizoneTofSensor::initialize() {
    Serial.println("Initializing TOF sensor...");

    if (sensor.begin() != 0) {
        Serial.println("Failed to begin sensor communication");
        return false;
    }

    // Initialize the sensor (with no parameters as per the example)
    if (sensor.init_sensor()) {
        Serial.println(F("Sensor initialization failed"));
        return false;
    }
    
    // Configure sensor settings
    sensor.vl53l7cx_set_resolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);
    sensor.vl53l7cx_set_ranging_frequency_hz(TOF_RANGING_FREQUENCY);

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

void MultizoneTofSensor::startRanging() {
    Serial.println("Starting to range");
    sensor.vl53l7cx_start_ranging();
}

void MultizoneTofSensor::stopRanging() {
    sensor.vl53l7cx_stop_ranging();
}
