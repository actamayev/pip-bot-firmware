#pragma once
#include <Wire.h>
#include <vl53l7cx_class.h>
#include "utils/utils.h"
#include "utils/config.h"
#include "utils/singleton.h"

// Define ROI dimensions
#define ROI_ROWS 2        // Rows 3-4
#define ROI_COLS 6        // Columns 1-6

class MultizoneTofSensor : public Singleton<MultizoneTofSensor> {
    friend class Singleton<MultizoneTofSensor>;

    public:
        // Constructor that takes TwoWire instance and optional pin parameters
        MultizoneTofSensor() 
            : sensor(&DEV_I2C, -1, -1) {}

        bool initialize();
        bool canRetryInitialization() const;

        VL53L7CX_ResultsData getTofData();
        bool isObjectDetected();
        void printResult(VL53L7CX_ResultsData *Result);

        unsigned int getInitRetryCount() const { return initRetryCount; }
        unsigned int getMaxInitRetries() const { return MAX_INIT_RETRIES; }
        bool needsInitialization() const { return !isInitialized; }
        void turnOffSensor();

    private:
        VL53L7CX sensor;
        void measureDistance();
        VL53L7CX_ResultsData sensorData;
        bool configureSensor();
        bool resetSensor();
        bool checkWatchdog();
    
        void startRanging();
        void stopRanging();
        
        bool isInitialized = false;
        unsigned long lastInitAttempt = 0;
        unsigned int initRetryCount = 0;
        static const unsigned int MAX_INIT_RETRIES = 5;
        static const unsigned long INIT_RETRY_INTERVAL = 1000; // 1 second between retries

        // New functions for point history tracking
        void initializePointHistories();
        void updatePointHistory(int rowIdx, int colIdx, float distance);
        bool isPointObstacleConsistent(int rowIdx, int colIdx);
        // void printPointHistoriesStatus(VL53L7CX_ResultsData *Result);

        // Configuration parameters
        uint16_t maxDistance = 1000;       // Maximum valid distance (mm)
        uint16_t minDistance = 10;         // Minimum distance threshold to filter out phantom readings
        uint8_t signalThreshold = 5;       // Minimum signal quality threshold (reduced for better hand detection)
        uint8_t tofResolution = VL53L7CX_RESOLUTION_8X8; // Sensor resolution
        uint8_t rangingFrequency = 15; // Ranging frequency in Hz
        uint16_t obstacleDistanceThreshold = 200; // Distance threshold to consider obstacle (mm)
        uint16_t xtalkMargin = 120;         // Xtalk margin for noise filtering
        uint8_t sharpenerPercent = 1;      // Sharpener percentage (0-99)
        uint32_t integrationTimeMs = 20;   // Integration time in milliseconds

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
        uint32_t watchdogTimeout = 2000;   // 2 seconds without data before reset
        unsigned long lastValidDataTime = millis();   // Track when we last got valid data
        bool sensorActive = false;        // Flag to track if sensor is actively ranging
        // Structure to hold history for each point in ROI
};
