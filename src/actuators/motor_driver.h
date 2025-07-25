#pragma once
#include <Arduino.h>
#include "utils/config.h"
#include "sensors/encoder_manager.h"
#include "demos/straight_line_drive.h"

class MotorDriver {
    public:
        MotorDriver();  // Constructor to initialize pins
        void stop_both_motors();
        void left_motor_stop();
        void right_motor_forward(uint8_t speed = MAX_MOTOR_SPEED);
        void right_motor_backward(uint8_t speed = MAX_MOTOR_SPEED);
        void right_motor_stop();

        void set_motor_speeds(int16_t leftTarget, int16_t rightTarget);
        void update_motor_speeds(bool should_ramp_up);

        void force_reset_motors();
        void brake_both_motors();
        void brake_if_moving();
    private:
        int16_t _targetLeftSpeed = 0;
        int16_t _targetRightSpeed = 0;
        int16_t _currentLeftSpeed = 0;
        int16_t _currentRightSpeed = 0;
        static constexpr int16_t SPEED_RAMP_STEP = 50;

        void left_motor_forward(uint8_t speed = MAX_MOTOR_SPEED);
        void left_motor_backward(uint8_t speed = MAX_MOTOR_SPEED);

        void release_left_brake();
        void release_right_brake();
        void brake_right_motor();
        void brake_left_motor();

        bool _leftMotorBraking = false;
        bool _rightMotorBraking = false;
        static constexpr float MOTOR_STOPPED_THRESHOLD = 0.5; // RPM threshold for considering motor stopped
};

extern MotorDriver motorDriver;
