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

    // Validate bytecode size (must be multiple of 10 now)
    if (size % 10 != 0 || size / 10 > MAX_PROGRAM_SIZE) {
        return false;
    }
    
    programSize = size / 10;
    program = new BytecodeInstruction[programSize];
    
    Serial.println("=== Bytecode Dump ===");
    // Iterate through program indices (0 to programSize-1)
    for (uint16_t i = 0; i < programSize; i++) {
        // Calculate byte offset for each instruction
        uint16_t offset = i * 10;
        
        // Read opcode (2 bytes, little-endian)
        program[i].opcode = (BytecodeOpCode)(byteCode[offset] | (byteCode[offset + 1] << 8));
        
        // Read operands (2 bytes each, little-endian)
        program[i].operand1 = byteCode[offset + 2] | (byteCode[offset + 3] << 8);
        program[i].operand2 = byteCode[offset + 4] | (byteCode[offset + 5] << 8);
        program[i].operand3 = byteCode[offset + 6] | (byteCode[offset + 7] << 8);
        program[i].operand4 = byteCode[offset + 8] | (byteCode[offset + 9] << 8);
        
        // Add detailed logging
        Serial.printf("Instruction %d: opcode=%d operand1=%d operand2=%d operand3=%d operand4=%d\n", 
                     i, program[i].opcode, program[i].operand1, program[i].operand2, 
                     program[i].operand3, program[i].operand4);
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

    // Execute current instruction
    executeInstruction(program[pc]);
    pc++; // Move to next instruction
}

bool BytecodeVM::compareValues(ComparisonOp op, int16_t leftOperand, int16_t rightValue) {
    float leftValue;
    
    // Check if high bit is set, indicating a register
    // For int16_t, use bit 15 (the sign bit) as the register indicator
    if (leftOperand & 0x8000) {
        uint16_t regId = leftOperand & 0x7FFF;  // Use lower 15 bits for register ID
        
        if (regId < MAX_REGISTERS && registerInitialized[regId]) {
            // Get value from register (unchanged)
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
    
    // Perform comparison (unchanged)
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
            uint16_t delayMs = instr.operand1;  // Now operand1 directly stores the delay time
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
            int16_t leftValue = instr.operand2;
            int16_t rightValue = instr.operand3;
            
            lastComparisonResult = compareValues(op, leftValue, rightValue);

            break;
        }

        case OP_JUMP: {
            // Unconditional jump
            uint16_t jumpOffset = instr.operand1;  // Now operand1 directly stores the offset
            
            // Calculate new PC - we need to subtract 1 because the PC will be incremented
            // after this instruction executes
            uint16_t targetInstruction = pc + (jumpOffset / 10);  // 10 bytes per instruction
            pc = targetInstruction - 1;
        
            break;
        }

        case OP_JUMP_BACKWARD: {
            // Backward jump (used in for loops)
            uint16_t jumpOffset = instr.operand1;  // Now operand1 directly stores the offset
            
            // For backward jumps, SUBTRACT the offset
            uint16_t targetInstruction = pc - (jumpOffset / 10);  // 10 bytes per instruction
            pc = targetInstruction - 1; // -1 because pc will be incremented after
            break;
        }

        case OP_JUMP_IF_TRUE: {
            // Conditional jump if last comparison was true
            if (lastComparisonResult) {
                uint16_t jumpOffset = instr.operand1;  // Now operand1 directly stores the offset
                uint16_t targetInstruction = pc + (jumpOffset / 10);  // 10 bytes per instruction
                pc = targetInstruction - 1;
            }
            break;
        }

        case OP_JUMP_IF_FALSE: {
            // Conditional jump if last comparison was false
            if (!lastComparisonResult) {
                uint16_t jumpOffset = instr.operand1;  // Now operand1 directly stores the offset
                uint16_t targetInstruction = pc + (jumpOffset / 10);  // 10 bytes per instruction
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
            uint16_t regId = instr.operand1; // Now can support more than 255 registers
            
            if (regId < MAX_REGISTERS) {
                switch (registerTypes[regId]) {
                    case VAR_FLOAT: {
                        // Reconstruct float from int16_t values (with some precision loss)
                        // Using a simple approach: convert first two operands to a float
                        float value = static_cast<float>(instr.operand2) + 
                                     (static_cast<float>(instr.operand3) / 10000.0f);
                        registers[regId].asFloat = value;
                        break;
                    }
                    case VAR_INT: {
                        // Use operand2 and operand3 to create a 32-bit integer
                        int32_t value = ((int32_t)instr.operand3 << 16) | 
                                       (instr.operand2 & 0xFFFF);
                        registers[regId].asInt = value;
                        break;
                    }
                    case VAR_BOOL:
                        registers[regId].asBool = (instr.operand2 != 0);
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
            uint16_t offsetToStart = instr.operand1;  // Now operand1 directly stores the offset
            
            // Calculate the new PC (jumping backwards)
            // We need to make sure we don't go out of bounds
            if (offsetToStart <= pc * 10) {  // 10 bytes per instruction
                pc = pc - (offsetToStart / 10);
            } else {
                // Safety check - if offset is invalid, stop execution
                pc = programSize;
                Serial.println("Invalid loop jump - stopping execution");
            }
            break;
        }

        case OP_FOR_INIT: {
            // Initialize loop counter
            uint16_t regId = instr.operand1;
            
            if (regId < MAX_REGISTERS) {
                registerTypes[regId] = VAR_INT;
                
                // Use operand2 and operand3 to create a 32-bit integer
                int32_t initValue = ((int32_t)instr.operand3 << 16) | 
                                   (instr.operand2 & 0xFFFF);
                
                registers[regId].asInt = initValue;
                registerInitialized[regId] = true;
            }
            break;
        }
        
        case OP_FOR_CONDITION: {
            // Check if counter < end value
            uint16_t regId = instr.operand1;
            
            if (regId < MAX_REGISTERS && registerInitialized[regId]) {
                // Use operand2 and operand3 to create a 32-bit integer
                int32_t endValue = ((int32_t)instr.operand3 << 16) | 
                                  (instr.operand2 & 0xFFFF);
                
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
            uint16_t regId = instr.operand1;

            if (regId < MAX_REGISTERS && registerInitialized[regId]) {
                registers[regId].asInt++;
            }
            break;
        }

        default:
            // Unknown opcode, stop execution
            pc = programSize;
            Serial.printf("Unknown code - stopping execution %d\n", instr.opcode);
            break;
    }
}
