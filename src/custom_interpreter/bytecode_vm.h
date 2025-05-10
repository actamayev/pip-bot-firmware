#pragma once
#include <Arduino.h>
#include "../sensors/imu.h"
#include "./bytecode_structs.h"
#include "../utils/singleton.h"
#include "../actuators/buttons.h"
#include "../actuators/led/rgb_led.h"
#include "../actuators/motor_driver.h"
#include "../sensors/multizone_tof_sensor.h"

class BytecodeVM : public Singleton<BytecodeVM> {
    friend class Singleton<BytecodeVM>;
    
    public:
        BytecodeVM() = default;
        ~BytecodeVM();

        // Load bytecode program into the VM
        bool loadProgram(const uint8_t* byteCode, uint16_t size);
        void stopProgram();

        // Update VM - call this regularly from main loop
        void update();

    private:
        BytecodeInstruction* program = nullptr;
        uint16_t programSize = 0;
        uint16_t pc = 0; // Program counter
        unsigned long delayUntil = 0; // For handling delays
        bool waitingForDelay = false;
        bool lastComparisonResult = false; // Stores result of last comparison
        
        static const uint16_t MAX_REGISTERS = 512; // Changed from uint8_t to uint16_t
                                              // to handle values > 255
    
        static const uint8_t INSTRUCTION_SIZE = 20;
        // Union to store different variable types in the same memory
        union RegisterValue {
            float asFloat;
            int32_t asInt;
            bool asBool;
            uint8_t asBytes[4];
        };
        
        RegisterValue registers[MAX_REGISTERS];
        BytecodeVarType registerTypes[MAX_REGISTERS];
        bool registerInitialized[MAX_REGISTERS] = {false};
        
        // Execute instruction implementation
        void executeInstruction(const BytecodeInstruction& instr);
        
        // Helper method for comparisons
        bool compareValues(ComparisonOp op, float leftOperand, float rightValue);

        bool turningInProgress = false;
        const int turnTimeout = 2000; // 1 second timeout for turn operations
        float targetTurnDegrees = 0;
        float initialTurnYaw = 0;
        bool turnClockwise = true;
        unsigned long turnStartTime = 0;
        
        // Helper method for turn operations
        void updateTurning();

        bool timedMotorMovementInProgress = false;
        unsigned long motorMovementEndTime = 0;
        
        // Helper method for timed motor operations
        void updateTimedMotorMovement();

        bool distanceMovementInProgress = false;
        float targetDistanceCm = 0.0f;
        
        // Helper method for distance-based motor operations
        void updateDistanceMovement();

        static const uint16_t LEFT_PROXIMITY_THRESHOLD = 650;
        static const uint16_t RIGHT_PROXIMITY_THRESHOLD = 650;

        bool waitingForButtonPress = false;
        bool waitingForButtonRelease = false;
        void resetStateVariables();
};
