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
    // I2C setup
    Wire.setPins(I2C_SDA_1, I2C_SCL_1);
    Wire.begin(I2C_SDA_1, I2C_SCL_1, I2C_CLOCK_SPEED);

    Wire1.setPins(I2C_SDA_2, I2C_SCL_2);
    Wire1.begin(I2C_SDA_2, I2C_SCL_2, I2C_CLOCK_SPEED);

    SerialQueueManager::getInstance().initialize();
    TaskManager::createSerialQueueTask();

    // Check hold-to-wake condition BEFORE any other initialization
    if (!holdToWake()) {
        // Function handles going back to sleep if conditions aren't met
        // This return should never be reached, but added for completeness
        return;
    }
    
    // 1. Buttons
    TaskManager::createButtonTask();
    
    // 2. Display (initialize display hardware first, then create task)
    TaskManager::createDisplayInitTask(); // New task to handle display init separately
    
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
    
    // 8. LEDs (moved later in sequence)
    rgbLed.turn_all_leds_off(); // Still turn off LEDs early for safety
    TaskManager::createLedTask();

    // 9. Sensors (Init task creates Polling task)
    TaskManager::createSensorInitTask();
    
    // 10. BytecodeVM
    TaskManager::createBytecodeVMTask();
    
    // Note: StackMonitor can be enabled for debugging
    // TaskManager::createStackMonitorTask();
}

// Main loop runs on Core 1
void loop() {
    // Main loop can remain mostly empty as tasks handle the work
    vTaskDelay(pdMS_TO_TICKS(1));
}
