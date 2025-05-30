#pragma once

#include <WiFi.h>
#include <vector>
#include <algorithm>
#include "utils/config.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "websocket_manager.h"
#include "actuators/led/rgb_led.h"
#include "sensors/encoder_manager.h"
#include "network_state_mangager.h"
#include "wifi_selection/wifi_selection_manager.h"

class WiFiManager : public Singleton<WiFiManager> {
    friend class Singleton<WiFiManager>;

	public:
		void printNetworkList(const std::vector<WiFiNetworkInfo>& networks);
		int getSelectedNetworkIndex() const { return _selectedNetworkIndex; }
		void setSelectedNetworkIndex(int index);
		const std::vector<WiFiNetworkInfo>& getAvailableNetworks() const { return _availableNetworks; }

		void storeWiFiCredentials(const String& ssid, const String& password, int index);
		void checkAndReconnectWiFi();

		struct WiFiTestResult {
			bool wifiConnected;
			bool websocketConnected;
		};
		
		WiFiTestResult testWiFiCredentials(const String& ssid, const String& password);

	private:
		WiFiManager();

		void connectToStoredWiFi();
		WiFiCredentials getStoredWiFiCredentials();
		bool attemptNewWifiConnection(WiFiCredentials wifiCredentials);

		std::vector<WiFiNetworkInfo> scanWiFiNetworkInfos();
		void sortNetworksBySignalStrength(std::vector<WiFiNetworkInfo>& networks);
		
		std::vector<WiFiNetworkInfo> _availableNetworks;
		int _selectedNetworkIndex = 0;

		bool attemptDirectConnectionToSavedNetworks();
		unsigned long _lastReconnectAttempt = 0;
		bool _isConnecting = false;
        static constexpr unsigned long WIFI_RECONNECT_TIMEOUT = 3000; // 3 second timeout

		const unsigned long CONNECT_TO_SINGLE_NETWORK_TIMEOUT = 5000;  // 5-second timeout

		const unsigned long printInterval = 100;  // Print dots every 100ms
		const unsigned long checkInterval = 500;  // Check serial every 500ms

		bool testConnectionOnly(const String& ssid, const String& password);
};
