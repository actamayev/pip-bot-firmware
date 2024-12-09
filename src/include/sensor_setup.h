#pragma once

#include <Wire.h>
#include <Adafruit_VL53L1X.h>

class SensorSetup {
    public:
        void sensor_setup();

    private:
        bool setupTofSensors();  
};

extern SensorSetup sensorSetup;
