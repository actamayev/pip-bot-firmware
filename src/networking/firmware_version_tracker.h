#pragma once

#include <HTTPUpdate.h>
#include "../utils/singleton.h"
#include "./wifi_manager.h"

class FirmwareVersionTracker : public Singleton<FirmwareVersionTracker> {
    friend class Singleton<FirmwareVersionTracker>;

    public:
        FirmwareVersionTracker();
        uint8_t firmwareVersion = 0;
        void retrieveLatestFirmwareFromServer();

    private:
        bool didRetrieveLatestVersionFromServer = false;
        bool isRetrievingFirmwareFromServer = false;
};
