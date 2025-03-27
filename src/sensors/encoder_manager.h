#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include "./utils/structs.h"
#include "./utils/singleton.h"
#include "../networking/wifi_manager.h"
#include "../actuators/motor_driver.h"

class EncoderManager : public Singleton<EncoderManager> {
    friend class Singleton<EncoderManager>;
    friend class LabDemoManager;  // Add this line to allow LabDemoManager to access private members
    friend class MotorDriver;  // Add this line to allow LabDemoManager to access private members

    public:
        // Constructor
        EncoderManager();

        WheelRPMs getBothWheelRPMs();

        void log_motor_rpm();

        bool should_log_motor_rpm = false;

        void initNetworkSelection();
        void updateNetworkSelection();
        void processNetworkSelection();

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
        static constexpr uint8_t ENCODER_CPR = 3;
        static constexpr unsigned long RPM_CALC_INTERVAL = 100; // ms

        // Update speed calculations - call this periodically
        void update();

        // For network selection scrolling
        int32_t _lastRightEncoderValue = 0;
        int _scrollSensitivity = 8; // Adjust this to change scrolling sensitivity
        bool _networkSelectionActive = false;
};

extern EncoderManager encoderManager;
