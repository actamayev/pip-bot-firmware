#include "config.h"
#include "wifi_manager.h"
#include "esp32_api_client.h"
#include "webserver_manager.h"

void setup() {
	Serial.begin(115200);
	delay(3000); // Ensure time for Serial Monitor to connect

	// Setup onboard LED
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW); // Turn off LED initially
    pinMode(DIGITAL_IR_PIN_3, INPUT);  // Set GPIO 34 as input
    // pinMode(DIGITAL_IR_PIN_1, INPUT);  // Set GPIO 32 as input

    // wifiManager.initializeWiFi();

    // // Try connecting to stored WiFi credentials
    // if (!wifiManager.connectToStoredWiFi()) {
    //     // If WiFi connection fails, start Access Point for WiFi configuration
    //     wifiManager.startAccessPoint();
    // } else {
    //     // If WiFi connection succeeds, initiate WebSocket connection
    //     apiClient.connectWebSocket();
    // }
}

void loop() {
    // Handle DNS and Web Server requests (non-blocking)
    // webServerManager.handleClientRequests();

    // // Handle WebSocket events (if connected to WiFi)
    // if (WiFi.status() == WL_CONNECTED) {
    //     apiClient.pollWebSocket();
    // }
    int sensorValue = digitalRead(DIGITAL_IR_PIN_3);

    if (sensorValue == LOW) {
        Serial.println("No object detected");
    } else {
        Serial.println("Object detected");
    }

    delay(500);  // Delay to slow down the readings for easier monitoring
}
