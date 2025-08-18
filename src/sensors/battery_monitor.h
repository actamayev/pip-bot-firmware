#pragma once

#include <Wire.h>
#include "utils/config.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "networking/serial_manager.h"
#include "networking/websocket_manager.h"
#include "sensors/bq27441_monitor/SparkFunBQ27441.h"

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

        // Status checks
        bool isCriticalBattery() const { return batteryState.isCriticalBattery; }

        unsigned long lastBatteryLogTime = 0;
        void sendBatteryMonitorDataOverWebSocket();

    private:
        BatteryMonitor() = default;
        ~BatteryMonitor() = default;

        // Configuration constants
        static constexpr unsigned int DEFAULT_BATTERY_CAPACITY = 1800; // mAh
        static constexpr unsigned int LOW_BATTERY_THRESHOLD = 20;      // %
        static constexpr unsigned int CRITICAL_BATTERY_THRESHOLD = 5;  // %

        BatteryState batteryState;
        unsigned long lastInitAttempt = 0;
        static constexpr unsigned long INIT_RETRY_INTERVAL_MS = 10000; // Retry init every 10 seconds
        static constexpr unsigned long BATTERY_LOG_INTERVAL_MS = 10000; // Log every 10 seconds
        // Mapping math:
        // https://www.desmos.com/calculator/a9oghafkp7
        static constexpr float slopeTerm = (100.0f / (100.0f - CRITICAL_BATTERY_THRESHOLD));
        static constexpr float yInterceptTerm = 100.0f * CRITICAL_BATTERY_THRESHOLD / (CRITICAL_BATTERY_THRESHOLD - 100.0f);

        // Helper methods
        void calculateTimeEstimates();
        void updateStatusFlags();
        void handleBatteryLogging();
        void retryInitializationIfNeeded();
        void sendBatteryMonitorDataOverSerial();
};
