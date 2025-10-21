#include "preferences_manager.h"

PreferencesManager::~PreferencesManager() {
    // Ensure preferences are closed when the application exits
    if (currentNamespace.length() > 0) {
        preferences.end();
    }
}

bool PreferencesManager::beginNamespace(const char* ns) {
    // If already in the requested namespace, do nothing
    if (currentNamespace.equals(ns)) return true;

    // Close current namespace if open
    if (currentNamespace.length() > 0) {
        preferences.end();
    }
    
    // Open the requested namespace
    bool success = preferences.begin(ns, false);
    if (success) {
        currentNamespace = ns;
    } else {
        currentNamespace = "";
        char logMessage[128];
        snprintf(logMessage, sizeof(logMessage), "Failed to open preferences namespace: %s", ns);
        SerialQueueManager::getInstance().queueMessage(logMessage);
    }
    
    return success;
}

// PIP ID methods
String PreferencesManager::getPipId() {
    if (!beginNamespace(NS_PIP_ID)) return String(getDefaultPipId());  // Changed this line

    String pipId = preferences.getString(KEY_PIP_ID, "");

    if (pipId.length() == 0) {
        // First boot - initialize with the compile-time default
        pipId = String(getDefaultPipId());  // Changed this line
        preferences.putString(KEY_PIP_ID, pipId);
    }
    
    return pipId;
}

// Firmware version methods
int PreferencesManager::getFirmwareVersion() {
    if (!beginNamespace(NS_FIRMWARE)) return 0;  // Fallback to 0 if can't access preferences
    
    return preferences.getInt(KEY_FW_VERSION, 0);
}

void PreferencesManager::setFirmwareVersion(int version) {
    if (!beginNamespace(NS_FIRMWARE)) return;  // Can't access preferences

    preferences.putInt(KEY_FW_VERSION, version);
}

// WiFi methods
void PreferencesManager::storeWiFiCredentials(const String& ssid, const String& password, int index) {
    if (!beginNamespace(NS_WIFI)) return;  // Can't access preferences
    
    // Generate keys with index
    char ssidKey[128], passwordKey[128];
    sprintf(ssidKey, "ssid_%d", index);
    sprintf(passwordKey, "pwd_%d", index);
    
    preferences.putString(ssidKey, ssid);
    preferences.putString(passwordKey, password);
    
    // Initialize wifi_count if it doesn't exist, or update if necessary
    int currentCount = 0;
    if (preferences.isKey(WIFI_COUNT)) {
        currentCount = preferences.getInt(WIFI_COUNT, 0);
    }
    
    if (index >= currentCount) {
        preferences.putInt(WIFI_COUNT, index + 1);
    }
}

bool PreferencesManager::hasStoredWiFiNetworks() {
    if (!beginNamespace(NS_WIFI)) return false;
    
    // Check if wifi_count key exists and is greater than 0
    if (preferences.isKey(WIFI_COUNT)) {
        return preferences.getInt(WIFI_COUNT, 0) > 0;
    }
    return false;
}

// Updated getAllStoredWiFiNetworks method:
std::vector<WiFiCredentials> PreferencesManager::getAllStoredWiFiNetworks() {
    std::vector<WiFiCredentials> networks;
    
    if (!beginNamespace(NS_WIFI)) return networks;  // Return empty vector if can't access preferences
    
    // Check if wifi_count key exists before trying to read it
    int count = 0;
    if (preferences.isKey(WIFI_COUNT)) {
        count = preferences.getInt(WIFI_COUNT, 0);
    }
    // If key doesn't exist, count remains 0 and we return empty vector
    
    for (int i = 0; i < count; i++) {
        WiFiCredentials creds;
        
        // Create keys with the SAME format as in storeWiFiCredentials
        char ssidKey[128], passwordKey[128];
        sprintf(ssidKey, "ssid_%d", i);
        sprintf(passwordKey, "pwd_%d", i);
        
        creds.ssid = preferences.getString(ssidKey, "");
        creds.password = preferences.getString(passwordKey, "");
        
        if (!creds.ssid.isEmpty()) {
            networks.push_back(creds);
        }
    }
    
    return networks;
}

// Side TOF calibration methods
bool PreferencesManager::hasSideTofCalibration(uint8_t sensorAddress) {
    if (!beginNamespace(NS_SIDE_TOF)) return false;
    
    char baselineKey[32];
    sprintf(baselineKey, "baseline_%02X", sensorAddress);
    
    return preferences.isKey(baselineKey);
}

void PreferencesManager::storeSideTofCalibration(uint8_t sensorAddress, uint16_t baseline, bool useHardwareCalibration) {
    if (!beginNamespace(NS_SIDE_TOF)) return;
    
    char baselineKey[32], hwCalibKey[32];
    sprintf(baselineKey, "baseline_%02X", sensorAddress);
    sprintf(hwCalibKey, "hw_calib_%02X", sensorAddress);
    
    preferences.putUShort(baselineKey, baseline);
    preferences.putBool(hwCalibKey, useHardwareCalibration);
}

uint16_t PreferencesManager::getSideTofBaseline(uint8_t sensorAddress) {
    if (!beginNamespace(NS_SIDE_TOF)) return 0;
    
    char baselineKey[32];
    sprintf(baselineKey, "baseline_%02X", sensorAddress);
    
    return preferences.getUShort(baselineKey, 0);
}

bool PreferencesManager::getSideTofUseHardwareCalibration(uint8_t sensorAddress) {
    if (!beginNamespace(NS_SIDE_TOF)) return false;
    
    char hwCalibKey[32];
    sprintf(hwCalibKey, "hw_calib_%02X", sensorAddress);
    
    return preferences.getBool(hwCalibKey, false);
}

bool PreferencesManager::forgetWiFiNetwork(const String& targetSSID) {
    if (!beginNamespace(NS_WIFI)) return false;
    
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
    int currentCount = preferences.getInt(WIFI_COUNT, 0);
    for (int i = 0; i < currentCount; i++) {
        char ssidKey[128], passwordKey[128];
        sprintf(ssidKey, "ssid_%d", i);
        sprintf(passwordKey, "pwd_%d", i);
        preferences.remove(ssidKey);
        preferences.remove(passwordKey);
    }
    
    // Re-store filtered networks with contiguous indices
    for (size_t i = 0; i < filteredNetworks.size(); i++) {
        char ssidKey[128], passwordKey[128];
        sprintf(ssidKey, "ssid_%d", (int)i);
        sprintf(passwordKey, "pwd_%d", (int)i);
        preferences.putString(ssidKey, filteredNetworks[i].ssid);
        preferences.putString(passwordKey, filteredNetworks[i].password);
    }
    
    // Update wifi_count
    preferences.putInt(WIFI_COUNT, filteredNetworks.size());
    
    return true;
}
