#include "utils/config.h"
#include "utils/task_manager.h"
#include "utils/hold_to_wake.h"
#include "actuators/buttons.h"
#include "sensors/battery_monitor.h"
#include "sensors/sensor_initializer.h"
#include "actuators/display_screen.h"

void setup() {
    Serial.setRxBufferSize(MAX_PROGRAM_SIZE); // This is here to make the serial buffer larger to accommodate for large serial messages (ie. when uploading bytecode programs over serial)
    Serial.setTxBufferSize(MAX_PROGRAM_SIZE); // This is here to make the serial buffer larger to accommodate for large serial messages (ie. when uploading bytecode programs over serial)
    Serial.begin(115200);
    pinMode(PWR_EN, OUTPUT);
    digitalWrite(PWR_EN, HIGH);

    // Init the Battery monitor I2C line first
    Wire1.begin(I2C_SDA_2, I2C_SCL_2, I2C_2_CLOCK_SPEED);

    TaskManager::createSerialQueueTask();
    
    // Initialize battery monitor SYNCHRONOUSLY before display to check battery level
    BatteryMonitor::getInstance().initialize();

    // Initialize
    Wire.begin(I2C_SDA_1, I2C_SCL_1, I2C_1_CLOCK_SPEED);

    // Check hold-to-wake condition first (handles display init for deep sleep wake)
    // Function handles going back to sleep if conditions aren't met
    if (!holdToWake()) return;

    // Handle low battery condition BEFORE normal display initialization
    // if (BatteryMonitor::getInstance().isCriticalBattery()) {
    //     // Initialize display directly without showing startup screen
    //     DisplayScreen::getInstance().init(false);
    //     DisplayScreen::getInstance().showLowBatteryScreen();
    //     vTaskDelay(pdMS_TO_TICKS(3000)); // Show low battery message for 3 seconds
    //     Buttons::getInstance().enterDeepSleep();
    //     return; // Should never reach here, but just in case
    // }
    
    // Initialize display normally after hold-to-wake check (only if battery is OK)
    // if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
    //     TaskManager::createDisplayInitTask();
    // } else {
    //     // For deep sleep wake, display was already initialized in holdToWake()
    // }

    // INITIALIZATION ORDER:
    // 1. Display
    // - For normal startup: Display initialized immediately above
    // - For deep sleep wake: Display initialized after 1-second button hold in holdToWake()
    
    // 3. SerialInput: To show user we're connected (in the UI)
    TaskManager::createSerialInputTask();
    
    // 4. Speaker: For the startup sound
    TaskManager::createSpeakerTask();

    // 5. Buttons: We have this later the procedure because the user doesn't need button access during startup
    TaskManager::createButtonTask();

    // Create battery monitor task for ongoing monitoring
    TaskManager::createBatteryMonitorTask();
    
    // 11. Motor (high priority, fast updates)
    TaskManager::createMotorTask();

    // 7. Network (Management task creates Communication task)
    TaskManager::createNetworkManagementTask();
    
    // 8. LEDs (moved later in sequence)
    rgbLed.turn_all_leds_off(); // Still turn off LEDs early for safety
    TaskManager::createLedTask();
    
    // 9. Initialize all sensors centrally to avoid I2C conflicts
    SensorInitializer::getInstance(); // This triggers centralized initialization
    
    // 10. Individual Sensor Tasks (wait for centralized init, then poll independently)
    // TaskManager::createImuSensorTask();
    // TaskManager::createEncoderSensorTask();
    // TaskManager::createMultizoneTofSensorTask();
    // TaskManager::createSideTofSensorTask();
    TaskManager::createColorSensorTask();
    // TaskManager::createIrSensorTask();
    
    // 12. DemoManager (high priority for demos)
    TaskManager::createDemoManagerTask();

    // 10. BytecodeVM
    TaskManager::createBytecodeVMTask();
    
    // Note: StackMonitor can be enabled for debugging
    // TaskManager::createStackMonitorTask();
    // TaskManager::createSensorLoggerTask();
}

// Main loop runs on Core 1
void loop() {
    // Main loop can remain mostly empty as tasks handle the work
    vTaskDelay(pdMS_TO_TICKS(1));
}
