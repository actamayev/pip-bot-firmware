#include "config.h"
#include "wifi_manager.h"
#include "sensor_setup.h"
#include "esp32_api_client.h"
#include "webserver_manager.h"

void setup() {
	Serial.begin(115200);
    while (!Serial) delay(10);

    sensorSetup.sensor_setup();

    wifiManager.initializeWiFi();

    // // Try connecting to stored WiFi credentials
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
    webServerManager.handleClientRequests();

    // Handle WebSocket events (if connected to WiFi)
    if (WiFi.status() == WL_CONNECTED) {
        apiClient.pollWebSocket();
    }
}
