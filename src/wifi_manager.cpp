#include "wifi_manager.h"
#include "config.h"
#include "webserver_manager.h"
#include "websocket_manager.h"
#include "esp32_api_client.h"

Preferences preferences;
WiFiManager wifiManager;  // Create global instance

void WiFiManager::initializeWiFi() {
    // Set WiFi mode to Station (client mode)
    WiFi.mode(WIFI_STA);

    // Register event handler for WiFi events
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiManager::onWiFiEvent, this, &instance_any_id);
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFiManager::onWiFiEvent, this, &instance_got_ip);
}

void WiFiManager::onWiFiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base != WIFI_EVENT || event_id != WIFI_EVENT_STA_DISCONNECTED) {
		return;
	}
	Serial.println("WiFi disconnected! Reconnecting...");
	digitalWrite(LED_PIN, LOW);  // Indicate failure with LED

	// Reconnect to WiFi (using the WiFiManager instance)
	WiFiManager* wifiManager = static_cast<WiFiManager*>(arg);
	wifiManager->connectToStoredWiFi();  // Attempt to reconnect to WiFi
}

void WiFiManager::onIpEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base != IP_EVENT || event_id != IP_EVENT_STA_GOT_IP) {
		return;
	}
	// Extract IP information from the event data
	ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data);

	// Convert the esp_ip4_addr_t to IPAddress
	IPAddress ip(event->ip_info.ip.addr);  // Use the addr directly to create an IPAddress object

	// Print the IP address using the IPAddress class
	Serial.println("WiFi connected! IP address: " + ip.toString());
	digitalWrite(LED_PIN, HIGH);  // Indicate success with LED

	// Now connect to the WebSocket
	apiClient.connectWebSocket();  // Try connecting to WebSocket after WiFi is connected
}

WiFiCredentials WiFiManager::getStoredWiFiCredentials() {
	WiFiCredentials creds;
	preferences.begin("wifi-creds", false);
	creds.ssid = preferences.getString("ssid", "");
	creds.password = preferences.getString("password", "");
	preferences.end();
	return creds;
}

bool WiFiManager::connectToStoredWiFi() {
	WiFiCredentials storedCreds = getStoredWiFiCredentials();

	if (storedCreds.ssid.isEmpty()) {
		Serial.println("No stored credentials found.");
		return false;
	}

	WiFi.begin(storedCreds.ssid.c_str(), storedCreds.password.c_str());
	WiFi.config(IPAddress(0, 0, 0, 0), IPAddress(8, 8, 8, 8), IPAddress(255, 255, 255, 0)); // Adding a fallback DNS server (Google DNS)
	Serial.println("Attempting to connect to saved Wi-Fi...");

    unsigned long startAttemptTime = millis();
    const unsigned long timeout = 10000;  // 10-second timeout

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
        delay(100);
        Serial.print(".");
    }

	if (WiFi.status() == WL_CONNECTED) {
		Serial.println("Connected to Wi-Fi!");
		digitalWrite(LED_PIN, HIGH);  // Turn on LED to show success
		return true;
	} else {
		Serial.println("Failed to connect to saved Wi-Fi.");
		return false;
	}
}

void WiFiManager::startAccessPoint() {
	WiFi.disconnect(true);
	WiFi.mode(WIFI_AP);
	WiFi.softAP(ap_ssid, ap_password);

	IPAddress apIP(192, 168, 4, 1);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

	Serial.println("Access Point started.");
	webServerManager.startWebServer();
}
