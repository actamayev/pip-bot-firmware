#include "./include/config.h"
#include "./include/user_code.h"
#include "./include/wifi_manager.h"
#include "./include/sensor_setup.h"
#include "./include/show_chip_info.h"
#include "./include/esp32_api_client.h"
#include "./include/webserver_manager.h"

// This task will run user code on Core 0
void UserCodeTask(void * parameter) {
    for(;;) {
        user_code();  // Your LED blinking code
        delay(1);     // Small delay to prevent watchdog reset
    }
}

// Main setup runs on Core 1
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.printf("setup() running on Core %d\n", xPortGetCoreID());

    printFlashInfo();

    // Setup WiFi, sensors, etc.
    apiClient = new ESP32ApiClient();
    sensorSetup.sensor_setup();
    wifiManager.initializeWiFi();
    wifiManager.connectToStoredWiFi();

    // Create task for user code on Core 0
    xTaskCreatePinnedToCore(
        UserCodeTask,  // Function to run
        "UserCode",    // Task name
        10000,         // Stack size
        NULL,          // Task parameters
        1,             // Priority
        NULL,          // Task handle
        0              // Run on Core 0
    );
}

// Main loop runs on Core 1
void loop() {
    // Network-related tasks on Core 1
    webServerManager.handleClientRequests();
    
    if (WiFi.status() == WL_CONNECTED) {
        apiClient->pollWebSocket();
    }
}
