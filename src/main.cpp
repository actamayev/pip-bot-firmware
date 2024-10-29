#include "config.h"
#include "wifi_manager.h"
#include "esp32_api_client.h"
#include "webserver_manager.h"
#include <Adafruit_VL53L1X.h>
#include <Wire.h>

Adafruit_VL53L1X vl53 = Adafruit_VL53L1X();

void resetSensor() {
  digitalWrite(TIME_OF_FLIGHT_XSHUT, LOW);
  delay(10);
  digitalWrite(TIME_OF_FLIGHT_XSHUT, HIGH);
  delay(10);
}

void setup() {
	Serial.begin(115200);
	// delay(3000); // Ensure time for Serial Monitor to connect
    while (!Serial) delay(10);

	// Setup onboard LED
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW); // Turn off LED initially

    // wifiManager.initializeWiFi();

    // // Try connecting to stored WiFi credentials
    // if (!wifiManager.connectToStoredWiFi()) {
    //     // If WiFi connection fails, start Access Point for WiFi configuration
    //     wifiManager.startAccessPoint();
    // } else {
    //     // If WiFi connection succeeds, initiate WebSocket connection
    //     apiClient.connectWebSocket();
    // }

    pinMode(TIME_OF_FLIGHT_XSHUT, OUTPUT);
    
    resetSensor();

    Wire.begin(TIME_OF_FLIGHT_SDA, TIME_OF_FLIGHT_SCL);
    
    Serial.println("Adafruit VL53L1X sensor test");

    if (!vl53.begin(0x29, &Wire)) {
        Serial.println("Failed to find VL53L1X chip");
        while (1) delay(10);
    }
    
    Serial.println("VL53L1X Found!");

    // Start continuous ranging
    vl53.startRanging();
}


void loop() {
    // Handle DNS and Web Server requests (non-blocking)
    // webServerManager.handleClientRequests();

    // // Handle WebSocket events (if connected to WiFi)
    // if (WiFi.status() == WL_CONNECTED) {
    //     apiClient.pollWebSocket();
    // }
    if (vl53.dataReady()) {
        // Get distance in millimeters
        int16_t distance = vl53.distance();

        if (distance == -1) {
            // Error case
            Serial.println("Error reading distance!");
        } else {
            // Print distance
            Serial.print("Distance: ");
            Serial.print(distance);
            Serial.println(" mm");

            // Example of using the distance for different ranges
            if (distance < 100) {
                Serial.println("Very close!");
            } else if (distance < 500) {
                Serial.println("Medium range");
            } else {
                Serial.println("Far away");
            }
        }

        // Clear the interrupt to be ready for next ranging
        vl53.clearInterrupt();
    }
    
    delay(500);  // Wait half second between measurements
}
