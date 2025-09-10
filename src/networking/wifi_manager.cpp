#include "wifi_manager.h"

WiFiManager::WiFiManager() {
    // Hard-coding Wifi creds during initialization
    // storeWiFiCredentials("Another Dimension", "Iforgotit123", 0);
    // storeWiFiCredentials("NETGEAR08", "breezyshoe123", 1);
    // storeWiFiCredentials("iPhone", "12345678", 0);
    // storeWiFiCredentials("MSTest", "!haftr2024!", 0);

    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    connectToStoredWiFi();

    // TODO: 8/15/25: Possibly implement this (this was done in the WiFi + ESP NOW to substantially decrease the ESP-now latency when connected to WiFi)
    // In receiver setup(), after WiFi connects:
    // WiFi.setSleep(false);  // Disable WiFi power saving
    // esp_wifi_set_ps(WIFI_PS_NONE);  // Disable power saving completely
}

void WiFiManager::connectToStoredWiFi() {
    // Try to connect directly to any saved network without scanning
    bool connectionStatus = attemptDirectConnectionToSavedNetworks();

    if (!connectionStatus) {
        startAsyncScan();
    } else {
        WebSocketManager::getInstance().connectToWebSocket();
    }
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

    // if (
    //     (ledAnimations.getCurrentAnimation() != LedTypes::BREATHING) &&
    //     (BytecodeVM::getInstance().isPaused == BytecodeVM::getInstance().PROGRAM_NOT_STARTED)
    // ) {
    //     rgbLed.set_led_red();
    //     ledAnimations.startBreathing();
    // }

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

    unsigned long startAttemptTime = millis();
    unsigned long lastPrintTime = startAttemptTime;
    unsigned long lastCheckTime = startAttemptTime;

    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < CONNECT_TO_SINGLE_NETWORK_TIMEOUT)) {
        // Give other tasks time to run - CRITICAL for sensor performance
        vTaskDelay(pdMS_TO_TICKS(250)); // 250ms delay reduces sensor impact
        
        // Non-blocking print of dots
        unsigned long currentTime = millis();
        if (currentTime - lastPrintTime >= printInterval) {
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

void WiFiManager::startAddPipWiFiTest(const String& ssid, const String& password) {
    _addPipSSID = ssid;
    _addPipPassword = password;
    _isTestingAddPipCredentials = true;
}

void WiFiManager::processAddPipMode() {
    if (!_isTestingAddPipCredentials) return;
    
    _isTestingAddPipCredentials = false;

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
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (websocketConnected) {
            storeWiFiCredentials(_addPipSSID, _addPipPassword, 0);
            NetworkStateManager::getInstance().setAddPipMode(false);
            SerialManager::getInstance().sendJsonMessage(RouteType::WIFI_CONNECTION_RESULT, "success");
        } else {
            WiFi.setAutoReconnect(false);
            WiFi.disconnect(true);
            SerialManager::getInstance().sendJsonMessage(RouteType::WIFI_CONNECTION_RESULT, "wifi_only");
        }
    } else {
        WiFi.setAutoReconnect(false);
        WiFi.disconnect(true);
        SerialManager::getInstance().sendJsonMessage(RouteType::WIFI_CONNECTION_RESULT, "failed");
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
        
        vTaskDelay(pdMS_TO_TICKS(500)); // Increased delay - less aggressive polling for better sensor performance
        
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

bool WiFiManager::startAsyncScan() {
    // Ignore if scan already in progress
    if (_asyncScanInProgress) {
        SerialQueueManager::getInstance().queueMessage("Scan already in progress, ignoring request");
        return false;
    }

    SerialQueueManager::getInstance().queueMessage("Starting async WiFi scan...");
    
    // Clear any previous scan results
    _availableNetworks.clear();
    _selectedNetworkIndex = 0;
    
    // Prepare WiFi for scanning
    WiFi.disconnect(true);
    WiFi.scanDelete();
    vTaskDelay(pdMS_TO_TICKS(100));
    WiFi.mode(WIFI_STA);

    // Set LED to indicate scanning
    rgbLed.set_led_purple();
    ledAnimations.startBreathing();

    // Start async scan
    int16_t result = WiFi.scanNetworks(true); // true = async
    
    if (result == WIFI_SCAN_RUNNING) {
        _asyncScanInProgress = true;
        _asyncScanStartTime = millis();
        SerialQueueManager::getInstance().queueMessage("Async scan initiated successfully");
        
        // Send scan started message to browser
        SerialManager::getInstance().sendScanStartedMessage();
        
        return true;
    } else {
        SerialQueueManager::getInstance().queueMessage("Failed to start async scan");
        rgbLed.turn_main_board_leds_off();
        return false;
    }
}

void WiFiManager::checkAsyncScanProgress() {
    if (!_asyncScanInProgress) return; // No scan in progress

    unsigned long currentTime = millis();
    unsigned long scanDuration = currentTime - _asyncScanStartTime;

    // Don't check status too soon - give the scan time to actually start
    if (scanDuration < ASYNC_SCAN_MIN_CHECK_DELAY) return; // Too early to check

    // Check for timeout
    if (scanDuration > ASYNC_SCAN_TIMEOUT_MS) {
        SerialQueueManager::getInstance().queueMessage("Async WiFi scan timed out after " + String(scanDuration) + "ms");
        _asyncScanInProgress = false;
        rgbLed.turn_main_board_leds_off();
        
        // Clean up any scan results
        WiFi.scanDelete();
        
        // Send empty scan complete message to browser to indicate timeout
        std::vector<WiFiNetworkInfo> emptyNetworks;
        SerialManager::getInstance().sendScanResultsResponse(emptyNetworks);
        return;
    }

    // Check scan status
    int16_t scanResult = WiFi.scanComplete();
    
    // For WIFI_SCAN_RUNNING (-1) and WIFI_SCAN_FAILED (-2), just keep waiting until timeout
    // The ESP32 WiFi library seems to return WIFI_SCAN_FAILED sometimes even when scan is progressing
    if (scanResult < 0) return;
    // Only handle completion (positive numbers) - ignore WIFI_SCAN_FAILED and WIFI_SCAN_RUNNING
    // Scan completed successfully
    SerialQueueManager::getInstance().queueMessage("Async WiFi scan completed in " + String(scanDuration) + "ms. Found " + String(scanResult) + " networks");
    _asyncScanInProgress = false;
    rgbLed.turn_main_board_leds_off();
    _lastScanCompleteTime = millis();
    // Process scan results
    std::vector<WiFiNetworkInfo> networks;
    
    for (int i = 0; i < scanResult; i++) {
        WiFiNetworkInfo network;
        network.ssid = WiFi.SSID(i);
        network.rssi = WiFi.RSSI(i);
        network.encryptionType = WiFi.encryptionType(i);
        networks.push_back(network);
    }
    
    // Sort networks by signal strength
    sortNetworksBySignalStrength(networks);
    
    // Update class members
    _availableNetworks = networks;
    _selectedNetworkIndex = 0;
    
    // Clean up scan results to free memory
    WiFi.scanDelete();
    
    // Send results to browser
    SerialManager::getInstance().sendScanResultsResponse(networks);
}

void WiFiManager::clearNetworksIfStale() {
    unsigned long now = millis();
    if (
        _availableNetworks.empty() ||
        (now - _lastScanCompleteTime <= STALE_SCAN_TIMEOUT_MS)
    ) return;
    _availableNetworks.clear();
    SerialQueueManager::getInstance().queueMessage("WiFi scan results cleared (stale > 30 min)");
}
