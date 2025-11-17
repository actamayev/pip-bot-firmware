#pragma once
#include <Wire.h>
#include <vl53l7cx_class.h>
#include "utils/utils.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "sensor_data_buffer.h"
#include "networking/serial_queue_manager.h"

// Define ROI dimensions
#define ROI_ROWS 2        // Rows 3-4
#define ROI_COLS 6        // Columns 1-6

class MultizoneTofSensor : public Singleton<MultizoneTofSensor> {
    friend class Singleton<MultizoneTofSensor>;
    friend class TaskManager;
    friend class SensorInitializer;
    
    private:
        // Constructor that takes TwoWire instance and optional pin parameters
        MultizoneTofSensor() 
            : sensor(&Wire, -1, -1) {}

        bool initialize();

        void turn_off_sensor();
        VL53L7CX sensor;
        bool configure_sensor();
        bool reset_sensor();
        bool check_watchdog();
        void enable_tof_sensor();
        void disable_tof_sensor();
    
        void start_ranging();
        void stop_ranging();
        
        bool isInitialized = false;
        bool sensorEnabled = false;  // Track if sensor is actively enabled

        // Point history tracking for obstacle detection
        void initialize_point_histories();
        void update_point_history(int rowIdx, int colIdx, float distance);
        bool is_point_obstacle_consistent(int rowIdx, int colIdx);
        bool process_obstacle_detection(const VL53L7CX_ResultsData& rawData);
        float calculate_front_distance(const VL53L7CX_ResultsData& rawData);

        // Configuration parameters
        uint16_t MAX_DISTANCE = 1000;       // Maximum valid distance (mm)
        uint16_t MIN_DISTANCE = 1;         // Minimum distance threshold to filter out phantom readings
        uint8_t SIGNAL_THRESHOLD = 5;       // Minimum signal quality threshold (reduced for better hand detection)
        uint8_t TOF_RESOLUTION = VL53L7CX_RESOLUTION_8X8; // Sensor resolution
        uint8_t RANGING_FREQUENCY = 15; // Ranging frequency in Hz
        uint16_t OBSTACLE_DISTANCE_THRESHOLD = 200; // Distance threshold to consider obstacle (mm)
        uint16_t X_TALK_MARGIN = 120;         // Xtalk margin for noise filtering
        uint8_t SHARPENER_PERCENT = 100;      // Sharpener percentage (0-99)
        uint32_t INTEGRATION_TIME_MS = 5;   // Integration time in milliseconds

        // Temporal tracking variables for weighted average
        static const uint8_t HISTORY_SIZE = 2;
        
        struct PointHistory {
            float distances[HISTORY_SIZE];     // Last 5 distance readings
            int index;              // Current index in circular buffer
            int validReadings;      // Count of valid readings collected so far
        };
        // Point-specific history tracking
        PointHistory pointHistories[ROI_ROWS][ROI_COLS];
        
        // Watchdog variables
        uint32_t WATCHDOG_TIMEOUT = 2000;   // 2 seconds without data before reset
        unsigned long lastValidDataTime = millis();   // Track when we last got valid data
        bool sensorActive = false;        // Flag to track if sensor is actively ranging

        // New buffer-based methods following IMU pattern
        void update_sensor_data();  // Single read, write to buffer
        bool should_be_polling() const;

        static const uint16_t CHECK_SENSOR_TIME = 20; //ms
};
