#pragma once
#include <Wire.h>
#include <Arduino.h>
#include <veml3328.h>
#include "./structs.h"

class ColorSensor {
    public:
        ColorSensor() = default;

        bool initialize();
        ColorSensorData getSensorData();
        bool isSensorConnected() { return sensorConnected; }

    private:
        void read_color_sensor();
        bool sensorConnected = false;  // Add this flag

        bool isCalibrated = true;
        const float scaleFactor = 128.0;

        // TODO: Give default values
        CalibrationValues calibrationValues = {
            // RED LED readings (R, G, B)
            195 * scaleFactor, 42 * scaleFactor, 5 * scaleFactor,
            
            // GREEN LED readings (R, G, B)
            29 * scaleFactor, 371 * scaleFactor, 146 * scaleFactor,
            
            // BLUE LED readings (R, G, B)
            26 * scaleFactor, 103 * scaleFactor, 511 * scaleFactor
        };
        ColorSensorData colorSensorData;

        unsigned long lastUpdateTime;

        void precompute_inverse_matrix();

        float invMatrix[3][3]; // Store pre-computed inverse matrix
};
