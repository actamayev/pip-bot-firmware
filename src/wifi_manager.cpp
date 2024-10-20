#include "wifi_manager.h"
#include "config.h"
#include "webserver_manager.h"

Preferences preferences;

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

	Serial.println("In connect to stored wifi: stored_ssid: " + storedCreds.ssid);
	Serial.println("In connect to stored wifi: stored_password: " + storedCreds.password);

	if (storedCreds.ssid == "" || storedCreds.password == "") {
		Serial.println("No stored credentials found.");
		return false;
	}

	// Try to connect to the saved Wi-Fi credentials
	WiFi.begin(storedCreds.ssid.c_str(), storedCreds.password.c_str());
	Serial.println("Attempting to connect to saved Wi-Fi...");

	int retries = 0;
	while (WiFi.status() != WL_CONNECTED && retries < 20) {
		delay(500);
		Serial.print(".");
		retries++;
	}

	const bool wifiStatus = WiFi.status();
	if (wifiStatus == WL_CONNECTED) {
		Serial.println("Connected to Wi-Fi!");
		digitalWrite(LED_PIN, HIGH);  // Turn on LED to show success
		return true;
	} else {
		Serial.println("Failed to connect to saved Wi-Fi.");
		return false;
	}
}

void WiFiManager::startAccessPoint() {
	// Start AP mode and the web server
	WiFi.disconnect(true);
	WiFi.mode(WIFI_AP);
	WiFi.softAP(ap_ssid, ap_password);

	IPAddress apIP(192, 168, 4, 1);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	webserverManager.startWebServer();
}

void WiFiManager::reconnectWiFi() {
	WiFiCredentials creds = getStoredWiFiCredentials();

	int retries = 0;
	WiFi.begin(creds.ssid.c_str(), creds.password.c_str());
	bool wifiStatus = WiFi.status();
	while (wifiStatus != WL_CONNECTED && retries < 10) {
		delay(500);
		Serial.print(".");
		retries++;
    }

	wifiStatus = WiFi.status();
	if (wifiStatus == WL_CONNECTED) {
		Serial.println("Reconnected to Wi-Fi!");
		digitalWrite(LED_PIN, HIGH);  // Turn on LED to indicate success
	} else {
		Serial.println("Failed to reconnect to Wi-Fi.");
	}
}

WiFiManager wifiManager;  // Create global instance
