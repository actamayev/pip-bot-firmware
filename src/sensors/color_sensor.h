#pragma once
#include "../lib/veml3328/veml3328.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "sensor_data_buffer.h"
#include "networking/serial_queue_manager.h"

class ColorSensor : public Singleton<ColorSensor> {
    friend class Singleton<ColorSensor>;
    friend class TaskManager;
    friend class SensorInitializer;
    
    private:
        ColorSensor() = default;
        bool initialize();
        void read_color_sensor();
        void calibrateBlackPoint();
        void calibrateWhitePoint();
        void printCalibrationValues();
        void enableColorSensor();
        void disableColorSensor();
        
        bool isInitialized = false;
        bool sensorConnected = false;
        bool sensorEnabled = false;  // Track if sensor is actively enabled

        struct CalibrationValues {
            uint16_t blackRed;
            uint16_t blackGreen;
            uint16_t blackBlue;
            uint16_t whiteRed;
            uint16_t whiteGreen;
            uint16_t whiteBlue;
        } calibration;
        
        bool isCalibrated = false;
        ColorSensorData colorSensorData;
        unsigned long lastUpdateTime = 0;
        static constexpr unsigned long DELAY_BETWEEN_READINGS = 20; // ms - minimal delay like performance test

        // New buffer-based methods following the established pattern
        void updateSensorData();  // Single read, write to buffer
        bool shouldBePolling() const;
        const uint8_t COLOR_SENSOR_LED_PIN = 5;
};
