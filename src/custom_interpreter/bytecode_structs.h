#pragma once

// Operation codes (opcodes)
enum BytecodeOpCode : uint8_t {
    // System operations
    OP_NOP = 0x00,      // No operation
    OP_END = 0x01,      // End sequence
    OP_DELAY = 0x02,    // Delay (ms)
    
    // LED operations
    OP_SET_LED = 0x10,  // Set specific LED
    OP_SET_ALL_LEDS = 0x11, // Set all LEDs
    
    // Reserved for future extensions
    OP_READ_SENSOR = 0x20,  // Reserved for sensors
    OP_COMPARE = 0x30,      // Reserved for conditionals
    OP_JUMP = 0x31,         // Reserved for control flow

    OP_DECLARE_VAR = 0x40,
    OP_SET_VAR = 0x41,
};

enum BytecodeVarType : uint8_t {
    VAR_FLOAT = 0x01,
    VAR_INT = 0x02,
    VAR_BOOL = 0x03
};

// LED IDs
enum BytecodeLedID : uint8_t {
    LED_ALL = 0,
    LED_TOP_LEFT = 1,
    LED_TOP_RIGHT = 2,
    LED_MIDDLE_LEFT = 3,
    LED_MIDDLE_RIGHT = 4,
    LED_BACK_LEFT = 5,
    LED_BACK_RIGHT = 6
};

// A single bytecode instruction (4 bytes)
struct BytecodeInstruction {
    BytecodeOpCode opcode;  // What operation to perform
    uint8_t operand1;       // First parameter
    uint8_t operand2;       // Second parameter 
    uint8_t operand3;       // Third parameter
    uint8_t operand4;       // Third parameter
};
