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
        bool is_enabled() const { return _straightDrivingEnabled; }
        
        // Debug info for display
        struct DebugInfo {
            float initialHeading = 0.0F;
            float currentHeading = 0.0F;
            float headingError = 0.0F;
            int16_t leftSpeed = 0;
            int16_t rightSpeed = 0;
            int16_t correction = 0;
        };
        const DebugInfo& get_debug_info() const { return _debugInfo; }

    private:
        StraightLineDrive() = default;
        
        // Straight driving state
        bool _straightDrivingEnabled = false;
        float _initialHeading = 0.0F;

        // Debug info for display
        DebugInfo _debugInfo;

        // Helper methods
        float calculate_heading_error(float currentHeading, float targetHeading);

        // Control constants
        static constexpr float KP_HEADING_TO_PWM = 100.0F;  // Proportional gain for heading error (degrees to PWM)
        static constexpr int16_t MIN_FORWARD_SPEED = 330;   // Minimum speed to maintain forward motion
        static constexpr float DEAD_ZONE_DEGREES = 0.5F;    // Ignore heading errors smaller than this (reduces oscillation)
};
