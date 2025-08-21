#pragma once

#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "utils/config.h"
#include "utils/singleton.h"
#include "actuators/led/rgb_led.h"
#include "utils/preferences_manager.h"
#include "sensors/sensor_data_buffer.h"
#include "networking/serial_queue_manager.h"

class FirmwareVersionTracker : public Singleton<FirmwareVersionTracker> {
    friend class Singleton<FirmwareVersionTracker>;

    public:
        int getFirmwareVersion() { return firmwareVersion; }
        void retrieveLatestFirmwareFromServer(uint16_t newVersion);

    private:
        FirmwareVersionTracker();
        int firmwareVersion = 0;
        int pendingVersion = 0;
        bool isRetrievingFirmwareFromServer = false;
        WiFiClient* httpClient = nullptr;
        WiFiClientSecure secureClient;
        WiFiClient insecureClient;

        HTTPUpdate httpUpdate;
        void updateProgressLeds(int progress, int total);
};
