#pragma once

#include <Arduino.h>
#include "../utils/singleton.h"
#include "../sensors/sensors.h"
#include "../networking/protocol.h"

class LabDemoManager : public Singleton<LabDemoManager> {
    friend class Singleton<LabDemoManager>;

    public:
        void processPendingCommands();
        LabDemoManager();

        // Add this method declaration
        void handleMotorControl(const uint8_t* data);
        void handleSoundCommand(SoundType soundType);
        void handleSpeakerMute(SpeakerStatus status);

        void handleBalanceCommand(BalanceStatus enableBalancing);
        void updateBalancing();
        BalanceStatus isBalancing() const { return _balancingEnabled; }

    private:
        void updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed);
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
        static constexpr uint8_t MIN_ENCODER_PULSES = 10;

        BalanceStatus _balancingEnabled = BalanceStatus::UNBALANCED;
        float _targetAngle = 93.0f; // Target is vertical (90 degrees)
        unsigned long _lastBalanceUpdateTime = 0;
        float _lastError = 0.0f;
        float _errorSum = 0.0f;

        // PID Constants for balancing - will need tuning
        static constexpr float BALANCE_P_GAIN = 30.0f;  // Start conservative
        static constexpr float BALANCE_I_GAIN = 0.1f;  // Start small
        static constexpr float BALANCE_D_GAIN = 5.0f;  // Start with some damping

        // Limits and safety parameters
        static constexpr float MAX_SAFE_ANGLE_DEVIATION = 20.0f; // ±15° safety range
        static constexpr int16_t MAX_BALANCE_POWER = 255; // Cap motor power for safety
        static constexpr unsigned long BALANCE_UPDATE_INTERVAL = 6; // 10ms (100Hz)

        static constexpr uint8_t ANGLE_BUFFER_SIZE = 10;
        float _angleBuffer[ANGLE_BUFFER_SIZE] = {0};
        uint8_t _angleBufferIndex = 0;
        uint8_t _angleBufferCount = 0;
        float _lastValidAngle = 0.0f; // Store the last valid reading

        float _controlBuffer[ANGLE_BUFFER_SIZE] = {0};
        uint8_t _controlBufferIndex = 0;
        uint8_t _controlBufferCount = 0;

        // Safety (unfiltered) buffer for tilt monitoring
        float _safetyBuffer[ANGLE_BUFFER_SIZE] = {0};
        uint8_t _safetyBufferIndex = 0;
        uint8_t _safetyBufferCount = 0;
};
