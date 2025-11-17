#pragma once
#include "../lib/veml3328/veml3328.h"
#include "networking/serial_queue_manager.h"
#include "sensor_data_buffer.h"
#include "utils/singleton.h"
#include "utils/structs.h"

class ColorSensor : public Singleton<ColorSensor> {
    friend class Singleton<ColorSensor>;
    friend class TaskManager;
    friend class SensorInitializer;

  private:
    ColorSensor() = default;
    bool initialize();
    void read_color_sensor();
    void calibrate_black_point();
    void calibrate_white_point();
    void print_calibration_values();
    void enable_color_sensor();
    void disable_color_sensor();

    bool isInitialized = false;
    bool sensorConnected = false;
    bool sensorEnabled = false; // Track if sensor is actively enabled

    struct CalibrationValues {
        uint16_t blackRed = 3338;
        uint16_t blackGreen = 5868;
        uint16_t blackBlue = 2993;
        uint16_t whiteRed = 37333;
        uint16_t whiteGreen = 65535;
        uint16_t whiteBlue = 40437;
    } calibration;

    bool isCalibrated = true;
    ColorSensorData colorSensorData;
    uint32_t lastUpdateTime = 0;
    static constexpr uint32_t DELAY_BETWEEN_READINGS = 20; // ms - minimal delay like performance test

    // New buffer-based methods following the established pattern
    void update_sensor_data(); // Single read, write to buffer
    bool should_be_polling() const;
    const uint8_t COLOR_SENSOR_LED_PIN = 5;

    static constexpr uint8_t COLOR_SENSOR_LED_BRIGHTNESS = 255; // use 255 to match bench
};
