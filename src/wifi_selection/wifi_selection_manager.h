// #pragma once
// #include <Arduino.h>
// #include <ESP32Encoder.h>
// #include "utils/structs.h"
// #include "utils/singleton.h"
// #include "networking/wifi_manager.h"
// #include "sensors/encoder_manager.h"
// #include "wifi_selection/haptic_feedback_manager.h"

// class WifiSelectionManager : public Singleton<WifiSelectionManager> {
//     friend class Singleton<WifiSelectionManager>;

//     public:
//         // Constructor
//         WifiSelectionManager() = default;

//         void initNetworkSelection();
//         void updateNetworkSelection();
//         void processNetworkSelection();

//     private:
//         // For network selection scrolling
//         int32_t _lastRightEncoderValue = 0;
//         int _scrollSensitivity = 5; // Adjust this to change scrolling sensitivity
//         bool _networkSelectionActive = false;

//         bool _scrollingEnabled = true;          // Flag to enable/disable scrolling during cooldown
//         unsigned long _scrollCooldownTime = 0;   // Time when cooldown started
//         unsigned long _scrollCooldownDuration = 40; // Cooldown duration in ms
// };
