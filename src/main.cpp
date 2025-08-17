#include "utils/config.h"
#include "utils/task_manager.h"
#include "utils/hold_to_wake.h"
#include "actuators/led/led_animations.h"

void setup() {
    Serial.setRxBufferSize(MAX_PROGRAM_SIZE); // This is here to make the serial buffer larger to accommodate for large serial messages (ie. when uploading bytecode programs over serial)
    Serial.setTxBufferSize(MAX_PROGRAM_SIZE); // This is here to make the serial buffer larger to accommodate for large serial messages (ie. when uploading bytecode programs over serial)
    Serial.begin(115200);
    pinMode(PWR_EN, OUTPUT);
    digitalWrite(PWR_EN, HIGH);
    
    // Initialize basic components first
    Wire.setPins(I2C_SDA_1, I2C_SCL_1);
    Wire.begin(I2C_SDA_1, I2C_SCL_1, I2C_CLOCK_SPEED);

    Wire1.setPins(I2C_SDA_2, I2C_SCL_2);
    Wire1.begin(I2C_SDA_2, I2C_SCL_2, I2C_CLOCK_SPEED);

    SerialQueueManager::getInstance().initialize();
    TaskManager::createSerialQueueTask();
    
    // For normal startup (not deep sleep wake), initialize display immediately
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
        TaskManager::createDisplayInitTask();
    }
    
    // Check hold-to-wake condition (handles display init for deep sleep wake)
    if (!holdToWake()) {
        // Function handles going back to sleep if conditions aren't met
        return;
    }

    // YOUR DESIRED INITIALIZATION ORDER:
    
    // 1. Buttons
    TaskManager::createButtonTask();
    
    // 2. Display
    // - For normal startup: Display initialized immediately above
    // - For deep sleep wake: Display initialized after 1-second button hold in holdToWake()
    
    // 3. Speaker
    TaskManager::createSpeakerTask();
    
    // 4. SerialInput
    TaskManager::createSerialInputTask();
    
    // 5. BatteryMonitor
    TaskManager::createBatteryMonitorTask();
    
    // 6. MessageProcessor
    TaskManager::createMessageProcessorTask();
    
    // 7. Network (Management task creates Communication task)
    TaskManager::createNetworkManagementTask();
    
    // 8. Sensors (Init task creates Polling task)
    TaskManager::createSensorInitTask();
    
    // 9. LEDs (moved later in sequence)
    rgbLed.turn_all_leds_off(); // Still turn off LEDs early for safety
    TaskManager::createLedTask();
    
    // 10. BytecodeVM
    TaskManager::createBytecodeVMTask();
    
    // Note: StackMonitor can be enabled for debugging
    TaskManager::createStackMonitorTask();
}

// Main loop runs on Core 1
void loop() {
    // Main loop can remain mostly empty as tasks handle the work
    vTaskDelay(pdMS_TO_TICKS(1));
}
