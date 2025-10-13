#pragma once
#include <Arduino.h>
#include "utils/config.h"
#include "sensors/sensor_data_buffer.h"
#include "demos/straight_line_drive.h"
#include "demos/demo_manager.h"
#include "networking/serial_queue_manager.h"

// LEDC channel assignments (ESP32-S3 has 8 channels: 0-7)
#define LEFT_MOTOR_CH_1   0
#define LEFT_MOTOR_CH_2   1
#define RIGHT_MOTOR_CH_1  2
#define RIGHT_MOTOR_CH_2  3

class MotorDriver {
    friend class TurningManager;
    friend class BalanceController;
    friend class TaskManager;
    friend class BytecodeVM;

    public:
        MotorDriver();  // Constructor to initialize pins
        void stop_both_motors();

        void update();

        void brake_both_motors();
        void brake_if_moving();

        // Motor command processing functions
        void updateMotorPwm(int16_t leftPwm, int16_t rightPwm);
        void resetCommandState(bool absoluteBrake);

    private:
        int16_t _targetLeftPwm = 0;
        int16_t _targetRightPwm = 0;
        int16_t _actualLeftPwm = 0;
        int16_t _actualRightPwm = 0;
        bool _shouldRampUp = true;
        static constexpr int16_t SPEED_RAMP_STEP = 600;

        void left_motor_forward(uint16_t speed = MAX_MOTOR_PWM);
        void left_motor_backward(uint16_t speed = MAX_MOTOR_PWM);
        void right_motor_forward(uint16_t speed = MAX_MOTOR_PWM);
        void right_motor_backward(uint16_t speed = MAX_MOTOR_PWM);

        void left_motor_stop();
        void right_motor_stop();

        void brake_right_motor();
        void brake_left_motor();
        void executeCommand(int16_t leftPwm, int16_t rightPwm);
        
        // Private method for immediate motor control (used by friend classes)
        void set_motor_speeds(int16_t leftTarget, int16_t rightTarget, bool shouldRampUp);
        void set_motor_speeds_immediate(int16_t leftTarget, int16_t rightTarget);

        void processPendingCommands();

        static constexpr float MOTOR_STOPPED_THRESHOLD = 0.5; // RPM threshold for considering motor stopped

        // Command execution state variables
        bool isExecutingCommand = false;
        int16_t _commandLeftPwm = 0;
        int16_t _commandRightPwm = 0;
        int64_t startLeftCount = 0;
        int64_t startRightCount = 0;

        unsigned long commandStartTime = 0;
        static constexpr unsigned long COMMAND_TIMEOUT_MS = 1000; // 1 second timeout
    
        // Next command (if any)
        bool hasNextCommand = false;
        int16_t nextLeftPwm = 0;
        int16_t nextRightPwm = 0;

        // Constants
        static constexpr uint8_t MIN_ENCODER_PULSES = 10;
        static constexpr uint16_t MOTOR_PWM_FREQ = 500;
        static constexpr uint8_t MOTOR_PWM_RES = 12;        // 12-bit (0-4095)
};

extern MotorDriver motorDriver;
