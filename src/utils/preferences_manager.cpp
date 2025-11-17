#include "preferences_manager.h"

PreferencesManager::~PreferencesManager() {
    // Ensure preferences are closed when the application exits
    if (_current_namespace.length() > 0) {
        _preferences.end();
    }
}

bool PreferencesManager::begin_namespace(const char* ns) {
    // If already in the requested namespace, do nothing
    if (_current_namespace.equals(ns)) {
        return true;
    }

    // Close current namespace if open
    if (_current_namespace.length() > 0) {
        _preferences.end();
    }

    // Open the requested namespace
    bool const success = _preferences.begin(ns, false);
    if (success) {
        _current_namespace = ns;
    } else {
        _current_namespace = "";
        const String log_message = String("Failed to open preferences namespace: ") + ns;
        SerialQueueManager::get_instance().queue_message(log_message);
    }

    return success;
}

// Cache loading methods
void PreferencesManager::load_pip_id_cache() {
    if (_cache.pip_id_loaded) {
        return;
    }

    if (!begin_namespace(NS_PIP_ID)) {
        _cache.pip_id = String(get_default_pip_id());
        _cache.pip_id_loaded = true;
        return;
    }

    _cache.pip_id = _preferences.getString(KEY_PIP_ID, "");

    if (_cache.pip_id.length() == 0) {
        _cache.pip_id = String(get_default_pip_id());
        _preferences.putString(KEY_PIP_ID, _cache.pip_id);
    }

    _cache.pip_id_loaded = true;
}

void PreferencesManager::load_firmware_version_cache() {
    if (_cache.firmware_version_loaded) {
        return;
    }

    if (!begin_namespace(NS_FIRMWARE)) {
        _cache.firmware_version = 0;
        _cache.firmware_version_loaded = true;
        return;
    }

    _cache.firmware_version = _preferences.getInt(KEY_FW_VERSION, 0);
    _cache.firmware_version_loaded = true;
}

void PreferencesManager::load_wifi_data_cache() {
    if (_cache.wifi_data_loaded) {
        return;
    }

    _cache.wifi_networks.clear();

    if (!begin_namespace(NS_WIFI)) {
        _cache.wifi_count = 0;
        _cache.wifi_data_loaded = true;
        return;
    }

    _cache.wifi_count = 0;
    if (_preferences.isKey(WIFI_COUNT)) {
        _cache.wifi_count = _preferences.getInt(WIFI_COUNT, 0);
    }

    for (int i = 0; i < _cache.wifi_count; i++) {
        WiFiCredentials creds;

        const String ssid_key = String("ssid_") + i;
        const String password_key = String("pwd_") + i;

        creds.ssid = _preferences.getString(ssid_key.c_str(), "");
        creds.password = _preferences.getString(password_key.c_str(), "");

        if (!creds.ssid.isEmpty()) {
            _cache.wifi_networks.push_back(creds);
        }
    }

    _cache.wifi_data_loaded = true;
}

// PIP ID methods
String PreferencesManager::get_pip_id() {
    load_pip_id_cache();
    return _cache.pip_id;
}

// Firmware version methods
int PreferencesManager::get_firmware_version() {
    load_firmware_version_cache();
    return _cache.firmware_version;
}

void PreferencesManager::set_firmware_version(int version) {
    if (!begin_namespace(NS_FIRMWARE)) {
        return; // Can't access preferences
    }

    _preferences.putInt(KEY_FW_VERSION, version);

    // Update cache
    _cache.firmware_version = version;
    _cache.firmware_version_loaded = true;
}

// WiFi methods
void PreferencesManager::store_wifi_credentials(const String& ssid, const String& password, int index) {
    if (!begin_namespace(NS_WIFI)) {
        return; // Can't access preferences
    }

    // Generate keys with index
    const String ssid_key = String("ssid_") + index;
    const String password_key = String("pwd_") + index;

    _preferences.putString(ssid_key.c_str(), ssid);
    _preferences.putString(password_key.c_str(), password);

    // Initialize wifi_count if it doesn't exist, or update if necessary
    int current_count = 0;
    if (_preferences.isKey(WIFI_COUNT)) {
        current_count = _preferences.getInt(WIFI_COUNT, 0);
    }

    if (index >= current_count) {
        _preferences.putInt(WIFI_COUNT, index + 1);
    }

    // Invalidate WiFi cache to force reload
    _cache.wifi_data_loaded = false;
    _cache.wifi_networks.clear();
    _cache.wifi_count = 0;
}

bool PreferencesManager::has_stored_wifi_networks() {
    load_wifi_data_cache();
    return _cache.wifi_count > 0;
}

// Updated get_all_stored_wifi_networks method:
std::vector<WiFiCredentials> PreferencesManager::get_all_stored_wifi_networks() {
    load_wifi_data_cache();
    return _cache.wifi_networks;
}

// Side TOF calibration methods
bool PreferencesManager::has_side_tof_calibration(uint8_t sensor_address) {
    if (!begin_namespace(NS_SIDE_TOF)) {
        return false;
    }

    const String baseline_key = String("baseline_") + String(sensor_address, HEX);

    return _preferences.isKey(baseline_key.c_str());
}

void PreferencesManager::store_side_tof_calibration(uint8_t sensor_address, uint16_t baseline, bool use_hardware_calibration) {
    if (!begin_namespace(NS_SIDE_TOF)) {
        return;
    }

    const String baseline_key = String("baseline_") + String(sensor_address, HEX);
    const String hw_calib_key = String("hw_calib_") + String(sensor_address, HEX);

    _preferences.putUShort(baseline_key.c_str(), baseline);
    _preferences.putBool(hw_calib_key.c_str(), use_hardware_calibration);
}

uint16_t PreferencesManager::get_side_tof_baseline(uint8_t sensor_address) {
    if (!begin_namespace(NS_SIDE_TOF)) {
        return 0;
    }

    const String baseline_key = String("baseline_") + String(sensor_address, HEX);

    return _preferences.getUShort(baseline_key.c_str(), 0);
}

bool PreferencesManager::get_side_tof_use_hardware_calibration(uint8_t sensor_address) {
    if (!begin_namespace(NS_SIDE_TOF)) {
        return false;
    }

    const String hw_calib_key = String("hw_calib_") + String(sensor_address, HEX);

    return _preferences.getBool(hw_calib_key.c_str(), false);
}

bool PreferencesManager::forget_wifi_network(const String& target_ssid) {
    if (!begin_namespace(NS_WIFI)) {
        return false;
    }

    // Get all current networks
    std::vector<WiFiCredentials> all_networks = get_all_stored_wifi_networks();

    // Filter out networks matching target_ssid
    std::vector<WiFiCredentials> filtered_networks;
    for (const auto& network : all_networks) {
        if (network.ssid != target_ssid) {
            filtered_networks.push_back(network);
        }
    }

    // If no networks were removed, return false
    if (filtered_networks.size() == all_networks.size()) {
        return false; // Network not found
    }

    // Clear all existing network data
    int const current_count = _preferences.getInt(WIFI_COUNT, 0);
    for (int i = 0; i < current_count; i++) {
        const String ssid_key = String("ssid_") + i;
        const String password_key = String("pwd_") + i;
        _preferences.remove(ssid_key.c_str());
        _preferences.remove(password_key.c_str());
    }

    // Re-store filtered networks with contiguous indices
    for (size_t i = 0; i < filtered_networks.size(); i++) {
        const String ssid_key = String("ssid_") + static_cast<int>(i);
        const String password_key = String("pwd_") + static_cast<int>(i);
        _preferences.putString(ssid_key.c_str(), filtered_networks[i].ssid);
        _preferences.putString(password_key.c_str(), filtered_networks[i].password);
    }

    // Update wifi_count
    _preferences.putInt(WIFI_COUNT, filtered_networks.size());

    // Invalidate WiFi cache to force reload
    _cache.wifi_data_loaded = false;
    _cache.wifi_networks.clear();
    _cache.wifi_count = 0;

    return true;
}
