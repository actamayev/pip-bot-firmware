#pragma once
#include <Arduino.h>

#include "actuators/motor_driver.h"
#include "networking/serial_queue_manager.h"
#include "sensors/sensor_data_buffer.h"

enum class TurningDirection : uint8_t { NONE, CLOCKWISE, COUNTER_CLOCKWISE };

enum class TurningState : uint8_t { IDLE, TURNING, OVERSHOOT_BRAKING };

class TurningManager : public Singleton<TurningManager> {
    friend class Singleton<TurningManager>;

  public:
    // Main interface
    static bool start_turn(float degrees);
    void update();
    void complete_navigation();
    bool is_active() {
        return _currentState != TurningState::IDLE;
    };

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
    const DebugInfo& get_debug_info() const {
        return _debugInfo;
    }

  private:
    TurningManager() = default;
    ~TurningManager() = default;

    // Core turning logic
    static void initialize_turn();
    static void update_cumulative_rotation();
    static void update_velocity();
    float calculate_remaining_angle() const;
    static float calculate_target_velocity(float remaining_angle);
    float calculate_velocity_error() const;
    static uint16_t calculate_pwm(float velocity_error);
    bool check_completion();
    static bool check_overshoot(float remaining_angle);
    static void apply_motor_control(uint16_t pwm, float velocity_error);
    void reset_turn_state();

    // State variables
    TurningState _currentState = TurningState::IDLE;
    TurningDirection _currentDirection = TurningDirection::NONE;

    float _targetTurnAngle = 0;
    // Turn parameters
    float _cumulativeRotation = 0.0f;
    float _lastHeadingForRotation = 0.0f;
    bool _rotationTrackingInitialized = false;

    // Velocity tracking
    float _currentVelocity = 0.0f;
    float _targetVelocity = 0.0f;
    float _lastHeading = 0.0f;
    uint32_t _lastTime = 0;
    uint32_t _lastIntegralTime = 0;

    // Control
    uint16_t _currentPWM = 0;
    float _integralTerm = 0.0f;
    float _kpContribution = 0.0f;
    float _kiContribution = 0.0f;

    // Completion detection
    uint32_t _completionStartTime = 0;
    bool _completionConfirmed = false;

    // Overshoot braking
    uint32_t _overshootBrakeStartTime = 0;

    // Constants - imported from new_turning
    static constexpr float CRUISE_VELOCITY = 45.0f;               // degrees/second
    static constexpr float MIN_VELOCITY = 20.0f;                  // degrees/second
    static constexpr float KP_VELOCITY = 4.0f;                    // Proportional gain
    static constexpr float KI_VELOCITY = 12.0f;                   // Integral gain
    static constexpr float COMPLETION_POSITION_THRESHOLD = 1.0f;  // degrees
    static constexpr float COMPLETION_VELOCITY_THRESHOLD = 1.0f;  // degrees/second
    static constexpr uint32_t COMPLETION_CONFIRMATION_TIME = 100; // milliseconds
    static constexpr uint32_t OVERSHOOT_BRAKE_DURATION = 100;     // milliseconds

    // Debug info
    mutable DebugInfo _debugInfo;
};
