#pragma once
#include "./imu.h"
#include "./config.h"
#include "./singleton.h"
#include "./ir_sensor.h"
#include "./time_of_flight_sensor.h"

class Sensors : public Singleton<Sensors> {
    friend class Singleton<Sensors>;

    public:
        // TOF methods
        bool getTofData(const VL53L5CX_ResultsData** leftData, const VL53L5CX_ResultsData** rightData);

        // IMU methods
        const QuaternionData& getQuaternion();
        const AccelerometerData& getAcceleration();
        const GyroscopeData& getGyroscope();
        const MagnetometerData& getMagneticField();

        // Raw sensor value access if needed
        bool getImuData();
        bool getIrData();
        void sendIrCommand(uint32_t command);

        const sh2_SensorValue_t& getImuSensorValue() const;

    private:
        // Sensors
        TimeOfFlightSensor leftTof;
        TimeOfFlightSensor rightTof;
        ImuSensor imu;
        IrSensor irSensor;

        // Private constructor
        Sensors() : leftTof(), rightTof() { initialize(); }

        void initialize();
        void initializeTofSensors();
        void initializeIMU();
        void initializeIrSensors();
};
