#include "bytecode_vm.h"

// Static mapping of opcodes to required sensors
const std::map<BytecodeOpCode, std::vector<BytecodeVM::SensorType>> BytecodeVM::opcodeToSensors = {
    // This map is to control what sensors need to be polled for various OpCodes
    // Ie: For motor turn, we need to poll encoders, and quaternion
    {OP_MOTOR_FORWARD_TIME, {SENSOR_QUATERNION}},
    {OP_MOTOR_BACKWARD_TIME, {SENSOR_QUATERNION}},
    {OP_MOTOR_FORWARD_DISTANCE, {SENSOR_QUATERNION}},
    {OP_MOTOR_BACKWARD_DISTANCE, {SENSOR_QUATERNION}},
    {OP_MOTOR_TURN, {SENSOR_QUATERNION}}
};

BytecodeVM::~BytecodeVM() {
    resetStateVariables(true);
}

bool BytecodeVM::loadProgram(const uint8_t* byteCode, uint16_t size) {
    // Free any existing program
    stopProgram();

    // Validate bytecode size (must be multiple of 20 now)
    if (size % INSTRUCTION_SIZE != 0 || size / INSTRUCTION_SIZE > MAX_PROGRAM_SIZE) {
        return false;
    }
    
    programSize = size / INSTRUCTION_SIZE;
    program = new(std::nothrow) BytecodeInstruction[programSize];
    if (!program) return false;

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

    scanProgramForMotors();
    activateSensorsForProgram(); // Activate sensors needed by the program
    stoppedDueToUsbSafety = false; // Reset safety flag on new program load

    return true;
}

void BytecodeVM::update() {
    checkUsbSafetyConditions();
    if (!program || isPaused == PauseState::PAUSED) return;

    // Check if program has naturally completed (pc reached or exceeded program size)
    if (pc >= programSize) {
        if (isPaused != PROGRAM_FINISHED) {
            isPaused = PROGRAM_FINISHED;
        }
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
    if (TurningManager::getInstance().isActive()) {
        TurningManager::getInstance().update();
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
            return false;  // Invalid register
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
            return false;  // Invalid register
        }
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

        case OP_SET_LED: {
            // Set specific LED to color
            BytecodeLedID ledId = static_cast<BytecodeLedID>(instr.operand1); // Cast to ensure range
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
            BytecodeSensorType sensorType = static_cast<BytecodeSensorType>(instr.operand1);  // Which sensor to read
            uint16_t regId = instr.operand2;       // Register to store result
            
            if (regId < MAX_REGISTERS) {
                float value = 0.0f;
                bool skipDefaultAssignment = false;  // Add this flag
                
                // Read the appropriate sensor
                switch (sensorType) {
                    case SENSOR_PITCH:
                        value = SensorDataBuffer::getInstance().getLatestPitch();
                        break;
                    case SENSOR_ROLL:
                        value = SensorDataBuffer::getInstance().getLatestRoll();
                        break;
                    case SENSOR_YAW:
                        value = SensorDataBuffer::getInstance().getLatestYaw();
                        break;
                    case SENSOR_ACCEL_X:
                        value = SensorDataBuffer::getInstance().getLatestXAccel();
                        break;
                    case SENSOR_ACCEL_Y:
                        value = SensorDataBuffer::getInstance().getLatestYAccel();
                        break;
                    case SENSOR_ACCEL_Z:
                        value = SensorDataBuffer::getInstance().getLatestZAccel();
                        break;
                    case SENSOR_ACCEL_MAG:
                        value = SensorDataBuffer::getInstance().getLatestAccelMagnitude();
                        break;
                    case SENSOR_ROT_RATE_X:
                        value = SensorDataBuffer::getInstance().getLatestXRotationRate();
                        break;
                    case SENSOR_ROT_RATE_Y:
                        value = SensorDataBuffer::getInstance().getLatestYRotationRate();
                        break;
                    case SENSOR_ROT_RATE_Z:
                        value = SensorDataBuffer::getInstance().getLatestZRotationRate();
                        break;
                    case SENSOR_MAG_FIELD_X:
                        value = SensorDataBuffer::getInstance().getLatestMagneticFieldX();
                        break;
                    case SENSOR_MAG_FIELD_Y:
                        value = SensorDataBuffer::getInstance().getLatestMagneticFieldY();
                        break;
                    case SENSOR_MAG_FIELD_Z:
                        value = SensorDataBuffer::getInstance().getLatestMagneticFieldZ();
                        break;
                    case SENSOR_SIDE_LEFT_PROXIMITY: {
                        uint16_t counts = SensorDataBuffer::getInstance().getLatestLeftSideTofCounts();
                        registers[regId].asBool = (counts > LEFT_PROXIMITY_THRESHOLD);
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;  // Set flag to skip default assignment
                        break;
                    }
                    case SENSOR_SIDE_RIGHT_PROXIMITY: {
                        uint16_t counts = SensorDataBuffer::getInstance().getLatestRightSideTofCounts();
                        registers[regId].asBool = (counts > RIGHT_PROXIMITY_THRESHOLD);
                        registerTypes[regId] = VAR_BOOL;
                        registerInitialized[regId] = true;
                        skipDefaultAssignment = true;  // Set flag to skip default assignment
                        break;
                    }
                    case SENSOR_FRONT_PROXIMITY: {
                        bool isObjectDetected = SensorDataBuffer::getInstance().isObjectDetectedTof();
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
            motorDriver.updateMotorPwm(motorSpeed, motorSpeed);
            break;
        }
        
        case OP_MOTOR_BACKWARD: {
            // Convert percentage (0-100) to motor speed (0-MAX_MOTOR_SPEED)
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand1), 0, 100);
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_SPEED);
            
            // Set both motors to backward (negative speed)
            motorDriver.updateMotorPwm(-motorSpeed, -motorSpeed);
            break;
        }
        
        case OP_MOTOR_STOP: {
            motorDriver.resetCommandState(true);
            break;
        }
        
        case OP_MOTOR_TURN: {
            bool clockwise = (instr.operand1 > 0);
            float degrees = instr.operand2;
            
            // Use TurningManager for precise turning
            float signedDegrees = clockwise ? degrees : -degrees;
            if (!TurningManager::getInstance().startTurn(signedDegrees)) {
                SerialQueueManager::getInstance().queueMessage("Failed to start turn - turn already in progress");
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
            motorDriver.updateMotorPwm(motorSpeed, motorSpeed);
            
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
            motorDriver.updateMotorPwm(-motorSpeed, -motorSpeed);
            
            // Set up timed movement
            timedMotorMovementInProgress = true;
            motorMovementEndTime = millis() + static_cast<unsigned long>(seconds * 1000.0f);
            
            break;
        }

        case OP_MOTOR_FORWARD_DISTANCE: {
            float distanceIn = instr.operand1;
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);
            
            // Validate parameters
            if (distanceIn <= 0.0f) {
                SerialQueueManager::getInstance().queueMessage("Invalid distance value for forward movement");
                break;
            }
            
            if (throttlePercent > 100) {
                throttlePercent = 100;  // Clamp to valid range
            }
            
            // Convert percentage to motor speed
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_SPEED);
            
            // Reset distance tracking - store current distance as starting point
            startingDistanceIn = SensorDataBuffer::getInstance().getLatestDistanceTraveledIn();
            
            // Set up distance movement
            distanceMovementInProgress = true;
            targetDistanceIn = distanceIn;

            // Set motors to forward motion
            motorDriver.updateMotorPwm(motorSpeed, motorSpeed);
            
            break;
        }

        case OP_MOTOR_BACKWARD_DISTANCE: {
            float distanceIn = instr.operand1;
            uint8_t throttlePercent = constrain(static_cast<uint8_t>(instr.operand2), 0, 100);
            
            // Validate parameters
            if (distanceIn <= 0.0f) {
                SerialQueueManager::getInstance().queueMessage("Invalid distance value for backward movement");
                break;
            }
            
            if (throttlePercent > 100) {
                throttlePercent = 100;  // Clamp to valid range
            }
            
            // Convert percentage to motor speed
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, MAX_MOTOR_SPEED);
            
            // Reset distance tracking - store current distance as starting point
            startingDistanceIn = SensorDataBuffer::getInstance().getLatestDistanceTraveledIn();
            
            // Set up distance movement
            distanceMovementInProgress = true;
            targetDistanceIn = distanceIn;
            
            // Set motors to backward motion
            motorDriver.updateMotorPwm(-motorSpeed, -motorSpeed);
            
            break;
        }

        case OP_WAIT_FOR_BUTTON: {
            waitingForButtonPressToStart = true;

            // Don't increment PC here - we'll do it when the button is pressed
            return; // Return without incrementing PC
        }

        case PLAY_SOUND: {
            // Extract sound ID from operand1
            uint8_t soundId = static_cast<uint8_t>(instr.operand1);
            SoundType soundType = static_cast<SoundType>(soundId);
            
            // Validate sound type is within valid range
            if (soundId >= static_cast<uint8_t>(SoundType::CHIME) && 
                soundId <= static_cast<uint8_t>(SoundType::ROBOT)) {
                Speaker::getInstance().playFile(soundType);
            } else {
                SerialQueueManager::getInstance().queueMessage("Invalid sound ID: " + String(soundId));
            }
            break;
        }

        default:
            // Unknown opcode, stop execution
            pc = programSize;
            break;
    }
}

void BytecodeVM::updateTimedMotorMovement() {
    // Check if the timed movement has completed
    if (millis() < motorMovementEndTime) return;
    // Movement complete - brake motors
    motorDriver.resetCommandState(true);

    // Reset timed movement state
    timedMotorMovementInProgress = false;
}

void BytecodeVM::updateDistanceMovement() {
    // Get distance traveled from sensor data buffer (relative to starting point)
    float totalDistance = SensorDataBuffer::getInstance().getLatestDistanceTraveledIn();
    float currentDistance = totalDistance - startingDistanceIn;
    
    // Check if we've reached or exceeded the target distance (with small tolerance for overshoot)
    float tolerance = 0.5f; // 0.5cm tolerance to prevent overshoot issues
    if (abs(currentDistance) < (targetDistanceIn - tolerance)) return;
    
    // Distance reached - brake motors and clear any pending commands
    motorDriver.resetCommandState(true);

    // **ADD THIS LINE:** Disable straight line drive when distance movement completes
    StraightLineDrive::getInstance().disable();
    vTaskDelay(pdMS_TO_TICKS(250));

    // Reset distance movement state
    distanceMovementInProgress = false;
}

void BytecodeVM::stopProgram() {
    resetStateVariables(true);
    return;
}

void BytecodeVM::resetStateVariables(bool isFullReset) {
    pc = 0;
    delayUntil = 0;
    waitingForDelay = false;
    lastComparisonResult = false;
    
    // Reset TurningManager state
    TurningManager::getInstance().completeNavigation(false);
    StraightLineDrive::getInstance().disable();
    // StraightLineDrive::getInstance().disable();
    timedMotorMovementInProgress = false;
    distanceMovementInProgress = false;
    motorMovementEndTime = 0;
    targetDistanceIn = 0.0f;
    waitingForButtonPressToStart = false;

    // ADD THESE USB safety resets:
    stoppedDueToUsbSafety = false;
    // Note: Don't reset programContainsMotors or lastUsbState here as they persist across pause/resume

    // Force reset motor driver state completely
    motorDriver.resetCommandState(false);

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
    Speaker::getInstance().stopAllSounds();
}

void BytecodeVM::pauseProgram() {
    if (!program || isPaused == PAUSED) return;

    resetStateVariables();    
    isPaused = PAUSED;
}

void BytecodeVM::resumeProgram() {
    if (!program || isPaused == RUNNING) {
        SerialQueueManager::getInstance().queueMessage("resumeProgram: Not paused or no program");
        return;
    }

    if (!canStartProgram()) return;

    // Reset state variables for finished programs to start fresh
    if (isPaused == PROGRAM_FINISHED) {
        resetStateVariables();
    }

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

void BytecodeVM::activateSensorsForProgram() {
    if (!program || programSize == 0) return;
    
    // Track which sensors are needed
    bool needQuaternion = false;
    bool needAccelerometer = false;  
    bool needGyroscope = false;
    bool needMagnetometer = false;
    bool needTof = false;
    bool needSideTof = false;
    
    // Scan through the entire program
    for (uint16_t i = 0; i < programSize; i++) {
        const BytecodeInstruction& instr = program[i];
        
        // Handle OP_READ_SENSOR dynamically based on sensor type
        if (instr.opcode == OP_READ_SENSOR) {
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
                    needTof = true;
                    break;
            }
        } else {
            // Handle other opcodes using the static mapping
            auto it = opcodeToSensors.find(instr.opcode);
            if (it != opcodeToSensors.end()) {
                for (SensorType sensorType : it->second) {
                    switch (sensorType) {
                        case SENSOR_QUATERNION: needQuaternion = true; break;
                        case SENSOR_ACCELEROMETER: needAccelerometer = true; break;
                        case SENSOR_GYROSCOPE: needGyroscope = true; break;
                        case SENSOR_MAGNETOMETER: needMagnetometer = true; break;
                        case SENSOR_TOF: needTof = true; break;
                        case SENSOR_SIDE_TOF: needSideTof = true; break;
                    }
                }
            }
        }
    }
    
    // Activate required sensors by setting their last_request timestamps
    SensorDataBuffer& buffer = SensorDataBuffer::getInstance();
    ReportTimeouts& timeouts = buffer.getReportTimeouts();
    uint32_t currentTime = millis();
    
    if (needQuaternion) {
        timeouts.quaternion_last_request.store(currentTime);
        SerialQueueManager::getInstance().queueMessage("Activated quaternion sensor for program");
    }
    if (needAccelerometer) {
        timeouts.accelerometer_last_request.store(currentTime);
        SerialQueueManager::getInstance().queueMessage("Activated accelerometer for program");
    }
    if (needGyroscope) {
        timeouts.gyroscope_last_request.store(currentTime);
        SerialQueueManager::getInstance().queueMessage("Activated gyroscope for program");
    }
    if (needMagnetometer) {
        timeouts.magnetometer_last_request.store(currentTime);
        SerialQueueManager::getInstance().queueMessage("Activated magnetometer for program");
    }
    if (needTof) {
        timeouts.tof_last_request.store(currentTime);
        SerialQueueManager::getInstance().queueMessage("Activated multizone TOF for program");
    }
    if (needSideTof) {
        timeouts.side_tof_last_request.store(currentTime);
        SerialQueueManager::getInstance().queueMessage("Activated side TOF for program");
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
                return;
            }
        }
    }
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
        stopProgram();
        stoppedDueToUsbSafety = true;
    }
}

bool BytecodeVM::canStartProgram() {
    // Block start if program contains motors and USB is connected
    if (!programContainsMotors || !SerialManager::getInstance().isSerialConnected()) return true;

    SerialManager::getInstance().sendJsonMessage(RouteType::MOTORS_DISABLED_USB, "Cannot start motor program while USB connected - disconnect USB first");
    return false;
}
