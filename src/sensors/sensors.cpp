#include "./sensors.h"
#include "../utils/utils.h"

void Sensors::initialize() {
    // Setup I2C
    Wire.begin(I2C_SDA, I2C_SCL, I2C_CLOCK_SPEED);

    // Initialize sensors
    initializeMultizoneTof();
    // initializeTofSensors();
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
    Serial.println("Initializing side TOFs...");
    if (!sideTimeOfFlightSensors.initialize()) {
        Serial.println("Color Sensor initialization failed");
        return;
    }
    Serial.println("Color Sensor setup complete");
}

VL53L5CX_ResultsData Sensors::getMultizoneTofData() {
    return multizoneTofSensor.getTofData();
}

EulerAngles& Sensors::getEulerAngles() {
    return imu.getEulerAngles();
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

float Sensors::getXAccel() {
    return imu.getPitch();
}

float Sensors::getYAccel() {
    return imu.getYaw();
}

float Sensors::getZAccel() {
    return imu.getRoll();
}

double Sensors::getAccelMagnitude() {
    return imu.getAccelMagnitude();
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

float Sensors::getMagneticFieldX() {
    return imu.getMagneticFieldX();
}

float Sensors::getMagneticFieldY() {
    return imu.getMagneticFieldY();
}

float Sensors::getMagneticFieldZ() {
    return imu.getMagneticFieldZ();
}

ColorSensorData Sensors::getColorSensorData() {
    return colorSensor.getSensorData();
}

SideTofDistances Sensors::getSideTofDistances() {
    return sideTimeOfFlightSensors.getBothDistances();
}
