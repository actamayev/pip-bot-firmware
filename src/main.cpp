#include <esp32-hal-timer.h>
#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/sensors.h"
#include "./include/user_code.h"
#include "./include/wifi_manager.h"
#include "./include/show_chip_info.h"
#include "./include/sensor_loggers.h"
#include "./include/encoder_manager.h"
#include "./include/lab_demo_manager.h"
#include "./include/webserver_manager.h"
#include "./include/websocket_manager.h"
#include "./include/send_data_to_server.h"

// Task to handle sensors and user code on Core 0
void SensorAndUserCodeTask(void * parameter) {
    disableCore0WDT();
    
    // Initialize sensors on Core 0
    Serial.println("Initializing sensors on Core 0...");
    Sensors::getInstance();
    EncoderManager::getInstance();
    Serial.println("Sensors initialized on Core 0");

    enableCore0WDT();

    // Main sensor and user code loop
    for(;;) {
        // tofLogger();
        // leftTofLogger();
        // rightTofLogger();
        // imuLogger();
        user_code();
        delay(1);
    }
}

// Task to handle WiFi and networking on Core 1
void NetworkTask(void * parameter) {
    // Initialize WiFi and networking components
    Serial.println("Initializing WiFi on Core 1...");
    WiFiManager::getInstance();
    WiFiManager::getInstance().connectToStoredWiFi();
    Serial.println("WiFi initialization complete on Core 1");

    // Main network loop
    for(;;) {
        WebServerManager::getInstance().handleClientRequests();

        if (WiFi.status() == WL_CONNECTED) {
            WebSocketManager::getInstance().pollWebSocket();
            SendDataToServer::getInstance().sendSensorDataToServer();
        }

        delay(100); // Similar to the original CHECK_INTERVAL
    }
}

// Main setup runs on Core 1
void setup() {
    Serial.begin(115200);
    // Only needed if we need to see the setup serial logs:
    // delay(2000);

    rgbLed.turn_led_off();

    // Create tasks for parallel execution
    xTaskCreatePinnedToCore(
        SensorAndUserCodeTask,  // Sensor and user code task
        "SensorAndUserCode",    // Task name
        SENSOR_STACK_SIZE,      // Stack size
        NULL,                   // Task parameters
        1,                      // Priority
        NULL,                   // Task handle
        0                       // Run on Core 0
    );

    xTaskCreatePinnedToCore(
        NetworkTask,            // Network handling task
        "Network",              // Task name
        NETWORK_STACK_SIZE,      // Stack size
        NULL,                   // Task parameters
        1,                      // Priority
        NULL,                   // Task handle
        1                       // Run on Core 1
    );
}

// Main loop runs on Core 1
void loop() {
    // Main loop can remain mostly empty as tasks handle the work
    vTaskDelay(1);  // FreeRTOS way to yield to other tasks
}
