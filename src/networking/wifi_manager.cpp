#include "wifi_manager.h"

WiFiManager::WiFiManager() {
    // Hard-coding Wifi creds during initialization (before we have encoders + screen)
    // storeWiFiCredentials("Another Dimension", "Iforgotit123", 0);
    // storeWiFiCredentials("NETGEAR08", "breezyshoe123", 1);
    // storeWiFiCredentials("iPhone", "12345678", 0);

	connectToStoredWiFi();
}

WiFiCredentials WiFiManager::getStoredWiFiCredentials() {
	WiFiCredentials credentials;
    credentials.ssid = PreferencesManager::getInstance().getWiFiSSID();
    credentials.password = PreferencesManager::getInstance().getWiFiPassword();
	return credentials;
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
    //     SerialQueueManager::getInstance().queueMessage("No networks found in full scan.");
    //     return;
    // }
    
    // // Print the available networks and select the first one
    // printNetworkList(networks);
    // setSelectedNetworkIndex(0);
    
    // // Init encoder for network selection
    // WifiSelectionManager::getInstance().initNetworkSelection();

    // SerialQueueManager::getInstance().queueMessage("Use the right motor to scroll through networks");
}

// 4/9/25 TODO: Connect to the network we've most recently connected to first.
bool WiFiManager::attemptDirectConnectionToSavedNetworks() {
    // Quick check if any networks exist before trying to get them
    if (!PreferencesManager::getInstance().hasStoredWiFiNetworks()) {
        SerialQueueManager::getInstance().queueMessage("No saved networks found");
        return false;
    }

    // Get all saved networks
    std::vector<WiFiCredentials> savedNetworks = PreferencesManager::getInstance().getAllStoredWiFiNetworks();

    if (
        (ledAnimations.getCurrentAnimation() != LedTypes::BREATHING) &&
        (BytecodeVM::getInstance().isPaused == BytecodeVM::getInstance().PROGRAM_NOT_STARTED)
    ) {
        rgbLed.set_led_red();
        ledAnimations.startBreathing();
    }

    SerialQueueManager::getInstance().queueMessage("Attempting direct connection to saved networks...");
    
    // Try to connect to each saved network without scanning first
    for (const WiFiCredentials& network : savedNetworks) {
        if (NetworkStateManager::getInstance().shouldStopWiFiOperations()) {
            SerialQueueManager::getInstance().queueMessage("Serial connection detected - aborting WiFi connection attempts");
            return false;
        }
        // SerialQueueManager::getInstance().queueMessage("Trying to connect to: %s\n", network.ssid.c_str());
        
        // Attempt connection
        if (attemptNewWifiConnection(network)) return true;
        
        // Brief delay before trying the next network
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    SerialQueueManager::getInstance().queueMessage("Failed to connect to any saved networks");
    return false;
}

bool WiFiManager::attemptNewWifiConnection(WiFiCredentials wifiCredentials) {
    // Set WiFi mode to Station (client mode)
    if (NetworkStateManager::getInstance().shouldStopWiFiOperations()) {
        SerialQueueManager::getInstance().queueMessage("Serial connection detected - aborting WiFi connection attempt");
        return false;
    }

    _isConnecting = true;
    WiFi.mode(WIFI_STA);

    if (wifiCredentials.ssid.isEmpty()) {
        SerialQueueManager::getInstance().queueMessage("No SSID supplied.");
        return false;
    }

    WiFi.begin(wifiCredentials.ssid, wifiCredentials.password);
    SerialQueueManager::getInstance().queueMessage("SSID: " + wifiCredentials.ssid);
    SerialQueueManager::getInstance().queueMessage("Password: " + wifiCredentials.password);
    SerialQueueManager::getInstance().queueMessage("Attempting to connect to Wi-Fi...");

    unsigned long startAttemptTime = millis();
    unsigned long lastPrintTime = startAttemptTime;
    unsigned long lastCheckTime = startAttemptTime;

    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < CONNECT_TO_SINGLE_NETWORK_TIMEOUT)) {
        // Non-blocking print of dots
        unsigned long currentTime = millis();
        if (currentTime - lastPrintTime >= printInterval) {
            SerialQueueManager::getInstance().queueMessage(".");
            lastPrintTime = currentTime;
            yield();  // Allow the ESP32 to handle background tasks
        }
        if (currentTime - lastCheckTime >= checkInterval) {
            lastCheckTime = currentTime;
            
            // Poll serial to update connection status
            SerialManager::getInstance().pollSerial();
            
            if (NetworkStateManager::getInstance().shouldStopWiFiOperations()) {
                SerialQueueManager::getInstance().queueMessage("\nSerial connection detected - aborting WiFi connection");
                _isConnecting = false;
                return false;
            }
        }
    }

    _isConnecting = false;

    if (WiFi.status() == WL_CONNECTED) {
        storeWiFiCredentials(wifiCredentials.ssid, wifiCredentials.password, 0);
        SerialQueueManager::getInstance().queueMessage("Connected to Wi-Fi!");
        return true;
    } else {
        SerialQueueManager::getInstance().queueMessage("Failed to connect to Wi-Fi.");
        return false;
    }
}

std::vector<WiFiNetworkInfo> WiFiManager::scanWiFiNetworkInfos() {
    std::vector<WiFiNetworkInfo> networks;
    int numNetworks = 0;
    
    SerialQueueManager::getInstance().queueMessage("Starting WiFi scan...");
    
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
            SerialQueueManager::getInstance().queueMessage("No networks found");
        } else if (numNetworks == -1) {
            SerialQueueManager::getInstance().queueMessage("Scan still running");
        } else if (numNetworks == -2) {
            SerialQueueManager::getInstance().queueMessage("Error with scan");
        } else {
            // SerialQueueManager::getInstance().queueMessage("Unknown scan error: %d\n", numNetworks);
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
        // SerialQueueManager::getInstance().queueMessage("  Network %d: %s (Signal: %d dBm)\n", 
        //              i + 1, network.ssid.c_str(), network.rssi);
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
    SerialQueueManager::getInstance().queueMessage("Available WiFi Networks (sorted by signal strength):");
    SerialQueueManager::getInstance().queueMessage("----------------------------------------------------");
    
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

        // SerialQueueManager::getInstance().queueMessage("%2d. %s | Signal: %d dBm | Security: %s %s\n", 
        //             (int)i + 1, 
        //             network.ssid.c_str(), 
        //             network.rssi,
        //             encryption.c_str(),
        //             (i == _selectedNetworkIndex) ? " <SELECTED>" : "");
    }
    SerialQueueManager::getInstance().queueMessage("----------------------------------------------------");
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
        // SerialQueueManager::getInstance().queueMessage("Selected Network: %s (Signal: %d dBm)\n", 
        //             _availableNetworks[_selectedNetworkIndex].ssid.c_str(),
        //             _availableNetworks[_selectedNetworkIndex].rssi);
    }
}

// For storing WiFi credentials:
void WiFiManager::storeWiFiCredentials(const String& ssid, const String& password, int index) {
    PreferencesManager::getInstance().storeWiFiCredentials(ssid, password, index);
}

void WiFiManager::checkAndReconnectWiFi() {
    // Check if WiFi is connected or already attempting connection
    if (WiFi.status() == WL_CONNECTED || _isConnecting) return;

    // Quick check if any networks exist before trying to get them
    if (!PreferencesManager::getInstance().hasStoredWiFiNetworks()) return;

    unsigned long currentTime = millis();

    if (currentTime - _lastReconnectAttempt < WIFI_RECONNECT_TIMEOUT) return;
    SerialQueueManager::getInstance().queueMessage("WiFi disconnected, attempting to reconnect...");
    
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

WiFiManager::WiFiTestResult WiFiManager::testWiFiCredentials(const String& ssid, const String& password) {
    WiFiTestResult result = {false, false};
    
    SerialQueueManager::getInstance().queueMessage("Testing WiFi credentials...");
    
    // Test WiFi connection without storing credentials
    if (testConnectionOnly(ssid, password)) {
        result.wifiConnected = true;
        SerialQueueManager::getInstance().queueMessage("WiFi connection successful");
        
        // Test WebSocket connection
        WebSocketManager::getInstance().connectToWebSocket();
        
        // Wait for WebSocket connection with timeout
        unsigned long startTime = millis();
        const unsigned long WEBSOCKET_TIMEOUT = 10000; // 10 seconds
        
        while (millis() - startTime < WEBSOCKET_TIMEOUT) {
            WebSocketManager::getInstance().pollWebSocket();
            if (WebSocketManager::getInstance().isConnected()) {
                result.websocketConnected = true;
                SerialQueueManager::getInstance().queueMessage("WebSocket connection successful");
                
                // Only store credentials after both WiFi and WebSocket success
                storeWiFiCredentials(ssid, password, 0);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (!result.websocketConnected) {
            SerialQueueManager::getInstance().queueMessage("WebSocket connection failed - likely captive portal");
            WiFi.disconnect();
        }
    }
    
    return result;
}

void WiFiManager::startAddPipWiFiTest(const String& ssid, const String& password) {
    _addPipSSID = ssid;
    _addPipPassword = password;
    _isTestingAddPipCredentials = true;
    
    SerialQueueManager::getInstance().queueMessage("Starting ADD_PIP_MODE WiFi test...");
}

void WiFiManager::processAddPipMode() {
    if (!_isTestingAddPipCredentials) return;
    
    _isTestingAddPipCredentials = false;

    SerialQueueManager::getInstance().queueMessage("=== Starting WiFi credential test ===");
    SerialQueueManager::getInstance().queueMessage("Target SSID: " + _addPipSSID);

    // Directly attempt connection without scanning
    if (testConnectionOnly(_addPipSSID, _addPipPassword)) {
        SerialQueueManager::getInstance().queueMessage("WiFi connection successful - testing WebSocket...");
        
        // Test WebSocket connection
        WebSocketManager::getInstance().connectToWebSocket();
        
        unsigned long startTime = millis();
        const unsigned long WEBSOCKET_TIMEOUT = 10000;
        bool websocketConnected = false;
        
        while (millis() - startTime < WEBSOCKET_TIMEOUT) {
            WebSocketManager::getInstance().pollWebSocket();
            if (WebSocketManager::getInstance().isConnected()) {
                websocketConnected = true;
                SerialQueueManager::getInstance().queueMessage("WebSocket connection successful!");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (websocketConnected) {
            SerialQueueManager::getInstance().queueMessage("=== Full connection successful - storing credentials ===");
            storeWiFiCredentials(_addPipSSID, _addPipPassword, 0);
            NetworkStateManager::getInstance().setAddPipMode(false);
            SerialManager::getInstance().sendJsonMessage("/wifi-connection-result", "success");
        } else {
            SerialQueueManager::getInstance().queueMessage("=== WebSocket connection failed - likely captive portal ===");
            WiFi.setAutoReconnect(false);
            WiFi.disconnect(true);
            SerialManager::getInstance().sendJsonMessage("/wifi-connection-result", "wifi_only");
        }
    } else {
        SerialQueueManager::getInstance().queueMessage("=== WiFi connection failed ===");
        WiFi.setAutoReconnect(false);
        WiFi.disconnect(true);
        SerialManager::getInstance().sendJsonMessage("/wifi-connection-result", "failed");
    }
    
    // Clear stored credentials
    _addPipSSID = "";
    _addPipPassword = "";
}

bool WiFiManager::testConnectionOnly(const String& ssid, const String& password) {
    SerialQueueManager::getInstance().queueMessage("Testing connection to: " + ssid);
    
    // Disable auto-reconnect before starting the test
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    unsigned long startTime = millis();
    unsigned long lastStatusLog = 0;
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < CONNECT_TO_SINGLE_NETWORK_TIMEOUT)) {
        // Log WiFi status every second for debugging
        if (millis() - lastStatusLog > 1000) {
            String statusStr = "";
            switch(WiFi.status()) {
                case WL_IDLE_STATUS: statusStr = "IDLE"; break;
                case WL_NO_SSID_AVAIL: statusStr = "NO_SSID_AVAIL"; break;
                case WL_SCAN_COMPLETED: statusStr = "SCAN_COMPLETED"; break;
                case WL_CONNECTED: statusStr = "CONNECTED"; break;
                case WL_CONNECT_FAILED: statusStr = "CONNECT_FAILED"; break;
                case WL_CONNECTION_LOST: statusStr = "CONNECTION_LOST"; break;
                case WL_DISCONNECTED: statusStr = "DISCONNECTED"; break;
                default: statusStr = "UNKNOWN(" + String(WiFi.status()) + ")"; break;
            }
            SerialQueueManager::getInstance().queueMessage("WiFi Status: " + statusStr);
            lastStatusLog = millis();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
        
        if (NetworkStateManager::getInstance().shouldStopWiFiOperations()) {
            SerialQueueManager::getInstance().queueMessage("Serial connection detected - aborting WiFi test");
            WiFi.disconnect(true);
            return false;
        }
    }
    
    bool connected = WiFi.status() == WL_CONNECTED;
    
    if (connected) {
        SerialQueueManager::getInstance().queueMessage("✓ WiFi connection successful!");
        SerialQueueManager::getInstance().queueMessage("IP Address: " + WiFi.localIP().toString());
    } else {
        WiFi.disconnect(true);
        SerialQueueManager::getInstance().queueMessage("✗ WiFi connection failed after " + String(CONNECT_TO_SINGLE_NETWORK_TIMEOUT) + "ms");
    }
    
    return connected;
}

void WiFiManager::clearAllWiFiData() {
    // Clear stored credentials from preferences
    bool prefsCleared = PreferencesManager::getInstance().clearAllWiFiNetworks();
    
    // Clear in-memory state
    _availableNetworks.clear();
    _selectedNetworkIndex = 0;
    
    // Disconnect from current WiFi if connected
    if (WiFi.status() == WL_CONNECTED) {
        SerialQueueManager::getInstance().queueMessage("Disconnecting from current WiFi...");
        WiFi.disconnect(true);
    }
    
    // Clear any ongoing test credentials
    _addPipSSID = "";
    _addPipPassword = "";
    _isTestingAddPipCredentials = false;
    
    if (prefsCleared) {
        SerialQueueManager::getInstance().queueMessage("All WiFi data cleared successfully");
    } else {
        SerialQueueManager::getInstance().queueMessage("WiFi data partially cleared - preferences clear failed");
    }
}

std::vector<WiFiCredentials> WiFiManager::getSavedNetworksForResponse() {
    // Check if any networks exist
    if (!PreferencesManager::getInstance().hasStoredWiFiNetworks()) {
        SerialQueueManager::getInstance().queueMessage("No saved networks found");
        return std::vector<WiFiCredentials>(); // Return empty vector
    }

    // Get all saved networks from preferences
    std::vector<WiFiCredentials> savedNetworks = PreferencesManager::getInstance().getAllStoredWiFiNetworks();
    
    SerialQueueManager::getInstance().queueMessage("Found " + String(savedNetworks.size()) + " saved networks");
    
    return savedNetworks;
}

std::vector<WiFiNetworkInfo> WiFiManager::scanAndReturnNetworks() {
    SerialQueueManager::getInstance().queueMessage("Performing WiFi scan for browser...");
    
    // Use your existing scan method
    std::vector<WiFiNetworkInfo> networks = scanWiFiNetworkInfos();
    
    SerialQueueManager::getInstance().queueMessage("Scan complete. Found " + String(networks.size()) + " networks");
    
    return networks;
}
