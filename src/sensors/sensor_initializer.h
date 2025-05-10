#pragma once
#include "./imu.h"
#include "./color_sensor.h"
#include "../utils/config.h"
#include "../utils/singleton.h"
#include "./multizone_tof_sensor.h"
#include "./side_time_of_flight_sensor.h"

// Forward declaration
class Sensors;

class SensorInitializer : public Singleton<SensorInitializer> {
    friend class Singleton<SensorInitializer>;
    friend class Sensors;

    public:
        enum SensorType {
            MULTIZONE_TOF,
            IMU,
            LEFT_SIDE_TOF,
            RIGHT_SIDE_TOF,
            // Add more sensors as needed
            SENSOR_COUNT
        };

        bool initializeAllSensors();
        bool tryInitializeMultizoneTof(MultizoneTofSensor& sensor);
        bool tryInitializeIMU(ImuSensor& sensor);
        bool tryInitializeLeftSideTof(SideTimeOfFlightSensor& sensor);
        bool tryInitializeRightSideTof(SideTimeOfFlightSensor& sensor);
        
        bool isSensorInitialized(SensorType sensor) const;
        bool areAllSensorsInitialized() const;
    private:
        SensorInitializer() {
            // Initialize the status array
            for (int i = 0; i < SENSOR_COUNT; i++) {
                sensorInitialized[i] = false;
            }
            MultizoneTofSensor& multizoneTofSensor = MultizoneTofSensor::getInstance();
            initializeMultizoneTof(multizoneTofSensor);
        }
        
        void initializeMultizoneTof(MultizoneTofSensor& sensor);
        void initializeIMU(ImuSensor& sensor);
        void initializeColorSensor(ColorSensor& sensor);
        void initializeSideTimeOfFlights(SideTimeOfFlightSensor& leftSensor, SideTimeOfFlightSensor& rightSensor);
        
        bool sensorInitialized[SENSOR_COUNT];
};
