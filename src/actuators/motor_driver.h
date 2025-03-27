#pragma once
#include "Arduino.h"
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
};

extern MotorDriver motorDriver;
