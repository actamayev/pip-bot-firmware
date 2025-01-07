#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/sensors.h"
#include "./include/user_code.h"
#include "./include/wifi_manager.h"
#include "./include/show_chip_info.h"
#include "./include/sensor_loggers.h"
#include "./include/webserver_manager.h"
#include "./include/websocket_manager.h"

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

    Serial.println(DEFAULT_ENVIRONMENT);
    Serial.println(DEFAULT_PIP_ID);
    rgbLed.turn_led_off();

    WiFiManager::getInstance(); // Initializes WiFi
    WiFiManager::getInstance().connectToStoredWiFi();

    // Sensors::getInstance();

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

//Main loop runs on Core 1
unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 200;  // 200ms interval

void loop() {
    unsigned long currentTime = millis();
    
    // Run these tasks every CHECK_INTERVAL milliseconds
    if (currentTime - lastCheckTime >= CHECK_INTERVAL) {
        // Network-related tasks on Core 1
        WebServerManager::getInstance().handleClientRequests();

        if (WiFi.status() == WL_CONNECTED) {
            WebSocketManager::getInstance().pollWebSocket();
        }
        // tofLogger();
        // imuLogger();
        // irLogger();

        lastCheckTime = currentTime;
    }

    yield();  // Allow system background tasks to run
}
