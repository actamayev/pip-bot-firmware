#pragma once

#include <Arduino.h>
#include "utils/utils.h"
#include "sensors/imu.h"
#include "utils/singleton.h"
#include "networking/protocol.h"
#include "actuators/led/rgb_led.h"
#include "actuators/motor_driver.h"
#include "networking/serial_queue_manager.h"
#include "demos/demo_manager.h"

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
        float _lastValidAngle = 0.0F;
        float _lastError = 0.0F;
        float _errorSum = 0.0F;
        unsigned long _lastUpdateTime = 0;

        // Fixed parameters - not configurable as per your request
        float _TARGET_ANGLE = 93.6F; // Fixed target angle

        // PID Constants
        float _P_GAIN = 28.5F;
        float _I_GAIN = 0.0F;
        float _D_GAIN = 0.0F;  // Previously 6.0f
        float _FF_GAIN = 0.0F; // Not using this

        // Limits and safety parameters
        float _MAX_SAFE_ANGLE_DEVIATION = 30.0F;
        unsigned long _UPDATE_INTERVAL = 3; // 3ms (333Hz)

        // Filtering buffers
        static constexpr uint8_t ANGLE_BUFFER_SIZE = 5;
        float _angleBuffer[ANGLE_BUFFER_SIZE] = {0};
        uint8_t _angleBufferIndex = 0;
        uint8_t _angleBufferCount = 0;

        // Safety buffer for tilt monitoring
        float _safetyBuffer[ANGLE_BUFFER_SIZE] = {0};
        uint8_t _safetyBufferIndex = 0;
        uint8_t _safetyBufferCount = 0;

        float _DEADBAND_ANGLE = 1.0F;
        float _MAX_STABLE_ROTATION = 0.1F; // degrees/second

        float _MIN_EFFECTIVE_PWM = 330;
};
