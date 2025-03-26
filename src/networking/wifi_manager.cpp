#include "../utils/config.h"
#include "../actuators/rgb_led.h"
#include "./wifi_manager.h"
#include "./websocket_manager.h"

Preferences preferences;

WiFiManager::WiFiManager() {
	Serial.println("WiFiManager constructor");
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
	WiFiCredentials storedCreds = getStoredWiFiCredentials();

	bool connectionStatus = attemptNewWifiConnection(storedCreds);
	if (connectionStatus == true) {
        return WebSocketManager::getInstance().connectToWebSocket();
    }
    // Scan for networks instead of immediately starting AP
    auto networks = scanWiFiNetworks();

    if (networks.empty()) {
        // If no networks found, start AP
        startAccessPoint();
        return;
    }
    // Print the available networks and select the first one
    printNetworkList(networks);
    setSelectedNetworkIndex(0);
    
    // Init encoder for network selection
    EncoderManager::getInstance().initNetworkSelection();
    
    // Don't start AP yet, let user scroll through networks
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

std::vector<WiFiManager::WiFiNetwork> WiFiManager::scanWiFiNetworks() {
    std::vector<WiFiNetwork> networks;

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
        WiFiNetwork network;
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

void WiFiManager::sortNetworksBySignalStrength(std::vector<WiFiNetwork>& networks) {
    // Sort networks by RSSI (higher values = stronger signal)
    std::sort(networks.begin(), networks.end(), 
              [](const WiFiNetwork& a, const WiFiNetwork& b) {
                  return a.rssi > b.rssi;
              });
}

void WiFiManager::printNetworkList(const std::vector<WiFiNetwork>& networks) {
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
