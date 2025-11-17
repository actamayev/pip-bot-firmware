#pragma once
#include <Wire.h>
#include <vl53l7cx_class.h>

#include "networking/serial_queue_manager.h"
#include "sensor_data_buffer.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "utils/utils.h"

// Define ROI dimensions
enum {
    ROI_ROWS = 2, // Rows 3-4
    ROI_COLS = 6  // Columns 1-6
};

class MultizoneTofSensor : public Singleton<MultizoneTofSensor> {
    friend class Singleton<MultizoneTofSensor>;
    friend class TaskManager;
    friend class SensorInitializer;

  private:
    // Constructor that takes TwoWire instance and optional pin parameters
    MultizoneTofSensor() : _sensor(&Wire, -1, -1) {}

    bool initialize();

    void turn_off_sensor();
    VL53L7CX _sensor;
    static bool configure_sensor();
    bool reset_sensor();
    static bool check_watchdog();
    void enable_tof_sensor();
    void disable_tof_sensor();

    void start_ranging();
    void stop_ranging();

    bool _isInitialized = false;
    bool _sensorEnabled = false; // Track if sensor is actively enabled

    // Point history tracking for obstacle detection
    void initialize_point_histories();
    static void update_point_history(int row_index, int col_index, float distance);
    static bool is_point_obstacle_consistent(int row_index, int col_index);
    bool process_obstacle_detection(const VL53L7CX_ResultsData& raw_data) const;
    static float calculate_front_distance(const VL53L7CX_ResultsData& raw_data);

    // Configuration parameters
    uint16_t _MAX_DISTANCE = 1000;                     // Maximum valid distance (mm)
    uint16_t _MIN_DISTANCE = 1;                        // Minimum distance threshold to filter out phantom readings
    uint8_t _SIGNAL_THRESHOLD = 5;                     // Minimum signal quality threshold (reduced for better hand detection)
    uint8_t _TOF_RESOLUTION = VL53L7CX_RESOLUTION_8X8; // Sensor resolution
    uint8_t _RANGING_FREQUENCY = 15;                   // Ranging frequency in Hz
    uint16_t _OBSTACLE_DISTANCE_THRESHOLD = 200;       // Distance threshold to consider obstacle (mm)
    uint16_t _X_TALK_MARGIN = 120;                     // Xtalk margin for noise filtering
    uint8_t _SHARPENER_PERCENT = 100;                  // Sharpener percentage (0-99)
    uint32_t _INTEGRATION_TIME_MS = 5;                 // Integration time in milliseconds

    // Temporal tracking variables for weighted average
    static const uint8_t HISTORY_SIZE = 2;

    struct PointHistory {
        float distances[HISTORY_SIZE]; // Last 5 distance readings
        int index;                     // Current index in circular buffer
        int validReadings;             // Count of valid readings collected so far
    };
    // Point-specific history tracking
    PointHistory _pointHistories[ROI_ROWS][ROI_COLS]{};

    // Watchdog variables
    const uint32_t _WATCHDOG_TIMEOUT = 2000; // 2 seconds without data before reset
    uint32_t _lastValidDataTime = millis();  // Track when we last got valid data
    bool _sensorActive = false;              // Flag to track if sensor is actively ranging

    // New buffer-based methods following IMU pattern
    void update_sensor_data(); // Single read, write to buffer
    static bool should_be_polling();

    static const uint16_t CHECK_SENSOR_TIME = 20; // ms
};
