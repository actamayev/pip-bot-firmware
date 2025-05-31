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
    std::vector<WiFiCredentials> savedNetworks = PreferencesManager::getInstance().getAllStoredWiFiNetworks();
    
    if (savedNetworks.empty()) {
        Serial.println("No saved networks found");
        return false;
    }

    if (
        (ledAnimations.getCurrentAnimation() != LedTypes::BREATHING) &&
        (BytecodeVM::getInstance().isPaused == BytecodeVM::getInstance().PROGRAM_NOT_STARTED)
    ) {
        rgbLed.set_led_red();
        ledAnimations.startBreathing();
    }

    Serial.println("Attempting direct connection to saved networks...");
    
    // Try to connect to each saved network without scanning first
    for (const WiFiCredentials& network : savedNetworks) {
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
        storeWiFiCredentials(wifiCredentials.ssid, wifiCredentials.password, 0);
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
    PreferencesManager::getInstance().storeWiFiCredentials(ssid, password, index);
}

void WiFiManager::checkAndReconnectWiFi() {
    // Check if WiFi is connected or already attempting connection
    if (WiFi.status() == WL_CONNECTED || _isConnecting) return;

    std::vector<WiFiCredentials> savedNetworks = PreferencesManager::getInstance().getAllStoredWiFiNetworks();
    if (savedNetworks.empty()) return;

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

WiFiManager::WiFiTestResult WiFiManager::testWiFiCredentials(const String& ssid, const String& password) {
    WiFiTestResult result = {false, false};
    
    Serial.println("Testing WiFi credentials...");
    
    // Test WiFi connection without storing credentials
    if (testConnectionOnly(ssid, password)) {
        result.wifiConnected = true;
        Serial.println("WiFi connection successful");
        
        // Test WebSocket connection
        WebSocketManager::getInstance().connectToWebSocket();
        
        // Wait for WebSocket connection with timeout
        unsigned long startTime = millis();
        const unsigned long WEBSOCKET_TIMEOUT = 10000; // 10 seconds
        
        while (millis() - startTime < WEBSOCKET_TIMEOUT) {
            WebSocketManager::getInstance().pollWebSocket();
            if (WebSocketManager::getInstance().isConnected()) {
                result.websocketConnected = true;
                Serial.println("WebSocket connection successful");
                
                // Only store credentials after both WiFi and WebSocket success
                storeWiFiCredentials(ssid, password, 0);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (!result.websocketConnected) {
            Serial.println("WebSocket connection failed - likely captive portal");
            WiFi.disconnect();
        }
    }
    
    return result;
}

bool WiFiManager::testConnectionOnly(const String& ssid, const String& password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    unsigned long startTime = millis();
    unsigned long lastStatusLog = 0;
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < CONNECT_TO_SINGLE_NETWORK_TIMEOUT)) {
        vTaskDelay(pdMS_TO_TICKS(100));
        
        if (NetworkStateManager::getInstance().shouldStopWiFiOperations()) {
            return false;
        }
    }
    
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::startAddPipWiFiTest(const String& ssid, const String& password) {
    _addPipSSID = ssid;
    _addPipPassword = password;
    _isTestingAddPipCredentials = true;
    
    Serial.println("Starting ADD_PIP_MODE WiFi test...");
}

void WiFiManager::processAddPipMode() {
    if (!_isTestingAddPipCredentials) return;
    
    _isTestingAddPipCredentials = false;

    // Test WiFi connection
    if (testConnectionOnly(_addPipSSID, _addPipPassword)) {
        SerialManager::safePrintln("WiFi connection successful - testing WebSocket...");
        
        // Test WebSocket connection
        WebSocketManager::getInstance().connectToWebSocket();
        
        unsigned long startTime = millis();
        const unsigned long WEBSOCKET_TIMEOUT = 10000;
        bool websocketConnected = false;
        
        while (millis() - startTime < WEBSOCKET_TIMEOUT) {
            WebSocketManager::getInstance().pollWebSocket();
            if (WebSocketManager::getInstance().isConnected()) {
                websocketConnected = true;
                SerialManager::safePrintln("WebSocket connection successful!");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (websocketConnected) {
            SerialManager::safePrintln("=== Full connection successful - storing credentials ===");
            storeWiFiCredentials(_addPipSSID, _addPipPassword, 0);
            NetworkStateManager::getInstance().setAddPipMode(false);
            SerialManager::getInstance().sendJsonMessage("/wifi-connection-result", "success");
        } else {
            SerialManager::safePrintln("=== WebSocket connection failed - likely captive portal ===");
            WiFi.disconnect();
            SerialManager::getInstance().sendJsonMessage("/wifi-connection-result", "wifi_only");
        }
    } else {
        SerialManager::safePrintln("=== WiFi connection failed ===");
        SerialManager::getInstance().sendJsonMessage("/wifi-connection-result", "failed");
    }
    
    // Clear stored credentials
    _addPipSSID = "";
    _addPipPassword = "";
}
