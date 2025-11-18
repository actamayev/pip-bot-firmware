#include "bytecode_vm.h"

#include <math.h>

#include <algorithm>
#include <cmath>

// Static mapping of opcodes to required sensors
const std::map<BytecodeOpCode, std::vector<BytecodeVM::SensorType>> BytecodeVM::opcodeToSensors = {
    // This map is to control what sensors need to be polled for various OpCodes
    // Ie: For motor turn, we need to poll encoders, and quaternion
    {OP_MOTOR_GO, {SENSOR_QUATERNION}},
    {OP_MOTOR_GO_TIME, {SENSOR_QUATERNION}},
    {OP_MOTOR_GO_DISTANCE, {SENSOR_QUATERNION}},
    {OP_MOTOR_TURN, {SENSOR_QUATERNION}}};

BytecodeVM::BytecodeVM() {
    _programMutex = xSemaphoreCreateMutex();
    if (_programMutex == nullptr) {
        SerialQueueManager::get_instance().queue_message("Failed to create BytecodeVM mutex");
    }
}

BytecodeVM::~BytecodeVM() {
    reset_state_variables(true);
    if (_programMutex != nullptr) {
        vSemaphoreDelete(_programMutex);
        _programMutex = nullptr;
    }
}

bool BytecodeVM::load_program(const uint8_t* byte_code, uint16_t size) {
    if (_programMutex == nullptr) {
        return false;
    }

    // Acquire mutex with timeout to prevent deadlock
    if (xSemaphoreTake(_programMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("load_program: Failed to acquire mutex");
        return false;
    }

    // Free any existing program (internal call - mutex already held)
    reset_state_variables(true);

    // Validate bytecode size (must be multiple of 20 now)
    if (size % INSTRUCTION_SIZE != 0 || size / INSTRUCTION_SIZE > MAX_PROGRAM_SIZE) {
        xSemaphoreGive(_programMutex);
        return false;
    }

    _programSize = size / INSTRUCTION_SIZE;
    _program = new (std::nothrow) BytecodeInstruction[_programSize];
    if (!_program) {
        xSemaphoreGive(_programMutex);
        return false;
    }

    // Iterate through program indices (0 to programSize-1)
    for (uint16_t i = 0; i < _programSize; i++) {
        uint16_t offset = i * INSTRUCTION_SIZE;

        // Read opcode (as float but cast to enum)
        float opcode_float = NAN;
        memcpy(&opcode_float, &byte_code[offset], sizeof(float));
        _program[i].opcode = static_cast<BytecodeOpCode>(static_cast<uint32_t>(opcode_float));

        // Read float operands - direct memory copy to preserve exact bit pattern
        memcpy(&_program[i].operand1, &byte_code[offset + 4], sizeof(float));
        memcpy(&_program[i].operand2, &byte_code[offset + 8], sizeof(float));
        memcpy(&_program[i].operand3, &byte_code[offset + 12], sizeof(float));
        memcpy(&_program[i].operand4, &byte_code[offset + 16], sizeof(float));
    }

    // Check if the first instruction is OP_WAIT_FOR_BUTTON
    if (_programSize > 0 && _program[0].opcode == OP_WAIT_FOR_BUTTON) {
        // Program has a start button - set to waiting state
        _isPaused = PROGRAM_NOT_STARTED;
        _waitingForButtonPressToStart = true;
    } else {
        // Program has no start button - set to auto-running
        _isPaused = RUNNING;
        _waitingForButtonPressToStart = false;
    }

    scan_program_for_motors();
    activate_sensors_for_program(); // Activate sensors needed by the program
    _stoppedDueToUsbSafety = false; // Reset safety flag on new program load

    xSemaphoreGive(_programMutex);
    return true;
}

void BytecodeVM::update() {
    if (_programMutex == nullptr) {
        return;
    }

    // Try to acquire mutex without blocking to avoid delays in real-time execution
    if (xSemaphoreTake(_programMutex, 0) != pdTRUE) {
        return; // Skip this update cycle if mutex is locked
    }

    check_usb_safety_conditions();
    if (!_program || _isPaused == PauseState::PAUSED || _isPaused == PauseState::PROGRAM_FINISHED) {
        xSemaphoreGive(_programMutex);
        return;
    }

    // Check if program has naturally completed (pc reached or exceeded program size)
    if (_pc >= _programSize) {
        if (_isPaused != PROGRAM_FINISHED) {
            _isPaused = PROGRAM_FINISHED;
            reset_state_variables(false);
        }
        xSemaphoreGive(_programMutex);
        return;
    }

    // Check if we're waiting for a delay to complete
    if (_waitingForDelay) {
        if (millis() < _delayUntil) {
            xSemaphoreGive(_programMutex);
            return; // Still waiting
        }
        _waitingForDelay = false;
    }

    if (_timedMotorMovementInProgress) {
        update_timed_motor_movement();
        xSemaphoreGive(_programMutex);
        return; // Don't execute next instruction until movement is complete
    }

    // Handle turning operation if in progress
    if (TurningManager::get_instance().is_active()) {
        TurningManager::get_instance().update();
        xSemaphoreGive(_programMutex);
        return; // Don't execute next instruction until turn is complete
    }

    if (_distanceMovementInProgress) {
        update_distance_movement();
        xSemaphoreGive(_programMutex);
        return; // Don't execute next instruction until movement is complete
    }

    // Execute current instruction
    execute_instruction(_program[_pc]);
    if (!_waitingForButtonPressToStart) {
        _pc++; // Move to next instruction
    }

    xSemaphoreGive(_programMutex);
}

bool BytecodeVM::compare_values(ComparisonOp op, float left_operand, float right_operand) {
    float left_value = NAN;
    float right_value = NAN;

    // Process left operand - check if high bit is set, indicating a register
    if (left_operand < 32768.0f) {
        // Direct value
        left_value = left_operand;
    } else {
        const uint16_t REG_ID = static_cast<uint16_t>(left_operand) & 0x7FFF;

        if (REG_ID < MAX_REGISTERS && _registerInitialized[REG_ID]) {
            // Get value from register
            if (_registerTypes[REG_ID] == VAR_FLOAT) {
                left_value = _registers[REG_ID].asFloat;
            } else if (_registerTypes[REG_ID] == VAR_INT) {
                left_value = _registers[REG_ID].asInt;
            } else {
                left_value = _registers[REG_ID].asBool ? 1.0f : 0.0f;
            }
        } else {
            return false; // Invalid register
        }
    }

    // Process right operand - check if high bit is set, indicating a register
    if (right_operand < 32768.0f) {
        // Direct value
        right_value = right_operand;
    } else {
        const uint16_t REG_ID = static_cast<uint16_t>(right_operand) & 0x7FFF;

        if (REG_ID < MAX_REGISTERS && _registerInitialized[REG_ID]) {
            // Get value from register
            if (_registerTypes[REG_ID] == VAR_FLOAT) {
                right_value = _registers[REG_ID].asFloat;
            } else if (_registerTypes[REG_ID] == VAR_INT) {
                right_value = _registers[REG_ID].asInt;
            } else {
                right_value = _registers[REG_ID].asBool ? 1.0f : 0.0f;
            }
        } else {
            return false; // Invalid register
        }
    }

    // Perform comparison with retrieved values
    switch (op) {
        case OP_EQUAL:
            return abs(left_value - right_value) < 0.0001f;
        case OP_NOT_EQUAL:
            return left_value != right_value;
        case OP_GREATER_THAN:
            return left_value > right_value;
        case OP_LESS_THAN:
            return left_value < right_value;
        case OP_GREATER_EQUAL:
            return left_value >= right_value;
        case OP_LESS_EQUAL:
            return left_value <= right_value;
        default:
            return false;
    }
}

void BytecodeVM::execute_instruction(const BytecodeInstruction& instr) {
    switch (instr.opcode) {
        case OP_NOP:
            // No operation, do nothing
            break;

        case OP_END:
            // End program execution
            _pc = _programSize; // Set PC past the end to stop execution
            _isPaused = PROGRAM_FINISHED;
            break;

        case OP_WAIT: {
            // Delay execution for specified milliseconds
            // This converts from seconds (ie. 1.5s into milliseconds).
            // delay_ms can be uint32_t since it will always be
            auto delay_ms = static_cast<uint32_t>(instr.operand1 * 1000.0f);
            _delayUntil = millis() + delay_ms;
            _waitingForDelay = true;
            break;
        }

        case OP_READ_SENSOR: {
            auto sensor_type = static_cast<BytecodeSensorType>(instr.operand1); // Which sensor to read
            uint16_t reg_id = instr.operand2;                                   // Register to store result

            if (reg_id < MAX_REGISTERS) {
                float value = 0.0f;
                bool skip_default_assignment = false; // Add this flag

                // Read the appropriate sensor
                switch (sensor_type) {
                    case SENSOR_PITCH:
                        value = SensorDataBuffer::get_instance().get_latest_pitch();
                        break;
                    case SENSOR_ROLL:
                        value = SensorDataBuffer::get_instance().get_latest_roll();
                        break;
                    case SENSOR_YAW:
                        value = SensorDataBuffer::get_instance().get_latest_yaw();
                        break;
                    case SENSOR_ACCEL_X:
                        value = SensorDataBuffer::get_instance().get_latest_x_accel();
                        break;
                    case SENSOR_ACCEL_Y:
                        value = SensorDataBuffer::get_instance().get_latest_y_accel();
                        break;
                    case SENSOR_ACCEL_Z:
                        value = SensorDataBuffer::get_instance().get_latest_z_accel();
                        break;
                    case SENSOR_ACCEL_MAG:
                        value = SensorDataBuffer::get_instance().get_latest_accel_magnitude();
                        break;
                    case SENSOR_ROT_RATE_X:
                        value = SensorDataBuffer::get_instance().get_latest_x_rotation_rate();
                        break;
                    case SENSOR_ROT_RATE_Y:
                        value = SensorDataBuffer::get_instance().get_latest_y_rotation_rate();
                        break;
                    case SENSOR_ROT_RATE_Z:
                        value = SensorDataBuffer::get_instance().get_latest_z_rotation_rate();
                        break;
                    case SENSOR_MAG_FIELD_X:
                        value = SensorDataBuffer::get_instance().get_latest_magnetic_field_x();
                        break;
                    case SENSOR_MAG_FIELD_Y:
                        value = SensorDataBuffer::get_instance().get_latest_magnetic_field_y();
                        break;
                    case SENSOR_MAG_FIELD_Z:
                        value = SensorDataBuffer::get_instance().get_latest_magnetic_field_z();
                        break;
                    case SENSOR_SIDE_LEFT_PROXIMITY: {
                        uint16_t counts = SensorDataBuffer::get_instance().get_latest_left_side_tof_counts();
                        _registers[reg_id].asBool = (counts > LEFT_PROXIMITY_THRESHOLD);
                        _registerTypes[reg_id] = VAR_BOOL;
                        _registerInitialized[reg_id] = true;
                        skip_default_assignment = true; // Set flag to skip default assignment
                        break;
                    }
                    case SENSOR_SIDE_RIGHT_PROXIMITY: {
                        uint16_t counts = SensorDataBuffer::get_instance().get_latest_right_side_tof_counts();
                        _registers[reg_id].asBool = (counts > RIGHT_PROXIMITY_THRESHOLD);
                        _registerTypes[reg_id] = VAR_BOOL;
                        _registerInitialized[reg_id] = true;
                        skip_default_assignment = true; // Set flag to skip default assignment
                        break;
                    }
                    case SENSOR_FRONT_PROXIMITY: {
                        bool is_object_detected = SensorDataBuffer::get_instance().is_object_detected_tof();
                        _registers[reg_id].asBool = is_object_detected;
                        _registerTypes[reg_id] = VAR_BOOL;
                        _registerInitialized[reg_id] = true;
                        skip_default_assignment = true; // Set flag
                        break;
                    }
                    case SENSOR_COLOR_RED: {
                        bool is_red = SensorDataBuffer::get_instance().is_object_red();
                        _registers[reg_id].asBool = is_red;
                        _registerTypes[reg_id] = VAR_BOOL;
                        _registerInitialized[reg_id] = true;
                        skip_default_assignment = true;
                        break;
                    }
                    case SENSOR_COLOR_GREEN: {
                        bool is_green = SensorDataBuffer::get_instance().is_object_green();
                        _registers[reg_id].asBool = is_green;
                        _registerTypes[reg_id] = VAR_BOOL;
                        _registerInitialized[reg_id] = true;
                        skip_default_assignment = true;
                        break;
                    }
                    case SENSOR_COLOR_BLUE: {
                        bool is_blue = SensorDataBuffer::get_instance().is_object_blue();
                        _registers[reg_id].asBool = is_blue;
                        _registerTypes[reg_id] = VAR_BOOL;
                        _registerInitialized[reg_id] = true;
                        skip_default_assignment = true;
                        break;
                    }
                    case SENSOR_COLOR_WHITE: {
                        bool is_white = SensorDataBuffer::get_instance().is_object_white();
                        _registers[reg_id].asBool = is_white;
                        _registerTypes[reg_id] = VAR_BOOL;
                        _registerInitialized[reg_id] = true;
                        skip_default_assignment = true;
                        break;
                    }
                    case SENSOR_COLOR_BLACK: {
                        bool is_black = SensorDataBuffer::get_instance().is_object_black();
                        _registers[reg_id].asBool = is_black;
                        _registerTypes[reg_id] = VAR_BOOL;
                        _registerInitialized[reg_id] = true;
                        skip_default_assignment = true;
                        break;
                    }
                    case SENSOR_COLOR_YELLOW: {
                        bool is_yellow = SensorDataBuffer::get_instance().is_object_yellow();
                        _registers[reg_id].asBool = is_yellow;
                        _registerTypes[reg_id] = VAR_BOOL;
                        _registerInitialized[reg_id] = true;
                        skip_default_assignment = true;
                        break;
                    }
                    case FRONT_TOF_DISTANCE: {
                        float front_distance = SensorDataBuffer::get_instance().get_front_tof_distance();
                        value = (front_distance < 0) ? 999.0f : front_distance; // Return 999 inches if no valid reading
                        break;
                    }
                    default: {
                        char log_message[32];
                        snprintf(log_message, sizeof(log_message), "Unknown sensor type: %u", sensor_type);
                        SerialQueueManager::get_instance().queue_message(log_message);
                        break;
                    }
                }

                if (!skip_default_assignment) {
                    _registers[reg_id].asFloat = value;
                    _registerTypes[reg_id] = VAR_FLOAT;
                    _registerInitialized[reg_id] = true;
                }
            }
            break;
        }

        case OP_SET_ALL_LEDS: {
            auto r = static_cast<uint8_t>(instr.operand1); // Cast to uint8_t for
            auto g = static_cast<uint8_t>(instr.operand2); // RGB values (0-255)
            auto b = static_cast<uint8_t>(instr.operand3);

            // operand4 is unused for this operation
            rgb_led.set_main_board_leds_to_color(r, g, b);
            break;
        }

        case OP_COMPARE: {
            // Compare two values and store the result
            auto op = static_cast<ComparisonOp>(static_cast<uint8_t>(instr.operand1));
            float left_value = instr.operand2;
            float right_value = instr.operand3;

            _lastComparisonResult = compare_values(op, left_value, right_value);

            break;
        }

        case OP_JUMP: {
            // Unconditional jump
            auto low = static_cast<uint8_t>(instr.operand1);
            auto high = static_cast<uint8_t>(instr.operand2);
            uint16_t jump_offset = (high << 8) | low;                             // Combine high and low bytes into 16-bit offset
            uint16_t target_instruction = _pc + (jump_offset / INSTRUCTION_SIZE); // 20 bytes per instruction
            _pc = target_instruction - 1;                                         // Subtract 1 because pc increments after this
            break;
        }

        case OP_JUMP_BACKWARD: {
            // Backward jump (used in for loops)
            auto low = static_cast<uint8_t>(instr.operand1);
            auto high = static_cast<uint8_t>(instr.operand2);
            uint16_t jump_offset = (high << 8) | low;                             // Combine high and low bytes into 16-bit offset
            uint16_t target_instruction = _pc - (jump_offset / INSTRUCTION_SIZE); // Subtract for backward jump
            _pc = target_instruction - 1;                                         // Subtract 1 because pc increments after
            break;
        }

        case OP_JUMP_IF_TRUE: {
            // Conditional jump if last comparison was true
            if (_lastComparisonResult) {
                auto low = static_cast<uint8_t>(instr.operand1);
                auto high = static_cast<uint8_t>(instr.operand2);
                uint16_t jump_offset = (high << 8) | low;                             // Combine high and low bytes into 16-bit offset
                uint16_t target_instruction = _pc + (jump_offset / INSTRUCTION_SIZE); // 20 bytes per instruction
                _pc = target_instruction - 1;                                         // Subtract 1 because pc increments after
            }
            break;
        }

        case OP_JUMP_IF_FALSE: {
            // Conditional jump if last comparison was false
            if (!_lastComparisonResult) {
                auto low = static_cast<uint8_t>(instr.operand1);
                auto high = static_cast<uint8_t>(instr.operand2);
                uint16_t jump_offset = (high << 8) | low;                             // Combine high and low bytes into 16-bit offset
                uint16_t target_instruction = _pc + (jump_offset / INSTRUCTION_SIZE); // 20 bytes per instruction
                _pc = target_instruction - 1;                                         // Subtract 1 because pc increments after
            }
            break;
        }

        case OP_DECLARE_VAR: {
            uint16_t reg_id = instr.operand1;
            auto type = (BytecodeVarType)instr.operand2;

            if (reg_id < MAX_REGISTERS) {
                _registerTypes[reg_id] = type;
                // Initialize with default values
                switch (type) {
                    case VAR_FLOAT:
                        _registers[reg_id].asFloat = 0.0f;
                        break;
                    case VAR_INT:
                        _registers[reg_id].asInt = 0;
                        break;
                    case VAR_BOOL:
                        _registers[reg_id].asBool = false;
                        break;
                }
                _registerInitialized[reg_id] = true;
            }
            break;
        }

        case OP_SET_VAR: {
            auto reg_id = static_cast<uint16_t>(instr.operand1);

            if (reg_id < MAX_REGISTERS) {
                switch (_registerTypes[reg_id]) {
                    case VAR_FLOAT: {
                        // Direct assignment - no reconstruction needed
                        _registers[reg_id].asFloat = instr.operand2;
                        break;
                    }
                    case VAR_INT: {
                        // Cast float to int directly
                        _registers[reg_id].asInt = static_cast<int32_t>(instr.operand2);
                        break;
                    }
                    case VAR_BOOL:
                        // Cast float to bool (non-zero = true)
                        _registers[reg_id].asBool = (instr.operand2 != 0.0f);
                        break;
                }
                _registerInitialized[reg_id] = true;
            }
            break;
        }
        case OP_WHILE_START: {
            // This is just a marker for the start of the loop
            // No special action needed
            break;
        }

        case OP_WHILE_END: {
            auto low = static_cast<uint8_t>(instr.operand1);
            auto high = static_cast<uint8_t>(instr.operand2);
            uint16_t offset_to_start = (high << 8) | low;
            if (offset_to_start <= _pc * INSTRUCTION_SIZE) {
                _pc = _pc - (offset_to_start / INSTRUCTION_SIZE);
            } else {
                _pc = _programSize;
                SerialQueueManager::get_instance().queue_message("Invalid loop jump - stopping execution");
            }
            break;
        }

        case OP_FOR_INIT: {
            // Initialize loop counter
            auto reg_id = static_cast<uint16_t>(instr.operand1);

            if (reg_id < MAX_REGISTERS) {
                _registerTypes[reg_id] = VAR_INT;

                // Cast the float value to int
                _registers[reg_id].asInt = static_cast<int32_t>(instr.operand2);
                _registerInitialized[reg_id] = true;
            }
            break;
        }

        case OP_FOR_CONDITION: {
            // Check if counter < end value
            auto reg_id = static_cast<uint16_t>(instr.operand1);

            if (reg_id < MAX_REGISTERS && _registerInitialized[reg_id]) {
                // Cast the float value to int
                auto end_value = static_cast<int32_t>(instr.operand2);

                // Compare counter with end value
                _lastComparisonResult = (_registers[reg_id].asInt < end_value);
            } else {
                // Invalid register, exit loop
                _lastComparisonResult = false;
            }
            break;
        }

        case OP_FOR_INCREMENT: {
            // Increment counter by 1
            auto reg_id = static_cast<uint16_t>(instr.operand1);

            if (reg_id < MAX_REGISTERS && _registerInitialized[reg_id]) {
                _registers[reg_id].asInt++;
            }
            break;
        }

        case OP_MOTOR_GO: {
            // operand1: direction (0=backward, 1=forward)
            // operand2: throttle percentage (0-100)
            bool is_forward = (instr.operand1 > 0.5f); // > 0.5 to handle float precision
            uint8_t throttle_percent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);
            uint16_t motor_speed = map(throttle_percent, 0, 100, 0, MAX_MOTOR_PWM);

            // Set both motors based on direction
            if (is_forward) {
                motor_driver.update_motor_pwm(motor_speed, motor_speed);
            } else {
                motor_driver.update_motor_pwm(-motor_speed, -motor_speed);
            }
            break;
        }

        case OP_MOTOR_STOP: {
            motor_driver.reset_command_state(true);
            break;
        }

        case MOTOR_SPIN: {
            // operand1: direction (0=counterclockwise, 1=clockwise)
            // operand2: speed percentage (0-100)
            bool clockwise = (instr.operand1 > 0.5f); // > 0.5 to handle float precision
            uint8_t speed_percent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);

            uint16_t motor_speed = 0;
            if (speed_percent == 0) {
                motor_speed = 0; // Stop motors completely at 0%
            } else {
                // Custom formula for speeds 1-100: ((MAX_SPIN_PWM - MIN_SPIN_PWM) / 100) * speed + MIN_SPIN_PWM
                motor_speed = ((MAX_SPIN_PWM - MIN_SPIN_PWM) * speed_percent / 100) + MIN_SPIN_PWM;
            }

            // Spin motors in opposite directions
            if (clockwise) {
                // Clockwise: left motor forward, right motor backward
                motor_driver.update_motor_pwm(motor_speed, -motor_speed);
            } else {
                // Counterclockwise: left motor backward, right motor forward
                motor_driver.update_motor_pwm(-motor_speed, motor_speed);
            }
            break;
        }

        case OP_MOTOR_TURN: {
            bool clockwise = (instr.operand1 > 0);
            float degrees = instr.operand2;

            // Use TurningManager for precise turning
            float signed_degrees = clockwise ? degrees : -degrees;
            if (!TurningManager::get_instance().start_turn(signed_degrees)) {
                SerialQueueManager::get_instance().queue_message("Failed to start turn - turn already in progress");
            }

            // The actual turn progress will be monitored in update()
            break;
        }

        case OP_MOTOR_GO_TIME: {
            // operand1: direction (0=backward, 1=forward)
            // operand2: time in seconds
            // operand3: throttle percentage (0-100)
            bool is_forward = (instr.operand1 > 0.5f); // > 0.5 to handle float precision
            float seconds = instr.operand2;
            uint8_t throttle_percent = constrain(static_cast<uint8_t>(instr.operand3), 0, 100);

            // Validate parameters
            if (seconds <= 0.0f) {
                SerialQueueManager::get_instance().queue_message("Invalid time value for timed movement");
                break;
            }

            throttle_percent = std::min<uint8_t>(throttle_percent, 100);

            // Convert percentage to motor speed
            uint16_t motor_speed = map(throttle_percent, 0, 100, 0, MAX_MOTOR_PWM);

            // Set motors based on direction
            if (is_forward) {
                motor_driver.update_motor_pwm(motor_speed, motor_speed);
            } else {
                motor_driver.update_motor_pwm(-motor_speed, -motor_speed);
            }

            // Set up timed movement
            _timedMotorMovementInProgress = true;
            _motorMovementEndTime = millis() + static_cast<uint32_t>(seconds * 1000.0f);

            break;
        }

        case OP_MOTOR_GO_DISTANCE: {
            // operand1: direction (0=backward, 1=forward)
            // operand2: distance in inches
            // operand3: throttle percentage (0-100)
            bool is_forward = (instr.operand1 > 0.5f); // > 0.5 to handle float precision
            float distance_in = instr.operand2;
            uint8_t throttle_percent = constrain(static_cast<uint8_t>(instr.operand3), 0, 100);

            // Validate parameters
            if (distance_in <= 0.0f) {
                SerialQueueManager::get_instance().queue_message("Invalid distance value for distance movement");
                break;
            }

            throttle_percent = std::min<uint8_t>(throttle_percent, 100);

            // Convert percentage to motor speed
            uint16_t motor_speed = map(throttle_percent, 0, 100, 0, MAX_MOTOR_PWM);

            // Reset distance tracking - store current distance as starting point
            _startingDistanceIn = SensorDataBuffer::get_instance().get_latest_distance_traveled_in();

            // Set up distance movement
            _distanceMovementInProgress = true;
            _targetDistanceIn = distance_in;

            // Set motors based on direction and store initial PWM with correct sign
            if (is_forward) {
                _initialDistancePwm = motor_speed;
                motor_driver.update_motor_pwm(motor_speed, motor_speed);
            } else {
                _initialDistancePwm = -motor_speed; // Store as negative for backward
                motor_driver.update_motor_pwm(-motor_speed, -motor_speed);
            }

            break;
        }

        case OP_WAIT_FOR_BUTTON: {
            _waitingForButtonPressToStart = true;

            // Don't increment PC here - we'll do it when the button is pressed
            return; // Return without incrementing PC
        }

        case CHECK_RIGHT_BUTTON_PRESS: {
            uint16_t reg_id = instr.operand1; // Register to store result

            if (reg_id < MAX_REGISTERS) {
                bool is_pressed = Buttons::get_instance().is_right_button_pressed();

                _registers[reg_id].asBool = is_pressed;
                _registerTypes[reg_id] = VAR_BOOL;
                _registerInitialized[reg_id] = true;
            }
            break;
        }

        case PLAY_TONE: {
            auto tone_value = static_cast<uint8_t>(instr.operand1);

            // ADD THESE DEBUG LINES:
            SerialQueueManager::get_instance().queue_message("PLAY_TONE opcode hit with value: " + String(tone_value));

            // tone_value = 0 means stop, 1-7 are valid tones
            if (tone_value <= 7) {
                auto tone_type = static_cast<ToneType>(tone_value);
                Speaker::get_instance().play_tone(tone_type);
            } else if (tone_value == 8) {
                Speaker::get_instance().stop_tone();
            } else {
                SerialQueueManager::get_instance().queue_message("Invalid tone value: " + String(tone_value));
            }
            break;
        }

        default:
            // Unknown opcode, stop execution
            _pc = _programSize;
            break;
    }
}

void BytecodeVM::update_timed_motor_movement() {
    // Check if the timed movement has completed
    if (millis() < _motorMovementEndTime) {
        return;
    }
    // Movement complete - brake motors
    motor_driver.reset_command_state(true);

    // Reset timed movement state
    _timedMotorMovementInProgress = false;
}

void BytecodeVM::update_distance_movement() {
    // Get distance traveled from sensor data buffer (relative to starting point)
    float total_distance = SensorDataBuffer::get_instance().get_latest_distance_traveled_in();
    float current_distance = abs(total_distance - _startingDistanceIn);
    float remaining_distance = _targetDistanceIn - current_distance;

    // Check if we've reached the target distance
    if (remaining_distance <= 0.0f) {
        // Distance reached - brake motors and clear any pending commands
        motor_driver.reset_command_state(true);

        // Reset distance movement state
        _distanceMovementInProgress = false;
        _targetDistanceIn = 0.0f;
        _startingDistanceIn = 0.0f;
        _initialDistancePwm = 0;
        vTaskDelay(pdMS_TO_TICKS(250));
        return;
    }

    // Calculate braking distance using physics equation
    int16_t abs_pwm = abs(_initialDistancePwm);
    float braking_distance = (abs_pwm * abs_pwm - MIN_DECELERATION_PWM * MIN_DECELERATION_PWM) / (2.0f * DECELERATION_RATE);
    SerialQueueManager::get_instance().queue_message("brakingDistance" + String(braking_distance));

    // Ensure braking distance doesn't exceed total distance (edge case for short distances)
    if (braking_distance > _targetDistanceIn) {
        braking_distance = _targetDistanceIn;
    }

    // Determine target PWM based on remaining distance
    int16_t target_pwm = 0;

    if (remaining_distance <= braking_distance) {
        // We're in the deceleration zone - apply inverse sigmoid
        float normalized_position = remaining_distance / braking_distance; // 1.0 at start of braking, 0.0 at end

        // Inverse sigmoid: gentle at start, aggressive near end
        float k = 10.0f / braking_distance; // Steepness factor
        float sigmoid_value = 1.0f / (1.0f + exp(k * (braking_distance / 2.0f - remaining_distance)));

        // Calculate PWM using sigmoid curve
        target_pwm = MIN_DECELERATION_PWM + ((abs_pwm - MIN_DECELERATION_PWM) * sigmoid_value);

        // Maintain direction (forward/backward)
        if (_initialDistancePwm < 0) {
            target_pwm = -target_pwm;
        }

        SerialQueueManager::get_instance().queue_message("sigmoidValue" + String(sigmoid_value));
        SerialQueueManager::get_instance().queue_message("targetPwm" + String(target_pwm));

        // Set motor speeds directly without ramping
        motor_driver.set_motor_speeds(target_pwm, target_pwm, false);
    }
    // If not in deceleration zone, continue at initial speed (StraightLineDrive will handle heading)
}

void BytecodeVM::stop_program() {
    if (_programMutex == nullptr) {
        return;
    }

    // Acquire mutex with timeout
    if (xSemaphoreTake(_programMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("stop_program: Failed to acquire mutex");
        return;
    }

    reset_state_variables(true);
    xSemaphoreGive(_programMutex);
}

void BytecodeVM::reset_state_variables(bool is_full_reset) {
    _pc = 0;
    _delayUntil = 0;
    _waitingForDelay = false;
    _lastComparisonResult = false;

    // Reset TurningManager state
    TurningManager::get_instance().complete_navigation();
    _timedMotorMovementInProgress = false;
    _distanceMovementInProgress = false;
    _initialDistancePwm = 0;
    _motorMovementEndTime = 0;
    _targetDistanceIn = 0.0f;
    _startingDistanceIn = 0.0f;
    _waitingForButtonPressToStart = false;

    // ADD THESE USB safety resets:
    _stoppedDueToUsbSafety = false;
    // Note: Don't reset _programContainsMotors or _lastUsbState here as they persist across pause/resume

    // Force reset motor driver state completely
    motor_driver.reset_command_state(false);

    // Reset registers
    for (uint16_t i = 0; i < MAX_REGISTERS; i++) {
        _registerInitialized[i] = false;
        _registers[i].asFloat = 0.0f;
        _registers[i].asInt = 0;
        _registers[i].asBool = false;
    }
    for (uint16_t i = 0; i < MAX_REGISTERS; i++) {
        _registerTypes[i] = VAR_FLOAT;
    }
    for (uint16_t i = 0; i < MAX_REGISTERS; i++) {
        _registerInitialized[i] = false;
    }
    if (is_full_reset) {
        delete[] _program;
        _program = nullptr;
        _isPaused = PROGRAM_NOT_STARTED;
        _programSize = 0;

        // ADD THESE for full reset:
        _programContainsMotors = false;
        _lastUsbState = false;
    }
    rgb_led.turn_all_leds_off();
    Speaker::get_instance().stop_all_sounds();
    SensorDataBuffer::get_instance().stop_polling_all_sensors();
}

void BytecodeVM::pause_program() {
    if (_programMutex == nullptr) {
        return;
    }

    if (xSemaphoreTake(_programMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("pause_program: Failed to acquire mutex");
        return;
    }

    if (!_program || _isPaused == PAUSED) {
        xSemaphoreGive(_programMutex);
        return;
    }

    reset_state_variables();
    _isPaused = PAUSED;
    xSemaphoreGive(_programMutex);
}

void BytecodeVM::resume_program() {
    if (_programMutex == nullptr) {
        return;
    }

    if (xSemaphoreTake(_programMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("resume_program: Failed to acquire mutex");
        return;
    }

    if (!_program || _isPaused == RUNNING) {
        SerialQueueManager::get_instance().queue_message("resume_program: Not paused or no program");
        xSemaphoreGive(_programMutex);
        return;
    }

    if (!can_start_program()) {
        xSemaphoreGive(_programMutex);
        return;
    }

    // Reset state variables for finished programs to start fresh
    if (_isPaused == PROGRAM_FINISHED) {
        reset_state_variables();
    }

    _isPaused = RUNNING;

    // Check if the first instruction is a WAIT_FOR_BUTTON (start block)
    if (_programSize > 0 && _program[0].opcode == OP_WAIT_FOR_BUTTON) {
        SerialQueueManager::get_instance().queue_message("Resuming program - skipping initial wait for button");
        _pc = 1;                               // Start after the wait for button instruction
        _waitingForButtonPressToStart = false; // ← FIX: Clear the flag!
    } else {
        SerialQueueManager::get_instance().queue_message("Resuming program from beginning");
        _pc = 0;                               // Start from the beginning for scripts without a start block
        _waitingForButtonPressToStart = false; // ← FIX: Clear here too for consistency
    }

    xSemaphoreGive(_programMutex);
}

void BytecodeVM::activate_sensors_for_program() {
    if (!_program || _programSize == 0) {
        return;
    }

    // Track which sensors are needed
    bool need_quaternion = false;
    bool need_accelerometer = false;
    bool need_gyroscope = false;
    bool need_magnetometer = false;
    bool need_tof = false;
    bool need_side_tof = false;
    bool need_color_sensor = false;

    // Scan through the entire program
    for (uint16_t i = 0; i < _programSize; i++) {
        const BytecodeInstruction& instr = _program[i];

        // Handle OP_READ_SENSOR dynamically based on sensor type
        if (instr.opcode != OP_READ_SENSOR) {
            // Handle other opcodes using the static mapping
            auto it = opcodeToSensors.find(instr.opcode);
            if (it != opcodeToSensors.end()) {
                for (SensorType sensorType : it->second) {
                    switch (sensorType) {
                        case SENSOR_QUATERNION:
                            need_quaternion = true;
                            break;
                        case SENSOR_ACCELEROMETER:
                            need_accelerometer = true;
                            break;
                        case SENSOR_GYROSCOPE:
                            need_gyroscope = true;
                            break;
                        case SENSOR_MAGNETOMETER:
                            need_magnetometer = true;
                            break;
                        case SENSOR_TOF:
                            need_tof = true;
                            break;
                        case SENSOR_SIDE_TOF:
                            need_side_tof = true;
                            break;
                    }
                }
            }
        } else {
            auto sensor_type = static_cast<BytecodeSensorType>(instr.operand1);

            switch (sensor_type) {
                case SENSOR_PITCH:
                case SENSOR_ROLL:
                case SENSOR_YAW:
                    need_quaternion = true;
                    break;
                case SENSOR_ACCEL_X:
                case SENSOR_ACCEL_Y:
                case SENSOR_ACCEL_Z:
                case SENSOR_ACCEL_MAG:
                    need_accelerometer = true;
                    break;
                case SENSOR_ROT_RATE_X:
                case SENSOR_ROT_RATE_Y:
                case SENSOR_ROT_RATE_Z:
                    need_gyroscope = true;
                    break;
                case SENSOR_MAG_FIELD_X:
                case SENSOR_MAG_FIELD_Y:
                case SENSOR_MAG_FIELD_Z:
                    need_magnetometer = true;
                    break;
                case SENSOR_SIDE_LEFT_PROXIMITY:
                case SENSOR_SIDE_RIGHT_PROXIMITY:
                    need_side_tof = true;
                    break;
                case SENSOR_FRONT_PROXIMITY:
                case FRONT_TOF_DISTANCE:
                    need_tof = true;
                    break;
                case SENSOR_COLOR_RED:
                case SENSOR_COLOR_BLACK:
                case SENSOR_COLOR_BLUE:
                case SENSOR_COLOR_GREEN:
                case SENSOR_COLOR_WHITE:
                case SENSOR_COLOR_YELLOW:
                    need_color_sensor = true;
            }
        }
    }

    // Activate required sensors by setting their last_request timestamps
    SensorDataBuffer& buffer = SensorDataBuffer::get_instance();
    ReportTimeouts& timeouts = buffer.get_report_timeouts();
    const uint32_t CURRENT_TIME = millis();

    if (need_quaternion) {
        timeouts.quaternion_last_request.store(CURRENT_TIME);
        SerialQueueManager::get_instance().queue_message("Activated quaternion sensor for program");
    }
    if (need_accelerometer) {
        timeouts.accelerometer_last_request.store(CURRENT_TIME);
        SerialQueueManager::get_instance().queue_message("Activated accelerometer for program");
    }
    if (need_gyroscope) {
        timeouts.gyroscope_last_request.store(CURRENT_TIME);
        SerialQueueManager::get_instance().queue_message("Activated gyroscope for program");
    }
    if (need_magnetometer) {
        timeouts.magnetometer_last_request.store(CURRENT_TIME);
        SerialQueueManager::get_instance().queue_message("Activated magnetometer for program");
    }
    if (need_tof) {
        timeouts.tof_last_request.store(CURRENT_TIME);
        SerialQueueManager::get_instance().queue_message("Activated multizone TOF for program");
    }
    if (need_side_tof) {
        timeouts.side_tof_last_request.store(CURRENT_TIME);
        SerialQueueManager::get_instance().queue_message("Activated side TOF for program");
    }
    if (need_color_sensor) {
        timeouts.color_last_request.store(CURRENT_TIME);
        SerialQueueManager::get_instance().queue_message("Activated colors Sensors for program");
    }
}

void BytecodeVM::scan_program_for_motors() {
    _programContainsMotors = false;

    if (!_program || _programSize == 0) {
        return;
    }

    // Define all motor opcodes that should block USB execution
    const BytecodeOpCode MOTOR_OPCODES[] = {OP_MOTOR_GO, OP_MOTOR_STOP, OP_MOTOR_TURN, OP_MOTOR_GO_TIME, OP_MOTOR_GO_DISTANCE, MOTOR_SPIN};

    // Scan entire program for any motor commands
    for (uint16_t i = 0; i < _programSize; i++) {
        for (const auto& motor_opcode : MOTOR_OPCODES) {
            if (_program[i].opcode == motor_opcode) {
                _programContainsMotors = true;
                return;
            }
        }
    }
}

void BytecodeVM::check_usb_safety_conditions() {
    bool current_usb_state = SerialManager::get_instance().is_serial_connected();

    // Detect USB connection change (disconnected -> connected)
    if (!_lastUsbState && current_usb_state) {
        handle_usb_connect();
    }

    _lastUsbState = current_usb_state;
}

void BytecodeVM::handle_usb_connect() {
    // If program contains motors and is currently running, stop it
    if (_programContainsMotors && _isPaused == RUNNING) {
        stop_program();
        _stoppedDueToUsbSafety = true;
    }
}

bool BytecodeVM::can_start_program() {
    // Block start if program contains motors and USB is connected
    return !_programContainsMotors || !SerialManager::get_instance().is_serial_connected();
}
