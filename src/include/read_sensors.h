#pragma once

#include <Adafruit_VL53L1X.h>

struct SensorReading {
    int16_t distance;
    bool valid;
};

class ReadSensors {
    public:
        void read_digital_ir();
        void read_analog_ir();
        SensorReading readSensor(Adafruit_VL53L1X& sensor, const char* sensorName);
};

extern ReadSensors readSensors;
