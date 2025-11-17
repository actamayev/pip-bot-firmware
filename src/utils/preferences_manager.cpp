#include "preferences_manager.h"

PreferencesManager::~PreferencesManager() {
    // Ensure preferences are closed when the application exits
    if (_currentNamespace.length() > 0) {
        _preferences.end();
    }
}

bool PreferencesManager::beginNamespace(const char* ns) {
    // If already in the requested namespace, do nothing
    if (_currentNamespace.equals(ns)) {
        return true;
    }

    // Close current namespace if open
    if (_currentNamespace.length() > 0) {
        _preferences.end();
    }

    // Open the requested namespace
    bool const SUCCESS = _preferences.begin(ns, false);
    if (SUCCESS) {
        _currentNamespace = ns;
    } else {
        _currentNamespace = "";
        const String LOG_MESSAGE = String("Failed to open preferences namespace: ") + ns;
        SerialQueueManager::getInstance().queueMessage(LOG_MESSAGE);
    }

    return SUCCESS;
}

// Cache loading methods
void PreferencesManager::loadPipIdCache() {
    if (_cache.pipIdLoaded) {
        return;
    }

    if (!beginNamespace(NS_PIP_ID)) {
        _cache.pipId = String(getDefaultPipId());
        _cache.pipIdLoaded = true;
        return;
    }

    _cache.pipId = _preferences.getString(KEY_PIP_ID, "");

    if (_cache.pipId.length() == 0) {
        _cache.pipId = String(getDefaultPipId());
        _preferences.putString(KEY_PIP_ID, _cache.pipId);
    }

    _cache.pipIdLoaded = true;
}

void PreferencesManager::loadFirmwareVersionCache() {
    if (_cache.firmwareVersionLoaded) {
        return;
    }

    if (!beginNamespace(NS_FIRMWARE)) {
        _cache.firmwareVersion = 0;
        _cache.firmwareVersionLoaded = true;
        return;
    }

    _cache.firmwareVersion = _preferences.getInt(KEY_FW_VERSION, 0);
    _cache.firmwareVersionLoaded = true;
}

void PreferencesManager::loadWifiDataCache() {
    if (_cache.wifiDataLoaded) {
        return;
    }

    _cache.wifiNetworks.clear();

    if (!beginNamespace(NS_WIFI)) {
        _cache.wifiCount = 0;
        _cache.wifiDataLoaded = true;
        return;
    }

    _cache.wifiCount = 0;
    if (_preferences.isKey(WIFI_COUNT)) {
        _cache.wifiCount = _preferences.getInt(WIFI_COUNT, 0);
    }

    for (int i = 0; i < _cache.wifiCount; i++) {
        WiFiCredentials creds;

        const String SSID_KEY = String("ssid_") + i;
        const String PASSWORD_KEY = String("pwd_") + i;

        creds.ssid = _preferences.getString(SSID_KEY.c_str(), "");
        creds.password = _preferences.getString(PASSWORD_KEY.c_str(), "");

        if (!creds.ssid.isEmpty()) {
            _cache.wifiNetworks.push_back(creds);
        }
    }

    _cache.wifiDataLoaded = true;
}

// PIP ID methods
String PreferencesManager::getPipId() {
    loadPipIdCache();
    return _cache.pipId;
}

// Firmware version methods
int PreferencesManager::getFirmwareVersion() {
    loadFirmwareVersionCache();
    return _cache.firmwareVersion;
}

void PreferencesManager::setFirmwareVersion(int version) {
    if (!beginNamespace(NS_FIRMWARE)) {
        return; // Can't access preferences
    }

    _preferences.putInt(KEY_FW_VERSION, version);

    // Update cache
    _cache.firmwareVersion = version;
    _cache.firmwareVersionLoaded = true;
}

// WiFi methods
void PreferencesManager::storeWiFiCredentials(const String& ssid, const String& password, int index) {
    if (!beginNamespace(NS_WIFI)) {
        return; // Can't access preferences
    }

    // Generate keys with index
    const String SSID_KEY = String("ssid_") + index;
    const String PASSWORD_KEY = String("pwd_") + index;

    _preferences.putString(SSID_KEY.c_str(), ssid);
    _preferences.putString(PASSWORD_KEY.c_str(), password);

    // Initialize wifi_count if it doesn't exist, or update if necessary
    int currentCount = 0;
    if (_preferences.isKey(WIFI_COUNT)) {
        currentCount = _preferences.getInt(WIFI_COUNT, 0);
    }

    if (index >= currentCount) {
        _preferences.putInt(WIFI_COUNT, index + 1);
    }

    // Invalidate WiFi cache to force reload
    _cache.wifiDataLoaded = false;
    _cache.wifiNetworks.clear();
    _cache.wifiCount = 0;
}

bool PreferencesManager::hasStoredWiFiNetworks() {
    loadWifiDataCache();
    return _cache.wifiCount > 0;
}

// Updated getAllStoredWiFiNetworks method:
std::vector<WiFiCredentials> PreferencesManager::getAllStoredWiFiNetworks() {
    loadWifiDataCache();
    return _cache.wifiNetworks;
}

// Side TOF calibration methods
bool PreferencesManager::hasSideTofCalibration(uint8_t sensorAddress) {
    if (!beginNamespace(NS_SIDE_TOF)) {
        return false;
    }

    const String BASELINE_KEY = String("baseline_") + String(sensorAddress, HEX);

    return _preferences.isKey(BASELINE_KEY.c_str());
}

void PreferencesManager::storeSideTofCalibration(uint8_t sensorAddress, uint16_t baseline, bool useHardwareCalibration) {
    if (!beginNamespace(NS_SIDE_TOF)) {
        return;
    }

    const String BASELINE_KEY = String("baseline_") + String(sensorAddress, HEX);
    const String HW_CALIB_KEY = String("hw_calib_") + String(sensorAddress, HEX);

    _preferences.putUShort(BASELINE_KEY.c_str(), baseline);
    _preferences.putBool(HW_CALIB_KEY.c_str(), useHardwareCalibration);
}

uint16_t PreferencesManager::getSideTofBaseline(uint8_t sensorAddress) {
    if (!beginNamespace(NS_SIDE_TOF)) {
        return 0;
    }

    const String BASELINE_KEY = String("baseline_") + String(sensorAddress, HEX);

    return _preferences.getUShort(BASELINE_KEY.c_str(), 0);
}

bool PreferencesManager::getSideTofUseHardwareCalibration(uint8_t sensorAddress) {
    if (!beginNamespace(NS_SIDE_TOF)) {
        return false;
    }

    const String HW_CALIB_KEY = String("hw_calib_") + String(sensorAddress, HEX);

    return _preferences.getBool(HW_CALIB_KEY.c_str(), false);
}

bool PreferencesManager::forgetWiFiNetwork(const String& targetSSID) {
    if (!beginNamespace(NS_WIFI)) {
        return false;
    }

    // Get all current networks
    std::vector<WiFiCredentials> allNetworks = getAllStoredWiFiNetworks();

    // Filter out networks matching targetSSID
    std::vector<WiFiCredentials> filteredNetworks;
    for (const auto& network : allNetworks) {
        if (network.ssid != targetSSID) {
            filteredNetworks.push_back(network);
        }
    }

    // If no networks were removed, return false
    if (filteredNetworks.size() == allNetworks.size()) {
        return false; // Network not found
    }

    // Clear all existing network data
    int const CURRENT_COUNT = _preferences.getInt(WIFI_COUNT, 0);
    for (int i = 0; i < CURRENT_COUNT; i++) {
        const String SSID_KEY = String("ssid_") + i;
        const String PASSWORD_KEY = String("pwd_") + i;
        _preferences.remove(SSID_KEY.c_str());
        _preferences.remove(PASSWORD_KEY.c_str());
    }

    // Re-store filtered networks with contiguous indices
    for (size_t i = 0; i < filteredNetworks.size(); i++) {
        const String SSID_KEY = String("ssid_") + static_cast<int>(i);
        const String PASSWORD_KEY = String("pwd_") + static_cast<int>(i);
        _preferences.putString(SSID_KEY.c_str(), filteredNetworks[i].ssid);
        _preferences.putString(PASSWORD_KEY.c_str(), filteredNetworks[i].password);
    }

    // Update wifi_count
    _preferences.putInt(WIFI_COUNT, filteredNetworks.size());

    // Invalidate WiFi cache to force reload
    _cache.wifiDataLoaded = false;
    _cache.wifiNetworks.clear();
    _cache.wifiCount = 0;

    return true;
}
