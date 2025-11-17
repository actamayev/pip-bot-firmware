#pragma once

#include <WiFi.h>

#include <algorithm>
#include <vector>

#include "actuators/led/rgb_led.h"
#include "esp_wifi.h"
#include "sensors/encoder_manager.h"
#include "serial_queue_manager.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "utils/structs.h"
#include "websocket_manager.h"

class WiFiManager : public Singleton<WiFiManager> {
    friend class Singleton<WiFiManager>;

  public:
    void print_network_list(const std::vector<WiFiNetworkInfo>& networks);
    int get_selected_network_index() const {
        return _selectedNetworkIndex;
    }
    void set_selected_network_index(int index);
    const std::vector<WiFiNetworkInfo>& get_available_networks() const {
        return _availableNetworks;
    }
    bool has_available_networks() const {
        return !_availableNetworks.empty();
    }
    uint32_t get_last_scan_complete_time() const {
        return _lastScanCompleteTime;
    }
    void clear_available_networks() {
        _availableNetworks.clear();
    }
    void clear_networks_if_stale();

    void store_wifi_credentials(const String& ssid, const String& password, int index);
    void check_and_reconnect_wifi();

    struct WiFiTestResult {
        bool wifiConnected;
        bool websocketConnected;
    };

    void start_wifi_credential_test(const String& ssid, const String& password);
    void process_wifi_credential_test();
    std::vector<WiFiCredentials> get_saved_networks_for_response();
    bool start_async_scan();
    void check_async_scan_progress();
    bool is_async_scan_in_progress() const {
        return _asyncScanInProgress;
    }
    bool is_connected_to_ssid(const String& ssid) const;

  private:
    WiFiManager();

    void connect_to_stored_wifi();
    bool attempt_new_wifi_connection(WiFiCredentials wifiCredentials);

    // std::vector<WiFiNetworkInfo> scanWiFiNetworkInfos();
    void sort_networks_by_signal_strength(std::vector<WiFiNetworkInfo>& networks);

    std::vector<WiFiNetworkInfo> _availableNetworks;
    int _selectedNetworkIndex = 0;

    bool attempt_direct_connection_to_saved_networks();
    uint32_t _lastReconnectAttempt = 0;
    bool _isConnecting = false;
    static constexpr uint32_t WIFI_RECONNECT_TIMEOUT = 3000; // 3 second timeout

    const uint32_t CONNECT_TO_SINGLE_NETWORK_TIMEOUT = 5000; // 5-second timeout

    const uint32_t printInterval = 100; // Print dots every 100ms
    const uint32_t checkInterval = 500; // Check serial every 500ms

    bool test_connection_only(const String& ssid, const String& password);

    bool _isTestingCredentials = false;
    String _testSSID = "";
    String _testPassword = "";
    bool _asyncScanInProgress = false;
    uint32_t _asyncScanStartTime = 0;
    static constexpr uint32_t ASYNC_SCAN_TIMEOUT_MS = 10000;    // 10 seconds
    static constexpr uint32_t ASYNC_SCAN_MIN_CHECK_DELAY = 500; // Don't check status for first 500ms
    uint32_t _lastScanCompleteTime = 0;
    static constexpr uint32_t STALE_SCAN_TIMEOUT_MS = 1800000; // 30 minutes
};
