#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>

struct WiFiCredentials {
	String ssid;
	String password;
};

class WiFiManager {
public:
	bool connectToStoredWiFi();
	void startAccessPoint();
	void reconnectWiFi();
	WiFiCredentials getStoredWiFiCredentials();  // Utility function declaration
};

extern WiFiManager wifiManager;
extern Preferences preferences;

#endif
