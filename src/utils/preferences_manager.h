#pragma once

#include <vector>
#include <Arduino.h>
#include <Preferences.h>
#include "utils/structs.h"
#include "utils/singleton.h"

class PreferencesManager : public Singleton<PreferencesManager> {
    friend class Singleton<PreferencesManager>;

    public:
        // PIP ID methods
        String getPipId();
        void setPipId(const String& id);

        // Firmware version methods
        int getFirmwareVersion();
        void setFirmwareVersion(int version);
        
        // WiFi methods
        void storeWiFiCredentials(const String& ssid, const String& password, int index);
        String getWiFiSSID(int index = 0);
        String getWiFiPassword(int index = 0);
        std::vector<WiFiCredentials> getAllStoredWiFiNetworks();
        bool hasStoredWiFiNetworks();  // Add this new method

    private:
        PreferencesManager() = default;
        ~PreferencesManager();
        
        // Single preferences instance
        Preferences preferences;
        
        // Current open namespace
        String currentNamespace = "";
        
        // Method to ensure correct namespace is open
        bool beginNamespace(const char* ns);
        
        // Namespace constants (akin to folders)
        static constexpr const char* NS_PIP_ID = "pip_id";
        static constexpr const char* NS_FIRMWARE = "firmware";
        static constexpr const char* NS_WIFI = "wifi";
        
        // Key constants (akin to files in those folders)
        static constexpr const char* KEY_PIP_ID = "id";
        static constexpr const char* KEY_FW_VERSION = "fw_version";
        static constexpr const char* WIFI_COUNT = "wifi_count";
};
