#include "config.h"
#include "wifi_manager.h"
#include "esp32_api_client.h"
#include "webserver_manager.h"

ESP32ApiClient apiClient;

void setup() {
	Serial.begin(115200);
	delay(3000); // Ensure time for Serial Monitor to connect

	// Setup onboard LED
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW); // Turn off LED initially

    wifiManager.initializeWiFi();

    // Try connecting to stored WiFi credentials
    if (!wifiManager.connectToStoredWiFi()) {
        // If WiFi connection fails, start Access Point for WiFi configuration
        wifiManager.startAccessPoint();
    } else {
        // If WiFi connection succeeds, initiate WebSocket connection
        apiClient.connectWebSocket();
    }
}

void loop() {
    // Handle DNS and Web Server requests (non-blocking)
    webserverManager.handleClientRequests();

    // Handle WebSocket events (if connected to WiFi)
    if (WiFi.status() == WL_CONNECTED) {
        apiClient.pollWebSocket();
    }
}
