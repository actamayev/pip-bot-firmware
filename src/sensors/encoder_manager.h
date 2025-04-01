#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include "./utils/structs.h"
#include "./utils/singleton.h"
#include "../actuators/motor_driver.h"
#include "../networking/wifi_manager.h"
#include "../wifi_selection/wifi_selection_manager.h"
#include "../wifi_selection/haptic_feedback_manager.h"

class EncoderManager : public Singleton<EncoderManager> {
    friend class Singleton<EncoderManager>;
    friend class LabDemoManager;  // Allows LabDemoManager to access private members
    friend class WifiSelectionManager;  // Allows WifiSelectionManager to access private members

    public:
        // Constructor
        EncoderManager();

        WheelRPMs getBothWheelRPMs();
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
};

extern EncoderManager encoderManager;
