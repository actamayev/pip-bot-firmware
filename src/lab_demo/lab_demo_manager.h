#pragma once

#include <Arduino.h>
#include "../utils/config.h"
#include "../utils/singleton.h"
#include "../sensors/sensors.h"
#include "./balance_controller.h"  
#include "../actuators/speaker.h"
#include "../networking/protocol.h"
#include "../actuators/motor_driver.h"
#include "../sensors/encoder_manager.h"
#include "../actuators/led/led_animations.h"

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
        void handleChangePidsCommand(NewBalancePids newBalancePids);
        void handleLightCommand(LightAnimationStatus lightAnimationStatus);
        void handleNewLightColors(NewLightColors newLightColors);
        void handleChangeMaxSpeedCommand(uint8_t newMaxSpeed);

    private:
        void updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed);
        void executeCommand(int16_t leftSpeed, int16_t rightSpeed);

        // Simple command tracking
        bool isExecutingCommand;
        int16_t currentLeftSpeed;
        int16_t currentRightSpeed;
        int64_t startLeftCount;
        int64_t startRightCount;

        unsigned long commandStartTime;
        static constexpr unsigned long COMMAND_TIMEOUT_MS = 1000; // 1 second timeout
    
        // Next command (if any)
        bool hasNextCommand;
        int16_t nextLeftSpeed;
        int16_t nextRightSpeed;

        // Constants
        static constexpr uint8_t MIN_ENCODER_PULSES = 1;
};
