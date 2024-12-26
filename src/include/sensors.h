#pragma once
#include "./imu.h"
#include "./config.h"
#include "./singleton.h"
#include "./time_of_flight_sensor.h"

class Sensors : public Singleton<Sensors> {
    friend class Singleton<Sensors>;

    public:
        // TOF methods
        bool getTofData(const VL53L5CX_ResultsData** leftData, const VL53L5CX_ResultsData** rightData);

        // IMU methods
        bool getQuaternion(float& qX, float& qY, float& qZ, float& qW);
        bool getAcceleration(float& aX, float& aY, float& aZ);
        bool getGyroscope(float& gX, float& gY, float& gZ);
        bool getMagneticField(float& mX, float& mY, float& mZ);

        // Raw sensor value access if needed
        bool getImuData();

        const sh2_SensorValue_t& getImuSensorValue() const;

    private:
        // Sensors
        TimeOfFlightSensor leftTof;
        TimeOfFlightSensor rightTof;
        ImuSensor imu;

        // Private constructor
        Sensors() : leftTof(), rightTof() { initialize(); }

        void initialize();
        void initializeTofSensors();
        void initializeIMU();
};
