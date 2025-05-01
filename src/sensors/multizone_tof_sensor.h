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
        float getAverageDistanceCenterline();

    private:
        VL53L7CX sensor;
        void measureDistance();
        VL53L7CX_ResultsData sensorData;
        
        // Configuration parameters
        uint16_t maxDistance = 1250;      // Maximum valid distance (mm)
        uint16_t minDistance = 30;      // Minimum valid distance (mm)
        uint8_t signalThreshold = 5;   // Minimum signal quality threshold
        uint16_t xtalkMargin = 100;      // Xtalk margin for noise filtering
        uint8_t sharpenerPercent = 20;  // Sharpener percentage (0-99)
        uint32_t integrationTimeMs = 20; // Integration time in milliseconds
};
