#pragma once
#include <Arduino.h>
#include "utils/config.h"
#include "sensors/sensor_data_buffer.h"
#include "demos/straight_line_drive.h"
#include "demos/demo_manager.h"
#include "networking/serial_queue_manager.h"

// LEDC channel assignments (ESP32-S3 has 8 channels: 0-7)
enum { LEFT_MOTOR_CH_1 = 0, LEFT_MOTOR_CH_2 = 1, RIGHT_MOTOR_CH_1 = 2, RIGHT_MOTOR_CH_2 = 3 };

class MotorDriver {
    friend class TurningManager;
    friend class BalanceController;
    friend class TaskManager;
    friend class BytecodeVM;

    public:
        MotorDriver();  // Constructor to initialize pins
        void stopBothMotors();

        void update();

        void brakeBothMotors();
        void brakeIfMoving();

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

        void leftMotorForward(uint16_t speed = MAX_MOTOR_PWM);
        void leftMotorBackward(uint16_t speed = MAX_MOTOR_PWM);
        void rightMotorForward(uint16_t speed = MAX_MOTOR_PWM);
        void rightMotorBackward(uint16_t speed = MAX_MOTOR_PWM);

        void leftMotorStop();
        void rightMotorStop();

        void brakeRightMotor();
        void brakeLeftMotor();
        void executeCommand(int16_t leftPwm, int16_t rightPwm);
        
        // Private method for immediate motor control (used by friend classes)
        void setMotorSpeeds(int16_t leftTarget, int16_t rightTarget, bool shouldRampUp);
        void setMotorSpeedsImmediate(int16_t leftTarget, int16_t rightTarget);

        void processPendingCommands();

        static constexpr float MOTOR_STOPPED_THRESHOLD = 0.5; // RPM threshold for considering motor stopped

        // Command execution state variables
        bool _isExecutingCommand = false;
        int16_t _commandLeftPwm = 0;
        int16_t _commandRightPwm = 0;
        int64_t _startLeftCount = 0;
        int64_t _startRightCount = 0;

        unsigned long _commandStartTime = 0;
        static constexpr unsigned long COMMAND_TIMEOUT_MS = 1000; // 1 second timeout
    
        // Next command (if any)
        bool _hasNextCommand = false;
        int16_t _nextLeftPwm = 0;
        int16_t _nextRightPwm = 0;

        // Brake timer state variables
        bool _leftBrakeActive = false;
        bool _rightBrakeActive = false;
        unsigned long _leftBrakeStartTime = 0;
        unsigned long _rightBrakeStartTime = 0;
        static constexpr unsigned long BRAKE_RELEASE_TIME_MS = 1000; // 1 second brake hold time

        // Constants
        static constexpr uint8_t MIN_ENCODER_PULSES = 10;
        static constexpr uint16_t MOTOR_PWM_FREQ = 500;
        static constexpr uint8_t MOTOR_PWM_RES = 12;        // 12-bit (0-4095)
};

extern MotorDriver motorDriver;
