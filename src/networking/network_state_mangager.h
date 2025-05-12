#pragma once

#include <WiFi.h>  // Need this for WiFi.status()
#include "utils/singleton.h"
#include "serial_manager.h"

enum class NetworkMode {
    SERIAL_MODE,
    WIFI_MODE,
    NONE
};

class NetworkStateManager : public Singleton<NetworkStateManager> {
    friend class Singleton<NetworkStateManager>;

    public:
        NetworkMode getCurrentMode();
        bool shouldStopWiFiOperations();

    private:
        NetworkStateManager() = default;
};
