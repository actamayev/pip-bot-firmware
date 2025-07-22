#pragma once

#include <Arduino.h>
#include "utils/config.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "actuators/speaker.h"
#include "demos/demo_manager.h"
#include "networking/protocol.h"
#include "utils/timeout_manager.h"
#include "actuators/motor_driver.h"
#include "sensors/encoder_manager.h"
#include "networking/serial_manager.h"
#include "utils/preferences_manager.h"
#include "actuators/led/led_animations.h"
#include "sensors/sensor_polling_manager.h"
#include "networking/serial_queue_manager.h"

class MessageProcessor : public Singleton<MessageProcessor> {
    friend class Singleton<MessageProcessor>;

    public:
        void processPendingCommands();
        MessageProcessor() = default; // Keep the constructor for any additional initialization

        // Method declarations
        void handleMotorControl(const uint8_t* data);
        void handleBalanceCommand(BalanceStatus enableBalancing);
        void handleChangePidsCommand(NewBalancePids newBalancePids);
        void handleLightCommand(LightAnimationStatus lightAnimationStatus);
        void handleNewLightColors(NewLightColors newLightColors);
        void handleObstacleAvoidanceCommand(ObstacleAvoidanceStatus status);
        void handleGetSavedWiFiNetworks();
        void handleScanWiFiNetworks();

        void processBinaryMessage(const uint8_t* data, uint16_t length);
        void resetCommandState();

    private:
        void updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed);
        void executeCommand(int16_t leftSpeed, int16_t rightSpeed);

        // Member variables with in-class initialization
        bool isExecutingCommand = false;
        int16_t currentLeftSpeed = 0;
        int16_t currentRightSpeed = 0;
        int64_t startLeftCount = 0;
        int64_t startRightCount = 0;

        unsigned long commandStartTime = 0;
        static constexpr unsigned long COMMAND_TIMEOUT_MS = 1000; // 1 second timeout
    
        // Next command (if any)
        bool hasNextCommand = false;
        int16_t nextLeftSpeed = 0;
        int16_t nextRightSpeed = 0;

        // Constants
        static constexpr uint8_t MIN_ENCODER_PULSES = 10;
};
