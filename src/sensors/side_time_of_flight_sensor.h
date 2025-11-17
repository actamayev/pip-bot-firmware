#pragma once
#include <Arduino.h>

#include "../lib/vcnl36828p/I2C_Functions.h"
#include "../lib/vcnl36828p/VCNL36828P.h"
#include "../lib/vcnl36828p/VCNL36828P_Application_Library.h"
#include "../lib/vcnl36828p/VCNL36828P_Prototypes.h"
#include "../lib/vcnl36828p/typedefinition.h"
#include "networking/serial_queue_manager.h"
#include "sensor_data_buffer.h"
#include "utils/config.h"
#include "utils/preferences_manager.h"
#include "utils/structs.h"

class SideTimeOfFlightSensor {
    friend class SideTofManager;

  private:
    SideTimeOfFlightSensor() = default;
    bool initialize(const uint8_t TOF_ADDRESS);
    bool needs_initialization() const {
        return !_isInitialized;
    }
    uint8_t _sensorAddress = 0; // Store the specific sensor address

    // Initialization retry variables
    bool _isInitialized = false;

    // Calibration data structures
    uint16_t _baselineValue = 0;
    bool _useHardwareCalibration = false;
    bool _isCalibrated = false;

    // Reset a specific sensor by address
    void reset_specific_sensor() const {
        Reset_Sensor(_sensorAddress);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Read proximity data from the sensor (with calibration applied)
    uint16_t read_proximity_data() const {
        uint16_t raw_reading = VCNL36828P_GET_PS_DATA(_sensorAddress);
        return apply_calibration(raw_reading);
    }

    static void basic_initialization_auto_mode();

    // Calibration methods
    static void load_calibration_from_preferences();
    static bool perform_calibration();
    static uint16_t capture_baseline_reading();
    static void apply_hardware_calibration(uint16_t baseline);
    static uint16_t apply_calibration(uint16_t raw_reading);

    // For rate limiting reads
    uint32_t _lastUpdateTime = 0;
    static constexpr uint32_t DELAY_BETWEEN_READINGS = 1; // ms - minimal delay like performance test

    // New buffer-based methods following the established pattern
    void update_sensor_data(); // Read sensor and return data (don't store locally)

    // For manager to get individual readings
    uint16_t get_current_counts();
};
