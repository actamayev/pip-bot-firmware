#pragma once

#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include <SparkFun_VL53L5CX_Library.h>
#include "./singleton.h"

class SensorSetup : public Singleton<SensorSetup> {
    friend class Singleton<SensorSetup>;

    private:
        SensorSetup();
        void sensor_setup();
        void i2c_setup();
        void setup_tof_sensors();
};
