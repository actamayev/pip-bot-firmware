#pragma once
#include <SparkFun_VL53L5CX_Library.h>
#include "./config.h"

class MultizoneTofSensor {
    public:
        MultizoneTofSensor() = default;

        bool initialize();
        bool getTofData();

        VL53L5CX_ResultsData sensorData;

        void startRanging();
        void stopRanging();
        SparkFun_VL53L5CX sensor;
};
