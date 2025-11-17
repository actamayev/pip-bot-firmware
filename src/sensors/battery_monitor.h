#pragma once

#include <Wire.h>

#include "../lib/bq27441_monitor/SparkFunBQ27441.h"
#include "networking/serial_manager.h"
#include "networking/serial_queue_manager.h"
#include "networking/websocket_manager.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "utils/structs.h"

class BatteryMonitor : public Singleton<BatteryMonitor> {
    friend class Singleton<BatteryMonitor>;
    friend class MessageProcessor;
    friend class TaskManager;

  public:
    bool initialize();
    const BatteryState& get_battery_state() const {
        return batteryState;
    }

    // Status checks
    bool is_critical_battery() const {
        return batteryState.isCriticalBattery;
    }

  private:
    // Main update method (call this periodically from task)
    void update();

    unsigned long lastBatteryLogTime = 0;
    void send_battery_monitor_data_over_websocket();
    BatteryMonitor() = default;
    ~BatteryMonitor() = default;

    // Update battery readings (call this periodically)
    void update_battery_state();

    // Configuration constants
    static constexpr unsigned int DEFAULT_BATTERY_CAPACITY = 1800; // mAh
    static constexpr unsigned int LOW_BATTERY_THRESHOLD = 20;      // %
    static constexpr unsigned int CRITICAL_BATTERY_THRESHOLD = 5;  // %

    BatteryState batteryState;
    unsigned long lastInitAttempt = 0;
    static constexpr unsigned long INIT_RETRY_INTERVAL_MS = 10000;  // Retry init every 10 seconds
    static constexpr unsigned long BATTERY_LOG_INTERVAL_MS = 10000; // Log every 10 seconds
    // Mapping math:
    // https://www.desmos.com/calculator/a9oghafkp7
    static constexpr float slopeTerm = (100.0f / (100.0f - CRITICAL_BATTERY_THRESHOLD));
    static constexpr float yInterceptTerm = 100.0f * CRITICAL_BATTERY_THRESHOLD / (CRITICAL_BATTERY_THRESHOLD - 100.0f);

    // Helper methods
    void calculate_time_estimates();
    void update_status_flags();
    void handle_battery_logging();
    void retry_initialization_if_needed();
    void send_battery_monitor_data_over_serial();
};
