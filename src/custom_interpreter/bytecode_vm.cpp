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

bool BytecodeVM::isRunning() const {
    return program != nullptr && pc < programSize;
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

        default:
            // Unknown opcode, stop execution
            pc = programSize;
            break;
    }
}
