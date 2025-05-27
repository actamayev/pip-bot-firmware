#pragma once

#include <Arduino.h>
#include "utils/utils.h"
#include "sensors/imu.h"
#include "utils/singleton.h"
#include "networking/protocol.h"
#include "actuators/led/rgb_led.h"
#include "actuators/motor_driver.h"

class BalanceController : public Singleton<BalanceController> {
    friend class Singleton<BalanceController>;

    public:
        void enable();
        void disable();
        void update();
        bool isEnabled() const { return _balancingEnabled == BalanceStatus::BALANCED; }
        void updateBalancePids(NewBalancePids newBalancePids);

    private:
        BalanceController() = default;

        // State
        BalanceStatus _balancingEnabled = BalanceStatus::UNBALANCED;
        float _lastValidAngle = 0.0f;
        float _lastError = 0.0f;
        float _errorSum = 0.0f;
        unsigned long _lastUpdateTime = 0;

        // Fixed parameters - not configurable as per your request
        float TARGET_ANGLE = 93.6f; // Fixed target angle

        // PID Constants
        float P_GAIN = 28.5f;
        float I_GAIN = 0.0f;
        float D_GAIN = 6.0f;
        float FF_GAIN = 0.0f;

        // Limits and safety parameters
        float MAX_SAFE_ANGLE_DEVIATION = 30.0f;
        static constexpr int16_t MAX_BALANCE_POWER = 255;
        unsigned long UPDATE_INTERVAL = 3; // 3ms (333Hz)

        // Filtering buffers
        static constexpr uint8_t ANGLE_BUFFER_SIZE = 5;
        float _angleBuffer[ANGLE_BUFFER_SIZE] = {0};
        uint8_t _angleBufferIndex = 0;
        uint8_t _angleBufferCount = 0;

        // Safety buffer for tilt monitoring
        float _safetyBuffer[ANGLE_BUFFER_SIZE] = {0};
        uint8_t _safetyBufferIndex = 0;
        uint8_t _safetyBufferCount = 0;

        float DEADBAND_ANGLE = 1.0f;
        float MAX_STABLE_ROTATION = 0.1f; // degrees/second

        float MIN_EFFECTIVE_PWM = 44; 
};
