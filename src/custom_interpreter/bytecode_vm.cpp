#include "bytecode_vm.h"

// Static mapping of opcodes to required sensors
const std::map<BytecodeOpCode, std::vector<BytecodeVM::SensorType>> BytecodeVM::opcodeToSensors = {
    // This map is to control what sensors need to be polled for various OpCodes
    // Ie: For motor turn, we need to poll encoders, and quaternion
    {OP_MOTOR_GO, {SENSOR_QUATERNION}},
    {OP_MOTOR_GO_TIME, {SENSOR_QUATERNION}},
    {OP_MOTOR_GO_DISTANCE, {SENSOR_QUATERNION}},
    {OP_MOTOR_TURN, {SENSOR_QUATERNION}}};

BytecodeVM::BytecodeVM() {
    programMutex = xSemaphoreCreateMutex();
    if (programMutex == nullptr) {
        SerialQueueManager::get_instance().queue_message("Failed to create BytecodeVM mutex");
    }
}

BytecodeVM::~BytecodeVM() {
    reset_state_variables(true);
    if (programMutex != nullptr) {
        vSemaphoreDelete(programMutex);
        programMutex = nullptr;
    }
}

bool BytecodeVM::load_program(const uint8_t* byteCode, uint16_t size) {
    if (programMutex == nullptr) return false;

    // Acquire mutex with timeout to prevent deadlock
    if (xSemaphoreTake(programMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("load_program: Failed to acquire mutex");
        return false;
    }

    // Free any existing program (internal call - mutex already held)
    reset_state_variables(true);

    // Validate bytecode size (must be multiple of 20 now)
    if (size % INSTRUCTION_SIZE != 0 || size / INSTRUCTION_SIZE > MAX_PROGRAM_SIZE) {
        xSemaphoreGive(programMutex);
        return false;
    }

    programSize = size / INSTRUCTION_SIZE;
    program = new (std::nothrow) BytecodeInstruction[programSize];
    if (!program) {
        xSemaphoreGive(programMutex);
        return false;
    }

    // Iterate through program indices (0 to programSize-1)
    for (uint16_t i = 0; i < programSize; i++) {
        uint16_t offset = i * INSTRUCTION_SIZE;

        // Read opcode (as float but cast to enum)
        float opcodeFloat;
        memcpy(&opcodeFloat, &byteCode[offset], sizeof(float));
        program[i].opcode = static_cast<BytecodeOpCode>(static_cast<uint32_t>(opcodeFloat));

        // Read float operands - direct memory copy to preserve exact bit pattern
        memcpy(&program[i].operand1, &byteCode[offset + 4], sizeof(float));
        memcpy(&program[i].operand2, &byteCode[offset + 8], sizeof(float));
        memcpy(&program[i].operand3, &byteCode[offset + 12], sizeof(float));
        memcpy(&program[i].operand4, &byteCode[offset + 16], sizeof(float));
    }

    // Check if the first instruction is OP_WAIT_FOR_BUTTON
    if (programSize > 0 && program[0].opcode == OP_WAIT_FOR_BUTTON) {
        // Program has a start button - set to waiting state
        isPaused = PROGRAM_NOT_STARTED;
        waitingForButtonPressToStart = true;
    } else {
        // Program has no start button - set to auto-running
        isPaused = RUNNING;
        waitingForButtonPressToStart = false;
    }

    scan_program_for_motors();
    activate_sensors_for_program(); // Activate sensors needed by the program
    stoppedDueToUsbSafety = false;  // Reset safety flag on new program load

    xSemaphoreGive(programMutex);
    return true;
}

void BytecodeVM::update() {
    if (programMutex == nullptr) return;

    // Try to acquire mutex without blocking to avoid delays in real-time execution
    if (xSemaphoreTake(programMutex, 0) != pdTRUE) {
        return; // Skip this update cycle if mutex is locked
    }

    check_usb_safety_conditions();
    if (!program || isPaused == PauseState::PAUSED || isPaused == PauseState::PROGRAM_FINISHED) {
        xSemaphoreGive(programMutex);
        return;
    }

    // Check if program has naturally completed (pc reached or exceeded program size)
    if (pc >= programSize) {
        if (isPaused != PROGRAM_FINISHED) {
            isPaused = PROGRAM_FINISHED;
            reset_state_variables(false);
        }
        xSemaphoreGive(programMutex);
        return;
    }

    // Check if we're waiting for a delay to complete
    if (waitingForDelay) {
        if (millis() < delayUntil) {
            xSemaphoreGive(programMutex);
            return; // Still waiting
        }
        waitingForDelay = false;
    }

    if (timedMotorMovementInProgress) {
        update_timed_motor_movement();
        xSemaphoreGive(programMutex);
        return; // Don't execute next instruction until movement is complete
    }

    // Handle turning operation if in progress
    if (TurningManager::get_instance().is_active()) {
        TurningManager::get_instance().update();
        xSemaphoreGive(programMutex);
        return; // Don't execute next instruction until turn is complete
    }

    if (distanceMovementInProgress) {
        update_distance_movement();
        xSemaphoreGive(programMutex);
        return; // Don't execute next instruction until movement is complete
    }

    // Execute current instruction
    execute_instruction(program[pc]);
    if (!waitingForButtonPressToStart) {
        pc++; // Move to next instruction
    }

    xSemaphoreGive(programMutex);
}

bool BytecodeVM::compare_values(ComparisonOp op, float leftOperand, float rightOperand) {
    float leftValue;
    float rightValue;

    // Process left operand - check if high bit is set, indicating a register
    if (leftOperand < 32768.0f) {
        // Direct value
        leftValue = leftOperand;
    } else {
        uint16_t regId = static_cast<uint16_t>(leftOperand) & 0x7FFF;

        if (regId < MAX_REGISTERS && registerInitialized[regId]) {
            // Get value from register
            if (registerTypes[regId] == VAR_FLOAT) {
                leftValue = registers[regId].asFloat;
            } else if (registerTypes[regId] == VAR_INT) {
                leftValue = registers[regId].asInt;
            } else {
                leftValue = registers[regId].asBool ? 1.0f : 0.0f;
            }
        } else {
            return false; // Invalid register
        }
    }

    // Process right operand - check if high bit is set, indicating a register
    if (rightOperand < 32768.0f) {
        // Direct value
        rightValue = rightOperand;
    } else {
        uint16_t regId = static_cast<uint16_t>(rightOperand) & 0x7FFF;

        if (regId < MAX_REGISTERS && registerInitialized[regId]) {
            // Get value from register
            if (registerTypes[regId] == VAR_FLOAT) {
                rightValue = registers[regId].asFloat;
            } else if (registerTypes[regId] == VAR_INT) {
                rightValue = registers[regId].asInt;
            } else {
                rightValue = registers[regId].asBool ? 1.0f : 0.0f;
            }
        } else {
            return false; // Invalid register
        }
    }

    // Perform comparison with retrieved values
    switch (op) {
        case OP_EQUAL:
            return abs(leftValue - rightValue) < 0.0001f;
        case OP_NOT_EQUAL:
            return leftValue != rightValue;
        case OP_GREATER_THAN:
            return leftValue > rightValue;
        case OP_LESS_THAN:
            return leftValue < rightValue;
        case OP_GREATER_EQUAL:
            return leftValue >= rightValue;
        case OP_LESS_EQUAL:
            return leftValue <= rightValue;
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
            pc = programSize; // Set PC past the end to stop execution
            isPaused = PROGRAM_FINISHED;
            break;

        case OP_WAIT: {
            // Delay execution for specified milliseconds
            // This converts from seconds (ie. 1.5s into milliseconds).
            // delayMs can be uint32_t since it will always be
            uint32_t delayMs = static_cast<uint32_t>(instr.operand1 * 1000.0f);
            delayUntil = millis() + delayMs;
            waitingForDelay = true;
            break;
        }

        case OP_READ_SENSOR: {
            BytecodeSensorType sensorType = static_cast<BytecodeSensorType>(instr.operand1); // Which sensor to read
            uint16_t regId = instr.operand2;                                                 // Register to store result

            if (regId < MAX_REGISTERS) {
                float value = 0.0f;
                bool skipDefaultAssignment = false; // Add this flag

                // Read the appropriate sensor
                switch (sensorType) {
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
                        registers[regId].asBool = (counts > LEFT_PROXIMITY_THRESHOLD);
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true; // Set flag to skip default assignment
                        break;
                    }
                    case SENSOR_SIDE_RIGHT_PROXIMITY: {
                        uint16_t counts = SensorDataBuffer::get_instance().get_latest_right_side_tof_counts();
                        registers[regId].asBool = (counts > RIGHT_PROXIMITY_THRESHOLD);
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true; // Set flag to skip default assignment
                        break;
                    }
                    case SENSOR_FRONT_PROXIMITY: {
                        bool isObjectDetected = SensorDataBuffer::get_instance().is_object_detected_tof();
                        registers[regId].asBool = isObjectDetected;
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true; // Set flag
                        break;
                    }
                    case SENSOR_COLOR_RED: {
                        bool isRed = SensorDataBuffer::get_instance().is_object_red();
                        registers[regId].asBool = isRed;
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;
                        break;
                    }
                    case SENSOR_COLOR_GREEN: {
                        bool isGreen = SensorDataBuffer::get_instance().is_object_green();
                        registers[regId].asBool = isGreen;
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;
                        break;
                    }
                    case SENSOR_COLOR_BLUE: {
                        bool isBlue = SensorDataBuffer::get_instance().is_object_blue();
                        registers[regId].asBool = isBlue;
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;
                        break;
                    }
                    case SENSOR_COLOR_WHITE: {
                        bool isWhite = SensorDataBuffer::get_instance().is_object_white();
                        registers[regId].asBool = isWhite;
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;
                        break;
                    }
                    case SENSOR_COLOR_BLACK: {
                        bool isBlack = SensorDataBuffer::get_instance().is_object_black();
                        registers[regId].asBool = isBlack;
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;
                        break;
                    }
                    case SENSOR_COLOR_YELLOW: {
                        bool isYellow = SensorDataBuffer::get_instance().is_object_yellow();
                        registers[regId].asBool = isYellow;
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;
                        break;
                    }
                    case FRONT_TOF_DISTANCE: {
                        float frontDistance = SensorDataBuffer::get_instance().get_front_tof_distance();
                        value = (frontDistance < 0) ? 999.0f : frontDistance; // Return 999 inches if no valid reading
                        break;
                    }
                    default: {
                        char logMessage[32];
                        snprintf(logMessage, sizeof(logMessage), "Unknown sensor type: %u", sensorType);
                        SerialQueueManager::get_instance().queue_message(logMessage);
                        break;
                    }
                }

                if (!skipDefaultAssignment) {
                    registers[regId].asFloat = value;
                    registerTypes[regId] = VAR_FLOAT;
                    registerInitialized[regId] = true;
                }
            }
            break;
        }

        case OP_SET_ALL_LEDS: {
            uint8_t r = static_cast<uint8_t>(instr.operand1); // Cast to uint8_t for
            uint8_t g = static_cast<uint8_t>(instr.operand2); // RGB values (0-255)
            uint8_t b = static_cast<uint8_t>(instr.operand3);

            // operand4 is unused for this operation
            rgbLed.set_main_board_leds_to_color(r, g, b);
            break;
        }

        case OP_COMPARE: {
            // Compare two values and store the result
            ComparisonOp op = (ComparisonOp) static_cast<uint8_t>(instr.operand1);
            float leftValue = instr.operand2;
            float rightValue = instr.operand3;

            lastComparisonResult = compare_values(op, leftValue, rightValue);

            break;
        }

        case OP_JUMP: {
            // Unconditional jump
            uint8_t low = static_cast<uint8_t>(instr.operand1);
            uint8_t high = static_cast<uint8_t>(instr.operand2);
            uint16_t jumpOffset = (high << 8) | low;                           // Combine high and low bytes into 16-bit offset
            uint16_t targetInstruction = pc + (jumpOffset / INSTRUCTION_SIZE); // 20 bytes per instruction
            pc = targetInstruction - 1;                                        // Subtract 1 because pc increments after this
            break;
        }

        case OP_JUMP_BACKWARD: {
            // Backward jump (used in for loops)
            uint8_t low = static_cast<uint8_t>(instr.operand1);
            uint8_t high = static_cast<uint8_t>(instr.operand2);
            uint16_t jumpOffset = (high << 8) | low;                           // Combine high and low bytes into 16-bit offset
            uint16_t targetInstruction = pc - (jumpOffset / INSTRUCTION_SIZE); // Subtract for backward jump
            pc = targetInstruction - 1;                                        // Subtract 1 because pc increments after
            break;
        }

        case OP_JUMP_IF_TRUE: {
            // Conditional jump if last comparison was true
            if (lastComparisonResult) {
                uint8_t low = static_cast<uint8_t>(instr.operand1);
                uint8_t high = static_cast<uint8_t>(instr.operand2);
                uint16_t jumpOffset = (high << 8) | low;                           // Combine high and low bytes into 16-bit offset
                uint16_t targetInstruction = pc + (jumpOffset / INSTRUCTION_SIZE); // 20 bytes per instruction
                pc = targetInstruction - 1;                                        // Subtract 1 because pc increments after
            }
            break;
        }

        case OP_JUMP_IF_FALSE: {
            // Conditional jump if last comparison was false
            if (!lastComparisonResult) {
                uint8_t low = static_cast<uint8_t>(instr.operand1);
                uint8_t high = static_cast<uint8_t>(instr.operand2);
                uint16_t jumpOffset = (high << 8) | low;                           // Combine high and low bytes into 16-bit offset
                uint16_t targetInstruction = pc + (jumpOffset / INSTRUCTION_SIZE); // 20 bytes per instruction
                pc = targetInstruction - 1;                                        // Subtract 1 because pc increments after
            }
            break;
        }

        case OP_DECLARE_VAR: {
            uint16_t regId = instr.operand1;
            BytecodeVarType type = (BytecodeVarType)instr.operand2;

            if (regId < MAX_REGISTERS) {
                registerTypes[regId] = type;
                // Initialize with default values
                switch (type) {
                    case VAR_FLOAT:
                        registers[regId].asFloat = 0.0f;
                        break;
                    case VAR_INT:
                        registers[regId].asInt = 0;
                        break;
                    case VAR_BOOL:
                        registers[regId].asBool = false;
                        break;
                }
                registerInitialized[regId] = true;
            }
            break;
        }

        case OP_SET_VAR: {
            uint16_t regId = static_cast<uint16_t>(instr.operand1);

            if (regId < MAX_REGISTERS) {
                switch (registerTypes[regId]) {
                    case VAR_FLOAT: {
                        // Direct assignment - no reconstruction needed
                        registers[regId].asFloat = instr.operand2;
                        break;
                    }
                    case VAR_INT: {
                        // Cast float to int directly
                        registers[regId].asInt = static_cast<int32_t>(instr.operand2);
                        break;
                    }
                    case VAR_BOOL:
                        // Cast float to bool (non-zero = true)
                        registers[regId].asBool = (instr.operand2 != 0.0f);
                        break;
                }
                registerInitialized[regId] = true;
            }
            break;
        }
        case OP_WHILE_START: {
            // This is just a marker for the start of the loop
            // No special action needed
            break;
        }

        case OP_WHILE_END: {
            uint8_t low = static_cast<uint8_t>(instr.operand1);
            uint8_t high = static_cast<uint8_t>(instr.operand2);
            uint16_t offsetToStart = (high << 8) | low;
            if (offsetToStart <= pc * INSTRUCTION_SIZE) {
                pc = pc - (offsetToStart / INSTRUCTION_SIZE);
            } else {
                pc = programSize;
                SerialQueueManager::get_instance().queue_message("Invalid loop jump - stopping execution");
            }
            break;
        }

        case OP_FOR_INIT: {
            // Initialize loop counter
            uint16_t regId = static_cast<uint16_t>(instr.operand1);

            if (regId < MAX_REGISTERS) {
                registerTypes[regId] = VAR_INT;

                // Cast the float value to int
                registers[regId].asInt = static_cast<int32_t>(instr.operand2);
                registerInitialized[regId] = true;
            }
            break;
        }

        case OP_FOR_CONDITION: {
            // Check if counter < end value
            uint16_t regId = static_cast<uint16_t>(instr.operand1);

            if (regId < MAX_REGISTERS && registerInitialized[regId]) {
                // Cast the float value to int
                int32_t endValue = static_cast<int32_t>(instr.operand2);

                // Compare counter with end value
                lastComparisonResult = (registers[regId].asInt < endValue);
            } else {
                // Invalid register, exit loop
                lastComparisonResult = false;
            }
            break;
        }

        case OP_FOR_INCREMENT: {
            // Increment counter by 1
            uint16_t regId = static_cast<uint16_t>(instr.operand1);

            if (regId < MAX_REGISTERS && registerInitialized[regId]) {
                registers[regId].asInt++;
            }
            break;
        }

        case OP_MOTOR_GO: {
            // operand1: direction (0=backward, 1=forward)
            // operand2: throttle percentage (0-100)
            bool isForward = (instr.operand1 > 0.5f); // > 0.5 to handle float precision
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);
            uint16_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_PWM);

            // Set both motors based on direction
            if (isForward) {
                motorDriver.update_motor_pwm(motorSpeed, motorSpeed);
            } else {
                motorDriver.update_motor_pwm(-motorSpeed, -motorSpeed);
            }
            break;
        }

        case OP_MOTOR_STOP: {
            motorDriver.reset_command_state(true);
            break;
        }

        case MOTOR_SPIN: {
            // operand1: direction (0=counterclockwise, 1=clockwise)
            // operand2: speed percentage (0-100)
            bool clockwise = (instr.operand1 > 0.5f); // > 0.5 to handle float precision
            uint8_t speedPercent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);

            uint16_t motorSpeed;
            if (speedPercent == 0) {
                motorSpeed = 0; // Stop motors completely at 0%
            } else {
                // Custom formula for speeds 1-100: ((MAX_SPIN_PWM - MIN_SPIN_PWM) / 100) * speed + MIN_SPIN_PWM
                motorSpeed = ((MAX_SPIN_PWM - MIN_SPIN_PWM) * speedPercent / 100) + MIN_SPIN_PWM;
            }

            // Spin motors in opposite directions
            if (clockwise) {
                // Clockwise: left motor forward, right motor backward
                motorDriver.update_motor_pwm(motorSpeed, -motorSpeed);
            } else {
                // Counterclockwise: left motor backward, right motor forward
                motorDriver.update_motor_pwm(-motorSpeed, motorSpeed);
            }
            break;
        }

        case OP_MOTOR_TURN: {
            bool clockwise = (instr.operand1 > 0);
            float degrees = instr.operand2;

            // Use TurningManager for precise turning
            float signedDegrees = clockwise ? degrees : -degrees;
            if (!TurningManager::get_instance().start_turn(signedDegrees)) {
                SerialQueueManager::get_instance().queue_message("Failed to start turn - turn already in progress");
            }

            // The actual turn progress will be monitored in update()
            break;
        }

        case OP_MOTOR_GO_TIME: {
            // operand1: direction (0=backward, 1=forward)
            // operand2: time in seconds
            // operand3: throttle percentage (0-100)
            bool isForward = (instr.operand1 > 0.5f); // > 0.5 to handle float precision
            float seconds = instr.operand2;
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand3), 0, 100);

            // Validate parameters
            if (seconds <= 0.0f) {
                SerialQueueManager::get_instance().queue_message("Invalid time value for timed movement");
                break;
            }

            if (throttlePercent > 100) {
                throttlePercent = 100; // Clamp to valid range
            }

            // Convert percentage to motor speed
            uint16_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_PWM);

            // Set motors based on direction
            if (isForward) {
                motorDriver.update_motor_pwm(motorSpeed, motorSpeed);
            } else {
                motorDriver.update_motor_pwm(-motorSpeed, -motorSpeed);
            }

            // Set up timed movement
            timedMotorMovementInProgress = true;
            motorMovementEndTime = millis() + static_cast<uint32_t>(seconds * 1000.0f);

            break;
        }

        case OP_MOTOR_GO_DISTANCE: {
            // operand1: direction (0=backward, 1=forward)
            // operand2: distance in inches
            // operand3: throttle percentage (0-100)
            bool isForward = (instr.operand1 > 0.5f); // > 0.5 to handle float precision
            float distanceIn = instr.operand2;
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand3), 0, 100);

            // Validate parameters
            if (distanceIn <= 0.0f) {
                SerialQueueManager::get_instance().queue_message("Invalid distance value for distance movement");
                break;
            }

            if (throttlePercent > 100) {
                throttlePercent = 100; // Clamp to valid range
            }

            // Convert percentage to motor speed
            uint16_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_PWM);

            // Reset distance tracking - store current distance as starting point
            startingDistanceIn = SensorDataBuffer::get_instance().get_latest_distance_traveled_in();

            // Set up distance movement
            distanceMovementInProgress = true;
            targetDistanceIn = distanceIn;

            // Set motors based on direction and store initial PWM with correct sign
            if (isForward) {
                initialDistancePwm = motorSpeed;
                motorDriver.update_motor_pwm(motorSpeed, motorSpeed);
            } else {
                initialDistancePwm = -motorSpeed; // Store as negative for backward
                motorDriver.update_motor_pwm(-motorSpeed, -motorSpeed);
            }

            break;
        }

        case OP_WAIT_FOR_BUTTON: {
            waitingForButtonPressToStart = true;

            // Don't increment PC here - we'll do it when the button is pressed
            return; // Return without incrementing PC
        }

        case CHECK_RIGHT_BUTTON_PRESS: {
            uint16_t regId = instr.operand1; // Register to store result

            if (regId < MAX_REGISTERS) {
                bool isPressed = Buttons::get_instance().is_right_button_pressed();

                registers[regId].asBool = isPressed;
                registerTypes[regId] = VAR_BOOL;
                registerInitialized[regId] = true;
            }
            break;
        }

        case PLAY_TONE: {
            uint8_t toneValue = static_cast<uint8_t>(instr.operand1);

            // ADD THESE DEBUG LINES:
            SerialQueueManager::get_instance().queue_message("PLAY_TONE opcode hit with value: " + String(toneValue));

            // toneValue = 0 means stop, 1-7 are valid tones
            if (toneValue <= 7) {
                ToneType toneType = static_cast<ToneType>(toneValue);
                Speaker::get_instance().play_tone(toneType);
            } else if (toneValue == 8) {
                Speaker::get_instance().stop_tone();
            } else {
                SerialQueueManager::get_instance().queue_message("Invalid tone value: " + String(toneValue));
            }
            break;
        }

        default:
            // Unknown opcode, stop execution
            pc = programSize;
            break;
    }
}

void BytecodeVM::update_timed_motor_movement() {
    // Check if the timed movement has completed
    if (millis() < motorMovementEndTime) return;
    // Movement complete - brake motors
    motorDriver.reset_command_state(true);

    // Reset timed movement state
    timedMotorMovementInProgress = false;
}

void BytecodeVM::update_distance_movement() {
    // Get distance traveled from sensor data buffer (relative to starting point)
    float totalDistance = SensorDataBuffer::get_instance().get_latest_distance_traveled_in();
    float currentDistance = abs(totalDistance - startingDistanceIn);
    float remainingDistance = targetDistanceIn - currentDistance;

    // Check if we've reached the target distance
    if (remainingDistance <= 0.0f) {
        // Distance reached - brake motors and clear any pending commands
        motorDriver.reset_command_state(true);

        // Reset distance movement state
        distanceMovementInProgress = false;
        targetDistanceIn = 0.0f;
        startingDistanceIn = 0.0f;
        initialDistancePwm = 0;
        vTaskDelay(pdMS_TO_TICKS(250));
        return;
    }

    // Calculate braking distance using physics equation
    int16_t absPwm = abs(initialDistancePwm);
    float brakingDistance = (absPwm * absPwm - MIN_DECELERATION_PWM * MIN_DECELERATION_PWM) / (2.0f * DECELERATION_RATE);
    SerialQueueManager::get_instance().queue_message("brakingDistance" + String(brakingDistance));

    // Ensure braking distance doesn't exceed total distance (edge case for short distances)
    if (brakingDistance > targetDistanceIn) {
        brakingDistance = targetDistanceIn;
    }

    // Determine target PWM based on remaining distance
    int16_t targetPwm;

    if (remainingDistance <= brakingDistance) {
        // We're in the deceleration zone - apply inverse sigmoid
        float normalizedPosition = remainingDistance / brakingDistance; // 1.0 at start of braking, 0.0 at end

        // Inverse sigmoid: gentle at start, aggressive near end
        float k = 10.0f / brakingDistance; // Steepness factor
        float sigmoidValue = 1.0f / (1.0f + exp(k * (brakingDistance / 2.0f - remainingDistance)));

        // Calculate PWM using sigmoid curve
        targetPwm = MIN_DECELERATION_PWM + (absPwm - MIN_DECELERATION_PWM) * sigmoidValue;

        // Maintain direction (forward/backward)
        if (initialDistancePwm < 0) {
            targetPwm = -targetPwm;
        }

        SerialQueueManager::get_instance().queue_message("sigmoidValue" + String(sigmoidValue));
        SerialQueueManager::get_instance().queue_message("targetPwm" + String(targetPwm));

        // Set motor speeds directly without ramping
        motorDriver.set_motor_speeds(targetPwm, targetPwm, false);
    }
    // If not in deceleration zone, continue at initial speed (StraightLineDrive will handle heading)
}

void BytecodeVM::stop_program() {
    if (programMutex == nullptr) return;

    // Acquire mutex with timeout
    if (xSemaphoreTake(programMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("stop_program: Failed to acquire mutex");
        return;
    }

    reset_state_variables(true);
    xSemaphoreGive(programMutex);
    return;
}

void BytecodeVM::reset_state_variables(bool isFullReset) {
    pc = 0;
    delayUntil = 0;
    waitingForDelay = false;
    lastComparisonResult = false;

    // Reset TurningManager state
    TurningManager::get_instance().complete_navigation();
    timedMotorMovementInProgress = false;
    distanceMovementInProgress = false;
    initialDistancePwm = 0;
    motorMovementEndTime = 0;
    targetDistanceIn = 0.0f;
    startingDistanceIn = 0.0f;
    waitingForButtonPressToStart = false;

    // ADD THESE USB safety resets:
    stoppedDueToUsbSafety = false;
    // Note: Don't reset programContainsMotors or lastUsbState here as they persist across pause/resume

    // Force reset motor driver state completely
    motorDriver.reset_command_state(false);

    // Reset registers
    for (uint16_t i = 0; i < MAX_REGISTERS; i++) {
        registerInitialized[i] = false;
        registers[i].asFloat = 0.0f;
        registers[i].asInt = 0;
        registers[i].asBool = false;
    }
    for (uint16_t i = 0; i < MAX_REGISTERS; i++) {
        registerTypes[i] = VAR_FLOAT;
    }
    for (uint16_t i = 0; i < MAX_REGISTERS; i++) {
        registerInitialized[i] = false;
    }
    if (isFullReset) {
        delete[] program;
        program = nullptr;
        isPaused = PROGRAM_NOT_STARTED;
        programSize = 0;

        // ADD THESE for full reset:
        programContainsMotors = false;
        lastUsbState = false;
    }
    rgbLed.turn_all_leds_off();
    Speaker::get_instance().stop_all_sounds();
    SensorDataBuffer::get_instance().stop_polling_all_sensors();
}

void BytecodeVM::pause_program() {
    if (programMutex == nullptr) return;

    if (xSemaphoreTake(programMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("pause_program: Failed to acquire mutex");
        return;
    }

    if (!program || isPaused == PAUSED) {
        xSemaphoreGive(programMutex);
        return;
    }

    reset_state_variables();
    isPaused = PAUSED;
    xSemaphoreGive(programMutex);
}

void BytecodeVM::resume_program() {
    if (programMutex == nullptr) return;

    if (xSemaphoreTake(programMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("resume_program: Failed to acquire mutex");
        return;
    }

    if (!program || isPaused == RUNNING) {
        SerialQueueManager::get_instance().queue_message("resume_program: Not paused or no program");
        xSemaphoreGive(programMutex);
        return;
    }

    if (!can_start_program()) {
        xSemaphoreGive(programMutex);
        return;
    }

    // Reset state variables for finished programs to start fresh
    if (isPaused == PROGRAM_FINISHED) {
        reset_state_variables();
    }

    isPaused = RUNNING;

    // Check if the first instruction is a WAIT_FOR_BUTTON (start block)
    if (programSize > 0 && program[0].opcode == OP_WAIT_FOR_BUTTON) {
        SerialQueueManager::get_instance().queue_message("Resuming program - skipping initial wait for button");
        pc = 1;                               // Start after the wait for button instruction
        waitingForButtonPressToStart = false; // ← FIX: Clear the flag!
    } else {
        SerialQueueManager::get_instance().queue_message("Resuming program from beginning");
        pc = 0;                               // Start from the beginning for scripts without a start block
        waitingForButtonPressToStart = false; // ← FIX: Clear here too for consistency
    }

    xSemaphoreGive(programMutex);
}

void BytecodeVM::activate_sensors_for_program() {
    if (!program || programSize == 0) return;

    // Track which sensors are needed
    bool needQuaternion = false;
    bool needAccelerometer = false;
    bool needGyroscope = false;
    bool needMagnetometer = false;
    bool needTof = false;
    bool needSideTof = false;
    bool needColorSensor = false;

    // Scan through the entire program
    for (uint16_t i = 0; i < programSize; i++) {
        const BytecodeInstruction& instr = program[i];

        // Handle OP_READ_SENSOR dynamically based on sensor type
        if (instr.opcode != OP_READ_SENSOR) {
            // Handle other opcodes using the static mapping
            auto it = opcodeToSensors.find(instr.opcode);
            if (it != opcodeToSensors.end()) {
                for (SensorType sensorType : it->second) {
                    switch (sensorType) {
                        case SENSOR_QUATERNION:
                            needQuaternion = true;
                            break;
                        case SENSOR_ACCELEROMETER:
                            needAccelerometer = true;
                            break;
                        case SENSOR_GYROSCOPE:
                            needGyroscope = true;
                            break;
                        case SENSOR_MAGNETOMETER:
                            needMagnetometer = true;
                            break;
                        case SENSOR_TOF:
                            needTof = true;
                            break;
                        case SENSOR_SIDE_TOF:
                            needSideTof = true;
                            break;
                    }
                }
            }
        } else {
            BytecodeSensorType sensorType = static_cast<BytecodeSensorType>(instr.operand1);

            switch (sensorType) {
                case SENSOR_PITCH:
                case SENSOR_ROLL:
                case SENSOR_YAW:
                    needQuaternion = true;
                    break;
                case SENSOR_ACCEL_X:
                case SENSOR_ACCEL_Y:
                case SENSOR_ACCEL_Z:
                case SENSOR_ACCEL_MAG:
                    needAccelerometer = true;
                    break;
                case SENSOR_ROT_RATE_X:
                case SENSOR_ROT_RATE_Y:
                case SENSOR_ROT_RATE_Z:
                    needGyroscope = true;
                    break;
                case SENSOR_MAG_FIELD_X:
                case SENSOR_MAG_FIELD_Y:
                case SENSOR_MAG_FIELD_Z:
                    needMagnetometer = true;
                    break;
                case SENSOR_SIDE_LEFT_PROXIMITY:
                case SENSOR_SIDE_RIGHT_PROXIMITY:
                    needSideTof = true;
                    break;
                case SENSOR_FRONT_PROXIMITY:
                case FRONT_TOF_DISTANCE:
                    needTof = true;
                    break;
                case SENSOR_COLOR_RED:
                case SENSOR_COLOR_BLACK:
                case SENSOR_COLOR_BLUE:
                case SENSOR_COLOR_GREEN:
                case SENSOR_COLOR_WHITE:
                case SENSOR_COLOR_YELLOW:
                    needColorSensor = true;
            }
        }
    }

    // Activate required sensors by setting their last_request timestamps
    SensorDataBuffer& buffer = SensorDataBuffer::get_instance();
    ReportTimeouts& timeouts = buffer.get_report_timeouts();
    uint32_t current_time = millis();

    if (needQuaternion) {
        timeouts.quaternion_last_request.store(current_time);
        SerialQueueManager::get_instance().queue_message("Activated quaternion sensor for program");
    }
    if (needAccelerometer) {
        timeouts.accelerometer_last_request.store(current_time);
        SerialQueueManager::get_instance().queue_message("Activated accelerometer for program");
    }
    if (needGyroscope) {
        timeouts.gyroscope_last_request.store(current_time);
        SerialQueueManager::get_instance().queue_message("Activated gyroscope for program");
    }
    if (needMagnetometer) {
        timeouts.magnetometer_last_request.store(current_time);
        SerialQueueManager::get_instance().queue_message("Activated magnetometer for program");
    }
    if (needTof) {
        timeouts.tof_last_request.store(current_time);
        SerialQueueManager::get_instance().queue_message("Activated multizone TOF for program");
    }
    if (needSideTof) {
        timeouts.side_tof_last_request.store(current_time);
        SerialQueueManager::get_instance().queue_message("Activated side TOF for program");
    }
    if (needColorSensor) {
        timeouts.color_last_request.store(current_time);
        SerialQueueManager::get_instance().queue_message("Activated colors Sensors for program");
    }
}

void BytecodeVM::scan_program_for_motors() {
    programContainsMotors = false;

    if (!program || programSize == 0) return;

    // Define all motor opcodes that should block USB execution
    const BytecodeOpCode motorOpcodes[] = {OP_MOTOR_GO, OP_MOTOR_STOP, OP_MOTOR_TURN, OP_MOTOR_GO_TIME, OP_MOTOR_GO_DISTANCE, MOTOR_SPIN};

    // Scan entire program for any motor commands
    for (uint16_t i = 0; i < programSize; i++) {
        for (const auto& motorOpcode : motorOpcodes) {
            if (program[i].opcode == motorOpcode) {
                programContainsMotors = true;
                return;
            }
        }
    }
}

void BytecodeVM::check_usb_safety_conditions() {
    bool currentUsbState = SerialManager::get_instance().is_serial_connected();

    // Detect USB connection change (disconnected -> connected)
    if (!lastUsbState && currentUsbState) {
        handle_usb_connect();
    }

    lastUsbState = currentUsbState;
}

void BytecodeVM::handle_usb_connect() {
    // If program contains motors and is currently running, stop it
    if (programContainsMotors && isPaused == RUNNING) {
        stop_program();
        stoppedDueToUsbSafety = true;
    }
}

bool BytecodeVM::can_start_program() {
    // Block start if program contains motors and USB is connected
    if (!programContainsMotors || !SerialManager::get_instance().is_serial_connected()) return true;

    SerialManager::get_instance().send_json_message(ToSerialMessage::MOTORS_DISABLED_USB,
                                                    "Cannot start motor program while USB connected - disconnect USB first");
    return false;
}
