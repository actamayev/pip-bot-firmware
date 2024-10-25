#include "config.h"
#include "wifi_manager.h"
#include "websocket_manager.h"
#include "webserver_manager.h"

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
        websocketManager.connectToWebSocket();
    }
}

void loop() {
    // Handle DNS and Web Server requests (non-blocking)
    webserverManager.handleClientRequests();

    // Handle WebSocket events (if connected to WiFi)
    if (WiFi.status() == WL_CONNECTED) {
        websocketManager.pollWebSocket();
    }
}
