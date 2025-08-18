// #pragma once
// #include "veml3328/veml3328.h"
// #include "utils/structs.h"
// #include "utils/singleton.h"
// #include "sensor_data_buffer.h"
// #include "networking/serial_queue_manager.h"

// class ColorSensor : public Singleton<ColorSensor> {
//     friend class Singleton<ColorSensor>;
//     friend class TaskManager;

//     public:
//         ColorSensor() = default;

//         bool initialize();
//         bool canRetryInitialization() const;
//         bool needsInitialization() const { return !isInitialized; }

//     private:
//         void read_color_sensor();
//         void precompute_inverse_matrix();
//         void enableColorSensor();
//         void disableColorSensor();
        
//         bool isInitialized = false;
//         bool sensorConnected = false;
//         bool sensorEnabled = false;  // Track if sensor is actively enabled
        
//         // Initialization retry variables
//         unsigned long lastInitAttempt = 0;
//         unsigned int initRetryCount = 0;
//         static const unsigned int MAX_INIT_RETRIES = 5;
//         static const unsigned long INIT_RETRY_INTERVAL = 1000; // 1 second between retries

//         bool isCalibrated = true;
//         const float scaleFactor = 128.0;

//         CalibrationValues calibrationValues = {
//             // RED LED readings (R, G, B)
//             195 * scaleFactor, 42 * scaleFactor, 5 * scaleFactor,
            
//             // GREEN LED readings (R, G, B)
//             29 * scaleFactor, 371 * scaleFactor, 146 * scaleFactor,
            
//             // BLUE LED readings (R, G, B)
//             26 * scaleFactor, 103 * scaleFactor, 511 * scaleFactor
//         };
//         ColorSensorData colorSensorData;
//         float invMatrix[3][3]; // Store pre-computed inverse matrix
//         unsigned long lastUpdateTime = 0;
//         static constexpr unsigned long DELAY_BETWEEN_READINGS = 50; // ms - rate limiting

//         // New buffer-based methods following the established pattern
//         void updateSensorData();  // Single read, write to buffer
//         bool shouldBePolling() const;
//         const uint8_t COLOR_SENSOR_LED_PIN = 5;
// };
