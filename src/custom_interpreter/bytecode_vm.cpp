#include "./bytecode_vm.h"

BytecodeVM::~BytecodeVM() {
    if (program) {
        delete[] program;
    }
}

bool BytecodeVM::loadProgram(const uint8_t* byteCode, uint16_t size) {
    // Free any existing program
    if (program) {
        delete[] program;
        program = nullptr;
    }
    motorDriver.force_reset_motors();

    // Validate bytecode size (must be multiple of 20 now)
    if (size % INSTRUCTION_SIZE != 0 || size / INSTRUCTION_SIZE > MAX_PROGRAM_SIZE) {
        return false;
    }
    
    programSize = size / INSTRUCTION_SIZE;
    program = new BytecodeInstruction[programSize];
    
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

    // Reset VM state
    pc = 0;
    waitingForDelay = false;
    lastComparisonResult = false;
    
    return true;
}

void BytecodeVM::update() {
    if (!program || pc >= programSize) {
        return;
    }

    // Check if we're waiting for a delay to complete
    if (waitingForDelay) {
        if (millis() < delayUntil) {
            return; // Still waiting
        }
        waitingForDelay = false;
    }
    
    // Handle turning operation if in progress
    if (turningInProgress) {
        updateTurning();
        return; // Don't execute next instruction until turn is complete
    }

    // Execute current instruction
    executeInstruction(program[pc]);
    pc++; // Move to next instruction
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
        case OP_EQUAL: return leftValue == rightValue;
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
            }
            break;
        }

        case OP_READ_SENSOR: {
            uint8_t sensorType = static_cast<uint8_t>(instr.operand1);  // Which sensor to read
            uint16_t regId = instr.operand2;       // Register to store result
            
            if (regId < MAX_REGISTERS) {
                float value = 0.0f;
                
                // Read the appropriate sensor
                switch (sensorType) {
                    case SENSOR_PITCH:
                        value = Sensors::getInstance().getPitch();
                        break;
                    case SENSOR_ROLL:
                        value = Sensors::getInstance().getRoll();
                        break;
                    case SENSOR_YAW:
                        value = Sensors::getInstance().getYaw();
                        break;
                    case SENSOR_ACCEL_X:
                        value = Sensors::getInstance().getXAccel();
                        break;
                    case SENSOR_ACCEL_Y:
                        value = Sensors::getInstance().getYAccel();
                        break;
                    case SENSOR_ACCEL_Z:
                        value = Sensors::getInstance().getZAccel();
                        break;
                    case SENSOR_ACCEL_MAG:
                        value = Sensors::getInstance().getAccelMagnitude();
                        break;
                    case SENSOR_ROT_RATE_X:
                        value = Sensors::getInstance().getXRotationRate();
                        break;
                    case SENSOR_ROT_RATE_Y:
                        value = Sensors::getInstance().getYRotationRate();
                        break;
                    case SENSOR_ROT_RATE_Z:
                        value = Sensors::getInstance().getZRotationRate();
                        break;
                    case SENSOR_MAG_FIELD_X:
                        value = Sensors::getInstance().getMagneticFieldX();
                        break;
                    case SENSOR_MAG_FIELD_Y:
                        value = Sensors::getInstance().getMagneticFieldY();
                        break;
                    case SENSOR_MAG_FIELD_Z:
                        value = Sensors::getInstance().getMagneticFieldZ();
                        break;
                    default:
                        // Unknown sensor type
                        Serial.print("Unknown sensor type: ");
                        Serial.println(sensorType);
                        break;
                }
                
                // Store the sensor value in the register
                registers[regId].asFloat = value;
                registerTypes[regId] = VAR_FLOAT;
                registerInitialized[regId] = true;
            }
            break;
        }

        case OP_SET_ALL_LEDS: {
            uint8_t r = static_cast<uint8_t>(instr.operand1);     // Cast to uint8_t for
            uint8_t g = static_cast<uint8_t>(instr.operand2);     // RGB values (0-255)
            uint8_t b = static_cast<uint8_t>(instr.operand3);        

            // operand4 is unused for this operation
            rgbLed.set_all_leds_to_color(r, g, b);
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
            uint16_t jumpOffset = static_cast<uint16_t>(instr.operand1);
            
            // Calculate new PC - we need to subtract 1 because the PC will be incremented
            // after this instruction executes
            uint16_t targetInstruction = pc + (jumpOffset / INSTRUCTION_SIZE);  // 20 bytes per instruction
            pc = targetInstruction - 1;
        
            break;
        }

        case OP_JUMP_BACKWARD: {
            // Backward jump (used in for loops)
            uint16_t jumpOffset = static_cast<uint16_t>(instr.operand1);
            
            // For backward jumps, SUBTRACT the offset
            uint16_t targetInstruction = pc - (jumpOffset / INSTRUCTION_SIZE);  // 20 bytes per instruction
            pc = targetInstruction - 1; // -1 because pc will be incremented after
            break;
        }

        case OP_JUMP_IF_TRUE: {
            // Conditional jump if last comparison was true
            if (lastComparisonResult) {
                uint16_t jumpOffset = static_cast<uint16_t>(instr.operand1);
                uint16_t targetInstruction = pc + (jumpOffset / INSTRUCTION_SIZE);  // 20 bytes per instruction
                pc = targetInstruction - 1;
            }
            break;
        }

        case OP_JUMP_IF_FALSE: {
            // Conditional jump if last comparison was false
            if (!lastComparisonResult) {
                uint16_t jumpOffset = static_cast<uint16_t>(instr.operand1);
                uint16_t targetInstruction = pc + (jumpOffset / INSTRUCTION_SIZE);  // 20 bytes per instruction
                pc = targetInstruction - 1;
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
            // Jump back to the corresponding WHILE_START
            uint16_t offsetToStart = static_cast<uint16_t>(instr.operand1);
            
            // Calculate the new PC (jumping backwards)
            // We need to make sure we don't go out of bounds
            if (offsetToStart <= static_cast<float>(pc * INSTRUCTION_SIZE)) {  // 20 bytes per instruction
                pc = pc - (offsetToStart / INSTRUCTION_SIZE);
            } else {
                // Safety check - if offset is invalid, stop execution
                pc = programSize;
                Serial.println("Invalid loop jump - stopping execution");
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
            // Convert percentage (0-100) to motor speed (0-255)
            uint8_t throttlePercent = static_cast<uint8_t>(instr.operand1);
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, 255);
            
            // Set both motors to forward at calculated speed
            motorDriver.set_motor_speeds(motorSpeed, motorSpeed);
            motorDriver.update_motor_speeds(true); // Optional: enable ramping
            break;
        }
        
        case OP_MOTOR_BACKWARD: {
            // Convert percentage (0-100) to motor speed (0-255)
            uint8_t throttlePercent = static_cast<uint8_t>(instr.operand1);
            uint8_t motorSpeed = map(throttlePercent, 0, 100, 0, 255);
            
            // Set both motors to backward (negative speed)
            motorDriver.set_motor_speeds(-motorSpeed, -motorSpeed);
            motorDriver.update_motor_speeds(true); // Optional: enable ramping
            break;
        }
        
        case OP_MOTOR_STOP: {
            // Stop both motors
            motorDriver.brake_both_motors();
            break;
        }
        
        case OP_MOTOR_TURN: {
            bool clockwise = (instr.operand1 > 0);
            float degrees = instr.operand2;
            
            // Initialize turning state
            turningInProgress = true;
            targetTurnDegrees = degrees;
            initialTurnYaw = Sensors::getInstance().getYaw();
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

        default:
            // Unknown opcode, stop execution
            pc = programSize;
            Serial.printf("Unknown code - stopping execution %d\n", instr.opcode);
            break;
    }
}

void BytecodeVM::updateTurning() {
    // Get current yaw
    float currentYaw = Sensors::getInstance().getYaw();
    
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
    bool timeout = elapsed > 10000; // 10 second timeout
    
    // Check if turn is complete
    if (rotationDelta >= targetTurnDegrees || timeout) {
        // Turn complete - stop motors
        motorDriver.brake_both_motors();
        turningInProgress = false;
    }
}

bool BytecodeVM::stopProgram() {
    bool programStopped = false;
    if (program) {
        delete[] program;
        program = nullptr;
        programStopped = true;
    }
    pc = 0;
    waitingForDelay = false;
    lastComparisonResult = false;

    rgbLed.turn_led_off();
    motorDriver.brake_both_motors();
    return programStopped;
}
