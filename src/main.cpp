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

	const bool isConnectedToWifi = wifiManager.connectToStoredWiFi();
	if (isConnectedToWifi == false) {
		wifiManager.startAccessPoint();
	} else {
		websocketManager.connectToWebSocket();
	}
}

void loop() {
	// Periodically handle DNS and web server requests (non-blocking)
	webserverManager.handleClientRequests();

	// Periodically check WebSocket connection
	websocketManager.pollWebSocket();

	// Check if Wi-Fi is still connected
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("Lost Wi-Fi connection. Reconnecting...");
		digitalWrite(LED_PIN, LOW);  // Turn off LED to indicate failure
		wifiManager.reconnectWiFi();
		delay(1000);
	}

	const bool wifiStatus = WiFi.status();
	Serial.println("wifiStatus" + wifiStatus);
	// Check if WebSocket is still connected, if not, try to reconnect
	if (!wsClient.available() && wifiStatus == WL_CONNECTED) {
		Serial.println("WebSocket connection lost. Reconnecting...");
		websocketManager.connectToWebSocket();
	}

	delay(500);  // Short delay to prevent tight looping
}
