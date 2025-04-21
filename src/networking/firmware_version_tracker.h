#pragma once

#include <WiFi.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include "../utils/config.h"
#include "../utils/singleton.h"

class FirmwareVersionTracker : public Singleton<FirmwareVersionTracker> {
    friend class Singleton<FirmwareVersionTracker>;

    public:
        int getFirmwareVersion() { return firmwareVersion; }
        void setFirmwareVersion(int version);
        void setPendingVersion(int version);
        void retrieveLatestFirmwareFromServer(uint16_t newVersion);
        bool isUpdating() { return isRetrievingFirmwareFromServer; }
        
    private:
        FirmwareVersionTracker();
        Preferences preferences;
        int firmwareVersion = 0;
        int pendingVersion = 0;
        bool isRetrievingFirmwareFromServer = false;
        WiFiClientSecure client;  // Change to WiFiClientSecure
        HTTPUpdate httpUpdate;
};
