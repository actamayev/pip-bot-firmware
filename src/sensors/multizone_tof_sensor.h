#pragma once
#include <SparkFun_VL53L5CX_Library.h>
#include "../utils/config.h"

class MultizoneTofSensor {
    public:
        MultizoneTofSensor() = default;

        bool initialize();
        VL53L5CX_ResultsData getTofData();

        void startRanging();
        void stopRanging();

    private:
        SparkFun_VL53L5CX sensor;
        void measureDistance();
        VL53L5CX_ResultsData sensorData;
};
