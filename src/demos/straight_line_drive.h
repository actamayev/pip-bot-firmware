#pragma once

#include <Arduino.h>
#include "utils/utils.h"
#include "utils/singleton.h"
#include "actuators/motor_driver.h"
#include "sensors/sensor_data_buffer.h"
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
            int64_t leftCounts = 0;
            int64_t rightCounts = 0;
            int64_t countError = 0;
            int16_t leftSpeed = 0;
            int16_t rightSpeed = 0;
            int16_t correction = 0;
        };
        const DebugInfo& getDebugInfo() const { return _debugInfo; }

    private:
        StraightLineDrive() = default;
        
        // Straight driving state
        bool _straightDrivingEnabled = false; 
        int64_t _initialLeftCount = 0;
        int64_t _initialRightCount = 0;
        
        // Debug info for display
        DebugInfo _debugInfo;

        // Proportional control constants (from SLD)
        static constexpr float KP_COUNTS_TO_PWM = 0.5f;  // Same as SLD
        static constexpr int16_t MAX_CORRECTION_PWM = 40;  // Same as SLD
        static constexpr int16_t MIN_FORWARD_SPEED = 10;   // Minimum speed to maintain forward motion
};
