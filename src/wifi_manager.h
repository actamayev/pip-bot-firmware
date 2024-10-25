#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>

struct WiFiCredentials {
	String ssid;
	String password;
};

class WiFiManager {
	private:
		static void onWiFiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		static void onIpEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
	public:
		void initializeWiFi();
		bool connectToStoredWiFi();
		void startAccessPoint();
		WiFiCredentials getStoredWiFiCredentials();
};

extern WiFiManager wifiManager;
extern Preferences preferences;

#endif
