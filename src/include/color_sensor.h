#pragma once
#include <Wire.h>
#include <Arduino.h>
#include <veml3328.h>
#include "./structs.h"

class ColorSensor {
    public:
        ColorSensor();
        ColorSensorData getSensorData();
        bool isSensorConnected() { return sensorConnected; }

    private:
        void read_color_sensor();
        bool sensorConnected = false;  // Add this flag

        bool isCalibrated = true;
        const float scaleFactor = 128.0;

        // TODO: Give default values
        CalibrationValues calibration;
        ColorSensorData colorSensorData;

        unsigned long lastUpdateTime;
};

extern ColorSensor colorSensor;
