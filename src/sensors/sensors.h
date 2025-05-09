#pragma once
#include "./imu.h"
#include "../utils/config.h"
#include "../utils/singleton.h"
#include "./color_sensor.h"
#include "./multizone_tof_sensor.h"
#include "./side_time_of_flight_sensor.h"
#include "./sensor_initializer.h"

class Sensors : public Singleton<Sensors> {
    friend class Singleton<Sensors>;

    public:
        // Multizone TOF methods
        VL53L7CX_ResultsData getMultizoneTofData();

        // IMU methods
        float getPitch();
        float getYaw();
        float getRoll();
        const EulerAngles& getEulerAngles();

        float getXAccel();
        float getYAccel();
        float getZAccel();
        double getAccelMagnitude();
        const AccelerometerData& getAccelerometerData();

        float getXRotationRate();
        float getYRotationRate();
        float getZRotationRate();
        const GyroscopeData& getGyroscopeData();

        float getMagneticFieldX();
        float getMagneticFieldY();
        float getMagneticFieldZ();
        const MagnetometerData& getMagnetometerData();

        // Color Sensor Methods:
        ColorSensorData getColorSensorData();

        // Side TOFs:
        SideTofCounts getBothSideTofCounts();

        uint16_t getLeftSideTofCounts();
        uint16_t getRightSideTofCounts();

        // Initialization status methods (delegated to initializer)
        bool isSensorInitialized(SensorInitializer::SensorType sensor) const;
        bool areAllSensorsInitialized() const;
        
        // Initialization attempt methods
        bool tryInitializeIMU();
        bool tryInitializeMultizoneTof();
        bool tryInitializeLeftSideTof();
        bool tryInitializeRightSideTof();

        // Multizone Tof Sensor:
        bool isObjectDetected();
        void printMultizoneTofResult(VL53L7CX_ResultsData *Result);
        void turnOffMultizone();

    private:
        ImuSensor imu;
        MultizoneTofSensor multizoneTofSensor;
        SideTimeOfFlightSensor leftSideTofSensor;
        SideTimeOfFlightSensor rightSideTofSensor;
        ColorSensor colorSensor;
        SensorInitializer& initializer;

        // Private constructor - now modified to store and pass the Wire instance
        Sensors() : initializer(SensorInitializer::getInstance())
        { initialize(); }

        void initialize();
};
