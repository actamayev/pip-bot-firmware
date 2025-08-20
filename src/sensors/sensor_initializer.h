#pragma once
#include "imu.h"
#include "color_sensor.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "side_tof_manager.h"
#include "multizone_tof_sensor.h"
#include "side_time_of_flight_sensor.h"
#include "ir_sensor.h"
#include "encoder_manager.h"
#include "networking/serial_queue_manager.h"

class SensorInitializer : public Singleton<SensorInitializer> {
    friend class Singleton<SensorInitializer>;

    public:
        enum SensorType {
            MULTIZONE_TOF,
            IMU,
            SIDE_TOFS,        // Added side TOF support
            IR_SENSORS,
            COLOR_SENSOR,
            ENCODER,              // Added encoder support
            // Add more sensors as needed
            SENSOR_COUNT
        };

        bool tryInitializeMultizoneTof();
        bool tryInitializeIMU();
        bool tryInitializeSideTofs();      // Added side TOF method
        bool tryInitializeColorSensor();  // Added color sensor method
        bool tryInitializeIrSensors();  // Add this method
        bool tryInitializeEncoder();       // Added encoder method

        bool isSensorInitialized(SensorType sensor) const;
        bool areAllSensorsInitialized() const;

    private:
        SensorInitializer();
        
        void initializeMultizoneTof();
        void initializeIMU();
        void initializeSideTofs();         // Added side TOF initialization
        void initializeColorSensor();
        void initializeIRSensors();
        void initializeEncoder();          // Added encoder initialization
        
        bool sensorInitialized[SENSOR_COUNT];
};
