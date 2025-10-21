#pragma once

#include <vector>
#include <Arduino.h>
#include <Preferences.h>
#include "utils/structs.h"
#include "utils/singleton.h"
#include "utils/pip_config.h"
#include "networking/serial_queue_manager.h"

class PreferencesManager : public Singleton<PreferencesManager> {
    friend class Singleton<PreferencesManager>;

    public:
        String getPipId();

        // Firmware version methods
        int getFirmwareVersion();
        void setFirmwareVersion(int version);
        
        // WiFi methods
        void storeWiFiCredentials(const String& ssid, const String& password, int index);
        std::vector<WiFiCredentials> getAllStoredWiFiNetworks();
        bool hasStoredWiFiNetworks();  // Add this new method

        // Side TOF calibration methods
        bool hasSideTofCalibration(uint8_t sensorAddress);
        void storeSideTofCalibration(uint8_t sensorAddress, uint16_t baseline, bool useHardwareCalibration);
        uint16_t getSideTofBaseline(uint8_t sensorAddress);
        bool getSideTofUseHardwareCalibration(uint8_t sensorAddress);
        bool forgetWiFiNetwork(const String& targetSSID);

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
        static constexpr const char* NS_SIDE_TOF = "side_tof";
        
        // Key constants (akin to files in those folders)
        static constexpr const char* KEY_PIP_ID = "id";
        static constexpr const char* KEY_FW_VERSION = "fw_version";
        static constexpr const char* WIFI_COUNT = "wifi_count";
};
