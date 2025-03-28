#include "../utils/config.h"
#include "../actuators/rgb_led.h"
#include "./wifi_manager.h"
#include "./websocket_manager.h"

Preferences preferences;

WiFiManager::WiFiManager() {
	Serial.println("WiFiManager constructor");

    // Hard-coding Wifi creds during initialization (before we have encoders + screen)
    preferences.begin("wifi-creds", false);
    storeWiFiCredentials("Another Dimension", "Iforgotit123", 0);

	initializeWiFi();
}

void WiFiManager::initializeWiFi() {
    // Register event handler for WiFi events
	// 1/10/25 TODO: The wifi event handler isn't registering correctly.
    esp_err_t err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiManager::onWiFiEvent, this, &wifi_event_instance);
    // if (err != ESP_OK) {
    //     Serial.print("Failed to register WiFi event handler. Error: ");
    //     Serial.println(err);
    // } else {
    //     Serial.println("WiFi event handler registered successfully");
    // }
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFiManager::onIpEvent, this, &ip_event_instance);
	Serial.println("WiFi event handlers registered");
}

void WiFiManager::onWiFiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base != WIFI_EVENT || event_id == WIFI_EVENT_STA_DISCONNECTED) return;
	wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
	Serial.printf("WiFi disconnected! Reason: %d\n", event->reason);
	rgbLed.turn_led_off();

	// Reconnect to WiFi
	WiFiManager* wifiManager = static_cast<WiFiManager*>(arg);
	wifiManager->connectToStoredWiFi();
}

void WiFiManager::onIpEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (WiFi.getMode() != WIFI_STA || event_base != IP_EVENT || event_id != IP_EVENT_STA_GOT_IP) {
		return;
	}
	// Extract IP information from the event data
	ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data);

	// Convert the esp_ip4_addr_t to IPAddress
	IPAddress ip(event->ip_info.ip.addr);  // Use the addr directly to create an IPAddress object

	// Print the IP address using the IPAddress class
	Serial.println("WiFi connected! IP address: " + ip.toString());
	rgbLed.set_led_blue();

	// Now connect to the WebSocket
    WebSocketManager::getInstance().connectToWebSocket();
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
        // If no networks found, start AP
        Serial.println("No networks found in full scan, starting Access Point");
        startAccessPoint();
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
    WiFi.mode(WIFI_AP_STA);

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

void WiFiManager::startAccessPoint() {
	WiFi.disconnect(true);
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAP(getAPSSID().c_str(), NULL);

	IPAddress apIP(192, 168, 4, 1);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

	Serial.println("Access Point started.");
}

std::vector<WiFiNetworkInfo> WiFiManager::scanWiFiNetworkInfos() {
    std::vector<WiFiNetworkInfo> networks;

    WiFi.disconnect(true);
    WiFi.scanDelete();
    delay(100);
    // Set WiFi mode to station before scanning
    WiFi.mode(WIFI_STA);

    // Turn on LED to indicate scanning 
    rgbLed.set_led_purple();

    Serial.println("Scanning for WiFi networks...");
    int numNetworks = WiFi.scanNetworks(false);
    Serial.printf("Scan completed, found %d networks\n", numNetworks);

    // Turn off LED after scanning
    rgbLed.turn_led_off();

    if (numNetworks <= 0) {
        if (numNetworks == 0) {
            Serial.println("No networks found");
        } else if (numNetworks == -1) {
            Serial.println("No Scan currently running");
        } else if (numNetworks == -2) {
            Serial.println("Error with scan");
        } else {
            Serial.printf("Unknown scan error: %d\n", numNetworks);
        }
        return networks;
    }

    // Store the networks
    for (int i = 0; i < numNetworks; i++) {
        WiFiNetworkInfo network;
        network.ssid = WiFi.SSID(i);
        network.rssi = WiFi.RSSI(i);
        network.encryptionType = WiFi.encryptionType(i);
        networks.push_back(network);
    }

    // Sort networks by signal strength
    sortNetworksBySignalStrength(networks);
    
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
