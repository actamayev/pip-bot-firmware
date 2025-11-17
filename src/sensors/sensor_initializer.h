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

        static bool is_sensor_initialized(SensorType sensor);
        static void initialize_multizone_tof();
        static void initialize_imu();
        static void initialize_color_sensor();

        bool _sensorInitialized[SENSOR_COUNT]{};
};
