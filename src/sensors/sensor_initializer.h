#pragma once
#include "imu.h"
#include "color_sensor.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "side_tof_manager.h"
#include "multizone_tof_sensor.h"
#include "side_time_of_flight_sensor.h"
#include "ir_sensor.h"
#include "networking/serial_queue_manager.h"

class SensorInitializer : public Singleton<SensorInitializer> {
    friend class Singleton<SensorInitializer>;

    public:
        enum SensorType {
            // MULTIZONE_TOF,
            IMU,
            // LEFT_SIDE_TOF,
            // RIGHT_SIDE_TOF,
            // IR_SENSORS,
            // COLOR_SENSOR,
            // Add more sensors as needed
            SENSOR_COUNT
        };

        // bool tryInitializeMultizoneTof();
        bool tryInitializeIMU();
        // bool tryInitializeLeftSideTof();
        // bool tryInitializeRightSideTof();
        
        bool isSensorInitialized(SensorType sensor) const;
        bool areAllSensorsInitialized() const;
    private:
        SensorInitializer();
        
        // void initializeMultizoneTof();
        void initializeIMU();
        void initializeColorSensor();
        // void initializeSideTimeOfFlights();
        void initializeIRSensors();
        
        bool sensorInitialized[SENSOR_COUNT];
};
