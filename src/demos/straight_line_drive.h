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
            float initialHeading = 0.0f;
            float currentHeading = 0.0f;
            float headingError = 0.0f;
            int16_t leftSpeed = 0;
            int16_t rightSpeed = 0;
            int16_t correction = 0;
            float correctionScale = 1.0f;  // Speed-adaptive scaling factor
        };
        const DebugInfo& getDebugInfo() const { return _debugInfo; }

    private:
        StraightLineDrive() = default;
        
        // Straight driving state
        bool _straightDrivingEnabled = false; 
        float _initialHeading = 0.0f;
        
        // Debug info for display
        DebugInfo _debugInfo;

        // Helper methods
        float calculateHeadingError(float currentHeading, float targetHeading);
        float calculateCorrectionScale(int16_t leftSpeed, int16_t rightSpeed);

        // Control constants
        static constexpr float KP_HEADING_TO_PWM = 2.0f;  // Proportional gain for heading error (degrees to PWM)
        static constexpr int16_t MAX_CORRECTION_PWM = 40;  // Maximum correction PWM
        static constexpr int16_t MIN_FORWARD_SPEED = 10;   // Minimum speed to maintain forward motion
        static constexpr float MAX_HEADING_ERROR = 20.0f;  // Maximum heading error to apply corrections (degrees)
        
        // Speed-adaptive correction constants
        static constexpr float MIN_CORRECTION_SCALE = 0.3f;  // Minimum correction scaling at low speeds
        static constexpr float MAX_CORRECTION_SCALE = 2.0f;  // Maximum correction scaling at high speeds
        static constexpr int16_t SPEED_SCALE_THRESHOLD = 50; // Speed threshold for scaling calculations
};
