#pragma once

#include <Arduino.h>

#include "actuators/dance_manager.h"
#include "actuators/display_screen.h"
#include "actuators/led/led_animations.h"
#include "actuators/led/rgb_led.h"
#include "actuators/motor_driver.h"
#include "actuators/speaker.h"
#include "career_quest/career_quest_triggers.h"
#include "demos/demo_manager.h"
#include "games/game_manager.h"
#include "networking/protocol.h"
#include "networking/send_sensor_data.h"
#include "networking/serial_manager.h"
#include "networking/serial_queue_manager.h"
#include "networking/wifi_manager.h"
#include "sensors/encoder_manager.h"
#include "sensors/sensor_data_buffer.h"
#include "utils/config.h"
#include "utils/preferences_manager.h"
#include "utils/singleton.h"
#include "utils/structs.h"
#include "utils/timeout_manager.h"

class MessageProcessor : public Singleton<MessageProcessor> {
    friend class Singleton<MessageProcessor>;

  public:
    void process_binary_message(const uint8_t* data, uint16_t length);

  private:
    MessageProcessor() = default;
    // Method declarations
    static void handle_motor_control(const uint8_t* data);
    static void handle_balance_command(BalanceStatus enable_balancing);
    static void handle_light_command(LightAnimationStatus light_animation_status);
    static void handle_new_light_colors(NewLightColors new_light_colors);
    static void handle_obstacle_avoidance_command(ObstacleAvoidanceStatus status);
    void handle_get_saved_wifi_networks();
    static void handle_soft_scan_wifi_networks();
    static void handle_hard_scan_wifi_networks();
};
