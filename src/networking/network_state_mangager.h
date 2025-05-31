#pragma once

#include <WiFi.h>  // Need this for WiFi.status()
#include "utils/singleton.h"
#include "serial_manager.h"

enum class NetworkMode {
    SERIAL_MODE,
    ADD_PIP_MODE,     // NEW: For adding pip to account
    WIFI_MODE,
    NONE
};

class NetworkStateManager : public Singleton<NetworkStateManager> {
    friend class Singleton<NetworkStateManager>;

    public:
        NetworkMode getCurrentMode();
        bool shouldStopWiFiOperations();
        void setAddPipMode(bool enabled);     // NEW: Method to control ADD_PIP_MODE
        bool isInAddPipMode() const;          // NEW: Check if in ADD_PIP_MODE

    private:
        NetworkStateManager() = default;
        bool _isInAddPipMode = false;         // NEW: Track ADD_PIP_MODE state
};
