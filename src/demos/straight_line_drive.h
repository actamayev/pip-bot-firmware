#pragma once

#include <Arduino.h>
#include "utils/utils.h"
#include "utils/singleton.h"
#include "actuators/motor_driver.h"
#include "sensors/imu.h"
#include "networking/serial_queue_manager.h"

class StraightLineDrive : public Singleton<StraightLineDrive> {
    friend class Singleton<StraightLineDrive>;

    public:
        void enable();
        void disable();
        void update(int16_t& leftSpeed, int16_t& rightSpeed);
        bool isEnabled() const { return _straightDrivingEnabled; }
        
        // Debug info for display
        struct DebugInfo {
            float initialYaw = 0.0f;
            float currentYaw = 0.0f;
            float yawError = 0.0f;
            int16_t leftSpeed = 0;
            int16_t rightSpeed = 0;
            int16_t correction = 0;
        };
        const DebugInfo& getDebugInfo() const { return _debugInfo; }

    private:
        StraightLineDrive() = default;
        
        // Straight driving state
        bool _straightDrivingEnabled = false; 
        float _initialYaw = 0.0f;
        float _lastYawError = 0.0f;
        float _integralError = 0.0f;
        float _lastRawYaw = 0.0f;
        int16_t _lastCorrection = 0;
        
        // Debug info for display
        DebugInfo _debugInfo;

        // PID Constants
        static constexpr float YAW_P_GAIN = 3.5f;
        static constexpr float YAW_I_GAIN = 0.1f;
        static constexpr float YAW_D_GAIN = 2.0f;
        static constexpr float YAW_I_MAX = 100.0f;

        // Yaw filtering buffer
        static constexpr uint8_t YAW_BUFFER_SIZE = 3;
        float _yawBuffer[YAW_BUFFER_SIZE] = {0};
        uint8_t _yawBufferIndex = 0;
        uint8_t _yawBufferCount = 0;

        // Angle normalization utilities
        float normalizeAngle(float angle);
        float shortestAnglePath(float from, float to);

        const int16_t MIN_FORWARD_SPEED = 35;
        const int16_t MAX_CORRECTION_PER_CYCLE = 20;
        const float DEADBAND = 1.0f;
};
