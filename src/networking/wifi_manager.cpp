#include "./wifi_manager.h"

Preferences preferences;

WiFiManager::WiFiManager() {
    // Hard-coding Wifi creds during initialization (before we have encoders + screen)
    preferences.begin("wifi-creds", false);
    storeWiFiCredentials("Another Dimension", "Iforgotit123", 0);
    // storeWiFiCredentials("NETGEAR08", "breezyshoe123", 1);

	connectToStoredWiFi();
}

WiFiCredentials WiFiManager::getStoredWiFiCredentials() {
	WiFiCredentials creds;
	preferences.begin("wifi-creds", false);
	creds.ssid = preferences.getString("ssid", "");
	creds.password = preferences.getString("password", "");
	preferences.end();
	return creds;
}

void WiFiManager::connectToStoredWiFi() {
    // Try to connect directly to any saved network without scanning
    bool connectionStatus = attemptDirectConnectionToSavedNetworks();

    if (connectionStatus) {
        return WebSocketManager::getInstance().connectToWebSocket();
    }

    // 4/29/25 NOTE: When serial was implemented, this was commented out because the code got stuck in wifi scan mode when the serial code was brought in in main.cpp

    // If direct connection failed, do a full scan for all networks
    // auto networks = scanWiFiNetworkInfos();

    // if (networks.empty()) {
    //     // If no networks found
    //     Serial.println("No networks found in full scan.");
    //     return;
    // }
    
    // // Print the available networks and select the first one
    // printNetworkList(networks);
    // setSelectedNetworkIndex(0);
    
    // // Init encoder for network selection
    // WifiSelectionManager::getInstance().initNetworkSelection();

    // Serial.println("Use the right motor to scroll through networks");
}

// 4/9/25 TODO: Connect to the network we've most recently connected to first.
bool WiFiManager::attemptDirectConnectionToSavedNetworks() {
    // Get all saved networks
    std::vector<WiFiCredentials> savedNetworks = getAllStoredNetworks();
    
    if (savedNetworks.empty()) {
        Serial.println("No saved networks found");
        return false;
    }
    // if (ledAnimations.getCurrentAnimation() != LedTypes::BREATHING) {
    //     rgbLed.set_led_red();
    //     ledAnimations.startBreathing();
    // }

    Serial.println("Attempting direct connection to saved networks...");
    
    // Try to connect to each saved network without scanning first
    for (const auto& network : savedNetworks) {
        if (NetworkStateManager::getInstance().shouldStopWiFiOperations()) {
            Serial.println("Serial connection detected - aborting WiFi connection attempts");
            return false;
        }
        Serial.printf("Trying to connect to: %s\n", network.ssid.c_str());
        
        // Attempt connection
        if (attemptNewWifiConnection(network)) return true;
        
        // Brief delay before trying the next network
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    Serial.println("Failed to connect to any saved networks");
    return false;
}

bool WiFiManager::attemptNewWifiConnection(WiFiCredentials wifiCredentials) {
    // Set WiFi mode to Station (client mode)
    if (NetworkStateManager::getInstance().shouldStopWiFiOperations()) {
        Serial.println("Serial connection detected - aborting WiFi connection attempt");
        return false;
    }

    _isConnecting = true;
    WiFi.mode(WIFI_STA);

    if (wifiCredentials.ssid.isEmpty()) {
        Serial.println("No SSID supplied.");
        return false;
    }

    WiFi.begin(wifiCredentials.ssid, wifiCredentials.password);
    Serial.println("SSID: " + wifiCredentials.ssid);
    Serial.println("Password: " + wifiCredentials.password);
    Serial.println("Attempting to connect to Wi-Fi...");

    unsigned long startAttemptTime = millis();
    unsigned long lastPrintTime = startAttemptTime;
    unsigned long lastCheckTime = startAttemptTime;

    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < CONNECT_TO_SINGLE_NETWORK_TIMEOUT)) {
        // Non-blocking print of dots
        unsigned long currentTime = millis();
        if (currentTime - lastPrintTime >= printInterval) {
            Serial.print(".");
            lastPrintTime = currentTime;
            yield();  // Allow the ESP32 to handle background tasks
        }
        if (currentTime - lastCheckTime >= checkInterval) {
            lastCheckTime = currentTime;
            
            // Poll serial to update connection status
            SerialManager::getInstance().pollSerial();
            
            if (NetworkStateManager::getInstance().shouldStopWiFiOperations()) {
                Serial.println("\nSerial connection detected - aborting WiFi connection");
                _isConnecting = false;
                return false;
            }
        }
    }

    _isConnecting = false;

    if (WiFi.status() == WL_CONNECTED) {
        preferences.begin("wifi-creds", false);
        storeWiFiCredentials(wifiCredentials.ssid, wifiCredentials.password, 0);
        preferences.end();
        Serial.println("Connected to Wi-Fi!");
        return true;
    } else {
        Serial.println("Failed to connect to Wi-Fi.");
        return false;
    }
}

std::vector<WiFiNetworkInfo> WiFiManager::scanWiFiNetworkInfos() {
    std::vector<WiFiNetworkInfo> networks;
    int numNetworks = 0;
    
    Serial.println("Starting WiFi scan...");
    
    WiFi.disconnect(true);
    WiFi.scanDelete();
    vTaskDelay(pdMS_TO_TICKS(100));
    // Set WiFi mode to station before scanning
    WiFi.mode(WIFI_STA);

    rgbLed.set_led_purple();
    ledAnimations.startBreathing();

    // Perform synchronous scan
    numNetworks = WiFi.scanNetworks(false);

    // Turn off LED after scanning
    rgbLed.turn_led_off();
    
    // Handle scan results
    if (numNetworks <= 0) {
        if (numNetworks == 0) {
            Serial.println("No networks found");
        } else if (numNetworks == -1) {
            Serial.println("Scan still running");
        } else if (numNetworks == -2) {
            Serial.println("Error with scan");
        } else {
            Serial.printf("Unknown scan error: %d\n", numNetworks);
        }
        return networks;  // Return empty network list
    }
    
    // Extract network information
    for (int i = 0; i < numNetworks; i++) {
        WiFiNetworkInfo network;
        network.ssid = WiFi.SSID(i);
        network.rssi = WiFi.RSSI(i);
        network.encryptionType = WiFi.encryptionType(i);
        networks.push_back(network);
        
        // Debug output
        Serial.printf("  Network %d: %s (Signal: %d dBm)\n", 
                     i + 1, network.ssid.c_str(), network.rssi);
    }
    
    // Clean up scan results to free memory
    WiFi.scanDelete();
    
    // Sort networks by signal strength
    sortNetworksBySignalStrength(networks);
    
    // Update class members
    _availableNetworks = networks;
    _selectedNetworkIndex = 0;
    
    return networks;
}

void WiFiManager::sortNetworksBySignalStrength(std::vector<WiFiNetworkInfo>& networks) {
    // Sort networks by RSSI (higher values = stronger signal)
    std::sort(networks.begin(), networks.end(), 
              [](const WiFiNetworkInfo& a, const WiFiNetworkInfo& b) {
                  return a.rssi > b.rssi;
              });
}

void WiFiManager::printNetworkList(const std::vector<WiFiNetworkInfo>& networks) {
    Serial.println("Available WiFi Networks (sorted by signal strength):");
    Serial.println("----------------------------------------------------");
    
    for (uint16_t i = 0; i < networks.size(); i++) {
        const auto& network = networks[i];
        String encryption = "";
        
        // Convert encryption type to readable format
        switch (network.encryptionType) {
            case WIFI_AUTH_OPEN: encryption = "Open"; break;
            case WIFI_AUTH_WEP: encryption = "WEP"; break;
            case WIFI_AUTH_WPA_PSK: encryption = "WPA"; break;
            case WIFI_AUTH_WPA2_PSK: encryption = "WPA2"; break;
            case WIFI_AUTH_WPA_WPA2_PSK: encryption = "WPA/WPA2"; break;
            default: encryption = "Unknown"; break;
        }

        Serial.printf("%2d. %s | Signal: %d dBm | Security: %s %s\n", 
                    (int)i + 1, 
                    network.ssid.c_str(), 
                    network.rssi,
                    encryption.c_str(),
                    (i == _selectedNetworkIndex) ? " <SELECTED>" : "");
    }
    Serial.println("----------------------------------------------------");
}

void WiFiManager::setSelectedNetworkIndex(int index) {
    if (_availableNetworks.empty()) return;
    
    // Apply bounds checking
    if (index < 0) {
        _selectedNetworkIndex = 0;
    } else if (index >= (int)_availableNetworks.size()) {
        _selectedNetworkIndex = _availableNetworks.size() - 1;
    } else {
        _selectedNetworkIndex = index;
    }
    
    // Print the selected network
    if (!_availableNetworks.empty()) {
        Serial.printf("Selected Network: %s (Signal: %d dBm)\n", 
                    _availableNetworks[_selectedNetworkIndex].ssid.c_str(),
                    _availableNetworks[_selectedNetworkIndex].rssi);
    }
}

// For storing WiFi credentials:
void WiFiManager::storeWiFiCredentials(const String& ssid, const String& password, int index) {
    preferences.begin("wifi-creds", false);
    
    // Fix: Create strings first, then use c_str()
    String ssidKey = "ssid" + String(index);
    String passKey = "pass" + String(index);
    
    preferences.putString(ssidKey.c_str(), ssid);
    preferences.putString(passKey.c_str(), password);
    
    // Keep track of the number of saved networks
    int count = preferences.getInt("count", 0);
    if (index >= count) {
        preferences.putInt("count", index + 1);
    }
    preferences.end();
}

// For retrieving WiFi credentials:
std::vector<WiFiCredentials> WiFiManager::getAllStoredNetworks() {
    std::vector<WiFiCredentials> networks;
    
    preferences.begin("wifi-creds", false);
    int count = preferences.getInt("count", 0);
    
    for (int i = 0; i < count; i++) {
        WiFiCredentials creds;
        
        // Fix: Create strings first, then use c_str()
        String ssidKey = "ssid" + String(i);
        String passKey = "pass" + String(i);
        
        creds.ssid = preferences.getString(ssidKey.c_str(), "");
        creds.password = preferences.getString(passKey.c_str(), "");
        
        if (!creds.ssid.isEmpty()) {
            networks.push_back(creds);
        }
    }
    
    preferences.end();
    return networks;
}

void WiFiManager::checkAndReconnectWiFi() {
    // Check if WiFi is connected or already attempting connection
    if (WiFi.status() == WL_CONNECTED || _isConnecting) return;
    unsigned long currentTime = millis();

    if (currentTime - _lastReconnectAttempt < WIFI_RECONNECT_TIMEOUT) return;
    Serial.println("WiFi disconnected, attempting to reconnect...");
    
    // Update last reconnect attempt time
    _lastReconnectAttempt = currentTime;

    // Set connecting flag
    _isConnecting = true;
    
    // Try to reconnect to stored WiFi
    bool connectionStatus = attemptDirectConnectionToSavedNetworks();
    
    // Reset connecting flag
    _isConnecting = false;
    
    if (connectionStatus) {
        WebSocketManager::getInstance().connectToWebSocket();
    }
}
