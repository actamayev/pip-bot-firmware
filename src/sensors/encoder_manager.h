#pragma once
#include <Arduino.h>

#include <ESP32Encoder.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h> // ADD this for mutex

#include "networking/wifi_manager.h"
#include "sensor_data_buffer.h"
#include "utils/structs.h"

// Forward declarations
class MotorDriver;

class EncoderManager : public Singleton<EncoderManager> {
    friend class Singleton<EncoderManager>;
    friend class MessageProcessor;  // Allows MessageProcessor to access private members
    friend class MotorDriver;       // Allows MotorDriver to access private members
    friend class TaskManager;       // Allows TaskManager to access private methods
    friend class SensorInitializer; // Allows TaskManager to access private methods

  private:
    // Constructor
    EncoderManager();

    // Standard sensor interface methods
    bool initialize();
    // ESP32Encoder objects
    ESP32Encoder _leftEncoder;
    ESP32Encoder _rightEncoder;

    float _leftWheelRPM;
    float _rightWheelRPM;
    bool isInitialized = false;

    // Timing
    uint32_t _lastUpdateTime;

    // Standard sensor interface methods (for TaskManager)
    void update_sensor_data(); // Single read, write to buffer
    bool should_be_polling() const;

    // Constants for calculations
    static constexpr float GEAR_RATIO = 50.0;
    static constexpr uint8_t MOTOR_ENCODER_CPR = 12; // cycles per revolution on motor shaft
    // attachFullQuad counts all 4 edges (4x resolution)
    static constexpr uint8_t PULSES_PER_REVOLUTION = MOTOR_ENCODER_CPR;
    static constexpr uint32_t RPM_CALC_INTERVAL = 20; // ms

    // Wheel physical properties
    static constexpr float WHEEL_DIAMETER_IN = 1.535; // 39mm converted to inches
    static constexpr float WHEEL_CIRCUMFERENCE_IN = WHEEL_DIAMETER_IN * PI;

    // Internal update method (now private - called by updateSensorData)
    void update();

    int64_t _leftEncoderStartCount;
    int64_t _rightEncoderStartCount;

    // For RPM calculation without clearing encoder counts
    int64_t _leftLastCount;
    int64_t _rightLastCount;

    // Wheel physical properties
    static constexpr float WHEEL_DIAMETER_CM = 3.9; // Replace with actual wheel diameter
    static constexpr float WHEEL_CIRCUMFERENCE_CM = WHEEL_DIAMETER_CM * PI;
    static constexpr int MUTEX_REFRESH_FREQUENCY_MS = 5;
};
