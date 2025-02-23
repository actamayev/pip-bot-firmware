#pragma once
#include "./config.h"
#include "Arduino.h"

class MotorDriver {
    public:
        void both_motors_forward(uint8_t speed);
        void both_motors_backward(uint8_t speed);
        void rotate_clockwise(uint8_t speed);
        void rotate_counterclockwise(uint8_t speed);
        void stop_both_motors();
        void left_motor_forward(uint8_t speed);
        void left_motor_backward(uint8_t speed);
        void left_motor_stop();
        void right_motor_forward(uint8_t speed);
        void right_motor_backward(uint8_t speed);
        void right_motor_stop();
};

extern MotorDriver motorDriver;
