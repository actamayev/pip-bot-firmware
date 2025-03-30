#pragma once

#include <WiFi.h>
#include <vector>
#include <algorithm>
#include <Preferences.h>
#include "../utils/structs.h"
#include "../utils/singleton.h"
#include "../sensors/encoder_manager.h"

class WiFiManager : public Singleton<WiFiManager> {
    friend class Singleton<WiFiManager>;

	public:
		void connectToStoredWiFi();
		WiFiCredentials getStoredWiFiCredentials();
		bool attemptNewWifiConnection(WiFiCredentials wifiCredentials);

		std::vector<WiFiNetworkInfo> scanWiFiNetworkInfos();
		void sortNetworksBySignalStrength(std::vector<WiFiNetworkInfo>& networks);
		void printNetworkList(const std::vector<WiFiNetworkInfo>& networks);
		int getSelectedNetworkIndex() const { return _selectedNetworkIndex; }
		void setSelectedNetworkIndex(int index);
		const std::vector<WiFiNetworkInfo>& getAvailableNetworks() const { return _availableNetworks; }

		void storeWiFiCredentials(const String& ssid, const String& password, int index);
		std::vector<WiFiCredentials> getAllStoredNetworks();
	private:
		WiFiManager();
		void initializeWiFi();

		std::vector<WiFiNetworkInfo> _availableNetworks;
		int _selectedNetworkIndex = 0;

		static void onWiFiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		static void onIpEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		esp_event_handler_instance_t wifi_event_instance;
		esp_event_handler_instance_t ip_event_instance;
		bool attemptDirectConnectionToSavedNetworks();
};

extern Preferences preferences;
