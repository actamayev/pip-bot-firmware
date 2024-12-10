#pragma once
#include <SparkFun_VL53L5CX_Library.h>
#include "./config.h"

class TimeOfFlightSensor {
    public:
        TimeOfFlightSensor() = default;

        bool initialize();
        bool getData();

        const VL53L5CX_ResultsData& getSensorData() const;

        void startRanging();
        void stopRanging();
        SparkFun_VL53L5CX sensor;

    private:
        VL53L5CX_ResultsData sensorData;
};
