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
        void stop_both_motors();

        void update();

        void brake_both_motors();
        void brake_if_moving();

        // Motor command processing functions
        void update_motor_pwm(int16_t left_pwm, int16_t right_pwm);
        void reset_command_state(bool absolute_brake);

    private:
        int16_t _target_left_pwm = 0;
        int16_t _target_right_pwm = 0;
        int16_t _actual_left_pwm = 0;
        int16_t _actual_right_pwm = 0;
        bool _should_ramp_up = true;
        static constexpr int16_t SPEED_RAMP_STEP = 600;

        void left_motor_forward(uint16_t speed = MAX_MOTOR_PWM);
        void left_motor_backward(uint16_t speed = MAX_MOTOR_PWM);
        void right_motor_forward(uint16_t speed = MAX_MOTOR_PWM);
        void right_motor_backward(uint16_t speed = MAX_MOTOR_PWM);

        void left_motor_stop();
        void right_motor_stop();

        void brake_right_motor();
        void brake_left_motor();
        void execute_command(int16_t left_pwm, int16_t right_pwm);
        
        // Private method for immediate motor control (used by friend classes)
        void set_motor_speeds(int16_t left_target, int16_t right_target, bool should_ramp_up);
        void set_motor_speeds_immediate(int16_t left_target, int16_t right_target);

        void process_pending_commands();

        static constexpr float MOTOR_STOPPED_THRESHOLD = 0.5; // RPM threshold for considering motor stopped

        // Command execution state variables
        bool _is_executing_command = false;
        int16_t _command_left_pwm = 0;
        int16_t _command_right_pwm = 0;
        int64_t _start_left_count = 0;
        int64_t _start_right_count = 0;

        unsigned long _command_start_time = 0;
        static constexpr unsigned long COMMAND_TIMEOUT_MS = 1000; // 1 second timeout
    
        // Next command (if any)
        bool _has_next_command = false;
        int16_t _next_left_pwm = 0;
        int16_t _next_right_pwm = 0;

        // Brake timer state variables
        bool _left_brake_active = false;
        bool _right_brake_active = false;
        unsigned long _left_brake_start_time = 0;
        unsigned long _right_brake_start_time = 0;
        static constexpr unsigned long BRAKE_RELEASE_TIME_MS = 1000; // 1 second brake hold time

        // Constants
        static constexpr uint8_t MIN_ENCODER_PULSES = 10;
        static constexpr uint16_t MOTOR_PWM_FREQ = 500;
        static constexpr uint8_t MOTOR_PWM_RES = 12;        // 12-bit (0-4095)
};

extern MotorDriver motorDriver;
