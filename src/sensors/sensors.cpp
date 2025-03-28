#include "./sensors.h"
#include "../utils/utils.h"

void Sensors::initialize() {
    // Initialize sensors
    // initializeMultizoneTof();
    // initializeIMU();
    // initializeColorSensor();
    // initializeSideTimeOfFlights();
}

void Sensors::initializeMultizoneTof() {
    Serial.println("Initializing Multizone sensor...");

    if (!multizoneTofSensor.initialize()) {
        Serial.println("Multizone sensor initialization failed");
        return;
    }

    Serial.println("Multizone sensor setup complete");
}

void Sensors::initializeIMU() {
    Serial.println("Initializing IMU...");

    if (!imu.initialize()) {
        Serial.println("IMU initialization failed");
        return;
    }
    Serial.println("IMU setup complete");
}

void Sensors::initializeColorSensor() {
    Serial.println("Initializing Color Sensor...");

    if (!colorSensor.initialize()) {
        Serial.println("Color Sensor initialization failed");
        return;
    }
    Serial.println("Color Sensor setup complete");
}

void Sensors::initializeSideTimeOfFlights() {
    Serial.println("Initializing left side TOF...");
    if (!leftSideTofSensor.initialize(LEFT_TOF_ADDRESS)) {
        Serial.println("Left TOF initialization failed");
        return;
    }
    Serial.println("Initializing right side TOF...");
    if (!rightSideTofSensor.initialize(RIGHT_TOF_ADDRESS)) {
        Serial.println("Right TOF initialization failed");
        return;
    }
    Serial.println("Side TOF setup complete");
}

VL53L5CX_ResultsData Sensors::getMultizoneTofData() {
    return multizoneTofSensor.getTofData();
}

float Sensors::getPitch() {
    return imu.getPitch();
}

float Sensors::getYaw() {
    return imu.getYaw();
}

float Sensors::getRoll() {
    return imu.getRoll();
}

const EulerAngles& Sensors::getEulerAngles() {
    return imu.getEulerAngles();
}

float Sensors::getXAccel() {
    return imu.getXAccel();
}

float Sensors::getYAccel() {
    return imu.getYAccel();
}

float Sensors::getZAccel() {
    return imu.getZAccel();
}

double Sensors::getAccelMagnitude() {
    return imu.getAccelMagnitude();
}

const AccelerometerData& Sensors::getAccelerometerData() {
    return imu.getAccelerometerData();
}

float Sensors::getXRotationRate() {
    return imu.getXRotationRate();
}

float Sensors::getYRotationRate() {
    return imu.getYRotationRate();
}

float Sensors::getZRotationRate() {
    return imu.getZRotationRate();
}

const GyroscopeData& Sensors::getGyroscopeData() {
    return imu.getGyroscopeData();
}

float Sensors::getMagneticFieldX() {
    return imu.getMagneticFieldX();
}

float Sensors::getMagneticFieldY() {
    return imu.getMagneticFieldY();
}

float Sensors::getMagneticFieldZ() {
    return imu.getMagneticFieldZ();
}

const MagnetometerData& Sensors::getMagnetometerData() {
    return imu.getMagnetometerData();
}

ColorSensorData Sensors::getColorSensorData() {
    return colorSensor.getSensorData();
}

SideTofDistances Sensors::getBothSideTofDistances() {
    uint16_t leftDistance = leftSideTofSensor.getDistance();
    uint16_t rightDistance = rightSideTofSensor.getDistance();

    return {
        leftDistance,
        rightDistance
    };
}
