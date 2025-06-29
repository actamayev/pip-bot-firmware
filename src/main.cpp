#include <esp32-hal-timer.h>
#include "utils/config.h"
#include "actuators/buttons.h"
#include "actuators/speaker.h"
#include "utils/task_manager.h"
#include "utils/hold_to_wake.h"
#include "utils/sensor_loggers.h"
#include "utils/show_chip_info.h"
#include "utils/timeout_manager.h"
#include "sensors/encoder_manager.h"
#include "networking/wifi_manager.h"
#include "actuators/display_screen.h"
#include "sensors/sensor_initializer.h"
#include "networking/message_processor.h"
#include "networking/websocket_manager.h"
#include "actuators/led/led_animations.h"
#include "networking/send_data_to_server.h"
#include "custom_interpreter/bytecode_vm.h"
#include "networking/network_state_mangager.h"
#include "networking/firmware_version_tracker.h"
#include "wifi_selection/wifi_selection_manager.h"
#include "wifi_selection/haptic_feedback_manager.h"

void setup() {
    Serial.setRxBufferSize(MAX_PROGRAM_SIZE); // This is here to make the serial buffer larger to accommodate for large serial messages (ie. when uploading bytecode programs over serial)
    Serial.setTxBufferSize(MAX_PROGRAM_SIZE); // This is here to make the serial buffer larger to accommodate for large serial messages (ie. when uploading bytecode programs over serial)
    Serial.begin(115200);
    // I2C setup
    Wire.setPins(I2C_SDA, I2C_SCL);
    Wire.begin(I2C_SDA, I2C_SCL, I2C_CLOCK_SPEED);

    SerialQueueManager::getInstance().initialize();
    TaskManager::createSerialQueueTask();

    rgbLed.turn_led_off(); // Start with LEDs off
    TaskManager::createLedTask(); // Start LED task so updates work

    // Check hold-to-wake condition BEFORE any other initialization
    if (!holdToWake()) {
        // Function handles going back to sleep if conditions aren't met
        // This return should never be reached, but added for completeness
        return;
    }

    TaskManager::createButtonTask();
    // Create remaining tasks
    TaskManager::createSerialInputTask(); // Internal function
    TaskManager::createMessageProcessorTask();  // Core 0 - motor control
    TaskManager::createBytecodeVMTask();        // Core 1 - user programs
    TaskManager::createSensorInitTask();  // Only create init task at startup
    TaskManager::createNetworkManagementTask();    // External function
    TaskManager::createStackMonitorTask();   // Enable in debug builds
}

// Main loop runs on Core 1
void loop() {
    // Main loop can remain mostly empty as tasks handle the work
    vTaskDelay(pdMS_TO_TICKS(1));
}
