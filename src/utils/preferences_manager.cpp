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
        // SerialQueueManager::getInstance().queueMessage("Failed to open preferences namespace: %s\n", ns);
    }
    
    return success;
}

// PIP ID methods
String PreferencesManager::getPipId() {
    if (!beginNamespace(NS_PIP_ID)) return String(DEFAULT_PIP_ID);  // Fallback to default if can't access preferences

    // Check if PIP ID exists in preferences
    String pipId = preferences.getString(KEY_PIP_ID, "");
    
    if (pipId.length() == 0) {
        // First boot - initialize with the compile-time default
        pipId = String(DEFAULT_PIP_ID);
        preferences.putString(KEY_PIP_ID, pipId);
        // SerialQueueManager::getInstance().queueMessage("First boot: Initialized PIP ID to default: %s\n", pipId.c_str());
    }
    
    return pipId;
}

void PreferencesManager::setPipId(const String& id) {
    if (!beginNamespace(NS_PIP_ID)) return;
    preferences.putString(KEY_PIP_ID, id);
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

String PreferencesManager::getWiFiSSID(int index) {
    if (!beginNamespace(NS_WIFI)) return "";  // Can't access preferences

    char ssidKey[128];
    sprintf(ssidKey, "ssid_%d", index);
    
    return preferences.getString(ssidKey, "");
}

String PreferencesManager::getWiFiPassword(int index) {
    if (!beginNamespace(NS_WIFI)) return "";  // Can't access preferences
    
    char passwordKey[128];
    sprintf(passwordKey, "pwd_%d", index);
    
    return preferences.getString(passwordKey, "");
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
