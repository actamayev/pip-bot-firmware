#pragma once

#include <Wire.h>
#include <SparkFunBQ27441.h>
#include "utils/config.h"
#include "utils/singleton.h"
#include "networking/serial_manager.h"
#include "utils/structs.h"

class BatteryMonitor : public Singleton<BatteryMonitor> {
    friend class Singleton<BatteryMonitor>;

    public:
        // Initialization
        bool initialize();
        bool isInitialized() const { return batteryState.isInitialized; }
        
        // Main update method (call this periodically from task)
        void update();
        
        // Update battery readings (call this periodically)
        void updateBatteryState();
        
        // Get current battery state
        const BatteryState& getBatteryState() const { return batteryState; }
        
        // Individual getters for convenience
        unsigned int getStateOfCharge() const { return batteryState.stateOfCharge; }
        unsigned int getVoltage() const { return batteryState.voltage; }
        float getVoltageVolts() const { return batteryState.voltage / 1000.0f; }
        int getCurrent() const { return batteryState.current; }
        int getPower() const { return batteryState.power; }
        unsigned int getRemainingCapacity() const { return batteryState.remainingCapacity; }
        unsigned int getFullCapacity() const { return batteryState.fullCapacity; }
        int getHealth() const { return batteryState.health; }
        
        // Status checks
        bool isCharging() const { return batteryState.isCharging; }
        bool isDischarging() const { return batteryState.isDischarging; }
        bool isLowBattery() const { return batteryState.isLowBattery; }
        bool isCriticalBattery() const { return batteryState.isCriticalBattery; }
        
        // Time estimates
        float getEstimatedTimeToEmpty() const { return batteryState.estimatedTimeToEmpty; }
        float getEstimatedTimeToFull() const { return batteryState.estimatedTimeToFull; }

        unsigned long lastBatteryLogTime = 0;
    private:
        BatteryMonitor() = default;
        ~BatteryMonitor() = default;

        // Configuration constants
        static constexpr unsigned int DEFAULT_BATTERY_CAPACITY = 1800; // mAh
        static constexpr unsigned int LOW_BATTERY_THRESHOLD = 20;      // %
        static constexpr unsigned int CRITICAL_BATTERY_THRESHOLD = 10;  // %
        
        BatteryState batteryState;
        unsigned long lastUpdateTime = 0;
        unsigned long lastLowBatteryWarning = 0;
        unsigned long lastInitAttempt = 0;
        static constexpr unsigned long LOW_BATTERY_WARNING_INTERVAL_MS = 30000; // Warn every 30 seconds
        static constexpr unsigned long INIT_RETRY_INTERVAL_MS = 10000; // Retry init every 10 seconds
        static constexpr unsigned long BATTERY_LOG_INTERVAL_MS = 5000; // Log every 30 seconds
        
        // Helper methods
        void calculateTimeEstimates();
        void updateStatusFlags();
        void handleWarnings();
        void handleBatteryLogging();
        void retryInitializationIfNeeded();
};
