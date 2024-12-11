#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/sensors.h"
#include "./include/user_code.h"
#include "./include/wifi_manager.h"
#include "./include/show_chip_info.h"
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

    Sensors::getInstance();

    // WiFiManager::getInstance(); // Initializes WiFi
    // WiFiManager::getInstance().connectToStoredWiFi();

    // // Create task for user code on Core 0
    // xTaskCreatePinnedToCore(
    //     UserCodeTask,  // Function to run
    //     "UserCode",    // Task name
    //     10000,         // Stack size
    //     NULL,          // Task parameters
    //     1,             // Priority
    //     NULL,          // Task handle
    //     0              // Run on Core 0
    // );
}

void tofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print every second
    
    if (millis() - lastPrintTime >= PRINT_INTERVAL) {
        auto& sensors = Sensors::getInstance();
        const VL53L5CX_ResultsData* leftData;
        const VL53L5CX_ResultsData* rightData;
    
        if (sensors.getTofData(&leftData, &rightData)) {
            // Print in 8x8 grid format
            const int imageWidth = 8;
            Serial.println("\nDistance readings (mm):");

            for (int y = 0; y <= imageWidth * (imageWidth - 1); y += imageWidth) {
                for (int x = imageWidth - 1; x >= 0; x--) {
                    Serial.print("\t");
                    Serial.print(leftData->distance_mm[x + y]);
                }
                Serial.println();
            }
        } else {
            Serial.println("Failed to get TOF data");
        }
        
        lastPrintTime = millis();
    }

    delay(5); // Small delay to prevent watchdog issues
}

//Main loop runs on Core 1
void loop() {
    // Network-related tasks on Core 1
    // WebServerManager::getInstance().handleClientRequests();

    // if (WiFi.status() == WL_CONNECTED) {
    //     WebSocketManager::getInstance().pollWebSocket();
    // }
    tofLogger();
}
