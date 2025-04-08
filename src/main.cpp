#include <esp32-hal-timer.h>
#include "./utils/config.h"
#include "./sensors/sensors.h"
#include "./actuators/buttons.h"
#include "./actuators/speaker.h"
#include "./user_code/user_code.h"
#include "./utils/show_chip_info.h"
#include "./utils/sensor_loggers.h"
#include "./actuators/led/rgb_led.h"
#include "./sensors/encoder_manager.h"
#include "./networking/wifi_manager.h"
#include "./actuators/display_screen.h"
#include "./lab_demo/lab_demo_manager.h"
#include "./networking/websocket_manager.h"
#include "./networking/send_data_to_server.h"
#include "./wifi_selection/wifi_selection_manager.h"
#include "./wifi_selection/haptic_feedback_manager.h"

// Task to handle sensors and user code on Core 0
void SensorAndUserCodeTask(void * parameter) {
    disableCore0WDT();
    delay(10);
    // Initialize sensors on Core 0
    Serial.println("Initializing sensors on Core 0...");
    Sensors::getInstance();
    Buttons::getInstance().begin();  // Initialize the buttons
    setupButtonLoggers();
    // if (!DisplayScreen::getInstance().init(Wire)) {
    //     Serial.println("Display initialization failed");
    // }
    Serial.println("Sensors initialized on Core 0");

    enableCore0WDT();

    // Main sensor and user code loop
    for(;;) {
        rgbLed.update();
        if (!Sensors::getInstance().sensors_initialized) continue;
        Buttons::getInstance().update();  // Update button states
        // multizoneTofLogger();
        // imuLogger();
        // sideTofsLogger();
        // DisplayScreen::getInstance().update();
        LabDemoManager::getInstance().processPendingCommands();
        user_code();
        delay(5);
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
        HapticFeedbackManager::getInstance().update();
        if (WiFi.status() != WL_CONNECTED) {
            WifiSelectionManager::getInstance().processNetworkSelection();
        } else {
            // Other network operations can use internal timing
            WebSocketManager::getInstance().pollWebSocket();
            SendDataToServer::getInstance().sendSensorDataToServer();
        }

        // Short delay to yield to other tasks
        // Small delay to avoid overwhelming the websocket and allow IMU data to be processed
        delay(5);
    }
}

// Main setup runs on Core 1
void setup() {
    Serial.begin(115200);
    // Only needed if we need to see the setup serial logs:
    delay(2000);
    Wire.setPins(I2C_SDA, I2C_SCL);
    Wire.begin(I2C_SDA, I2C_SCL, I2C_CLOCK_SPEED);
    delay(10);

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
