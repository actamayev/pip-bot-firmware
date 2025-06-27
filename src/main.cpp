#include <esp32-hal-timer.h>
#include "utils/config.h"
#include "actuators/buttons.h"
#include "actuators/speaker.h"
#include "utils/task_manager.h"
#include "utils/show_chip_info.h"
#include "utils/sensor_loggers.h"
#include "utils/timeout_manager.h"
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

// Function to check if we should proceed with full startup after deep sleep wake
bool checkHoldToWakeCondition() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    // Only apply hold-to-wake behavior if woken from deep sleep by button
    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0) {
        rgbLed.setDefaultColors(0, 0, MAX_LED_BRIGHTNESS);
        ledAnimations.startBreathing(2000, 0.0f);
        return true; // Normal startup for other wake reasons
    }
    
    SerialQueueManager::getInstance().queueMessage("Woke up from deep sleep due to button press. Checking hold duration...");
    
    // Configure button pin for reading
    pinMode(BUTTON_PIN_1, INPUT_PULLUP);
    
    // Check if button is still pressed (LOW due to INPUT_PULLUP)
    if (digitalRead(BUTTON_PIN_1) == HIGH) {
        SerialQueueManager::getInstance().queueMessage("Button already released. Going back to sleep...");
        Buttons::getInstance().enterDeepSleep();
        return false;
    }
    
    // Start timing - button must be held for 1000ms
    const uint32_t HOLD_DURATION_MS = 1000;
    uint32_t startTime = millis();
    
    while ((millis() - startTime) < HOLD_DURATION_MS) {
        if (digitalRead(BUTTON_PIN_1) == HIGH) {
            uint32_t heldTime = millis() - startTime;
            String message = "Button released after " + String(heldTime) + "ms. Going back to sleep...";
            SerialQueueManager::getInstance().queueMessage(message.c_str());
            Buttons::getInstance().enterDeepSleep();
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // WAKE-UP SUCCESSFUL - Reset button state so this press doesn't count toward shutdown
    SerialQueueManager::getInstance().queueMessage("Wake-up successful - clearing button state");
    rgbLed.setDefaultColors(0, 0, MAX_LED_BRIGHTNESS);
    ledAnimations.startBreathing(2000, 0.0f);

    // Wait for user to release the button before proceeding
    while (digitalRead(BUTTON_PIN_1) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    SerialQueueManager::getInstance().queueMessage("Button released - ready for normal operation");
    
    return true; // Proceed with full startup
}

// Task to handle sensors and bytecode on Core 0
void SensorAndBytecodeTask(void * parameter) {
    disableCore0WDT();
    vTaskDelay(pdMS_TO_TICKS(10));
    // Initialize sensors on Core 0
    SerialQueueManager::getInstance().queueMessage("Initializing sensors on Core 0...");

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
        // SerialManager::getInstance().pollSerial();
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
        WiFiManager::getInstance().checkAsyncScanProgress();
        TimeoutManager::getInstance().update();

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
    SerialQueueManager::getInstance().initialize();

    rgbLed.turn_led_off(); // Start with LEDs off
    TaskManager::createLedTask(); // Start LED task so updates work

    // Check hold-to-wake condition BEFORE any other initialization
    if (!checkHoldToWakeCondition()) {
        // Function handles going back to sleep if conditions aren't met
        // This return should never be reached, but added for completeness
        return;
    }

    // Clean task creation - all managed by TaskManager
    TaskManager::createButtonTask();

    // I2C setup
    Wire.setPins(I2C_SDA, I2C_SCL);
    Wire.begin(I2C_SDA, I2C_SCL, I2C_CLOCK_SPEED);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Create remaining tasks
    TaskManager::createSensorTask();     // External function
    TaskManager::createSerialInputTask(); // Internal function
    TaskManager::createNetworkTask();    // External function
}

// Main loop runs on Core 1
void loop() {
    // Main loop can remain mostly empty as tasks handle the work
    vTaskDelay(pdMS_TO_TICKS(1));
}
