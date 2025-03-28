#pragma once

#include <Arduino.h>
#include "../utils/singleton.h"

class LabDemoManager : public Singleton<LabDemoManager> {
    friend class Singleton<LabDemoManager>;

    public:
        void handleBinaryMessage(const char* data);
        void updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed);
        void handleChimeCommand(); // New method for chime command
        void processPendingCommands();
        LabDemoManager();

    private:
        void executeCommand(int16_t leftSpeed, int16_t rightSpeed);

        static constexpr uint8_t MOTOR_CONTROL_MSG_TYPE = 1;
        static constexpr uint8_t CHIME_MSG_TYPE = 2; // New message type for chime

        // Simple command tracking
        bool isExecutingCommand;
        int16_t currentLeftSpeed;
        int16_t currentRightSpeed;
        int64_t startLeftCount;
        int64_t startRightCount;

        // Next command (if any)
        bool hasNextCommand;
        int16_t nextLeftSpeed;
        int16_t nextRightSpeed;

        // Constants
        static constexpr uint8_t MIN_ENCODER_PULSES = 10;
};
