// balance_controller.h
#pragma once

#include <Arduino.h>
#include "../utils/utils.h"
#include "../utils/singleton.h"
#include "../sensors/sensors.h"
#include "../networking/protocol.h"

class BalanceController : public Singleton<BalanceController> {
    friend class Singleton<BalanceController>;

    public:
        void enable();
        void disable();
        void update();
        bool isEnabled() const { return _balancingEnabled == BalanceStatus::BALANCED; }
        
    private:
        BalanceController();
        
        // State
        BalanceStatus _balancingEnabled = BalanceStatus::UNBALANCED;
        float _lastValidAngle = 0.0f;
        float _lastError = 0.0f;
        float _errorSum = 0.0f;
        unsigned long _lastUpdateTime = 0;

        // Fixed parameters - not configurable as per your request
        static constexpr float TARGET_ANGLE = 91.5f; // Fixed target angle

        // PID Constants - fixed in the class as requested
        static constexpr float P_GAIN = 36.0f;
        static constexpr float I_GAIN = 0.0f;  // Non-zero to improve steady-state error
        static constexpr float D_GAIN = 3.5f;

        // Limits and safety parameters
        static constexpr float MAX_SAFE_ANGLE_DEVIATION = 20.0f;
        static constexpr int16_t MAX_BALANCE_POWER = 255;
        static constexpr unsigned long UPDATE_INTERVAL = 5; // 5ms (200Hz)

        // Filtering buffers
        static constexpr uint8_t ANGLE_BUFFER_SIZE = 10;
        float _angleBuffer[ANGLE_BUFFER_SIZE] = {0};
        uint8_t _angleBufferIndex = 0;
        uint8_t _angleBufferCount = 0;

        // Safety buffer for tilt monitoring
        float _safetyBuffer[ANGLE_BUFFER_SIZE] = {0};
        uint8_t _safetyBufferIndex = 0;
        uint8_t _safetyBufferCount = 0;

        static constexpr float DEADBAND_ANGLE = 0.25f;
        static constexpr float MAX_STABLE_ROTATION = 0.1f; // degrees/second
};
