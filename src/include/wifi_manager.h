#pragma once

#include <WiFi.h>
#include <Preferences.h>

struct WiFiCredentials {
	String ssid;
	String password;
};

class WiFiManager {
	public:
		static WiFiManager& getInstance() {
			if (instance == nullptr) {
				instance = new WiFiManager();
			}
			return *instance;
		}
		void connectToStoredWiFi();
		WiFiCredentials getStoredWiFiCredentials();
		bool attemptNewWifiConnection(String ssid, String password);
	private:
		static WiFiManager* instance;

		WiFiManager();
		void initializeWiFi();

		static void onWiFiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		static void onIpEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		void startAccessPoint();

		WiFiManager(const WiFiManager&) = delete;
		WiFiManager& operator=(const WiFiManager&) = delete;
};

extern Preferences preferences;
