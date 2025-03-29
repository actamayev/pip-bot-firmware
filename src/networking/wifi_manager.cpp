#include "../utils/config.h"
#include "../actuators/rgb_led.h"
#include "./wifi_manager.h"
#include "./websocket_manager.h"

Preferences preferences;

WiFiManager::WiFiManager() {
    // Hard-coding Wifi creds during initialization (before we have encoders + screen)
    preferences.begin("wifi-creds", false);
    storeWiFiCredentials("Another Dimension", "Iforgotit123", 0);

	initializeWiFi();
}

void WiFiManager::initializeWiFi() {
    // Clear any previous handlers
    WiFi.disconnect(true);
    delay(100);

    // Set WiFi mode
    WiFi.mode(WIFI_STA);

    // Register single event handler for all WiFi events
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        this->handleWiFiEvent(event, info);
    });

    Serial.println("WiFi event handler registered");
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
    // Try to connect to strongest saved network
    bool connectionStatus = connectToStrongestSavedNetwork();

    if (connectionStatus) {
        return WebSocketManager::getInstance().connectToWebSocket();
    }

    // If no saved networks are in range or connection failed,
    // do a full scan for all networks
    auto networks = scanWiFiNetworkInfos();
    
    if (networks.empty()) {
        // If no networks found
        Serial.println("No networks found in full scan.");
        return;
    }
    
    // Print the available networks and select the first one
    printNetworkList(networks);
    setSelectedNetworkIndex(0);
    
    // Init encoder for network selection
    EncoderManager::getInstance().initNetworkSelection();
    
    Serial.println("Use the right motor to scroll through networks");
}

bool WiFiManager::attemptNewWifiConnection(WiFiCredentials wifiCredentials) {
    // Set WiFi mode to Station (client mode)
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
    const unsigned long timeout = 10000;  // 10-second timeout
    const unsigned long printInterval = 100;  // Print dots every 100ms

    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < timeout)) {
        // Non-blocking print of dots
        unsigned long currentTime = millis();
        if (currentTime - lastPrintTime >= printInterval) {
            Serial.print(".");
            lastPrintTime = currentTime;
            yield();  // Allow the ESP32 to handle background tasks
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to Wi-Fi!");
        rgbLed.set_led_blue();
        return true;
    } else {
        Serial.println("Failed to connect to Wi-Fi.");
        rgbLed.turn_led_off();
        return false;
    }
}

std::vector<WiFiNetworkInfo> WiFiManager::scanWiFiNetworkInfos() {
    std::vector<WiFiNetworkInfo> networks;
    
    // First try the hard reset before scanning
    hardResetWiFi();
    
    // Try async scan instead of blocking scan
    Serial.println("Starting async WiFi scan...");
    if (WiFi.scanNetworks(true) == WIFI_SCAN_RUNNING) {
        Serial.println("Async scan started successfully");
        
        // Wait for scan completion with timeout
        unsigned long scanStart = millis();
        const unsigned long SCAN_TIMEOUT = 10000; // 10 seconds timeout
        
        while (WiFi.scanComplete() < 0 && millis() - scanStart < SCAN_TIMEOUT) {
            delay(100);
        }
        
        // Check scan results
        int numNetworks = WiFi.scanComplete();
        
        if (numNetworks > 0) {
            Serial.printf("Found %d networks\n", numNetworks);
            
            // Process networks here
            for (int i = 0; i < numNetworks; i++) {
                WiFiNetworkInfo network;
                network.ssid = WiFi.SSID(i);
                network.rssi = WiFi.RSSI(i);
                network.encryptionType = WiFi.encryptionType(i);
                networks.push_back(network);
            }
            
            // Sort and store
            sortNetworksBySignalStrength(networks);
            _availableNetworks = networks;
            _selectedNetworkIndex = 0;
            
            // Clean up
            WiFi.scanDelete();
        } else {
            Serial.printf("Async scan failed with code %d\n", numNetworks);
        }
    } else {
        Serial.println("Failed to start async scan");
    }
    
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
    
    for (size_t i = 0; i < networks.size(); i++) {
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

struct WiFiNetworkInfo WiFiManager::scanForSpecificNetwork(const String& ssid) {
    WiFiNetworkInfo result;
    result.ssid = "";
    result.rssi = -100; // Default weak signal
    
    // Disconnect to ensure clean scan state
    WiFi.disconnect(true);
    delay(100);
    
    Serial.printf("Scanning for network: %s\n", ssid.c_str());
   
    // Targeted scan for this SSID (faster than full scan)
    int numNetworks = WiFi.scanNetworks(false, false, false, 300, 0, ssid.c_str());

    if (numNetworks > 0) {
        for (int i = 0; i < numNetworks; i++) {
            if (WiFi.SSID(i) == ssid) {
                result.ssid = WiFi.SSID(i);
                result.rssi = WiFi.RSSI(i);
                result.encryptionType = WiFi.encryptionType(i);
                break;
            }
        }
    }

    // Clean up scan results
    WiFi.scanDelete();
    
    return result;
}

bool WiFiManager::connectToStrongestSavedNetwork() {
    // Turn on LED to indicate scanning
    rgbLed.set_led_purple();
    
    // Get all saved networks
    std::vector<WiFiCredentials> savedNetworks = getAllStoredNetworks();
    
    if (savedNetworks.empty()) {
        Serial.println("No saved networks found");
        rgbLed.turn_led_off();
        return false;
    }
    
    // Structure to track available networks with their signal strength
    struct AvailableSavedNetwork {
        WiFiCredentials credentials;
        int32_t rssi;
    };
    std::vector<AvailableSavedNetwork> availableNetworks;
    
    // For each saved network, do a targeted scan
    for (const auto& network : savedNetworks) {
        // Scan specifically for this network
        WiFiNetworkInfo scanResult = scanForSpecificNetwork(network.ssid);
        
        if (!scanResult.ssid.isEmpty()) {
            // Found the network, add it to available networks
            AvailableSavedNetwork available;
            available.credentials = network;
            available.rssi = scanResult.rssi;
            availableNetworks.push_back(available);
            
            Serial.printf("Found saved network %s with signal strength %d dBm\n", 
                      network.ssid.c_str(), scanResult.rssi);
        }
    }
    
    // Turn off scanning LED
    rgbLed.turn_led_off();
    
    if (availableNetworks.empty()) {
        Serial.println("None of the saved networks are in range");
        return false;
    }
    
    // Sort networks by signal strength (strongest first)
    std::sort(availableNetworks.begin(), availableNetworks.end(),
            [](const AvailableSavedNetwork& a, const AvailableSavedNetwork& b) {
                return a.rssi > b.rssi;
            });
    
    // Try to connect to the strongest network
    Serial.printf("Connecting to strongest network: %s (signal: %d dBm)\n",
                availableNetworks[0].credentials.ssid.c_str(),
                availableNetworks[0].rssi);
    
    return attemptNewWifiConnection(availableNetworks[0].credentials);
}

void WiFiManager::handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi disconnected!");
            rgbLed.turn_led_off();

            // Attempt to reconnect
            delay(200); // Wait a bit before reconnecting
            connectToStoredWiFi();
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Got IP: ");
            Serial.println(WiFi.localIP());
            rgbLed.set_led_blue();
            break;
    }
}

bool WiFiManager::hardResetWiFi() {
    Serial.println("Performing hard WiFi reset...");

    // Complete WiFi shutdown
    WiFi.disconnect(true, true);  // Disconnect and clear credentials
    WiFi.scanDelete();
    esp_wifi_stop();              // Stop WiFi at driver level
    esp_wifi_deinit();            // Complete deinitialization
    delay(500);
    
    // Reinitialize WiFi from scratch
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        Serial.printf("WiFi init failed with error %d\n", err);
        return false;
    }
    
    err = esp_wifi_start();
    if (err != ESP_OK) {
        Serial.printf("WiFi start failed with error %d\n", err);
        return false;
    }
    
    WiFi.mode(WIFI_STA);
    delay(500);
    
    Serial.println("WiFi hard reset complete");
    return true;
}
