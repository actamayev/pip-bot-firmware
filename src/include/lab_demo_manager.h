#pragma once

#include <Arduino.h>
#include "./singleton.h"

class LabDemoManager : public Singleton<LabDemoManager> {
    friend class Singleton<LabDemoManager>;

    public:
        void handleBinaryMessage(const char* data);
        void updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed);
        void processPendingCommands();
        LabDemoManager();

    private:
        void executeCommand(int16_t leftSpeed, int16_t rightSpeed);

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
        static constexpr int MIN_ENCODER_PULSES = 10;
};
