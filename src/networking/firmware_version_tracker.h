#pragma once

#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "utils/config.h"
#include "utils/singleton.h"
#include "actuators/led/rgb_led.h"
#include "utils/preferences_manager.h"
#include "sensors/sensor_data_buffer.h"
#include "networking/serial_queue_manager.h"
#include "career_quest/career_quest_triggers.h"
#include "actuators/led/led_animations.h"

class FirmwareVersionTracker : public Singleton<FirmwareVersionTracker> {
    friend class Singleton<FirmwareVersionTracker>;

    public:
        int get_firmware_version() { return firmwareVersion; }
        void retrieve_latest_firmware_from_server(uint16_t newVersion);

    private:
        FirmwareVersionTracker();
        int firmwareVersion = 0;
        int pendingVersion = 0;
        bool isRetrievingFirmwareFromServer = false;
        WiFiClient* httpClient = nullptr;
        WiFiClientSecure secureClient;
        WiFiClient insecureClient;

        HTTPUpdate httpUpdate;
        void update_progress_leds(int progress, int total);
};
