#pragma once

#include <WiFi.h>
#include <Preferences.h>
#include "../utils/structs.h"
#include "../utils/singleton.h"

class WiFiManager : public Singleton<WiFiManager> {
    friend class Singleton<WiFiManager>;

	public:
		void connectToStoredWiFi();
		WiFiCredentials getStoredWiFiCredentials();
		bool attemptNewWifiConnection(WiFiCredentials wifiCredentials);
		void startAccessPoint();

	private:
		WiFiManager();
		void initializeWiFi();

		static void onWiFiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		static void onIpEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		esp_event_handler_instance_t wifi_event_instance;
		esp_event_handler_instance_t ip_event_instance;
};

extern Preferences preferences;
