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
            HAPTIC_PULSE_ACTIVE,
            HAPTIC_PULSE_RECOVERY
        };

        void start_haptic_feedback(int8_t direction, uint8_t strength, uint8_t duration_ms);
        void update_haptic_feedback();

    private:
        HapticState _hapticState = HAPTIC_IDLE;
        uint8_t _hapticStrength = 0;
        unsigned long _hapticStartTime = 0;
        unsigned long _hapticDuration = 0;
};

extern MotorDriver motorDriver;
