#include "actuators/buttons.h"
#include "actuators/display_screen.h"
#include "sensors/battery_monitor.h"
#include "sensors/sensor_initializer.h"
#include "utils/config.h"
#include "utils/hold_to_wake.h"
#include "utils/task_manager.h"

void setup() {
    // This is here to make the serial buffer larger to accommodate for large serial messages (ie. when uploading bytecode programs over serial)
    Serial.setRxBufferSize(MAX_PROGRAM_SIZE);
    // This is here to make the serial buffer larger to accommodate for large serial messages (ie. // when uploading bytecode programs over serial)
    Serial.setTxBufferSize(MAX_PROGRAM_SIZE);

    Serial.begin(115200);
    pinMode(PWR_EN, OUTPUT);
    digitalWrite(PWR_EN, HIGH);

    // Init the Battery monitor, IMU line I2C line first
    Wire1.begin(I2C_SDA_2, I2C_SCL_2, I2C_2_CLOCK_SPEED);

    TaskManager::create_serial_queue_task();

    // Initialize battery monitor SYNCHRONOUSLY before display to check battery level
    BatteryMonitor::get_instance().initialize();

    // Initialize
    Wire.begin(I2C_SDA_1, I2C_SCL_1, I2C_1_CLOCK_SPEED);

    // Check hold-to-wake condition first (handles display init for deep sleep wake)
    // Function handles going back to sleep if conditions aren't met
    if (!hold_to_wake()) {
        return;
    }

    // Handle low battery condition BEFORE normal display initialization
    if (BatteryMonitor::get_instance().is_critical_battery()) {
        // Initialize display directly without showing startup screen
        DisplayScreen::get_instance().init(false);
        DisplayScreen::get_instance().show_low_battery_screen();
        vTaskDelay(pdMS_TO_TICKS(3000)); // Show low battery message for 3 seconds
        Buttons::get_instance().enter_deep_sleep();
        return; // Should never reach here, but just in case
    }

    // Initialize display normally after hold-to-wake check (only if battery is OK)
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
        TaskManager::create_display_init_task();
    } else {
        // For deep sleep wake, display was already initialized in holdToWake()
    }

    // INITIALIZATION ORDER:
    // 1. Display
    // - For normal startup: Display initialized immediately above
    // - For deep sleep wake: Display initialized after 1-second button hold in holdToWake()

    // 3. SerialInput: To communicate with the user over Serial (in the BrowserUI)
    TaskManager::create_serial_input_task();

    // 4. Speaker
    TaskManager::create_speaker_task();

    // 5. Buttons: We have this later the procedure because the user doesn't need button access during startup
    TaskManager::create_button_task();

    // Create battery monitor task for ongoing monitoring
    TaskManager::create_battery_monitor_task();

    // 11. Motor (high priority, fast updates)
    TaskManager::create_motor_task();

    // 7. Network (Management task creates SendSensorData task)
    TaskManager::create_network_management_task();

    // 8. WebSocket polling (separate from communication for non-blocking sensor data)
    TaskManager::create_web_socket_polling_task();

    // 9. LEDs (moved later in sequence)
    rgbLed.turn_all_leds_off(); // Still turn off LEDs early for safety
    TaskManager::create_led_task();

    // 10. Initialize all sensors centrally to avoid I2C conflicts
    SensorInitializer::get_instance(); // This triggers centralized initialization

    // 11. Individual Sensor Tasks (wait for centralized init, then poll independently)
    TaskManager::create_imu_sensor_task();
    TaskManager::create_encoder_sensor_task();
    TaskManager::create_multizone_tof_sensor_task();
    TaskManager::create_side_tof_sensor_task();
    TaskManager::create_color_sensor_task();

    // 12. DemoManager (high priority for demos)
    // TaskManager::createDemoManagerTask();

    // 13. GameManager (interactive games)
    TaskManager::create_game_manager_task();

    // 14. CareerQuest (trigger system)
    TaskManager::create_career_quest_task();

    // 15. BytecodeVM
    TaskManager::create_bytecode_vm_task();

    // Note: StackMonitor can be enabled for debugging
    // TaskManager::createStackMonitorTask();
    // TaskManager::createSensorLoggerTask();
}

// Main loop runs on Core 1
void loop() {
    // Main loop can remain mostly empty as tasks handle the work
    vTaskDelay(pdMS_TO_TICKS(1));
}
