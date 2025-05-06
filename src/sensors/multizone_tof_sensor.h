#pragma once
#include <Wire.h>
#include <vl53l7cx_class.h>
#include "../utils/utils.h"
#include "../utils/config.h"

// Define ROI dimensions
#define ROI_ROWS 2        // Rows 3-4
#define ROI_COLS 6        // Columns 1-6

class MultizoneTofSensor {
    public:
        // Constructor that takes TwoWire instance and optional pin parameters
        MultizoneTofSensor(TwoWire *wire = &Wire) 
            : sensor(wire, -1, -1) {}

        bool initialize();
        bool canRetryInitialization() const;

        VL53L7CX_ResultsData getTofData();
        bool isObjectDetected();
        float getAverageDistanceCenterline();
        void printResult(VL53L7CX_ResultsData *Result);
        
        // New functions for point history tracking
        void initializePointHistories();
        void updatePointHistory(int rowIdx, int colIdx, float distance);
        bool isPointObstacleConsistent(int rowIdx, int colIdx);
        // void printPointHistoriesStatus(VL53L7CX_ResultsData *Result);

        unsigned int getInitRetryCount() const { return initRetryCount; }
        unsigned int getMaxInitRetries() const { return MAX_INIT_RETRIES; }
        bool needsInitialization() const { return !isInitialized; }
    
        uint16_t getMaxDistance() const { return maxDistance; }
        uint16_t getMinDistance() const { return minDistance; }
        uint16_t getSignalThreshold() const { return signalThreshold; }
        uint16_t getMinValidPoints() const { return minValidPoints; }
        uint16_t getObstacleDistanceThreshold() const { return obstacleDistanceThreshold; }
        uint16_t getXtalkMargin() const { return xtalkMargin; }
        uint16_t getSharpenerPercent() const { return sharpenerPercent; }
        uint16_t getIntegrationTimeMs() const { return integrationTimeMs; }
        uint16_t getRangingFrequency() const { return rangingFrequency; }

    private:
        VL53L7CX sensor;
        void measureDistance();
        VL53L7CX_ResultsData sensorData;
        bool configureSensor();
        bool resetSensor();
        void resetHistory();
        bool checkWatchdog();
        float getWeightedAverageDistance();
    
        void startRanging();
        void stopRanging();
        
        bool isInitialized = false;
        unsigned long lastInitAttempt = 0;
        unsigned int initRetryCount = 0;
        static const unsigned int MAX_INIT_RETRIES = 5;
        static const unsigned long INIT_RETRY_INTERVAL = 1000; // 1 second between retries

        // Configuration parameters
        uint16_t maxDistance = 1250;       // Maximum valid distance (mm)
        uint16_t minDistance = 30;         // Minimum distance threshold to filter out phantom readings
        uint8_t signalThreshold = 5;       // Minimum signal quality threshold (reduced for better hand detection)
        uint8_t minValidPoints = 3;        // Minimum valid points for obstacle detection
        uint8_t tofResolution = VL53L7CX_RESOLUTION_8X8; // Sensor resolution
        uint16_t obstacleDistanceThreshold = 125; // Distance threshold to consider obstacle (mm)
        uint16_t xtalkMargin = 120;         // Xtalk margin for noise filtering
        uint8_t sharpenerPercent = 1;      // Sharpener percentage (0-99)
        uint32_t integrationTimeMs = 20;   // Integration time in milliseconds
        uint8_t rangingFrequency = 15;     // Ranging frequency in Hz
        
        // Target status constants
        const uint8_t TARGET_STATUS_VALID = 5;
        const uint8_t TARGET_STATUS_VALID_LARGE_PULSE = 9;
        const uint8_t TARGET_STATUS_VALID_WRAPPED = 6;
        
        // Temporal tracking variables for weighted average
        static const uint8_t HISTORY_SIZE = 3;
        float previousCenterlineDistances[HISTORY_SIZE] = {0};
        int historyIndex = 0;
        float approachingThreshold = 20.0f; // mm change needed to trigger approaching detection
        
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
