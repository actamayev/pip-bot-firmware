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
    bool needsInitialization() const {
        return !isInitialized;
    }
    uint8_t sensorAddress = 0; // Store the specific sensor address

    // Initialization retry variables
    bool isInitialized = false;

    // Calibration data structures
    uint16_t baselineValue = 0;
    bool useHardwareCalibration = false;
    bool isCalibrated = false;

    // Reset a specific sensor by address
    void reset_specific_sensor() {
        Reset_Sensor(sensorAddress);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Read proximity data from the sensor (with calibration applied)
    uint16_t read_proximity_data() {
        uint16_t rawReading = VCNL36828P_GET_PS_DATA(sensorAddress);
        return apply_calibration(rawReading);
    }

    void basic_initialization_auto_mode();

    // Calibration methods
    void load_calibration_from_preferences();
    bool perform_calibration();
    uint16_t capture_baseline_reading();
    void apply_hardware_calibration(uint16_t baseline);
    uint16_t apply_calibration(uint16_t rawReading);

    // For rate limiting reads
    uint32_t _lastUpdateTime = 0;
    static constexpr uint32_t DELAY_BETWEEN_READINGS = 1; // ms - minimal delay like performance test

    // New buffer-based methods following the established pattern
    void update_sensor_data(); // Read sensor and return data (don't store locally)

    // For manager to get individual readings
    uint16_t get_current_counts();
};
