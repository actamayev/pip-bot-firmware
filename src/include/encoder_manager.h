#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include "./singleton.h"
#include "./structs.h"

class EncoderManager : public Singleton<EncoderManager> {
    friend class Singleton<EncoderManager>;

    public:
        // Constructor
        EncoderManager();

        WheelRPMs getBothWheelRPMs();

        void log_motor_rpm();

        bool should_log_motor_rpm = false;
    private:
        // ESP32Encoder objects
        ESP32Encoder _leftEncoder;
        ESP32Encoder _rightEncoder;
        
        float _leftWheelRPM;
        float _rightWheelRPM;

        // Timing
        unsigned long _lastUpdateTime;
        unsigned long _lastLogTime;

        // Constants for calculations
        static constexpr float GEAR_RATIO = 297.924;
        static constexpr int ENCODER_CPR = 3;
        static constexpr unsigned long RPM_CALC_INTERVAL = 100; // ms

        // Update speed calculations - call this periodically
        void update();
};

extern EncoderManager encoderManager;
