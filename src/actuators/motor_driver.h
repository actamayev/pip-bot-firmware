#pragma once
#include <Arduino.h>
#include "../utils/config.h"
#include "../sensors/encoder_manager.h"

class MotorDriver {
    public:
        MotorDriver();  // Constructor to initialize pins
        void both_motors_forward(uint8_t speed = 255);
        void both_motors_backward(uint8_t speed = 255);
        void rotate_clockwise(uint8_t speed = 255);
        void rotate_counterclockwise(uint8_t speed = 255);
        void stop_both_motors();
        void left_motor_forward(uint8_t speed = 255);
        void left_motor_backward(uint8_t speed = 255);
        void left_motor_stop();
        void right_motor_forward(uint8_t speed = 255);
        void right_motor_backward(uint8_t speed = 255);
        void right_motor_stop();

        enum HapticState {
            HAPTIC_IDLE,
            HAPTIC_RESISTANCE,      // Stage 1: Initial resistance against movement
            HAPTIC_REVERSE_PULSE,   // Stage 2: Helping pulse in direction of movement
            HAPTIC_CENTERING,       // Stage 3: Gentle centering force
            HAPTIC_FINAL_BUMP,      // Stage 4: Final "click" into place
            HAPTIC_RECOVERY         // Cool-down period before accepting new input
        };

        void start_haptic_feedback(int8_t direction, uint8_t strength, uint8_t duration_ms);
        void update_haptic_feedback();

        void set_motor_speeds(int16_t leftTarget, int16_t rightTarget);
        void update_motor_speeds();

        void enable_straight_driving();
        void disable_straight_driving();
        void update_straight_driving();

    private:
        HapticState _hapticState = HAPTIC_IDLE;
        int8_t _hapticDirection = 0;
        uint8_t _resistanceStrength = 0;
        uint8_t _reversePulseStrength = 0;
        uint8_t _centeringStrength = 0;
        uint8_t _finalBumpStrength = 0;    // Added for the final bump
        unsigned long _hapticStartTime = 0;
        unsigned long _resistanceDuration = 0;
        unsigned long _reversePulseDuration = 0;
        unsigned long _centeringDuration = 0;
        unsigned long _finalBumpDuration = 0;  // Added for the final bump
        unsigned long _recoveryDuration = 0;

        int16_t _targetLeftSpeed = 0;
        int16_t _targetRightSpeed = 0;
        int16_t _currentLeftSpeed = 0;
        int16_t _currentRightSpeed = 0;
        unsigned long _lastSpeedUpdateTime = 0;
        static constexpr int16_t SPEED_RAMP_STEP = 50;
        static constexpr unsigned long SPEED_RAMP_INTERVAL = 15;  // ms between updates        

        bool _straightDrivingEnabled = false;
        float _initialYaw = 0.0f;
        float _lastYawError = 0.0f;
        float _integralError = 0.0f;
        static constexpr float YAW_P_GAIN = 5.0f;
        static constexpr float YAW_I_GAIN = 0.05f;
        static constexpr float YAW_D_GAIN = 0.5f;
        static constexpr float YAW_I_MAX = 100.0f;

        static constexpr uint8_t YAW_BUFFER_SIZE = 10;
        float _yawBuffer[YAW_BUFFER_SIZE] = {0};
        uint8_t _yawBufferIndex = 0;
        uint8_t _yawBufferCount = 0;

        float addYawReadingAndGetAverage(float newYaw);

        float normalizeAngle(float angle);
        float shortestAnglePath(float from, float to);
};

extern MotorDriver motorDriver;
