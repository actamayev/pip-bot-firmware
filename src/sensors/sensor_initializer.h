#pragma once
#include "imu.h"
#include "color_sensor.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "side_tof_manager.h"
#include "multizone_tof_sensor.h"
#include "side_time_of_flight_sensor.h"
#include "networking/serial_queue_manager.h"

class SensorInitializer : public Singleton<SensorInitializer> {
    friend class Singleton<SensorInitializer>;
    friend class TaskManager;

    private:
        SensorInitializer();
    
        enum SensorType {
            MULTIZONE_TOF,
            IMU,
            COLOR_SENSOR,
            // Add more sensors as needed
            SENSOR_COUNT
        };

        bool isSensorInitialized(SensorType sensor) const;
        void initializeMultizoneTof();
        void initializeIMU();
        void initializeColorSensor();

        bool sensorInitialized[SENSOR_COUNT];
};
