#pragma once

#include <Wire.h>
#include <SparkFunBQ27441.h>
#include "utils/singleton.h"
#include "utils/config.h"

struct BatteryState {
    unsigned int stateOfCharge = 0;      // Battery percentage (0-100%)
    unsigned int voltage = 0;            // Battery voltage (mV)
    int current = 0;                     // Current draw/charge (mA, + = discharging, - = charging)
    int power = 0;                       // Power consumption (mW)
    unsigned int remainingCapacity = 0;  // Remaining capacity (mAh)
    unsigned int fullCapacity = 0;       // Full capacity (mAh)
    int health = 0;                      // Battery health (0-100%)
    bool isCharging = false;             // True if battery is charging
    bool isDischarging = false;          // True if battery is discharging
    bool isLowBattery = false;           // True if battery is below threshold
    bool isCriticalBattery = false;      // True if battery is critically low
    float estimatedTimeToEmpty = 0.0;    // Hours until empty (0 if charging/standby)
    float estimatedTimeToFull = 0.0;     // Hours until full (0 if discharging/standby)
    bool isInitialized = false;          // True if BQ27441 is successfully initialized
};

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
        
        // Battery status as string
        String getBatteryStatusString() const;
        String getChargingStatusString() const;

    private:
        BatteryMonitor() = default;
        ~BatteryMonitor() = default;
        
        // Configuration constants
        static constexpr unsigned int DEFAULT_BATTERY_CAPACITY = 1800; // mAh
        static constexpr unsigned int LOW_BATTERY_THRESHOLD = 20;      // %
        static constexpr unsigned int CRITICAL_BATTERY_THRESHOLD = 10;  // %
        static constexpr int CHARGING_CURRENT_THRESHOLD = -50;         // mA (negative = charging)
        static constexpr int DISCHARGING_CURRENT_THRESHOLD = 50;       // mA (positive = discharging)
        
        BatteryState batteryState;
        unsigned long lastUpdateTime = 0;
        unsigned long lastLowBatteryWarning = 0;
        unsigned long lastInitAttempt = 0;
        static constexpr unsigned long UPDATE_INTERVAL_MS = 1000; // Update every second
        static constexpr unsigned long LOW_BATTERY_WARNING_INTERVAL_MS = 30000; // Warn every 30 seconds
        static constexpr unsigned long INIT_RETRY_INTERVAL_MS = 10000; // Retry init every 10 seconds
        
        // Helper methods
        void calculateTimeEstimates();
        void updateStatusFlags();
        void handleWarnings();
        void retryInitializationIfNeeded();
        String formatTime(float hours) const;
};
