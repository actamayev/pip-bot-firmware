#include "battery_monitor.h"
#include "networking/serial_queue_manager.h"

bool BatteryMonitor::initialize() {
    SerialQueueManager::getInstance().queueMessage("Initializing BQ27441 battery monitor...");

    // Initialize the fuel gauge
    if (!lipo.begin(Wire1)) {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to connect to BQ27441 - check wiring and I2C address");
        batteryState.isInitialized = false;
        return false;
    }
    SerialQueueManager::getInstance().queueMessage("✓ BQ27441 connected successfully");
    
    // Set the battery capacity if it hasn't been set already
    if (lipo.capacity(FULL) != DEFAULT_BATTERY_CAPACITY) {
        lipo.setCapacity(DEFAULT_BATTERY_CAPACITY);
    }
    
    batteryState.isInitialized = true;
    
    // Initial battery state update
    updateBatteryState();
    
    SerialQueueManager::getInstance().queueMessage("✓ Battery monitor initialized - SOC: " + String(batteryState.realStateOfCharge) + "%");
    
    return true;
}

void BatteryMonitor::updateBatteryState() {
    if (!batteryState.isInitialized) return;
    
    // Read battery parameters from BQ27441
    batteryState.realStateOfCharge = lipo.soc();
    batteryState.voltage = lipo.voltage();
    batteryState.current = lipo.current(AVG);
    batteryState.power = lipo.power();
    batteryState.remainingCapacity = lipo.capacity(REMAIN);
    batteryState.fullCapacity = lipo.capacity(FULL);
    batteryState.health = lipo.soh();
    // Prevent sub-0 values.
    batteryState.displayedStateOfCharge = max(0.0f, 
        (batteryState.realStateOfCharge) * (slopeTerm) + yInterceptTerm);

    // Update derived status flags
    updateStatusFlags();
    
    // Calculate time estimates
    calculateTimeEstimates();
}

void BatteryMonitor::updateStatusFlags() {
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

void BatteryMonitor::calculateTimeEstimates() {
    batteryState.estimatedTimeToEmpty = 0.0f;
    batteryState.estimatedTimeToFull = 0.0f;
    
    if (batteryState.isDischarging && batteryState.current < 0 && batteryState.remainingCapacity > 0) {
        // Calculate time to empty
        batteryState.estimatedTimeToEmpty = (float)batteryState.remainingCapacity / batteryState.current;
    } else if (batteryState.isCharging && batteryState.current > 0) {
        // Calculate time to full
        int chargingCurrent = abs(batteryState.current);
        int remainingToFull = batteryState.fullCapacity - batteryState.remainingCapacity;
        if (chargingCurrent > 0 && remainingToFull > 0) {
            batteryState.estimatedTimeToFull = (float)remainingToFull / chargingCurrent;
        }
    }
}

void BatteryMonitor::update() {
    // Try to initialize if not already done
    if (!batteryState.isInitialized) {
        retryInitializationIfNeeded();
        return;
    }
    
    // Update battery state
    updateBatteryState();
    
    // Handle periodic battery logging when connected to serial
    handleBatteryLogging();

    if (!batteryState.isCriticalBattery) return;
    DisplayScreen::getInstance().showLowBatteryScreen();
    vTaskDelay(pdMS_TO_TICKS(3000)); // Show low battery message for 3 seconds
    Buttons::getInstance().enterDeepSleep();
}

void BatteryMonitor::handleBatteryLogging() {
    if (!batteryState.isInitialized) return;
    
    // Only log if connected to serial
    if (
        !SerialManager::getInstance().isSerialConnected() &&
        !WebSocketManager::getInstance().isConnected()
    ) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastBatteryLogTime < BATTERY_LOG_INTERVAL_MS) return;
    // Log battery data every 30 seconds

    sendBatteryMonitorDataOverSerial();
    sendBatteryMonitorDataOverWebSocket();
}

void BatteryMonitor::retryInitializationIfNeeded() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastInitAttempt > INIT_RETRY_INTERVAL_MS) {
        initialize();
        lastInitAttempt = currentTime;
    }
}

void BatteryMonitor::sendBatteryMonitorDataOverSerial() {
    if (!SerialManager::getInstance().isSerialConnected()) return;
    SerialManager::getInstance().sendBatteryMonitorData();
    lastBatteryLogTime = millis();
}

void BatteryMonitor::sendBatteryMonitorDataOverWebSocket() {
    if (!WebSocketManager::getInstance().isConnected()) return;
    WebSocketManager::getInstance().sendBatteryMonitorData();
    lastBatteryLogTime = millis();
}
