#include "battery_monitor.h"
#include "networking/serial_queue_manager.h"

bool BatteryMonitor::initialize() {
    SerialQueueManager::getInstance().queueMessage("Initializing BQ27441 battery monitor...");

    // Initialize the fuel gauge
    if (lipo.begin()) {
        SerialQueueManager::getInstance().queueMessage("✓ BQ27441 connected successfully");
        
        // Set the battery capacity if it hasn't been set already
        if (lipo.capacity(FULL) != DEFAULT_BATTERY_CAPACITY) {
            SerialQueueManager::getInstance().queueMessage("Setting battery capacity to " + String(DEFAULT_BATTERY_CAPACITY) + "mAh");
            lipo.setCapacity(DEFAULT_BATTERY_CAPACITY);
        }
        
        batteryState.isInitialized = true;
        
        // Initial battery state update
        updateBatteryState();
        
        SerialQueueManager::getInstance().queueMessage("✓ Battery monitor initialized - SOC: " + String(batteryState.stateOfCharge) + "%");
        
        // Send battery data immediately if connected to serial
        if (SerialManager::getInstance().isConnected) {
            SerialQueueManager::getInstance().queueMessage("Sending battery data on initialization...");
            SerialManager::getInstance().sendBatteryMonitorData(batteryState);
            lastBatteryLogTime = millis();
        }
        
        return true;
    } else {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to connect to BQ27441 - check wiring and I2C address");
        batteryState.isInitialized = false;
        return false;
    }
}

void BatteryMonitor::updateBatteryState() {
    if (!batteryState.isInitialized) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime < UPDATE_INTERVAL_MS) {
        return; // Don't update too frequently
    }
    lastUpdateTime = currentTime;
    
    // Read battery parameters from BQ27441
    batteryState.stateOfCharge = lipo.soc();
    batteryState.voltage = lipo.voltage();
    batteryState.current = lipo.current(AVG);
    batteryState.power = lipo.power();
    batteryState.remainingCapacity = lipo.capacity(REMAIN);
    batteryState.fullCapacity = lipo.capacity(FULL);
    batteryState.health = lipo.soh();
    
    // Update derived status flags
    updateStatusFlags();
    
    // Calculate time estimates
    calculateTimeEstimates();
}

void BatteryMonitor::updateStatusFlags() {
    // Charging/discharging status
    if (batteryState.current < CHARGING_CURRENT_THRESHOLD) {
        batteryState.isCharging = true;
        batteryState.isDischarging = false;
    } else if (batteryState.current > DISCHARGING_CURRENT_THRESHOLD) {
        batteryState.isCharging = false;
        batteryState.isDischarging = true;
    } else {
        batteryState.isCharging = false;
        batteryState.isDischarging = false;
    }
    
    // Battery level warnings
    batteryState.isLowBattery = (batteryState.stateOfCharge <= LOW_BATTERY_THRESHOLD);
    batteryState.isCriticalBattery = (batteryState.stateOfCharge <= CRITICAL_BATTERY_THRESHOLD);
}

void BatteryMonitor::calculateTimeEstimates() {
    batteryState.estimatedTimeToEmpty = 0.0f;
    batteryState.estimatedTimeToFull = 0.0f;
    
    if (batteryState.isDischarging && batteryState.current > 0 && batteryState.remainingCapacity > 0) {
        // Calculate time to empty
        batteryState.estimatedTimeToEmpty = (float)batteryState.remainingCapacity / batteryState.current;
    } else if (batteryState.isCharging && batteryState.current < 0) {
        // Calculate time to full
        int chargingCurrent = abs(batteryState.current);
        int remainingToFull = batteryState.fullCapacity - batteryState.remainingCapacity;
        if (chargingCurrent > 0 && remainingToFull > 0) {
            batteryState.estimatedTimeToFull = (float)remainingToFull / chargingCurrent;
        }
    }
}

String BatteryMonitor::getBatteryStatusString() const {
    if (!batteryState.isInitialized) return "Not initialized";

    String status = String(batteryState.stateOfCharge) + "% ";
    
    if (batteryState.stateOfCharge > 80) {
        status += "(Excellent)";
    } else if (batteryState.stateOfCharge > 60) {
        status += "(Good)";
    } else if (batteryState.stateOfCharge > 40) {
        status += "(Fair)";
    } else if (batteryState.stateOfCharge > 20) {
        status += "(Low)";
    } else if (batteryState.stateOfCharge > 10) {
        status += "(Very Low)";
    } else {
        status += "(Critical)";
    }
    
    return status;
}

String BatteryMonitor::getChargingStatusString() const {
    if (!batteryState.isInitialized) return "Unknown";
    
    if (batteryState.isCharging) {
        String status = "Charging";
        if (batteryState.estimatedTimeToFull > 0) {
            status += " (" + formatTime(batteryState.estimatedTimeToFull) + " to full)";
        }
        return status;
    } else if (batteryState.isDischarging) {
        String status = "Discharging";
        if (batteryState.estimatedTimeToEmpty > 0) {
            status += " (" + formatTime(batteryState.estimatedTimeToEmpty) + " remaining)";
        }
        return status;
    } else {
        return "Standby";
    }
}

String BatteryMonitor::formatTime(float hours) const {
    if (hours < 0) return "Unknown";
    
    int totalMinutes = (int)(hours * 60);
    int displayHours = totalMinutes / 60;
    int displayMinutes = totalMinutes % 60;
    
    if (displayHours > 0) {
        return String(displayHours) + "h " + String(displayMinutes) + "m";
    } else {
        return String(displayMinutes) + "m";
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
    
    // Handle warnings and alerts
    handleWarnings();
    
    // Handle periodic battery logging when connected to serial
    handleBatteryLogging();
}

void BatteryMonitor::handleWarnings() {
    if (!batteryState.isInitialized) return;
    
    unsigned long currentTime = millis();
    
    if (batteryState.isCriticalBattery) {
        SerialQueueManager::getInstance().queueMessage(
            "🚨 CRITICAL: Battery at " + String(batteryState.stateOfCharge) + "%!", 
            SerialPriority::CRITICAL
        );
    } else if (batteryState.isLowBattery) {
        // Log low battery warning less frequently
        if (currentTime - lastLowBatteryWarning > LOW_BATTERY_WARNING_INTERVAL_MS) {
            SerialQueueManager::getInstance().queueMessage(
                "⚠️  WARNING: Battery low at " + String(batteryState.stateOfCharge) + "%", 
                SerialPriority::HIGH_PRIO
            );
            lastLowBatteryWarning = currentTime;
        }
    }
}

void BatteryMonitor::handleBatteryLogging() {
    if (!batteryState.isInitialized) return;
    
    // Only log if connected to serial
    if (!SerialManager::getInstance().isConnected) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastBatteryLogTime < BATTERY_LOG_INTERVAL_MS) return;
    // Log battery data every 30 seconds
    SerialManager::getInstance().sendBatteryMonitorData(batteryState);
    SerialQueueManager::getInstance().queueMessage("Battery data logged", SerialPriority::HIGH_PRIO);
    lastBatteryLogTime = currentTime;
}

void BatteryMonitor::retryInitializationIfNeeded() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastInitAttempt > INIT_RETRY_INTERVAL_MS) {
        initialize();
        lastInitAttempt = currentTime;
    }
}
