#pragma once
#include <Arduino.h>
#include "utils/config.h"
#include "sensors/encoder_manager.h"
#include "demos/straight_line_drive.h"
#include "demos/demo_manager.h"
#include "networking/serial_queue_manager.h"

class MotorDriver {
    public:
        MotorDriver();  // Constructor to initialize pins
        void stop_both_motors();
        void left_motor_stop();
        void right_motor_forward(uint8_t speed = MAX_MOTOR_SPEED);
        void right_motor_backward(uint8_t speed = MAX_MOTOR_SPEED);
        void right_motor_stop();

        void set_motor_speeds(int16_t leftTarget, int16_t rightTarget, bool shouldRampUp);
        void update();

        void force_reset_motors();
        void brake_both_motors();
        void brake_if_moving();

        // Motor command processing functions
        void updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed);
        void executeCommand(int16_t leftSpeed, int16_t rightSpeed);
        void processPendingCommands();
        void resetCommandState();

    private:
        int16_t _targetLeftSpeed = 0;
        int16_t _targetRightSpeed = 0;
        int16_t _currentLeftSpeed = 0;
        int16_t _currentRightSpeed = 0;
        bool _shouldRampUp = true;
        static constexpr int16_t SPEED_RAMP_STEP = 50;

        void left_motor_forward(uint8_t speed = MAX_MOTOR_SPEED);
        void left_motor_backward(uint8_t speed = MAX_MOTOR_SPEED);

        void brake_right_motor();
        void brake_left_motor();

        static constexpr float MOTOR_STOPPED_THRESHOLD = 0.5; // RPM threshold for considering motor stopped

        // Command execution state variables
        bool isExecutingCommand = false;
        int16_t currentLeftSpeed = 0;
        int16_t currentRightSpeed = 0;
        int64_t startLeftCount = 0;
        int64_t startRightCount = 0;

        unsigned long commandStartTime = 0;
        static constexpr unsigned long COMMAND_TIMEOUT_MS = 1000; // 1 second timeout
    
        // Next command (if any)
        bool hasNextCommand = false;
        int16_t nextLeftSpeed = 0;
        int16_t nextRightSpeed = 0;

        // Constants
        static constexpr uint8_t MIN_ENCODER_PULSES = 10;
};

extern MotorDriver motorDriver;
