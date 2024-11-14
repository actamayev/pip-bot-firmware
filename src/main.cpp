#include "./include/config.h"
#include "./include/wifi_manager.h"
#include "./include/sensor_setup.h"
#include "./include/esp32_api_client.h"
#include "./include/webserver_manager.h"
#include "./include/user_code.h"

void setup() {
	Serial.begin(115200);
    delay(2000);

    // Force creation of apiClient
    apiClient = new ESP32ApiClient();

    sensorSetup.sensor_setup();

    wifiManager.initializeWiFi();

    wifiManager.connectToStoredWiFi();
}

void loop() {
    // Handle DNS and Web Server requests (non-blocking)
    webServerManager.handleClientRequests();

    // Handle WebSocket events (if connected to WiFi)
    if (WiFi.status() == WL_CONNECTED) {
        apiClient->pollWebSocket();
    }
    user_code();
}
