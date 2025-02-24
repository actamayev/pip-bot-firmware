#pragma once
#include "./config.h"
#include "Arduino.h"

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
        // Encoder-related methods
    //     void updateMotorControl();  // Called in loop() to manage distance
    //     void setTargetDistance(int16_t leftSpeed, int16_t rightSpeed);  // Set distance based on command

    // private:
    //     // Encoder variables
    //     volatile long leftEncoderCount = 0;
    //     volatile long rightEncoderCount = 0;
    //     long leftTargetCount = 0;
    //     long rightTargetCount = 0;
    //     uint8_t leftCurrentSpeed = 0;
    //     uint8_t rightCurrentSpeed = 0;
    //     static void IRAM_ATTR encoder1_isr();  // Left encoder ISR
    //     static void IRAM_ATTR encoder2_isr();  // Right encoder ISR
    //     static constexpr int TICKS_PER_COMMAND = 10;  // Distance per command
};

extern MotorDriver motorDriver;
