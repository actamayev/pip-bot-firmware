#pragma once

#include <vector>
#include <Arduino.h>
#include <Preferences.h>
#include "utils/structs.h"
#include "utils/singleton.h"
#include "utils/pip_config.h"
#include "networking/serial_queue_manager.h"

// Cache structure for frequently accessed preferences
struct PreferencesCache {
    // PIP ID cache
    String pipId;
    bool pipIdLoaded = false;

    // Firmware version cache
    int firmwareVersion = 0;
    bool firmwareVersionLoaded = false;

    // WiFi networks cache
    std::vector<WiFiCredentials> wifiNetworks;
    int wifiCount = 0;
    bool wifiDataLoaded = false;
};

class PreferencesManager : public Singleton<PreferencesManager> {
    friend class Singleton<PreferencesManager>;

    public:
      String get_pip_id();

      // Firmware version methods
      int get_firmware_version();
      void set_firmware_version(int version);

      // WiFi methods
      void store_wi_fi_credentials(const String& ssid, const String& password, int index);
      std::vector<WiFiCredentials> get_all_stored_wi_fi_networks();
      bool has_stored_wi_fi_networks(); // Add this new method

      // Side TOF calibration methods
      bool has_side_tof_calibration(uint8_t sensor_address);
      void store_side_tof_calibration(uint8_t sensor_address, uint16_t baseline, bool use_hardware_calibration);
      uint16_t get_side_tof_baseline(uint8_t sensor_address);
      bool get_side_tof_use_hardware_calibration(uint8_t sensor_address);
      bool forget_wi_fi_network(const String& target_ssid);

    private:
        PreferencesManager() = default;
        ~PreferencesManager();

        // Single preferences instance
        Preferences _preferences;

        // Current open namespace
        String _currentNamespace = "";

        // Cache for frequently accessed data
        PreferencesCache _cache;

        // Method to ensure correct namespace is open
        bool begin_namespace(const char* ns);

        // Cache loading methods
        void load_pip_id_cache();
        void load_firmware_version_cache();
        void load_wifi_data_cache();

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
