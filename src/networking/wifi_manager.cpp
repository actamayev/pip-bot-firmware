#include "wifi_manager.h"

WiFiManager::WiFiManager() {
    // Hard-coding Wifi creds during initialization
    // store_wifi_credentials("MSTest", "!haftr2024!", 0);
    // store_wifi_credentials("Another Dimension", "Iforgotit123", 1);
    // store_wifi_credentials("NETGEAR08", "breezyshoe123", 1);
    // store_wifi_credentials("iPhone", "12345678", 1);

    WiFi.setSleep(false);                // Disable WiFi sleep mode
    esp_wifi_set_ps(WIFI_PS_NONE);       // Disable power saving completely
    WiFi.setTxPower(WIFI_POWER_19_5dBm); // Your existing line
    connect_to_stored_wifi();

    // TODO: Consider implementing auto reconnect for when we lose wifi connection (need to be careful to not autoreconnect to networks that we
    // failed) WiFi.setAutoConnect(false);              // Disable auto-connect (use your custom logic) WiFi.setAutoReconnect(true);             //
    // Enable automatic reconnection
}

void WiFiManager::connect_to_stored_wifi() {
    // Try to connect directly to any saved network without scanning
    bool connectionStatus = attempt_direct_connection_to_saved_networks();

    if (!connectionStatus) {
        start_async_scan();
    } else {
        WebSocketManager::get_instance().connect_to_websocket();
    }
}

// 4/9/25 TODO: Connect to the network we've most recently connected to first.
bool WiFiManager::attempt_direct_connection_to_saved_networks() {
    // Quick check if any networks exist before trying to get them
    if (!PreferencesManager::get_instance().has_stored_wifi_networks()) {
        SerialQueueManager::get_instance().queue_message("No saved networks found");
        return false;
    }

    // Get all saved networks
    std::vector<WiFiCredentials> savedNetworks = PreferencesManager::get_instance().get_all_stored_wifi_networks();

    SerialQueueManager::get_instance().queue_message("Attempting direct connection to saved networks...");

    // Try to connect to each saved network without scanning first
    for (const WiFiCredentials& network : savedNetworks) {
        // Attempt connection
        if (attempt_new_wifi_connection(network)) return true;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    SerialQueueManager::get_instance().queue_message("Failed to connect to any saved networks");
    return false;
}

bool WiFiManager::attempt_new_wifi_connection(WiFiCredentials wifiCredentials) {
    // Set WiFi mode to Station (client mode)
    _isConnecting = true;
    WiFi.mode(WIFI_STA);

    if (wifiCredentials.ssid.isEmpty()) {
        SerialQueueManager::get_instance().queue_message("No SSID supplied.");
        return false;
    }

    WiFi.begin(wifiCredentials.ssid, wifiCredentials.password);

    uint32_t startAttemptTime = millis();
    uint32_t lastPrintTime = startAttemptTime;
    uint32_t lastCheckTime = startAttemptTime;

    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < CONNECT_TO_SINGLE_NETWORK_TIMEOUT)) {
        // Give other tasks time to run - CRITICAL for sensor performance
        vTaskDelay(pdMS_TO_TICKS(250)); // 250ms delay reduces sensor impact

        // Non-blocking print of dots
        uint32_t current_time = millis();
        if (current_time - lastPrintTime >= printInterval) {
            lastPrintTime = current_time;
            yield(); // Allow the ESP32 to handle background tasks
        }
        if (current_time - lastCheckTime >= checkInterval) {
            lastCheckTime = current_time;

            // Poll serial to update connection status
            SerialManager::get_instance().poll_serial();
        }
    }

    _isConnecting = false;

    if (WiFi.status() == WL_CONNECTED) {
        store_wifi_credentials(wifiCredentials.ssid, wifiCredentials.password, 0);
        SerialQueueManager::get_instance().queue_message("Connected to Wi-Fi!");
        return true;
    } else {
        SerialQueueManager::get_instance().queue_message("Failed to connect to Wi-Fi.");
        return false;
    }
}

void WiFiManager::sort_networks_by_signal_strength(std::vector<WiFiNetworkInfo>& networks) {
    // Sort networks by RSSI (higher values = stronger signal)
    std::sort(networks.begin(), networks.end(), [](const WiFiNetworkInfo& a, const WiFiNetworkInfo& b) { return a.rssi > b.rssi; });
}

void WiFiManager::print_network_list(const std::vector<WiFiNetworkInfo>& networks) {
    SerialQueueManager::get_instance().queue_message("Available WiFi Networks (sorted by signal strength):");
    SerialQueueManager::get_instance().queue_message("----------------------------------------------------");

    for (uint16_t i = 0; i < networks.size(); i++) {
        const auto& network = networks[i];
        String encryption = "";

        // Convert encryption type to readable format
        switch (network.encryptionType) {
            case WIFI_AUTH_OPEN:
                encryption = "Open";
                break;
            case WIFI_AUTH_WEP:
                encryption = "WEP";
                break;
            case WIFI_AUTH_WPA_PSK:
                encryption = "WPA";
                break;
            case WIFI_AUTH_WPA2_PSK:
                encryption = "WPA2";
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                encryption = "WPA/WPA2";
                break;
            default:
                encryption = "Unknown";
                break;
        }
    }
    SerialQueueManager::get_instance().queue_message("----------------------------------------------------");
}

void WiFiManager::set_selected_network_index(int index) {
    if (_availableNetworks.empty()) return;

    // Apply bounds checking
    if (index < 0) {
        _selectedNetworkIndex = 0;
    } else if (index >= (int)_availableNetworks.size()) {
        _selectedNetworkIndex = _availableNetworks.size() - 1;
    } else {
        _selectedNetworkIndex = index;
    }
}

// For storing WiFi credentials:
void WiFiManager::store_wifi_credentials(const String& ssid, const String& password, int index) {
    PreferencesManager::get_instance().store_wifi_credentials(ssid, password, index);
}

void WiFiManager::check_and_reconnect_wifi() {
    // Check if WiFi is connected or already attempting connection
    if (WiFi.status() == WL_CONNECTED || _isConnecting) return;

    // Quick check if any networks exist before trying to get them
    if (!PreferencesManager::get_instance().has_stored_wifi_networks()) return;

    uint32_t current_time = millis();

    if (current_time - _lastReconnectAttempt < WIFI_RECONNECT_TIMEOUT) return;
    SerialQueueManager::get_instance().queue_message("WiFi disconnected, attempting to reconnect...");

    // Update last reconnect attempt time
    _lastReconnectAttempt = current_time;

    // Set connecting flag
    _isConnecting = true;

    // Try to reconnect to stored WiFi
    bool connectionStatus = attempt_direct_connection_to_saved_networks();

    // Reset connecting flag
    _isConnecting = false;

    if (connectionStatus) {
        WebSocketManager::get_instance().connect_to_websocket();
    }
}

void WiFiManager::start_wifi_credential_test(const String& ssid, const String& password) {
    _testSSID = ssid;
    _testPassword = password;
    _isTestingCredentials = true;
}

void WiFiManager::process_wifi_credential_test() {
    if (!_isTestingCredentials) return;

    _isTestingCredentials = false;

    // Directly attempt connection without scanning
    if (!test_connection_only(_testSSID, _testPassword)) {
        WiFi.setAutoReconnect(false);
        WiFi.disconnect(true);
        SerialManager::get_instance().send_json_message(ToSerialMessage::WIFI_CONNECTION_RESULT, "failed");
    } else {
        SerialQueueManager::get_instance().queue_message("WiFi connection successful - testing WebSocket...");

        // Test WebSocket connection
        WebSocketManager::get_instance().connect_to_websocket();

        uint32_t startTime = millis();
        const uint32_t WEBSOCKET_TIMEOUT = 10000;
        bool websocketConnected = false;

        while (millis() - startTime < WEBSOCKET_TIMEOUT) {
            WebSocketManager::get_instance().poll_websocket();
            if (WebSocketManager::get_instance().is_ws_connected()) {
                websocketConnected = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (websocketConnected) {
            store_wifi_credentials(_testSSID, _testPassword, 0);
            SerialManager::get_instance().send_json_message(ToSerialMessage::WIFI_CONNECTION_RESULT, "success");
        } else {
            WiFi.setAutoReconnect(false);
            WiFi.disconnect(true);
            SerialManager::get_instance().send_json_message(ToSerialMessage::WIFI_CONNECTION_RESULT, "wifi_only");
        }
    }

    // Clear test credentials
    _testSSID = "";
    _testPassword = "";
}

bool WiFiManager::test_connection_only(const String& ssid, const String& password) {
    SerialQueueManager::get_instance().queue_message("Testing connection to: " + ssid);

    // Disable auto-reconnect before starting the test
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint32_t startTime = millis();
    uint32_t lastStatusLog = 0;

    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < CONNECT_TO_SINGLE_NETWORK_TIMEOUT)) {
        // Log WiFi status every second for debugging
        if (millis() - lastStatusLog > 1000) {
            String statusStr = "";
            switch (WiFi.status()) {
                case WL_IDLE_STATUS:
                    statusStr = "IDLE";
                    break;
                case WL_NO_SSID_AVAIL:
                    statusStr = "NO_SSID_AVAIL";
                    break;
                case WL_SCAN_COMPLETED:
                    statusStr = "SCAN_COMPLETED";
                    break;
                case WL_CONNECTED:
                    statusStr = "CONNECTED";
                    break;
                case WL_CONNECT_FAILED:
                    statusStr = "CONNECT_FAILED";
                    break;
                case WL_CONNECTION_LOST:
                    statusStr = "CONNECTION_LOST";
                    break;
                case WL_DISCONNECTED:
                    statusStr = "DISCONNECTED";
                    break;
                default:
                    statusStr = "UNKNOWN(" + String(WiFi.status()) + ")";
                    break;
            }
            SerialQueueManager::get_instance().queue_message("WiFi Status: " + statusStr);
            lastStatusLog = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // Increased delay - less aggressive polling for better sensor performance
    }

    bool connected = WiFi.status() == WL_CONNECTED;

    if (connected) {
        SerialQueueManager::get_instance().queue_message("✓ WiFi connection successful!");
        SerialQueueManager::get_instance().queue_message("IP Address: " + WiFi.localIP().toString());
    } else {
        WiFi.disconnect(true);
        SerialQueueManager::get_instance().queue_message("✗ WiFi connection failed after " + String(CONNECT_TO_SINGLE_NETWORK_TIMEOUT) + "ms");
    }

    return connected;
}

std::vector<WiFiCredentials> WiFiManager::get_saved_networks_for_response() {
    // Check if any networks exist
    if (!PreferencesManager::get_instance().has_stored_wifi_networks()) {
        SerialQueueManager::get_instance().queue_message("No saved networks found");
        return std::vector<WiFiCredentials>(); // Return empty vector
    }

    // Get all saved networks from preferences
    std::vector<WiFiCredentials> savedNetworks = PreferencesManager::get_instance().get_all_stored_wifi_networks();

    SerialQueueManager::get_instance().queue_message("Found " + String(savedNetworks.size()) + " saved networks");

    return savedNetworks;
}

bool WiFiManager::start_async_scan() {
    // Ignore if scan already in progress
    if (_asyncScanInProgress) {
        SerialQueueManager::get_instance().queue_message("Scan already in progress, ignoring request");
        return false;
    }

    SerialQueueManager::get_instance().queue_message("Starting async WiFi scan...");

    // Clear any previous scan results
    _availableNetworks.clear();
    _selectedNetworkIndex = 0;

    // Prepare WiFi for scanning
    WiFi.disconnect(true);
    WiFi.scanDelete();
    vTaskDelay(pdMS_TO_TICKS(100));
    WiFi.mode(WIFI_STA);

    // Start async scan
    int16_t result = WiFi.scanNetworks(true); // true = async

    if (result == WIFI_SCAN_RUNNING) {
        _asyncScanInProgress = true;
        _asyncScanStartTime = millis();
        SerialQueueManager::get_instance().queue_message("Async scan initiated successfully");

        // Send scan started message to browser
        SerialManager::get_instance().send_scan_started_message();

        return true;
    } else {
        SerialQueueManager::get_instance().queue_message("Failed to start async scan");
        rgbLed.turn_main_board_leds_off();
        return false;
    }
}

void WiFiManager::check_async_scan_progress() {
    if (!_asyncScanInProgress) return; // No scan in progress

    uint32_t current_time = millis();
    uint32_t scanDuration = current_time - _asyncScanStartTime;

    // Don't check status too soon - give the scan time to actually start
    if (scanDuration < ASYNC_SCAN_MIN_CHECK_DELAY) return; // Too early to check

    // Check for timeout
    if (scanDuration > ASYNC_SCAN_TIMEOUT_MS) {
        SerialQueueManager::get_instance().queue_message("Async WiFi scan timed out after " + String(scanDuration) + "ms");
        _asyncScanInProgress = false;
        rgbLed.turn_main_board_leds_off();

        // Clean up any scan results
        WiFi.scanDelete();

        // Send empty scan complete message to browser to indicate timeout
        std::vector<WiFiNetworkInfo> emptyNetworks;
        SerialManager::get_instance().send_scan_results_response(emptyNetworks);
        return;
    }

    // Check scan status
    int16_t scanResult = WiFi.scanComplete();

    // For WIFI_SCAN_RUNNING (-1) and WIFI_SCAN_FAILED (-2), just keep waiting until timeout
    // The ESP32 WiFi library seems to return WIFI_SCAN_FAILED sometimes even when scan is progressing
    if (scanResult < 0) return;
    // Only handle completion (positive numbers) - ignore WIFI_SCAN_FAILED and WIFI_SCAN_RUNNING
    // Scan completed successfully
    SerialQueueManager::get_instance().queue_message("Async WiFi scan completed in " + String(scanDuration) + "ms. Found " + String(scanResult) +
                                                     " networks");
    _asyncScanInProgress = false;
    rgbLed.turn_main_board_leds_off();
    _lastScanCompleteTime = millis();
    // Process scan results
    std::vector<WiFiNetworkInfo> networks;

    for (int i = 0; i < scanResult; i++) {
        WiFiNetworkInfo network;
        network.ssid = WiFi.SSID(i);
        network.rssi = WiFi.RSSI(i);
        network.encryptionType = WiFi.encryptionType(i);
        networks.push_back(network);
    }

    // Sort networks by signal strength
    sort_networks_by_signal_strength(networks);

    // Update class members
    _availableNetworks = networks;
    _selectedNetworkIndex = 0;

    // Clean up scan results to free memory
    WiFi.scanDelete();

    // Send results to browser
    SerialManager::get_instance().send_scan_results_response(networks);
}

void WiFiManager::clear_networks_if_stale() {
    uint32_t now = millis();
    if (_availableNetworks.empty() || (now - _lastScanCompleteTime <= STALE_SCAN_TIMEOUT_MS)) return;
    _availableNetworks.clear();
    SerialQueueManager::get_instance().queue_message("WiFi scan results cleared (stale > 30 min)");
}

bool WiFiManager::is_connected_to_ssid(const String& ssid) const {
    return WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid;
}
