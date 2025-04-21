#pragma once

#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "../utils/config.h"
#include "../utils/singleton.h"
#include "./wifi_manager.h"

extern Preferences preferences;

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
        int firmwareVersion = 0;
        int pendingVersion = 0;
        bool isRetrievingFirmwareFromServer = false;
        WiFiClient* httpClient = nullptr;
        WiFiClientSecure secureClient;
        WiFiClient insecureClient;

        HTTPUpdate httpUpdate;
};
