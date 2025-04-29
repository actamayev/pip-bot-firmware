#pragma once
#include <vl53l7cx_class.h>
#include <Wire.h>
#include "../utils/config.h"

class MultizoneTofSensor {
    public:
        // Constructor that takes TwoWire instance and optional pin parameters
        MultizoneTofSensor(TwoWire *wire = &Wire) 
            : sensor(wire, -1, -1) {}

        bool initialize();
        VL53L7CX_ResultsData getTofData();

        void startRanging();
        void stopRanging();

    private:
        VL53L7CX sensor; // Now properly initialized through the constructor
        void measureDistance();
        VL53L7CX_ResultsData sensorData;
};
