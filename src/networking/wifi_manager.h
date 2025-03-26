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
		void startAccessPoint();

		struct WiFiNetwork {
			String ssid;
			int32_t rssi;
			uint8_t encryptionType;
		};

		std::vector<WiFiNetwork> scanWiFiNetworks();
		void sortNetworksBySignalStrength(std::vector<WiFiNetwork>& networks);
		void printNetworkList(const std::vector<WiFiNetwork>& networks);
		int getSelectedNetworkIndex() const { return _selectedNetworkIndex; }
		void setSelectedNetworkIndex(int index);
		const std::vector<WiFiNetwork>& getAvailableNetworks() const { return _availableNetworks; }

	private:
		WiFiManager();
		void initializeWiFi();

		static void onWiFiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		static void onIpEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
		esp_event_handler_instance_t wifi_event_instance;
		esp_event_handler_instance_t ip_event_instance;

		std::vector<WiFiNetwork> _availableNetworks;
		int _selectedNetworkIndex = 0;
};

extern Preferences preferences;
