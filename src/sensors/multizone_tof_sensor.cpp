#include "./multizone_tof_sensor.h"

bool MultizoneTofSensor::initialize() {
    Serial.println("Initializing TOF sensor...");

    if (sensor.begin() == false) {
      Serial.println(F("Sensor not found - check your wiring"));
      return false;
    }
    sensor.setResolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);
    sensor.setRangingFrequency(TOF_RANGING_FREQUENCY);

    // Start ranging
    startRanging();
    
    Serial.println("TOF sensor initialization complete");
    return true;
}

void MultizoneTofSensor::measureDistance() {
    if (!sensor.isDataReady()) return;
    sensor.getRangingData(&sensorData);
}

VL53L5CX_ResultsData MultizoneTofSensor::getTofData() {
    measureDistance();
    return sensorData;
}

void MultizoneTofSensor::startRanging() {
    Serial.println("starting to range");
    sensor.startRanging();
}

void MultizoneTofSensor::stopRanging() {
    sensor.stopRanging();
}
