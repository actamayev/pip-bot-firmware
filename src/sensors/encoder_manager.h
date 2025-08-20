#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>  // ADD this for mutex
#include "utils/structs.h"
#include "networking/wifi_manager.h"
#include "sensor_data_buffer.h"

// Forward declarations
class MotorDriver;

class EncoderManager {
    friend class MessageProcessor;  // Allows MessageProcessor to access private members
    friend class MotorDriver;       // Allows MotorDriver to access private members
    friend class TaskManager;       // Allows TaskManager to access private methods

    public:
        // Constructor
        EncoderManager();

        // Standard sensor interface methods
        bool initialize();
        bool needsInitialization() const { return !isInitialized; }
        bool canRetryInitialization() const { return true; }  // Encoders can always retry

    private:
        // ESP32Encoder objects
        ESP32Encoder _leftEncoder;
        ESP32Encoder _rightEncoder;
        
        float _leftWheelRPM;
        float _rightWheelRPM;
        bool isInitialized = false;

        // Timing
        unsigned long _lastUpdateTime;
        
        // Standard sensor interface methods (for TaskManager)
        void updateSensorData();  // Single read, write to buffer
        bool shouldBePolling() const;

        // Constants for calculations
        static constexpr float GEAR_RATIO = 297.924;
        static constexpr uint8_t ENCODER_CPR = 3;
        static constexpr unsigned long RPM_CALC_INTERVAL = 20; // ms

        // Internal update method (now private - called by updateSensorData)
        void update();

        int64_t _leftEncoderStartCount;
        int64_t _rightEncoderStartCount;
        
        // Wheel physical properties
        static constexpr float WHEEL_DIAMETER_CM = 3.9; // Replace with actual wheel diameter
        static constexpr float WHEEL_CIRCUMFERENCE_CM = WHEEL_DIAMETER_CM * PI;
        static constexpr int MUTEX_REFRESH_FREQUENCY_MS = 5; // Replace with actual wheel diameter

        SemaphoreHandle_t encoderMutex = nullptr;
};

extern EncoderManager encoderManager;
