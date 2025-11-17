#pragma once

#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

#include "actuators/led/led_animations.h"
#include "actuators/led/rgb_led.h"
#include "career_quest/career_quest_triggers.h"
#include "networking/serial_queue_manager.h"
#include "sensors/sensor_data_buffer.h"
#include "utils/config.h"
#include "utils/preferences_manager.h"
#include "utils/singleton.h"

class FirmwareVersionTracker : public Singleton<FirmwareVersionTracker> {
    friend class Singleton<FirmwareVersionTracker>;

  public:
    int get_firmware_version() {
        return firmware_version;
    }
    void retrieve_latest_firmware_from_server(uint16_t new_version);

  private:
    FirmwareVersionTracker();
    int firmware_version = 0;
    int pending_version = 0;
    bool is_retrieving_firmware_from_server = false;
    WiFiClient* http_client = nullptr;
    WiFiClientSecure secure_client;
    WiFiClient insecure_client;

    HTTPUpdate http_update;
    void update_progress_leds(int progress, int total);
};
