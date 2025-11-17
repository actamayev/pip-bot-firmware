#pragma once
#include <Arduino.h>
#include "sensors/sensor_data_buffer.h"
#include "actuators/motor_driver.h"
#include "networking/serial_queue_manager.h"

enum class TurningDirection {
    NONE,
    CLOCKWISE,
    COUNTER_CLOCKWISE
};

enum class TurningState {
    IDLE,
    TURNING,
    OVERSHOOT_BRAKING
};

class TurningManager : public Singleton<TurningManager> {
    friend class Singleton<TurningManager>;

    public:
        // Main interface
        bool start_turn(float degrees);
        void update();
        void complete_navigation();
        const bool is_active() { return currentState != TurningState::IDLE; };

        // Debug info structure
        struct DebugInfo {
            float targetAngle = 0.0f;
            float cumulativeRotation = 0.0f;
            float remainingAngle = 0.0f;
            float currentVelocity = 0.0f;
            float targetVelocity = 0.0f;
            float velocityError = 0.0f;
            uint16_t currentPWM = 0;
            float kpContribution = 0.0f;
            float kiContribution = 0.0f;
            bool inOvershootBraking = false;
        };
        const DebugInfo& get_debug_info() const { return _debugInfo; }

    private:
        TurningManager() = default;
        ~TurningManager() = default;

        // Core turning logic
        void initialize_turn();
        void update_cumulative_rotation();
        void update_velocity();
        float calculate_remaining_angle() const;
        float calculate_target_velocity(float remainingAngle) const;
        float calculate_velocity_error() const;
        uint16_t calculate_pwm(float velocityError);
        bool check_completion();
        bool check_overshoot(float remainingAngle);
        void apply_motor_control(uint16_t pwm, float velocityError);
        void reset_turn_state();
        
        // State variables
        TurningState currentState = TurningState::IDLE;
        TurningDirection currentDirection = TurningDirection::NONE;
        
        float targetTurnAngle = 0;
        // Turn parameters
        float cumulativeRotation = 0.0f;
        float lastHeadingForRotation = 0.0f;
        bool rotationTrackingInitialized = false;
        
        // Velocity tracking
        float currentVelocity = 0.0f;
        float targetVelocity = 0.0f;
        float lastHeading = 0.0f;
        unsigned long lastTime = 0;
        unsigned long lastIntegralTime = 0;

        // Control
        uint16_t currentPWM = 0;
        float integralTerm = 0.0f;
        float kpContribution = 0.0f;
        float kiContribution = 0.0f;

        // Completion detection
        unsigned long completionStartTime = 0;
        bool completionConfirmed = false;

        // Overshoot braking
        unsigned long overshootBrakeStartTime = 0;
        
        // Constants - imported from new_turning
        static constexpr float CRUISE_VELOCITY = 45.0f;      // degrees/second
        static constexpr float MIN_VELOCITY = 20.0f;          // degrees/second
        static constexpr float KP_VELOCITY = 4.0f;                   // Proportional gain
        static constexpr float KI_VELOCITY = 12.0f;                   // Integral gain
        static constexpr float COMPLETION_POSITION_THRESHOLD = 1.0f;  // degrees
        static constexpr float COMPLETION_VELOCITY_THRESHOLD = 1.0f;  // degrees/second
        static constexpr unsigned long COMPLETION_CONFIRMATION_TIME = 100;         // milliseconds
        static constexpr unsigned long OVERSHOOT_BRAKE_DURATION = 100; // milliseconds

        // Debug info
        mutable DebugInfo _debugInfo;
};
