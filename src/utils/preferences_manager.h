#pragma once

#include <Arduino.h>

#include <Preferences.h>

#include <vector>

#include "networking/serial_queue_manager.h"
#include "utils/pip_config.h"
#include "utils/singleton.h"
#include "utils/structs.h"

// Cache structure for frequently accessed preferences
struct PreferencesCache {
    // PIP ID cache
    String pip_id;
    bool pip_id_loaded = false;

    // Firmware version cache
    int firmware_version = 0;
    bool firmware_version_loaded = false;

    // WiFi networks cache
    std::vector<WiFiCredentials> wifi_networks;
    int wifi_count = 0;
    bool wifi_data_loaded = false;
};

class PreferencesManager : public Singleton<PreferencesManager> {
    friend class Singleton<PreferencesManager>;

  public:
    String get_pip_id();

    // Firmware version methods
    int get_firmware_version();
    void set_firmware_version(int version);

    // WiFi methods
    void store_wifi_credentials(const String& ssid, const String& password, int index);
    std::vector<WiFiCredentials> get_all_stored_wifi_networks();
    bool has_stored_wifi_networks(); // Add this new method

    // Side TOF calibration methods
    bool has_side_tof_calibration(uint8_t sensor_address);
    void store_side_tof_calibration(uint8_t sensor_address, uint16_t baseline, bool use_hardware_calibration);
    uint16_t get_side_tof_baseline(uint8_t sensor_address);
    bool get_side_tof_use_hardware_calibration(uint8_t sensor_address);
    bool forget_wifi_network(const String& target_ssid);

  private:
    PreferencesManager() = default;
    ~PreferencesManager();

    // Single preferences instance
    Preferences _preferences;

    // Current open namespace
    String _current_namespace = "";

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
