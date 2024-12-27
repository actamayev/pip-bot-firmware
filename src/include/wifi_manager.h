#pragma once

#include <WiFi.h>
#include <Preferences.h>
#include "./structs.h"
#include "./singleton.h"

class WiFiManager : public Singleton<WiFiManager> {
    friend class Singleton<WiFiManager>;

	public:
		void connectToStoredWiFi();
		WiFiCredentials getStoredWiFiCredentials();
		bool attemptNewWifiConnection(String ssid, String password);

	private:
		WiFiManager();
		void initializeWiFi();

		static void onWiFiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		static void onIpEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		void startAccessPoint();
};

extern Preferences preferences;
