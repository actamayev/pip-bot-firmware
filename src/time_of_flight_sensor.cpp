#include "./include/time_of_flight_sensor.h"

bool TimeOfFlightSensor::initialize() {
    // Configure sensor
    sensor.setResolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);
    sensor.setRangingFrequency(TOF_RANGING_FREQUENCY);
    
    return true;
}

bool TimeOfFlightSensor::getData() {
    startRanging();
    if (sensor.isDataReady()) {
        if (sensor.getRangingData(&sensorData)) {
            // Data retrieved successfully
            return true;
        }
    }
    return false;
}

void TimeOfFlightSensor::startRanging() {
    sensor.startRanging();
}

void TimeOfFlightSensor::stopRanging() {
    sensor.stopRanging();
}
