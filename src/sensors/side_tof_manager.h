#pragma once
#include "utils/singleton.h"
#include "utils/config.h"
#include "side_time_of_flight_sensor.h"
#include "sensor_data_buffer.h"
#include "networking/serial_queue_manager.h"

class SideTofManager : public Singleton<SideTofManager> {
    friend class Singleton<SideTofManager>;
    friend class TaskManager;
    friend class SensorInitializer;
    
    private:
        SideTimeOfFlightSensor leftSideTofSensor;
        SideTimeOfFlightSensor rightSideTofSensor;

        // Following the established buffer pattern
        bool initialize();
        
        void turn_off_side_tofs();
        SideTofManager() = default;
        
        bool isInitialized = false;
        bool sensorsEnabled = false;  // Track if sensors are actively enabled
        
        void enable_side_tof_sensors();
        void disable_side_tof_sensors();

        // New buffer-based methods following IMU/TOF pattern
        void update_sensor_data();  // Single read, write to buffer
        bool should_be_polling() const;

        // Side TOFs
        const uint8_t LEFT_TOF_ADDRESS = 0x51;
        const uint8_t RIGHT_TOF_ADDRESS = 0x60;
};
