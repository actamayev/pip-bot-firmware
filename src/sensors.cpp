#include "./include/sensors.h"

void Sensors::initialize() {
    // Setup I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_CLOCK_SPEED);

    // Initialize sensors
    initializeTofSensors();
    initializeIMU();
}

void Sensors::initializeTofSensors() {
    Serial.println("Initializing Left TOF sensor...");

    if (!leftTof.initialize()) {
        Serial.println("Failed to initialize left TOF sensor");
        return;
    }

    Serial.println("TOF setup complete");
}

void Sensors::initializeIMU() {
    Serial.println("Initializing IMU...");

    if (!imu.initialize()) {
        Serial.println("IMU initialization failed");
        return;
    }
    Serial.println("IMU setup complete");
}

bool Sensors::getTofData(const VL53L5CX_ResultsData** leftData, const VL53L5CX_ResultsData** rightData) {
    bool leftSuccess = leftTof.getData();
    // bool rightSuccess = rightTof.getData();
    
    if (leftSuccess) *leftData = &leftTof.sensorData;
    // if (rightSuccess) *rightData = &rightTof.sensorData;

    return leftSuccess;// && rightSuccess;
}

bool Sensors::getQuaternion(float& qX, float& qY, float& qZ, float& qW) {
    return imu.getQuaternion(qX, qY, qZ, qW);
}

bool Sensors::getAcceleration(float& aX, float& aY, float& aZ) {
    return imu.getAcceleration(aX, aY, aZ);
}

bool Sensors::getGyroscope(float& gX, float& gY, float& gZ) {
    return imu.getGyroscope(gX, gY, gZ);
}

bool Sensors::getMagneticField(float& mX, float& mY, float& mZ) {
    return imu.getMagneticField(mX, mY, mZ);
}

// Raw sensor value access if needed
bool Sensors::getImuData() {
    return imu.getData();
}

const sh2_SensorValue_t& Sensors::getImuSensorValue() const {
    return imu.getSensorValue();
}
