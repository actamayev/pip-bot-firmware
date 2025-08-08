// #pragma once
// #include <Arduino.h>
// #include <ESP32Encoder.h>
// #include "utils/structs.h"
// #include "actuators/motor_driver.h"
// #include "networking/wifi_manager.h"
// #include "wifi_selection/wifi_selection_manager.h"
// #include "wifi_selection/haptic_feedback_manager.h"

// class EncoderManager {
//     friend class MessageProcessor;  // Allows MessageProcessor to access private members
//     friend class WifiSelectionManager;  // Allows WifiSelectionManager to access private members

//     public:
//         // Constructor
//         EncoderManager();

//         WheelRPMs getBothWheelRPMs();

//         void resetDistanceTracking();
//         float getDistanceTraveledCm();
//     private:
//         // ESP32Encoder objects
//         ESP32Encoder _leftEncoder;
//         ESP32Encoder _rightEncoder;
        
//         float _leftWheelRPM;
//         float _rightWheelRPM;

//         // Timing
//         unsigned long _lastUpdateTime;

//         // Constants for calculations
//         static constexpr float GEAR_RATIO = 297.924;
//         static constexpr uint8_t ENCODER_CPR = 3;
//         static constexpr unsigned long RPM_CALC_INTERVAL = 20; // ms

//         // Update speed calculations - call this periodically
//         void update();

//         int64_t _leftEncoderStartCount;
//         int64_t _rightEncoderStartCount;
        
//         // Wheel physical properties
//         static constexpr float WHEEL_DIAMETER_CM = 3.9; // Replace with actual wheel diameter
//         static constexpr float WHEEL_CIRCUMFERENCE_CM = WHEEL_DIAMETER_CM * PI;
// };

// extern EncoderManager encoderManager;
