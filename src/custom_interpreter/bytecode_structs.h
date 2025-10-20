#pragma once
#include <Arduino.h>

// Operation codes (opcodes)
enum BytecodeOpCode : uint32_t {
    // System operations
    OP_NOP = 0x00,      // No operation
    OP_END = 0x01,      // End sequence
    OP_WAIT = 0x02,    // Delay (s)
    OP_WAIT_FOR_BUTTON = 0x03,
    CHECK_RIGHT_BUTTON_PRESS = 0x04,

    // LED operations
    OP_SET_LED = 0x10,  // Set specific LED
    OP_SET_ALL_LEDS = 0x11, // Set all LEDs
    
    // Reserved for future extensions
    OP_READ_SENSOR = 0x20,  // Reserved for sensors
    
    // Control flow operations
    OP_COMPARE = 0x30,      // Compare values
    OP_JUMP = 0x31,         // Unconditional jump
    OP_JUMP_IF_TRUE = 0x32, // Jump if comparison was true
    OP_JUMP_IF_FALSE = 0x33, // Jump if comparison was false
    OP_WHILE_START = 0x34,  // Marks the start of a while loop
    OP_WHILE_END = 0x35,    // End of while loop, jumps back to start

    OP_FOR_INIT = 0x36,     // Initialize loop counter
    OP_FOR_CONDITION = 0x37, // Check loop condition
    OP_FOR_INCREMENT = 0x38, // Increment loop counter
    OP_JUMP_BACKWARD = 0x39, // Backward jump (for loops)

    // Variable operations
    OP_DECLARE_VAR = 0x40,
    OP_SET_VAR = 0x41,

    OP_MOTOR_GO = 0x50,   // Forward movement at specified throttle
    OP_MOTOR_STOP = 0x52,      // Stop all motors
    OP_MOTOR_TURN = 0x53,      // Turn by specified degrees
    OP_MOTOR_GO_TIME = 0x54,
    OP_MOTOR_GO_DISTANCE = 0x56,

    PLAY_SOUND = 0x60,
    PLAY_TONE = 0x61,
};

// Comparison operators
enum ComparisonOp : uint8_t {
    OP_EQUAL = 0x01,          // ==
    OP_NOT_EQUAL = 0x02,      // !=
    OP_GREATER_THAN = 0x03,   // >
    OP_LESS_THAN = 0x04,      // 
    OP_GREATER_EQUAL = 0x05,  // >=
    OP_LESS_EQUAL = 0x06,     // <=
};

enum BytecodeSensorType : uint8_t {
    SENSOR_PITCH = 0,
    SENSOR_ROLL = 1,
    SENSOR_YAW = 2,
    SENSOR_ACCEL_X = 3,
    SENSOR_ACCEL_Y = 4,
    SENSOR_ACCEL_Z = 5,
    SENSOR_ACCEL_MAG = 6,
    SENSOR_ROT_RATE_X = 7,
    SENSOR_ROT_RATE_Y = 8,
    SENSOR_ROT_RATE_Z = 9,
    SENSOR_MAG_FIELD_X = 10,
    SENSOR_MAG_FIELD_Y = 11,
    SENSOR_MAG_FIELD_Z = 12,
    SENSOR_SIDE_LEFT_PROXIMITY = 13,
    SENSOR_SIDE_RIGHT_PROXIMITY = 14,
    SENSOR_FRONT_PROXIMITY = 15,
    SENSOR_COLOR_RED = 16,
    SENSOR_COLOR_GREEN = 17,
    SENSOR_COLOR_BLUE = 18,
    SENSOR_COLOR_WHITE = 19,
    SENSOR_COLOR_BLACK = 20,
    SENSOR_COLOR_YELLOW = 21,
};

enum ToneType : uint8_t {
    TONE_A = 1,
    TONE_B = 2,
    TONE_C = 3,
    TONE_D = 4,
    TONE_E = 5,
    TONE_F = 6,
    TONE_G = 7
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
    LED_BACK_RIGHT = 6,
    LEFT_HEADLIGHT = 7,
    RIGHT_HEADLIGHT = 8,
};

// A single bytecode instruction (10 bytes)
struct BytecodeInstruction {
    BytecodeOpCode opcode;  // What operation to perform (4 bytes)
    float operand1;       // 4 bytes
    float operand2;       // 4 bytes
    float operand3;       // 4 bytes
    float operand4;       // 4 bytes
};

namespace ColorTypes {
    enum ColorType {
        COLOR_RED,
        COLOR_GREEN,
        COLOR_BLUE,
        COLOR_WHITE,
        COLOR_BLACK,
        COLOR_YELLOW,
        COLOR_NONE  // No clear color detected
    };
}
