#pragma once
#include "./imu.h"
#include "./config.h"
#include "./singleton.h"
#include "./color_sensor.h"
#include "./time_of_flight_sensor.h"
#include "./side_time_of_flight_sensors.h"

class Sensors : public Singleton<Sensors> {
    friend class Singleton<Sensors>;

    public:
        // TOF methods
        // bool getTofData(const VL53L5CX_ResultsData** leftData, const VL53L5CX_ResultsData** rightData);

        // IMU methods
        EulerAngles& getEulerAngles();
        float getPitch();
        float getYaw();
        float getRoll();

        float getXAccel();
        float getYAccel();
        float getZAccel();
        double getAccelMagnitude();

        float getXRotationRate();
        float getYRotationRate();
        float getZRotationRate();

        float getMagneticFieldX();
        float getMagneticFieldY();
        float getMagneticFieldZ();

        // Color Sensor Methods:
        ColorSensorData getColorSensorData();

        // Side TOFs:
        SideTofDistances getSideTofDistances();
    private:
        ImuSensor imu;
        SideTimeOfFlightSensors sideTimeOfFlightSensors;

        ColorSensor colorSensor;

        // Private constructor
        Sensors() {
            initialize();
        }

        void initialize();
        // void initializeTofSensors();
        void initializeIMU();
        void initializeColorSensor();
        void initializeSideTimeOfFlights();
};
