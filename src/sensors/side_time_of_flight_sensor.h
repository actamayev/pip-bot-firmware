#pragma once
#include <Arduino.h>
#include "./vcnl36828p/VCNL36828P.h"
#include "./vcnl36828p/VCNL36828P_Prototypes.h"
#include "./vcnl36828p/typedefinition.h"
#include "./vcnl36828p/VCNL36828P_Application_Library.h"
#include "utils/config.h"
#include "utils/structs.h"
#include "sensor_data_buffer.h"

class SideTimeOfFlightSensor {
    public:
        SideTimeOfFlightSensor() = default;

        bool initialize(const uint8_t TOF_ADDRESS);
        bool canRetryInitialization() const;
        bool needsInitialization() const { return !isInitialized; }
        unsigned int getInitRetryCount() const { return initRetryCount; }
        unsigned int getMaxInitRetries() const { return MAX_INIT_RETRIES; }
        
        // New buffer-based methods following the established pattern
        void updateSensorData();  // Read sensor and return data (don't store locally)
        bool shouldBePolling() const { return isInitialized; }  // Simple - just check if initialized
        
        // For manager to get individual readings
        uint16_t getCurrentCounts();

    private:
        uint8_t sensorAddress = 0; // Store the specific sensor address
        
        // Initialization retry variables
        bool isInitialized = false;
        unsigned long lastInitAttempt = 0;
        unsigned int initRetryCount = 0;
        static const unsigned int MAX_INIT_RETRIES = 5;
        static const unsigned long INIT_RETRY_INTERVAL = 1000; // 1 second between retries

        // Reset a specific sensor by address
        void Reset_Specific_Sensor() {
            Reset_Sensor(sensorAddress);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // Read proximity data from the sensor
        uint16_t Read_Proximity_Data() {
            return VCNL36828P_GET_PS_DATA(sensorAddress);
        }

        void Basic_Initialization_Auto_Mode();

        // For rate limiting reads
        unsigned long _lastUpdateTime = 0;
        static constexpr unsigned long DELAY_BETWEEN_READINGS = 10; //ms
};
