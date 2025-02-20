#pragma once
#include "./config.h"
#include "Arduino.h"

class MotorDriver {
    public:
        void both_motors_forward();
        void both_motors_backward();
        void rotate_clockwise();
        void rotate_counterclockwise();
        void stop_both_motors();

    private:
        void left_motor_forward();
        void left_motor_backward();
        void left_motor_stop();
        void right_motor_forward();
        void right_motor_backward();
        void right_motor_stop();
};

extern MotorDriver motorDriver;
