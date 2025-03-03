#pragma once

#include <Arduino.h>
#include "./singleton.h"
#include "./pid_controller.h"

class LabDemoManager : public Singleton<LabDemoManager> {
    friend class Singleton<LabDemoManager>;

    public:
        void handleBinaryMessage(const char* data);
        void updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed);
        void processPendingCommands();
        LabDemoManager();
        
        // Enable/disable PID control
        void enablePIDControl(bool enable) { pidEnabled = enable; }
        bool isPIDControlEnabled() const { return pidEnabled; }
        
    private:
        void executeCommand(int16_t leftSpeed, int16_t rightSpeed);
        void adjustMotorSpeeds();

        // Simple command tracking
        bool isExecutingCommand;
        int16_t currentLeftSpeed;
        int16_t currentRightSpeed;
        int16_t targetLeftSpeed;  // Added to store the target speed
        int16_t targetRightSpeed; // Added to store the target speed
        int64_t startLeftCount;
        int64_t startRightCount;

        // Next command (if any)
        bool hasNextCommand;
        int16_t nextLeftSpeed;
        int16_t nextRightSpeed;

        // PID control
        bool pidEnabled;
        PIDController* speedMatchPID;   // PID for matching wheel speeds
        unsigned long lastPIDUpdateTime;
        
        // Current adjusted PWM values
        int16_t adjustedLeftPWM;
        int16_t adjustedRightPWM;

        // Constants
        static constexpr int MIN_ENCODER_PULSES = 10;
        static constexpr unsigned long PID_UPDATE_INTERVAL = 50; // ms
};
