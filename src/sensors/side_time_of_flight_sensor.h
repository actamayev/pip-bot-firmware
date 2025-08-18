#pragma once
#include <Arduino.h>
#include "./vcnl36828p/VCNL36828P.h"
#include "./vcnl36828p/VCNL36828P_Prototypes.h"
#include "./vcnl36828p/typedefinition.h"
#include "./vcnl36828p/VCNL36828P_Application_Library.h"
#include "utils/config.h"
#include "utils/structs.h"
#include "sensor_data_buffer.h"
#include "utils/preferences_manager.h"
#include "networking/serial_queue_manager.h"

class SideTimeOfFlightSensor {
    friend class SideTofManager;

    public:
        SideTimeOfFlightSensor() = default;

        bool initialize(const uint8_t TOF_ADDRESS);
        bool canRetryInitialization() const;
        bool needsInitialization() const { return !isInitialized; }

    private:
        uint8_t sensorAddress = 0; // Store the specific sensor address
        
        // Initialization retry variables
        bool isInitialized = false;
        unsigned long lastInitAttempt = 0;
        unsigned int initRetryCount = 0;
        static const unsigned int MAX_INIT_RETRIES = 5;
        static const unsigned long INIT_RETRY_INTERVAL = 1000; // 1 second between retries

        // Calibration data structures
        uint16_t baselineValue = 0;
        bool useHardwareCalibration = false;
        bool isCalibrated = false;

        // Reset a specific sensor by address
        void Reset_Specific_Sensor() {
            Reset_Sensor(sensorAddress);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // Read proximity data from the sensor (with calibration applied)
        uint16_t Read_Proximity_Data() {
            uint16_t rawReading = VCNL36828P_GET_PS_DATA(sensorAddress);
            return applyCalibration(rawReading);
        }

        void Basic_Initialization_Auto_Mode();

        // Calibration methods
        void loadCalibrationFromPreferences();
        bool performCalibration();
        uint16_t captureBaselineReading();
        void applyHardwareCalibration(uint16_t baseline);
        uint16_t applyCalibration(uint16_t rawReading);

        // For rate limiting reads
        unsigned long _lastUpdateTime = 0;
        static constexpr unsigned long DELAY_BETWEEN_READINGS = 10; //ms

        // New buffer-based methods following the established pattern
        void updateSensorData();  // Read sensor and return data (don't store locally)
        
        // For manager to get individual readings
        uint16_t getCurrentCounts();
};
