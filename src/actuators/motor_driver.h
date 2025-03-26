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

        // Provide a short vibration/bump effect
        void provide_detent_feedback(uint8_t strength = 50, uint8_t duration_ms = 30);

        // Provide stronger boundary feedback
        void provide_boundary_feedback(uint8_t strength = 100, uint8_t duration_ms = 50);
};

extern MotorDriver motorDriver;
