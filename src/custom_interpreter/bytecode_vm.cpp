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

    // Validate bytecode size (must be multiple of 5)
    if (size % 5 != 0 || size / 5 > MAX_PROGRAM_SIZE) {
        return false;
    }
    
    // Convert bytecode to instructions
    programSize = size / 5;
    program = new BytecodeInstruction[programSize];
    
    for (uint16_t i = 0; i < programSize; i++) {
        program[i].opcode = (BytecodeOpCode)byteCode[i * 5];
        program[i].operand1 = byteCode[i * 5 + 1];
        program[i].operand2 = byteCode[i * 5 + 2];
        program[i].operand3 = byteCode[i * 5 + 3];
        program[i].operand4 = byteCode[i * 5 + 4];
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

bool BytecodeVM::compareValues(ComparisonOp op, int32_t left, int32_t right) {
    switch (op) {
        case OP_EQUAL:
            return left == right;
        case OP_NOT_EQUAL:
            return left != right;
        case OP_GREATER_THAN:
            return left > right;
        case OP_LESS_THAN:
            return left < right;
        case OP_GREATER_EQUAL:
            return left >= right;
        case OP_LESS_EQUAL:
            return left <= right;
        default:
            // Unknown operator, default to false
            Serial.print("Unknown comparison operator: ");
            Serial.println(op);
            return false;
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
            uint16_t delayMs = instr.operand1 | (instr.operand2 << 8);
            delayUntil = millis() + delayMs;
            waitingForDelay = true;
            break;
        }

        case OP_SET_LED: {
            // Set specific LED to color
            uint8_t ledId = instr.operand1;
            uint8_t r = instr.operand2;
            uint8_t g = instr.operand3;
            uint8_t b = instr.operand4;

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

        case OP_SET_ALL_LEDS: {
            // Set all LEDs to the same color
            uint8_t r = instr.operand1;
            uint8_t g = instr.operand2;
            uint8_t b = instr.operand3;
            // operand4 is unused for this operation
            rgbLed.set_all_leds_to_color(r, g, b);
            break;
        }

        case OP_COMPARE: {
            // Compare two values and store the result
            ComparisonOp op = (ComparisonOp)instr.operand1;
            int32_t leftValue = instr.operand2;
            int32_t rightValue = instr.operand3;
            
            lastComparisonResult = compareValues(op, leftValue, rightValue);

            break;
        }

        case OP_JUMP: {
            // Unconditional jump
            uint16_t jumpOffset = instr.operand1 | (instr.operand2 << 8);
            
            // Calculate new PC - we need to subtract 1 because the PC will be incremented
            // after this instruction executes
            uint16_t targetInstruction = pc + (jumpOffset / 5);
            pc = targetInstruction - 1;
        
            break;
        }

        case OP_JUMP_IF_TRUE: {
            // Conditional jump if last comparison was true
            if (lastComparisonResult) {
                uint16_t jumpOffset = instr.operand1 | (instr.operand2 << 8);
                uint16_t targetInstruction = pc + (jumpOffset / 5);
                pc = targetInstruction - 1;
            }
            break;
        }

        case OP_JUMP_IF_FALSE: {
            // Conditional jump if last comparison was false
            if (!lastComparisonResult) {
                uint16_t jumpOffset = instr.operand1 | (instr.operand2 << 8);
                uint16_t targetInstruction = pc + (jumpOffset / 5);
                pc = targetInstruction - 1;
            }
            break;
        }

        case OP_DECLARE_VAR: {
            uint8_t regId = instr.operand1;
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
            }
            break;
        }
        
        case OP_SET_VAR: {
            uint8_t regId = instr.operand1;
            
            if (regId < MAX_REGISTERS) {
                switch (registerTypes[regId]) {
                    case VAR_FLOAT: {
                        // Reconstruct float from bytes (with some precision loss)
                        registers[regId].asBytes[0] = instr.operand2;
                        registers[regId].asBytes[1] = instr.operand3;
                        registers[regId].asBytes[2] = instr.operand4;
                        registers[regId].asBytes[3] = 0; // Missing byte
                        break;
                    }
                    case VAR_INT: {
                        // 24-bit integer reconstruction
                        int32_t value = instr.operand2 | 
                                      (instr.operand3 << 8) | 
                                      (instr.operand4 << 16);
                        // Sign extension for negative numbers
                        if (instr.operand4 & 0x80) {
                            value |= 0xFF000000;
                        }
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
            uint16_t offsetToStart = instr.operand1 | (instr.operand2 << 8);
            
            // Calculate the new PC (jumping backwards)
            // We need to make sure we don't go out of bounds
            if (offsetToStart <= pc * 5) {
                pc = pc - (offsetToStart / 5);
            } else {
                // Safety check - if offset is invalid, stop execution
                pc = programSize;
                Serial.println("Invalid loop jump - stopping execution");
            }
            break;
        }

        case OP_FOR_INIT: {
            // Initialize loop counter
            uint8_t regId = instr.operand1;
            
            if (regId < MAX_REGISTERS) {
                registerTypes[regId] = VAR_INT;
                
                // Reconstruct integer from bytes
                int32_t initValue = instr.operand2 | 
                                   (instr.operand3 << 8) | 
                                   (instr.operand4 << 16);
                // Sign extension for negative values
                if (instr.operand4 & 0x80) {
                    initValue |= 0xFF000000;
                }
                
                registers[regId].asInt = initValue;
                registerInitialized[regId] = true;
            }
            break;
        }
        
        case OP_FOR_CONDITION: {
            // Check if counter < end value
            uint8_t regId = instr.operand1;
            
            if (regId < MAX_REGISTERS && registerInitialized[regId]) {
                // Reconstruct end value
                int32_t endValue = instr.operand2 | 
                                  (instr.operand3 << 8) | 
                                  (instr.operand4 << 16);
                // Sign extension
                if (instr.operand4 & 0x80) {
                    endValue |= 0xFF000000;
                }
                
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
            uint8_t regId = instr.operand1;

            if (regId < MAX_REGISTERS && registerInitialized[regId]) {
                registers[regId].asInt++;
            }
            break;
        }

        default:
            // Unknown opcode, stop execution
            pc = programSize;
            Serial.println("Unknown code - stopping execution");
            break;
    }
}
