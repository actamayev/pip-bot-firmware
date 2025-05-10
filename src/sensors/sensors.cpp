#include "./sensors.h"
#include "../utils/utils.h"

// void Sensors::initialize() {
//     // Initialize sensors using the initializer
//     // initializer.initializeMultizoneTof(multizoneTofSensor);
//     initializer.initializeIMU(imu);
//     // initializer.initializeColorSensor(colorSensor);
//     initializer.initializeSideTimeOfFlights(leftSideTofSensor, rightSideTofSensor);
// }

// bool Sensors::isSensorInitialized(SensorInitializer::SensorType sensor) const {
//     return initializer.isSensorInitialized(sensor);
// }

// bool Sensors::areAllSensorsInitialized() const {
//     return initializer.areAllSensorsInitialized();
// }

// bool Sensors::tryInitializeIMU() {
//     return initializer.tryInitializeIMU(imu);
// }

// bool Sensors::tryInitializeLeftSideTof() {
//     return initializer.tryInitializeLeftSideTof(leftSideTofSensor);
// }

// bool Sensors::tryInitializeRightSideTof() {
//     return initializer.tryInitializeRightSideTof(rightSideTofSensor);
// }

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

uint16_t Sensors::getLeftSideTofCounts() {
    return leftSideTofSensor.getCounts();
}

uint16_t Sensors::getRightSideTofCounts() {
    return rightSideTofSensor.getCounts();
}

SideTofCounts Sensors::getBothSideTofCounts() {
    uint16_t leftCounts = leftSideTofSensor.getCounts();
    uint16_t rightCounts = rightSideTofSensor.getCounts();

    return {
        leftCounts,
        rightCounts
    };
}

void Sensors::turnOffImu() {
    imu.turnOff();
}

void Sensors::turnOffSideTofs() {
    leftSideTofSensor.turnSensorOff();
    rightSideTofSensor.turnSensorOff();
}
