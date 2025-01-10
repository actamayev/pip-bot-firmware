#include "./include/time_of_flight_sensor.h"

bool TimeOfFlightSensor::initialize() {
    Serial.println("Initializing TOF sensor...");

    sensor.setResolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);
    sensor.setRangingFrequency(TOF_RANGING_FREQUENCY);

    // Start ranging
    startRanging();
    
    Serial.println("TOF sensor initialization complete");
    return true;
}

bool TimeOfFlightSensor::getTofData() {
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
