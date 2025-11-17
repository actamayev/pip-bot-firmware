#include "motor_driver.h"

MotorDriver motor_driver;

MotorDriver::MotorDriver() {
    // Configure LEDC timer and channels
    ledcSetup(LEFT_MOTOR_CH_1, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcSetup(LEFT_MOTOR_CH_2, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcSetup(RIGHT_MOTOR_CH_1, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcSetup(RIGHT_MOTOR_CH_2, MOTOR_PWM_FREQ, MOTOR_PWM_RES);

    // Attach channels to pins
    ledcAttachPin(LEFT_MOTOR_PIN_IN_1, LEFT_MOTOR_CH_1);
    ledcAttachPin(LEFT_MOTOR_PIN_IN_2, LEFT_MOTOR_CH_2);
    ledcAttachPin(RIGHT_MOTOR_PIN_IN_1, RIGHT_MOTOR_CH_1);
    ledcAttachPin(RIGHT_MOTOR_PIN_IN_2, RIGHT_MOTOR_CH_2);

    Serial.printf("Motor PWM initialized: %d Hz, %d-bit resolution\n", MOTOR_PWM_FREQ, MOTOR_PWM_RES);
}

void MotorDriver::stop_both_motors() {
    left_motor_stop();
    right_motor_stop();
}

void MotorDriver::left_motor_forward(uint16_t speed) {
    ledcWrite(LEFT_MOTOR_CH_2, 0); // Explicitly clear forward pin
    ledcWrite(LEFT_MOTOR_CH_1, speed);
    _left_brake_active = false;
}

void MotorDriver::left_motor_backward(uint16_t speed) {
    ledcWrite(LEFT_MOTOR_CH_1, 0); // Explicitly clear backward pin
    ledcWrite(LEFT_MOTOR_CH_2, speed);
    _left_brake_active = false;
}

void MotorDriver::left_motor_stop() {
    ledcWrite(LEFT_MOTOR_CH_1, 0);
    ledcWrite(LEFT_MOTOR_CH_2, 0);
}

void MotorDriver::right_motor_forward(uint16_t speed) {
    ledcWrite(RIGHT_MOTOR_CH_2, 0); // Explicitly clear forward pin
    ledcWrite(RIGHT_MOTOR_CH_1, speed);
    _right_brake_active = false;
}

void MotorDriver::right_motor_backward(uint16_t speed) {
    ledcWrite(RIGHT_MOTOR_CH_1, 0); // Explicitly clear backward pin
    ledcWrite(RIGHT_MOTOR_CH_2, speed);
    _right_brake_active = false;
}

void MotorDriver::right_motor_stop() {
    ledcWrite(RIGHT_MOTOR_CH_1, 0);
    ledcWrite(RIGHT_MOTOR_CH_2, 0);
}

// These methods explicitly hold the motor in brake.
void MotorDriver::brake_left_motor() {
    ledcWrite(LEFT_MOTOR_CH_1, MAX_MOTOR_PWM);
    ledcWrite(LEFT_MOTOR_CH_2, MAX_MOTOR_PWM);
    _left_brake_active = true;
    _left_brake_start_time = millis();
}

void MotorDriver::brake_right_motor() {
    ledcWrite(RIGHT_MOTOR_CH_1, MAX_MOTOR_PWM);
    ledcWrite(RIGHT_MOTOR_CH_2, MAX_MOTOR_PWM);
    _right_brake_active = true;
    _right_brake_start_time = millis();
}

void MotorDriver::brake_both_motors() {
    brake_left_motor();
    brake_right_motor();
}

void MotorDriver::brake_if_moving() {
    // Get current wheel speeds from sensor data buffer
    WheelRPMs rpms = SensorDataBuffer::get_instance().get_latest_wheel_rpms();

    // Check if left motor is moving
    if (abs(rpms.leftWheelRPM) > MOTOR_STOPPED_THRESHOLD) {
        // Left motor is moving, apply brake
        brake_left_motor();
    }
    // // Check if right motor is moving
    if (abs(rpms.rightWheelRPM) > MOTOR_STOPPED_THRESHOLD) {
        // Right motor is moving, apply brake
        brake_right_motor();
    }
}

// Use this to set speeds directly (without needing ramp up, or waiting for next command).
// The next command is used to make sure the current command is fully complete (ie for micro-turns in the garage)
void MotorDriver::set_motor_speeds(int16_t left_target, int16_t right_target, bool should_ramp_up) {
    // Store target speeds but don't change actual speeds immediately
    _target_left_pwm = constrain(left_target, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);
    _target_right_pwm = constrain(right_target, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);
    _should_ramp_up = should_ramp_up;
}

void MotorDriver::update() {
    // Check brake timers first
    uint32_t current_time = millis();

    if (_left_brake_active && (current_time - _left_brake_start_time >= BRAKE_RELEASE_TIME_MS)) {
        left_motor_stop();
        _left_brake_active = false;
    }

    if (_right_brake_active && (current_time - _right_brake_start_time >= BRAKE_RELEASE_TIME_MS)) {
        right_motor_stop();
        _right_brake_active = false;
    }

    bool speeds_changed = false;

    if (_should_ramp_up) {
        // Gradually ramp left motor speed toward target
        if (_actual_left_pwm < _target_left_pwm) {
            _actual_left_pwm = min(static_cast<int16_t>(_actual_left_pwm + SPEED_RAMP_STEP), _target_left_pwm);
            speeds_changed = true;
        } else if (_actual_left_pwm > _target_left_pwm) {
            _actual_left_pwm = max(static_cast<int16_t>(_actual_left_pwm - SPEED_RAMP_STEP), _target_left_pwm);
            speeds_changed = true;
        }

        // Gradually ramp right motor speed toward target
        if (_actual_right_pwm < _target_right_pwm) {
            _actual_right_pwm = min(static_cast<int16_t>(_actual_right_pwm + SPEED_RAMP_STEP), _target_right_pwm);
            speeds_changed = true;
        } else if (_actual_right_pwm > _target_right_pwm) {
            _actual_right_pwm = max(static_cast<int16_t>(_actual_right_pwm - SPEED_RAMP_STEP), _target_right_pwm);
            speeds_changed = true;
        }
    } else {
        // Skip ramping and set speeds immediately
        if (_actual_left_pwm != _target_left_pwm) {
            _actual_left_pwm = _target_left_pwm;
            speeds_changed = true;
        }

        if (_actual_right_pwm != _target_right_pwm) {
            _actual_right_pwm = _target_right_pwm;
            speeds_changed = true;
        }
    }

    int16_t left_adjusted = _actual_left_pwm;
    int16_t right_adjusted = _actual_right_pwm;

    if (StraightLineDrive::get_instance().is_enabled()) {
        StraightLineDrive::get_instance().update(left_adjusted, right_adjusted);
    }

    // Only update motor controls if speeds have changed or StraightLineDrive is applying corrections
    if (speeds_changed || StraightLineDrive::get_instance().is_enabled()) {
        // Apply the current speeds
        if (left_adjusted == 0) {
            brake_left_motor();
        } else if (left_adjusted > 0) {
            left_motor_forward(left_adjusted);
        } else {
            left_motor_backward(-left_adjusted);
        }

        if (right_adjusted == 0) {
            brake_right_motor();
        } else if (right_adjusted > 0) {
            right_motor_forward(right_adjusted);
        } else {
            right_motor_backward(-right_adjusted);
        }
    }
}

void MotorDriver::set_motor_speeds_immediate(int16_t left_target, int16_t right_target) {
    // Constrain speeds
    int16_t left_constrained = constrain(left_target, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);
    int16_t right_constrained = constrain(right_target, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);

    // Apply immediately without any ramping or state management
    if (left_constrained == 0) {
        brake_left_motor();
    } else if (left_constrained > 0) {
        left_motor_forward(left_constrained);
    } else {
        left_motor_backward(-left_constrained);
    }

    if (right_constrained == 0) {
        brake_right_motor();
    } else if (right_constrained > 0) {
        right_motor_forward(right_constrained);
    } else {
        right_motor_backward(-right_constrained);
    }
}

void MotorDriver::update_motor_pwm(int16_t left_pwm, int16_t right_pwm) {
    // Constrain speeds
    left_pwm = constrain(left_pwm, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);
    right_pwm = constrain(right_pwm, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);

    // Check if straight-line driving should be active
    if (left_pwm > 0 && right_pwm > 0 && left_pwm == right_pwm) {
        // Enable straight-line driving if not already active
        if (!StraightLineDrive::get_instance().is_enabled()) {
            StraightLineDrive::get_instance().enable();
        }
    } else {
        // Disable straight-line driving for all other cases
        StraightLineDrive::get_instance().disable();
    }

    // If we're not executing a command, start this one immediately
    if (!_is_executing_command) {
        execute_command(left_pwm, right_pwm);
    } else {
        // Store as next command
        _has_next_command = true;
        _next_left_pwm = left_pwm;
        _next_right_pwm = right_pwm;
    }
}

void MotorDriver::execute_command(int16_t left_pwm, int16_t right_pwm) {
    // Save command details
    _command_left_pwm = left_pwm;
    _command_right_pwm = right_pwm;

    // Get initial encoder counts from sensor data buffer
    auto starting_counts = SensorDataBuffer::get_instance().get_latest_encoder_counts();
    _start_left_count = starting_counts.first;
    _start_right_count = starting_counts.second;

    // Start the command timer
    _command_start_time = millis();

    set_motor_speeds(left_pwm, right_pwm, true); // ramp is default true for commands that are executed in series (ie driving in the garage)

    _is_executing_command = true;
    _has_next_command = false;
}

void MotorDriver::process_pending_commands() {
    // If a demo is running, don't process motor commands
    if (DemoManager::get_instance().is_any_demo_active()) {
        return;
    }

    if (!_is_executing_command) {
        // If we have a next command, execute it
        if (_has_next_command) {
            execute_command(_next_left_pwm, _next_right_pwm);
        }
        return;
    }

    bool is_movement_command = (_command_left_pwm != 0 || _command_right_pwm != 0);

    if (!is_movement_command) {
        _is_executing_command = false;

        if (_has_next_command) {
            execute_command(_next_left_pwm, _next_right_pwm);
        }
        return;
    }

    // Get current encoder counts from sensor data buffer
    auto current_counts = SensorDataBuffer::get_instance().get_latest_encoder_counts();
    int64_t current_left_count = current_counts.first;
    int64_t current_right_count = current_counts.second;

    // Calculate absolute change in encoder counts
    int64_t left_delta = abs(current_left_count - _start_left_count);
    int64_t right_delta = abs(current_right_count - _start_right_count);

    // Check for command completion conditions:
    bool encoder_threshold_met = (left_delta >= MIN_ENCODER_PULSES || right_delta >= MIN_ENCODER_PULSES);
    bool command_timed_out = (millis() - _command_start_time) >= COMMAND_TIMEOUT_MS;

    if (encoder_threshold_met || command_timed_out) {
        if (command_timed_out) {
            SerialQueueManager::get_instance().queue_message("Command timed out after 1 second - possible motor stall");
        } else {
            // SerialQueueManager::get_instance().queue_message("Command completed with pulses - Left: %lld, Right: %lld\n",
            //             left_delta, right_delta);
        }

        _is_executing_command = false;

        if (_has_next_command) {
            execute_command(_next_left_pwm, _next_right_pwm);
        }
    }
}

void MotorDriver::reset_command_state(bool absolute_brake) {
    // Clear all command state first to prevent any queued commands from executing
    _target_left_pwm = 0;
    _target_right_pwm = 0;
    _actual_left_pwm = 0;
    _actual_right_pwm = 0;
    _is_executing_command = false;
    _command_left_pwm = 0;
    _command_right_pwm = 0;
    _start_left_count = 0;
    _start_right_count = 0;
    _command_start_time = 0;
    _has_next_command = false;
    _next_left_pwm = 0;
    _next_right_pwm = 0;
    StraightLineDrive::get_instance().disable();

    // Apply brakes after clearing state
    if (absolute_brake) {
        brake_both_motors();
    } else {
        brake_if_moving();
    }
}
