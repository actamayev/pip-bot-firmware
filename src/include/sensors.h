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

        // Raw sensor value access if needed
        bool getIrData();
        void sendIrCommand(uint32_t command);

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
