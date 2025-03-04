#include "./include/multizone_tof_sensor.h"

bool MultizoneTofSensor::initialize() {
    Serial.println("Initializing TOF sensor...");

    sensor.setResolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);
    sensor.setRangingFrequency(TOF_RANGING_FREQUENCY);

    // Start ranging
    startRanging();
    
    Serial.println("TOF sensor initialization complete");
    return true;
}

bool MultizoneTofSensor::getTofData() {
    if (sensor.isDataReady()) {
        return sensor.getRangingData(&sensorData);
    }
    return false;
}

void MultizoneTofSensor::startRanging() {
    Serial.println("starting to range");
    sensor.startRanging();
}

void MultizoneTofSensor::stopRanging() {
    sensor.stopRanging();
}
