#pragma once
#include "./imu.h"
#include "../utils/config.h"
#include "../utils/singleton.h"
#include "./color_sensor.h"
#include "./multizone_tof_sensor.h"
#include "./side_time_of_flight_sensor.h"

class Sensors : public Singleton<Sensors> {
    friend class Singleton<Sensors>;

    public:
        // Multizone TOF methods
        VL53L5CX_ResultsData getMultizoneTofData();

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
        SideTofDistances getBothSideTofDistances();
    private:
        ImuSensor imu;
        MultizoneTofSensor multizoneTofSensor;
        SideTimeOfFlightSensor leftSideTofSensor;
        SideTimeOfFlightSensor rightSideTofSensor;
        ColorSensor colorSensor;

        // Private constructor
        Sensors() {
            initialize();
        }

        void initialize();
        void initializeMultizoneTof();
        void initializeIMU();
        void initializeColorSensor();
        void initializeSideTimeOfFlights();
};
