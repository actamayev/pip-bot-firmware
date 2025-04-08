#pragma once
#include <Arduino.h>
#include "../utils/config.h"
#include "../sensors/sensors.h"
#include "../lab_demo/straight_line_drive.h"

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

        void set_motor_speeds(int16_t leftTarget, int16_t rightTarget);
        void update_motor_speeds(bool should_ramp_up, int16_t speed_ramp_interval = SPEED_RAMP_INTERVAL);
        void update_max_motor_speed(uint8_t newMaxSpeed);

    private:
        int16_t _targetLeftSpeed = 0;
        int16_t _targetRightSpeed = 0;
        int16_t _currentLeftSpeed = 0;
        int16_t _currentRightSpeed = 0;
        unsigned long _lastSpeedUpdateTime = 0;
        static constexpr int16_t SPEED_RAMP_STEP = 50;
        static constexpr unsigned long SPEED_RAMP_INTERVAL = 5;  // ms between updates
        uint8_t _maxMotorSpeedPercent = 100; // Default to full speed

};

extern MotorDriver motorDriver;
