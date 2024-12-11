#include "./include/time_of_flight_sensor.h"

bool TimeOfFlightSensor::initialize() {
    Serial.println("Initializing TOF sensor...");
    
    // First initialize the sensor
    if (!sensor.begin()) {
        Serial.println("Sensor not found - check wiring.");
        return false;
    }

    Serial.println("Sensor found, configuring...");

    // Configure sensor resolution
    if (!sensor.setResolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION)) { // 8x8 = 64 pixels
        Serial.println("Failed to set resolution");
        return false;
    }

    // Start ranging
    startRanging();
    
    Serial.println("TOF sensor initialization complete");
    return true;
}

bool TimeOfFlightSensor::getData() {
    if (sensor.isDataReady()) {
        return sensor.getRangingData(&sensorData);
    }
    return false;
}

void TimeOfFlightSensor::startRanging() {
    Serial.println("starting to range");
    sensor.startRanging();
}

void TimeOfFlightSensor::stopRanging() {
    sensor.stopRanging();
}
