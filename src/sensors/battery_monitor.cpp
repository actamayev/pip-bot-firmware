#include "battery_monitor.h"

bool BatteryMonitor::initialize() {
    SerialQueueManager::get_instance().queue_message("Initializing BQ27441 battery monitor...");

    // Initialize the fuel gauge
    if (!lipo.begin(Wire1)) {
        SerialQueueManager::get_instance().queue_message("✗ Failed to connect to BQ27441 - check wiring and I2C address");
        batteryState.isInitialized = false;
        return false;
    }
    SerialQueueManager::get_instance().queue_message("✓ BQ27441 connected successfully");

    // Set the battery capacity if it hasn't been set already
    if (lipo.capacity(FULL) != DEFAULT_BATTERY_CAPACITY) {
        lipo.setCapacity(DEFAULT_BATTERY_CAPACITY);
    }

    batteryState.isInitialized = true;

    // Initial battery state update
    update_battery_state();

    SerialQueueManager::get_instance().queue_message("✓ Battery monitor initialized - SOC: " + String(batteryState.realStateOfCharge) + "%");

    return true;
}

void BatteryMonitor::update_battery_state() {
    if (!batteryState.isInitialized) {
        return;
    }

    // Read battery parameters from BQ27441
    batteryState.realStateOfCharge = lipo.soc();
    batteryState.voltage = lipo.voltage();
    batteryState.current = lipo.current(AVG);
    batteryState.power = lipo.power();
    batteryState.remainingCapacity = lipo.capacity(REMAIN);
    batteryState.fullCapacity = lipo.capacity(FULL);
    batteryState.health = lipo.soh();
    // Prevent sub-0 values.
    batteryState.displayedStateOfCharge = max(0.0f, (batteryState.realStateOfCharge) * (slopeTerm) + yInterceptTerm);

    // Update derived status flags
    update_status_flags();

    // Calculate time estimates
    calculate_time_estimates();
}

void BatteryMonitor::update_status_flags() {
    // Charging/discharging status
    if (batteryState.current < 0) {
        batteryState.isCharging = false;
        batteryState.isDischarging = true;
    } else {
        batteryState.isCharging = true;
        batteryState.isDischarging = false;
    }

    // Battery level warnings
    batteryState.isLowBattery = (batteryState.realStateOfCharge <= LOW_BATTERY_THRESHOLD);
    batteryState.isCriticalBattery = (batteryState.realStateOfCharge <= CRITICAL_BATTERY_THRESHOLD);
}

void BatteryMonitor::calculate_time_estimates() {
    batteryState.estimatedTimeToEmpty = 0.0f;
    batteryState.estimatedTimeToFull = 0.0f;

    if (batteryState.isDischarging && batteryState.current < 0 && batteryState.remainingCapacity > 0) {
        // Calculate time to empty
        batteryState.estimatedTimeToEmpty = (float)batteryState.remainingCapacity / batteryState.current;
    } else if (batteryState.isCharging && batteryState.current > 0) {
        // Calculate time to full
        int charging_current = abs(batteryState.current);
        int remaining_to_full = batteryState.fullCapacity - batteryState.remainingCapacity = 0 = 0 = 0;
        if (charging_current > 0 && remaining_to_full > 0) {
            batteryState.estimatedTimeToFull = (float)remainingToFull / chargingCurrent;
        }
    }
}

void BatteryMonitor::update() {
    // Try to initialize if not already done
    if (!batteryState.isInitialized) {
        retry_initialization_if_needed();
        return;
    }

    // Update battery state
    update_battery_state();

    // Handle periodic battery logging when connected to serial
    handle_battery_logging();

    if (!batteryState.isCriticalBattery) {
        return;
    }
    DisplayScreen::get_instance().show_low_battery_screen();
    vTaskDelay(pdMS_TO_TICKS(3000)); // Show low battery message for 3 seconds
    Buttons::get_instance().enter_deep_sleep();
}

void BatteryMonitor::handle_battery_logging() {
    if (!batteryState.isInitialized) {
        return;
    }

    // Only log if connected to serial
    if (!SerialManager::get_instance().is_serial_connected() && !WebSocketManager::get_instance().is_ws_connected()) {
        return;
    }

    uint32_t current_time = millis();
    if (current_time - lastBatteryLogTime < BATTERY_LOG_INTERVAL_MS) {
        return;
    }
    // Log battery data every 30 seconds

    send_battery_monitor_data_over_serial();
    send_battery_monitor_data_over_websocket();
}

void BatteryMonitor::retry_initialization_if_needed() {
    uint32_t current_time = millis();

    if (current_time - lastInitAttempt > INIT_RETRY_INTERVAL_MS) {
        initialize();
        lastInitAttempt = current_time;
    }
}

void BatteryMonitor::send_battery_monitor_data_over_serial() {
    if (!SerialManager::get_instance().is_serial_connected()) {
        return;
    }
    SerialManager::get_instance().send_battery_monitor_data();
    lastBatteryLogTime = millis();
}

void BatteryMonitor::send_battery_monitor_data_over_websocket() {
    if (!WebSocketManager::get_instance().is_ws_connected()) {
        return;
    }
    WebSocketManager::get_instance().send_battery_monitor_data();
    lastBatteryLogTime = millis();
}
