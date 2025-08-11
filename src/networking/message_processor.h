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
#include "networking/wifi_manager.h"
#include "actuators/display_screen.h"
#include "networking/serial_manager.h"
#include "utils/preferences_manager.h"
#include "sensors/sensor_data_buffer.h"
#include "actuators/led/led_animations.h"
#include "networking/serial_queue_manager.h"
#include "networking/network_state_mangager.h"

class MessageProcessor : public Singleton<MessageProcessor> {
    friend class Singleton<MessageProcessor>;

    public:
        void processPendingCommands();
        MessageProcessor() = default; // Keep the constructor for any additional initialization

        // Method declarations
        void handleMotorControl(const uint8_t* data);
        void handleBalanceCommand(BalanceStatus enableBalancing);
        void handleLightCommand(LightAnimationStatus lightAnimationStatus);
        void handleNewLightColors(NewLightColors newLightColors);
        void handleObstacleAvoidanceCommand(ObstacleAvoidanceStatus status);
        void handleGetSavedWiFiNetworks();
        void handleSoftScanWiFiNetworks();
        void handleHardScanWiFiNetworks();

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
