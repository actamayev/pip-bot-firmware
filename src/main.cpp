#include <esp32-hal-timer.h>
#include "utils/config.h"
#include "actuators/buttons.h"
#include "actuators/speaker.h"
#include "utils/show_chip_info.h"
#include "utils/sensor_loggers.h"
#include "sensors/encoder_manager.h"
#include "networking/wifi_manager.h"
#include "actuators/display_screen.h"
#include "networking/serial_manager.h"
#include "sensors/sensor_initializer.h"
#include "networking/message_processor.h"
#include "networking/websocket_manager.h"
#include "actuators/led/led_animations.h"
#include "networking/send_data_to_server.h"
#include "sensors/sensor_polling_manager.h"
#include "custom_interpreter/bytecode_vm.h"
#include "networking/network_state_mangager.h"
#include "networking/firmware_version_tracker.h"
#include "wifi_selection/wifi_selection_manager.h"
#include "wifi_selection/haptic_feedback_manager.h"

// Task to handle sensors and bytecode on Core 0
void SensorAndBytecodeTask(void * parameter) {
    disableCore0WDT();
    vTaskDelay(pdMS_TO_TICKS(10));
    // Initialize sensors on Core 0
    SerialQueueManager::getInstance().queueMessage("Initializing sensors on Core 0...");

    Buttons::getInstance();
    setupButtonLoggers();
    // if (!DisplayScreen::getInstance().init()) {
    //     SerialQueueManager::getInstance().queueMessage("Display initialization failed");
    // }

    SensorInitializer& initializer = SensorInitializer::getInstance();

    // First attempt to initialize each sensor before entering the main loop
    if (!initializer.isSensorInitialized(SensorInitializer::IMU)) {
        SerialQueueManager::getInstance().queueMessage("Trying to init imu...");
        initializer.tryInitializeIMU();
    }
    
    if (!initializer.isSensorInitialized(SensorInitializer::MULTIZONE_TOF)) {
        SerialQueueManager::getInstance().queueMessage("Trying to init MZ...");
        initializer.tryInitializeMultizoneTof();
    }
    
    if (!initializer.isSensorInitialized(SensorInitializer::LEFT_SIDE_TOF)) {
        SerialQueueManager::getInstance().queueMessage("Trying to init Left Tof...");
        initializer.tryInitializeLeftSideTof();
    }
    
    if (!initializer.isSensorInitialized(SensorInitializer::RIGHT_SIDE_TOF)) {
        SerialQueueManager::getInstance().queueMessage("Trying to init Right Tof...");
        initializer.tryInitializeRightSideTof();
    }
    
    SerialQueueManager::getInstance().queueMessage("Sensors initialized on Core 0");
    enableCore0WDT();

    // Only check if all sensors are initialized, but don't try to initialize here
    if (!initializer.areAllSensorsInitialized()) {
        while (1);
    }
    SensorPollingManager::getInstance().startPolling();
    // Main sensor and bytecode loop
    for(;;) {
        ledAnimations.update();
        Buttons::getInstance().update();  // Update button states
        SerialManager::getInstance().pollSerial();
        BytecodeVM::getInstance().update();
        MessageProcessor::getInstance().processPendingCommands();
        SensorPollingManager::getInstance().update();
        // DisplayScreen::getInstance().update();
        // multizoneTofLogger();
        // imuLogger();
        // sideTofsLogger();
        vTaskDelay(pdMS_TO_TICKS(5));  // Use proper FreeRTOS delay
    }
}

// Task to handle WiFi and networking on Core 1
void NetworkTask(void * parameter) {
    // Initialize WiFi and networking components
    SerialQueueManager::getInstance().queueMessage("Initializing WiFi on Core 1...");
    WiFiManager::getInstance();
    SerialQueueManager::getInstance().queueMessage("WiFi initialization complete on Core 1");
    FirmwareVersionTracker::getInstance();

    // Main network loop
    for(;;) {
        NetworkMode mode = NetworkStateManager::getInstance().getCurrentMode();

        switch (mode) {
            // TODO 5/1/25: Why isn't poll serial here?
            case NetworkMode::SERIAL_MODE:
                // When in serial mode, we don't do any WiFi operations
                // Could add specific serial mode indicators here
                break;

            case NetworkMode::ADD_PIP_MODE:
                // Process ADD_PIP_MODE WiFi testing
                WiFiManager::getInstance().processAddPipMode();
                break;

            case NetworkMode::WIFI_MODE:
                // WiFi connected mode - poll websocket and send data
                WebSocketManager::getInstance().pollWebSocket();
                SendDataToServer::getInstance().sendSensorDataToServer();
                break;

            case NetworkMode::NONE:
                // No connectivity - try to establish WiFi
                WiFiManager::getInstance().checkAndReconnectWiFi();
                HapticFeedbackManager::getInstance().update();
                WifiSelectionManager::getInstance().processNetworkSelection();
                break;
        }

        // Small delay to avoid overwhelming the websocket and allow IMU data to be processed
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// Main setup runs on Core 1
void setup() {
    Serial.setRxBufferSize(MAX_PROGRAM_SIZE); // This is here to make the serial buffer larger to accommodate for large serial messages (ie. when uploading bytecode programs over serial)
    Serial.setTxBufferSize(MAX_PROGRAM_SIZE); // This is here to make the serial buffer larger to accommodate for large serial messages (ie. when uploading bytecode programs over serial)
    Serial.begin(115200);
    // Only needed if we need to see the setup serial logs:
    // if (getEnvironment() == "local") {
    //     vTaskDelay(pdMS_TO_TICKS(2000));
    // }
    Wire.setPins(I2C_SDA, I2C_SCL);
    Wire.begin(I2C_SDA, I2C_SCL, I2C_CLOCK_SPEED);
    vTaskDelay(pdMS_TO_TICKS(10));
    SerialQueueManager::getInstance().initialize();

    rgbLed.turn_led_off();

    // Create tasks for parallel execution
    xTaskCreatePinnedToCore(
        SensorAndBytecodeTask,  // Sensor and bytecode task
        "SensorAndBytecode",    // Task name
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
    vTaskDelay(pdMS_TO_TICKS(1));
}
