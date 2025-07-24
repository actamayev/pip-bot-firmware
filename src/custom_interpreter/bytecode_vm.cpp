#include "bytecode_vm.h"

BytecodeVM::~BytecodeVM() {
    resetStateVariables(true);
}

bool BytecodeVM::loadProgram(const uint8_t* byteCode, uint16_t size) {
    // Free any existing program
    stopProgram();
    MessageProcessor::getInstance().resetCommandState();
    // SerialQueueManager::getInstance().queueMessage("Loading program of size %zu\n", size);

    // Validate bytecode size (must be multiple of 20 now)
    if (size % INSTRUCTION_SIZE != 0 || size / INSTRUCTION_SIZE > MAX_PROGRAM_SIZE) {
        return false;
    }
    
    programSize = size / INSTRUCTION_SIZE;
    program = new(std::nothrow) BytecodeInstruction[programSize];
    if (!program) {
        SerialQueueManager::getInstance().queueMessage("Failed to allocate memory for program");
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
        SerialQueueManager::getInstance().queueMessage("Program loaded with start button - waiting for button press to begin");
    } else {
        // Program has no start button - set to auto-running
        isPaused = RUNNING;
        waitingForButtonPressToStart = false;
        SerialQueueManager::getInstance().queueMessage("Program loaded without start button - auto-starting");
    }

    scanProgramForMotors();
    stoppedDueToUsbSafety = false; // Reset safety flag on new program load

    return true;
}

void BytecodeVM::update() {
    checkUsbSafetyConditions();
    if (program && pc < programSize && isPaused == RUNNING) {
        SensorPollingManager::getInstance().startPolling();
    }

    if (!program || pc >= programSize || isPaused == PauseState::PAUSED) {
        return;
    }

    // Check if we're waiting for a delay to complete
    if (waitingForDelay) {
        if (millis() < delayUntil) {
            return; // Still waiting
        }
        waitingForDelay = false;
    }
    
    if (timedMotorMovementInProgress) {
        updateTimedMotorMovement();
        return; // Don't execute next instruction until movement is complete
    }

    // Handle turning operation if in progress
    if (turningInProgress) {
        updateTurning();
        return; // Don't execute next instruction until turn is complete
    }

    if (distanceMovementInProgress) {
        updateDistanceMovement();
        return; // Don't execute next instruction until movement is complete
    }

    // Execute current instruction
    executeInstruction(program[pc]);
    if (!waitingForButtonPressToStart) {
        pc++; // Move to next instruction
    }
}

bool BytecodeVM::compareValues(ComparisonOp op, float leftOperand, float rightOperand) {
    float leftValue;
    float rightValue;
    
    // Process left operand - check if high bit is set, indicating a register
    if (leftOperand >= 32768.0f) {
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
            return false;  // Invalid register
        }
    } else {
        // Direct value
        leftValue = leftOperand;
    }
    
    // Process right operand - check if high bit is set, indicating a register
    if (rightOperand >= 32768.0f) {
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
            return false;  // Invalid register
        }
    } else {
        // Direct value
        rightValue = rightOperand;
    }
    
    // Perform comparison with retrieved values
    switch (op) {
        case OP_EQUAL: return abs(leftValue - rightValue) < 0.0001f;
        case OP_NOT_EQUAL: return leftValue != rightValue;
        case OP_GREATER_THAN: return leftValue > rightValue;
        case OP_LESS_THAN: return leftValue < rightValue;
        case OP_GREATER_EQUAL: return leftValue >= rightValue;
        case OP_LESS_EQUAL: return leftValue <= rightValue;
        default: return false;
    }
}

void BytecodeVM::executeInstruction(const BytecodeInstruction& instr) {
    switch (instr.opcode) {
        case OP_NOP:
            // No operation, do nothing
            break;

        case OP_END:
            // End program execution
            pc = programSize; // Set PC past the end to stop execution
            break;

        case OP_DELAY: {
            // Delay execution for specified milliseconds
            uint32_t delayMs = static_cast<uint32_t>(instr.operand1);
            delayUntil = millis() + delayMs;
            waitingForDelay = true;
            break;
        }

        case OP_SET_LED: {
            // Set specific LED to color
            uint8_t ledId = static_cast<uint8_t>(instr.operand1); // Cast to ensure range
            uint8_t r = static_cast<uint8_t>(instr.operand2);     // Cast to uint8_t for
            uint8_t g = static_cast<uint8_t>(instr.operand3);     // RGB values (0-255)
            uint8_t b = static_cast<uint8_t>(instr.operand4);        

            // Set the LED color
            switch (ledId) {
                case LED_TOP_LEFT:
                    rgbLed.set_top_left_led(r, g, b);
                    break;
                case LED_TOP_RIGHT:
                    rgbLed.set_top_right_led(r, g, b);
                    break;
                case LED_MIDDLE_LEFT:
                    rgbLed.set_middle_left_led(r, g, b);
                    break;
                case LED_MIDDLE_RIGHT:
                    rgbLed.set_middle_right_led(r, g, b);
                    break;
                case LED_BACK_LEFT:
                    rgbLed.set_back_left_led(r, g, b);
                    break;
                case LED_BACK_RIGHT:
                    rgbLed.set_back_right_led(r, g, b);
                    break;
                case LEFT_HEADLIGHT:
                    rgbLed.set_left_headlight(r, g, b);
                    break;
                case RIGHT_HEADLIGHT:
                    rgbLed.set_right_headlight(r, g, b);
                    break;
            }
            break;
        }

        case OP_READ_SENSOR: {
            uint8_t sensorType = static_cast<uint8_t>(instr.operand1);  // Which sensor to read
            uint16_t regId = instr.operand2;       // Register to store result
            
            if (regId < MAX_REGISTERS) {
                float value = 0.0f;
                bool skipDefaultAssignment = false;  // Add this flag
                
                // Read the appropriate sensor
                switch (sensorType) {
                    case SENSOR_PITCH:
                        value = ImuSensor::getInstance().getPitch();
                        break;
                    case SENSOR_ROLL:
                        value = ImuSensor::getInstance().getRoll();
                        break;
                    case SENSOR_YAW:
                        value = ImuSensor::getInstance().getYaw();
                        break;
                    case SENSOR_ACCEL_X:
                        value = ImuSensor::getInstance().getXAccel();
                        break;
                    case SENSOR_ACCEL_Y:
                        value = ImuSensor::getInstance().getYAccel();
                        break;
                    case SENSOR_ACCEL_Z:
                        value = ImuSensor::getInstance().getZAccel();
                        break;
                    case SENSOR_ACCEL_MAG:
                        value = ImuSensor::getInstance().getAccelMagnitude();
                        break;
                    case SENSOR_ROT_RATE_X:
                        value = ImuSensor::getInstance().getXRotationRate();
                        break;
                    case SENSOR_ROT_RATE_Y:
                        value = ImuSensor::getInstance().getYRotationRate();
                        break;
                    case SENSOR_ROT_RATE_Z:
                        value = ImuSensor::getInstance().getZRotationRate();
                        break;
                    case SENSOR_MAG_FIELD_X:
                        value = ImuSensor::getInstance().getMagneticFieldX();
                        break;
                    case SENSOR_MAG_FIELD_Y:
                        value = ImuSensor::getInstance().getMagneticFieldY();
                        break;
                    case SENSOR_MAG_FIELD_Z:
                        value = ImuSensor::getInstance().getMagneticFieldZ();
                        break;
                    case SENSOR_SIDE_LEFT_PROXIMITY: {
                        uint16_t counts = SideTofManager::getInstance().leftSideTofSensor.getCounts();
                        registers[regId].asBool = (counts > LEFT_PROXIMITY_THRESHOLD);
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;  // Set flag to skip default assignment
                        break;
                    }
                    case SENSOR_SIDE_RIGHT_PROXIMITY: {
                        uint16_t counts = SideTofManager::getInstance().rightSideTofSensor.getCounts();
                        registers[regId].asBool = (counts > RIGHT_PROXIMITY_THRESHOLD);
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;  // Set flag to skip default assignment
                        break;
                    }
                    case SENSOR_FRONT_PROXIMITY: {
                        float isObjectDetected = MultizoneTofSensor::getInstance().isObjectDetected();
                        registers[regId].asBool = isObjectDetected;
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;  // Set flag
                        break;
                    }
                    default: {
                        char logMessage[32];
                        snprintf(logMessage, sizeof(logMessage), "Unknown sensor type: %u", sensorType);
                        SerialQueueManager::getInstance().queueMessage(logMessage);
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
            uint8_t r = static_cast<uint8_t>(instr.operand1);     // Cast to uint8_t for
            uint8_t g = static_cast<uint8_t>(instr.operand2);     // RGB values (0-255)
            uint8_t b = static_cast<uint8_t>(instr.operand3);        

            // operand4 is unused for this operation
            rgbLed.set_main_board_leds_to_color(r, g, b);
            break;
        }

        case OP_COMPARE: {
            // Compare two values and store the result
            ComparisonOp op = (ComparisonOp)static_cast<uint8_t>(instr.operand1);
            float leftValue = instr.operand2;
            float rightValue = instr.operand3;
            
            lastComparisonResult = compareValues(op, leftValue, rightValue);

            break;
        }

        case OP_JUMP: {
            // Unconditional jump
            uint8_t low = static_cast<uint8_t>(instr.operand1);
            uint8_t high = static_cast<uint8_t>(instr.operand2);
            uint16_t jumpOffset = (high << 8) | low; // Combine high and low bytes into 16-bit offset
            uint16_t targetInstruction = pc + (jumpOffset / INSTRUCTION_SIZE); // 20 bytes per instruction
            pc = targetInstruction - 1; // Subtract 1 because pc increments after this
            break;
        }

        case OP_JUMP_BACKWARD: {
            // Backward jump (used in for loops)
            uint8_t low = static_cast<uint8_t>(instr.operand1);
            uint8_t high = static_cast<uint8_t>(instr.operand2);
            uint16_t jumpOffset = (high << 8) | low; // Combine high and low bytes into 16-bit offset
            uint16_t targetInstruction = pc - (jumpOffset / INSTRUCTION_SIZE); // Subtract for backward jump
            pc = targetInstruction - 1; // Subtract 1 because pc increments after
            break;
        }

        case OP_JUMP_IF_TRUE: {
            // Conditional jump if last comparison was true
            if (lastComparisonResult) {
                uint8_t low = static_cast<uint8_t>(instr.operand1);
                uint8_t high = static_cast<uint8_t>(instr.operand2);
                uint16_t jumpOffset = (high << 8) | low; // Combine high and low bytes into 16-bit offset
                uint16_t targetInstruction = pc + (jumpOffset / INSTRUCTION_SIZE); // 20 bytes per instruction
                pc = targetInstruction - 1; // Subtract 1 because pc increments after
            }
            break;
        }

        case OP_JUMP_IF_FALSE: {
            // Conditional jump if last comparison was false
            if (!lastComparisonResult) {
                uint8_t low = static_cast<uint8_t>(instr.operand1);
                uint8_t high = static_cast<uint8_t>(instr.operand2);
                uint16_t jumpOffset = (high << 8) | low; // Combine high and low bytes into 16-bit offset
                uint16_t targetInstruction = pc + (jumpOffset / INSTRUCTION_SIZE); // 20 bytes per instruction
                pc = targetInstruction - 1; // Subtract 1 because pc increments after
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
                SerialQueueManager::getInstance().queueMessage("Invalid loop jump - stopping execution");
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

        case OP_MOTOR_FORWARD: {
            // Convert percentage (0-100) to motor speed (0-MAX_MOTOR_SPEED)
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand1), 0, 100);
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_SPEED);
            
            // Set both motors to forward at calculated speed
            motorDriver.set_motor_speeds(motorSpeed, motorSpeed);
            motorDriver.update_motor_speeds(true); // Optional: enable ramping
            break;
        }
        
        case OP_MOTOR_BACKWARD: {
            // Convert percentage (0-100) to motor speed (0-MAX_MOTOR_SPEED)
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand1), 0, 100);
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_SPEED);
            
            // Set both motors to backward (negative speed)
            motorDriver.set_motor_speeds(-motorSpeed, -motorSpeed);
            motorDriver.update_motor_speeds(true); // Optional: enable ramping
            break;
        }
        
        case OP_MOTOR_STOP: {
            // Stop both motors
            // motorDriver.brake_both_motors();
            motorDriver.stop_both_motors();
            motorDriver.force_reset_motors();
            break;
        }
        
        case OP_MOTOR_TURN: {
            bool clockwise = (instr.operand1 > 0);
            float degrees = instr.operand2;
            
            // Initialize turning state
            turningInProgress = true;
            targetTurnDegrees = degrees;
            initialTurnYaw = ImuSensor::getInstance().getYaw();
            turnClockwise = clockwise;
            turnStartTime = millis();

            // Set motors for turning
            if (clockwise) {
                motorDriver.set_motor_speeds(100, -100); // Right turn
            } else {
                motorDriver.set_motor_speeds(-100, 100); // Left turn
            }
            
            // The actual turn progress will be monitored in update()
            break;
        }

        case OP_MOTOR_FORWARD_TIME: {
            float seconds = instr.operand1;
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);
            
            // Validate parameters
            if (seconds <= 0.0f) {
                SerialQueueManager::getInstance().queueMessage("Invalid time value for forward movement");
                break;
            }
            
            if (throttlePercent > 100) {
                throttlePercent = 100;  // Clamp to valid range
            }
            
            // Convert percentage to motor speed
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_SPEED);
            
            // Set motors to forward motion
            motorDriver.set_motor_speeds(motorSpeed, motorSpeed);
            motorDriver.update_motor_speeds(true);  // Enable ramping
            
            // Set up timed movement
            timedMotorMovementInProgress = true;
            motorMovementEndTime = millis() + static_cast<unsigned long>(seconds * 1000.0f);
            
            break;
        }

        case OP_MOTOR_BACKWARD_TIME: {
            float seconds = instr.operand1;
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);
            
            // Validate parameters
            if (seconds <= 0.0f) {
                SerialQueueManager::getInstance().queueMessage("Invalid time value for backward movement");
                break;
            }
            
            if (throttlePercent > 100) {
                throttlePercent = 100;  // Clamp to valid range
            }
            
            // Convert percentage to motor speed
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_SPEED);
            
            // Set motors to backward motion
            motorDriver.set_motor_speeds(-motorSpeed, -motorSpeed);
            motorDriver.update_motor_speeds(true);  // Enable ramping
            
            break;
        }

        case OP_MOTOR_FORWARD_DISTANCE: {
            float distanceCm = instr.operand1;
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);
            
            // Validate parameters
            if (distanceCm <= 0.0f) {
                SerialQueueManager::getInstance().queueMessage("Invalid distance value for forward movement");
                break;
            }
            
            if (throttlePercent > 100) {
                throttlePercent = 100;  // Clamp to valid range
            }
            
            // Convert percentage to motor speed
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_SPEED);
            
            // Reset distance tracking in encoder manager
            encoderManager.resetDistanceTracking();
            
            // Set up distance movement
            distanceMovementInProgress = true;
            targetDistanceCm = distanceCm;
            
            // Set motors to forward motion
            motorDriver.set_motor_speeds(motorSpeed, motorSpeed);
            motorDriver.update_motor_speeds(true);  // Enable ramping
            
            break;
        }

        case OP_MOTOR_BACKWARD_DISTANCE: {
            float distanceCm = instr.operand1;
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);
            
            // Validate parameters
            if (distanceCm <= 0.0f) {
                SerialQueueManager::getInstance().queueMessage("Invalid distance value for backward movement");
                break;
            }
            
            if (throttlePercent > 100) {
                throttlePercent = 100;  // Clamp to valid range
            }
            
            // Convert percentage to motor speed
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_SPEED);
            
            // Reset distance tracking in encoder manager
            encoderManager.resetDistanceTracking();
            
            // Set up distance movement
            distanceMovementInProgress = true;
            targetDistanceCm = distanceCm;
            
            // Set motors to backward motion
            motorDriver.set_motor_speeds(-motorSpeed, -motorSpeed);
            motorDriver.update_motor_speeds(true);  // Enable ramping
            
            break;
        }

        case OP_WAIT_FOR_BUTTON: {
            waitingForButtonPressToStart = true;

            // Don't increment PC here - we'll do it when the button is pressed
            return; // Return without incrementing PC
        }

        default:
            // Unknown opcode, stop execution
            pc = programSize;
            // SerialQueueManager::getInstance().queueMessage("Unknown code - stopping execution %d\n", instr.opcode);
            break;
    }
}

void BytecodeVM::updateTurning() {
    // Get current yaw
    float currentYaw = ImuSensor::getInstance().getYaw();
    
    // Calculate rotation delta with wraparound handling
    float rotationDelta;
    
    if (turnClockwise) {
        rotationDelta = initialTurnYaw - currentYaw;
        if (rotationDelta < 0) rotationDelta += 360;
    } else {
        rotationDelta = currentYaw - initialTurnYaw;
        if (rotationDelta < 0) rotationDelta += 360;
    }
    
    // Check for timeout (safety feature)
    unsigned long elapsed = millis() - turnStartTime;
    bool timeout = elapsed > TURN_TIMEOUT; // 10 second timeout
    
    // Check if turn is complete
    if (rotationDelta >= targetTurnDegrees || timeout) {
        // Turn complete - stop motors
        motorDriver.stop_both_motors();
        turningInProgress = false;
    }
}

void BytecodeVM::updateTimedMotorMovement() {
    // Check if the timed movement has completed
    if (millis() < motorMovementEndTime) return;
    // Movement complete - brake motors
    motorDriver.stop_both_motors();

    // Reset timed movement state
    timedMotorMovementInProgress = false;
}

void BytecodeVM::updateDistanceMovement() {
    // Get distance traveled from the encoder manager
    float currentDistance = encoderManager.getDistanceTraveledCm();
    
    // Check if we've reached or exceeded the target distance
    if (currentDistance < targetDistanceCm) return;
    // Distance reached - brake motors
    motorDriver.stop_both_motors();
    
    // Reset distance movement state
    distanceMovementInProgress = false;
}

void BytecodeVM::stopProgram() {
    resetStateVariables(true);
    MessageProcessor::getInstance().resetCommandState();

    stoppedDueToUsbSafety = false; // Reset safety flag when manually stopping

    Speaker::getInstance().setMuted(true);
    rgbLed.turn_all_leds_off();
    motorDriver.brake_if_moving();
    return;
}

void BytecodeVM::resetStateVariables(bool isFullReset) {
    pc = 0;
    delayUntil = 0;
    waitingForDelay = false;
    lastComparisonResult = false;
    
    turningInProgress = false;
    targetTurnDegrees = 0;
    initialTurnYaw = 0;
    turnClockwise = true;
    turnStartTime = 0;
    
    timedMotorMovementInProgress = false;
    distanceMovementInProgress = false;
    motorMovementEndTime = 0;
    targetDistanceCm = 0.0f;
    waitingForButtonPressToStart = false;

    // ADD THESE USB safety resets:
    stoppedDueToUsbSafety = false;
    // Note: Don't reset programContainsMotors or lastUsbState here as they persist across pause/resume

    // Force reset motor driver state completely
    motorDriver.force_reset_motors();

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
}

void BytecodeVM::togglePause() {
    if (!program || isPaused == PROGRAM_NOT_STARTED) {
        SerialQueueManager::getInstance().queueMessage("togglePause: No program loaded");
        return;
    }
    
    if (isPaused == PAUSED) resumeProgram();
    else pauseProgram();
}

void BytecodeVM::pauseProgram() {
    if (!program || isPaused == PAUSED) {
        SerialQueueManager::getInstance().queueMessage("pauseProgram: Already paused or no program");
        return;
    }
    
    resetStateVariables();
    MessageProcessor::getInstance().resetCommandState();

    Speaker::getInstance().setMuted(true);
    rgbLed.turn_all_leds_off();     
    motorDriver.brake_if_moving();
    
    isPaused = PAUSED;
}

void BytecodeVM::resumeProgram() {
    if (!program || isPaused == RUNNING) {
        SerialQueueManager::getInstance().queueMessage("resumeProgram: Not paused or no program");
        return;
    }
    // canStartProgram() already logs the reason
    if (!canStartProgram()) return;

    isPaused = RUNNING;
    
    // Check if the first instruction is a WAIT_FOR_BUTTON (start block)
    if (programSize > 0 && program[0].opcode == OP_WAIT_FOR_BUTTON) {
        SerialQueueManager::getInstance().queueMessage("Resuming program - skipping initial wait for button");
        pc = 1; // Start after the wait for button instruction
    } else {
        SerialQueueManager::getInstance().queueMessage("Resuming program from beginning");
        pc = 0; // Start from the beginning for scripts without a start block
    }
}

void BytecodeVM::scanProgramForMotors() {
    programContainsMotors = false;
    
    if (!program || programSize == 0) return;
    
    // Define all motor opcodes that should block USB execution
    const BytecodeOpCode motorOpcodes[] = {
        OP_MOTOR_FORWARD,
        OP_MOTOR_BACKWARD, 
        OP_MOTOR_STOP,
        OP_MOTOR_TURN,
        OP_MOTOR_FORWARD_TIME,
        OP_MOTOR_BACKWARD_TIME,
        OP_MOTOR_FORWARD_DISTANCE,
        OP_MOTOR_BACKWARD_DISTANCE
    };
    
    // Scan entire program for any motor commands
    for (uint16_t i = 0; i < programSize; i++) {
        for (const auto& motorOpcode : motorOpcodes) {
            if (program[i].opcode == motorOpcode) {
                programContainsMotors = true;
                SerialQueueManager::getInstance().queueMessage("Program contains motor commands - USB safety restrictions apply");
                return;
            }
        }
    }
    
    SerialQueueManager::getInstance().queueMessage("Program contains no motor commands - safe for USB execution");
}

void BytecodeVM::checkUsbSafetyConditions() {
    bool currentUsbState = SerialManager::getInstance().isSerialConnected();
    
    // Detect USB connection change (disconnected -> connected)
    if (!lastUsbState && currentUsbState) {
        handleUsbConnect();
    }
    
    lastUsbState = currentUsbState;
}

void BytecodeVM::handleUsbConnect() {
    // If program contains motors and is currently running, stop it
    if (programContainsMotors && isPaused == RUNNING) {
        SerialQueueManager::getInstance().queueMessage("USB connected - stopping motor program for safety");
        stopProgram();
        stoppedDueToUsbSafety = true;
    }
}

bool BytecodeVM::canStartProgram() {
    // Block start if program contains motors and USB is connected
    if (!programContainsMotors || !SerialManager::getInstance().isSerialConnected()) return true;

    SerialQueueManager::getInstance().queueMessage("Cannot start motor program while USB connected - disconnect USB first");
    SerialManager::getInstance().sendJsonMessage(RouteType::MOTORS_DISABLED_USB, "Cannot start motor program while USB connected - disconnect USB first");
    return false;
}
