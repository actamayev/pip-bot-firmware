#pragma once
#include <Wire.h>
#include <vl53l7cx_class.h>
#include "../utils/config.h"

class MultizoneTofSensor {
    public:
        // Constructor that takes TwoWire instance and optional pin parameters
        MultizoneTofSensor(TwoWire *wire = &Wire) 
            : sensor(wire, -1, -1) {
                lastValidDataTime = millis();
            }

        bool initialize();
        VL53L7CX_ResultsData getTofData();
        bool isObjectDetected();
        float getAverageDistanceCenterline();

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
        
        // Configuration parameters
        uint16_t maxDistance = 1250;       // Maximum valid distance (mm)
        uint16_t minDistance = 30;         // Minimum distance threshold to filter out phantom readings
        uint8_t signalThreshold = 5;       // Minimum signal quality threshold (reduced for better hand detection)
        uint8_t minValidPoints = 3;        // Minimum valid points for obstacle detection
        uint16_t obstacleDistanceThreshold = 500; // Distance threshold to consider obstacle (mm)
        uint16_t xtalkMargin = 120;         // Xtalk margin for noise filtering
        uint8_t sharpenerPercent = 5;      // Sharpener percentage (0-99)
        uint32_t integrationTimeMs = 20;   // Integration time in milliseconds
        uint8_t rangingFrequency = TOF_RANGING_FREQUENCY;     // Ranging frequency in Hz
        
        // Target status constants
        const uint8_t TARGET_STATUS_VALID = 5;
        const uint8_t TARGET_STATUS_VALID_LARGE_PULSE = 9;
        const uint8_t TARGET_STATUS_VALID_WRAPPED = 6;
        
        // Temporal tracking variables
        static const uint8_t HISTORY_SIZE = 5;
        float previousCenterlineDistances[HISTORY_SIZE] = {0};
        int historyIndex = 0;
        float approachingThreshold = 20.0f; // mm change needed to trigger approaching detection
        
        // Watchdog variables
        uint32_t watchdogTimeout = 2000;   // 2 seconds without data before reset
        unsigned long lastValidDataTime;   // Track when we last got valid data
        bool sensorActive = false;        // Flag to track if sensor is actively ranging
};
