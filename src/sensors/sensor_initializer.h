#pragma once
#include "color_sensor.h"
#include "imu.h"
#include "multizone_tof_sensor.h"
#include "networking/serial_queue_manager.h"
#include "side_time_of_flight_sensor.h"
#include "side_tof_manager.h"
#include "utils/config.h"
#include "utils/singleton.h"

class SensorInitializer : public Singleton<SensorInitializer> {
    friend class Singleton<SensorInitializer>;
    friend class TaskManager;

  private:
    SensorInitializer();

    enum SensorType : uint8_t {
        MULTIZONE_TOF,
        IMU,
        COLOR_SENSOR,
        // Add more sensors as needed
        SENSOR_COUNT
    };

    bool is_sensor_initialized(SensorType sensor);
    void initialize_multizone_tof();
    void initialize_imu();
    void initialize_color_sensor();

    bool _sensorInitialized[SENSOR_COUNT]{};
};
