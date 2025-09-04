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
#include "networking/send_sensor_data.h"
#include "career_quest/career_quest_triggers.h"
#include "networking/serial_queue_manager.h"
#include "networking/network_state_manager.h"
#include "actuators/dance_manager.h"
#include "games/game_manager.h"
#include "actuators/led/rgb_led.h"

class MessageProcessor : public Singleton<MessageProcessor> {
    friend class Singleton<MessageProcessor>;

    public:
        void processBinaryMessage(const uint8_t* data, uint16_t length);

    private:
        MessageProcessor() = default;
        // Method declarations
        void handleMotorControl(const uint8_t* data);
        void handleBalanceCommand(BalanceStatus enableBalancing);
        void handleLightCommand(LightAnimationStatus lightAnimationStatus);
        void handleNewLightColors(NewLightColors newLightColors);
        void handleObstacleAvoidanceCommand(ObstacleAvoidanceStatus status);
        void handleGetSavedWiFiNetworks();
        void handleSoftScanWiFiNetworks();
        void handleHardScanWiFiNetworks();
};
