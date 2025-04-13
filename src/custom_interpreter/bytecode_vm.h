#pragma once
#include <Arduino.h>
#include "./bytecode_structs.h"
#include "../utils/singleton.h"
#include "../actuators/led/rgb_led.h"

class BytecodeVM : public Singleton<BytecodeVM> {
    friend class Singleton<BytecodeVM>;
    
    public:
        BytecodeVM() = default;
        ~BytecodeVM();
        
        // Load bytecode program into the VM
        bool loadProgram(const uint8_t* byteCode, uint16_t size);
        
        // Update VM - call this regularly from main loop
        void update();
        
        // Check if a program is still running
        bool isRunning() const;
        
    private:
        static const uint16_t MAX_PROGRAM_SIZE = 128; // Max instructions
        
        BytecodeInstruction* program = nullptr;
        uint16_t programSize = 0;
        uint16_t pc = 0; // Program counter
        unsigned long delayUntil = 0; // For handling delays
        bool waitingForDelay = false;
        
        // Execute the current instruction
        void executeInstruction(const BytecodeInstruction& instr);
};
