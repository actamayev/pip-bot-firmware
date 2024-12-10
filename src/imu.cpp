#include "./include/imu.h"

bool ImuSensor::initialize() {
    if (!imu.begin_I2C()) {
        Serial.println("Failed to find BNO08x chip");
        return false;
    }

    Serial.println("BNO08x Found!");
    isInitialized = true;
    return true;
}

bool ImuSensor::enableGameRotationVector(uint32_t updateFreqMicros) {
    if (!isInitialized) return false;

    if (enabledReports.gameRotationVector == true) return true;

    if (!imu.enableReport(SH2_GAME_ROTATION_VECTOR, updateFreqMicros)) {
        Serial.println("Could not enable game rotation vector");
        return false;
    }
    
    enabledReports.gameRotationVector = true;
    return true;
}

bool ImuSensor::enableAccelerometer(uint32_t updateFreqMicros) {
    if (!isInitialized) return false;
    
    if (enabledReports.accelerometer == true) return true;

    if (!imu.enableReport(SH2_ACCELEROMETER, updateFreqMicros)) {
        Serial.println("Could not enable accelerometer");
        return false;
    }
    
    enabledReports.accelerometer = true;
    return true;
}

bool ImuSensor::enableGyroscope(uint32_t updateFreqMicros) {
    if (!isInitialized) return false;
    
    if (enabledReports.gyroscope == true) return true;

    if (!imu.enableReport(SH2_GYROSCOPE_CALIBRATED, updateFreqMicros)) {
        Serial.println("Could not enable gyroscope");
        return false;
    }
    
    enabledReports.gyroscope = true;
    return true;
}

bool ImuSensor::enableMagneticField(uint32_t updateFreqMicros) {
    if (!isInitialized) return false;
    
    if (enabledReports.magneticField == true) return true;

    if (!imu.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, updateFreqMicros)) {
        Serial.println("Could not enable magnetic field");
        return false;
    }
    
    enabledReports.magneticField = true;
    return true;
}

bool ImuSensor::getData() {
    if (!isInitialized) return false;
    return imu.getSensorEvent(&sensorValue);
}

const sh2_SensorValue_t& ImuSensor::getSensorValue() const {
    return sensorValue;
}

// Convenience methods to get specific data types
bool ImuSensor::getQuaternion(float& qX, float& qY, float& qZ, float& qW) {
    if (!enableGameRotationVector()) return false;

    if (getData() && sensorValue.sensorId == SH2_GAME_ROTATION_VECTOR) {
        qX = sensorValue.un.gameRotationVector.i;
        qY = sensorValue.un.gameRotationVector.j;
        qZ = sensorValue.un.gameRotationVector.k;
        qW = sensorValue.un.gameRotationVector.real;
        return true;
    }
    return false;
}

bool ImuSensor::getAcceleration(float& aX, float& aY, float& aZ) {
    if (!enableAccelerometer()) return false;
    
    if (getData() && sensorValue.sensorId == SH2_ACCELEROMETER) {
        aX = sensorValue.un.accelerometer.x;
        aY = sensorValue.un.accelerometer.y;
        aZ = sensorValue.un.accelerometer.z;
        return true;
    }
    return false;
}

bool ImuSensor::getGyroscope(float& gX, float& gY, float& gZ) {
    if (!enableGyroscope()) return false;
    
    if (getData() && sensorValue.sensorId == SH2_GYROSCOPE_CALIBRATED) {
        gX = sensorValue.un.gyroscope.x;
        gY = sensorValue.un.gyroscope.y;
        gZ = sensorValue.un.gyroscope.z;
        return true;
    }
    return false;
}

bool ImuSensor::getMagneticField(float& mX, float& mY, float& mZ) {
    if (!enableMagneticField()) return false;

    if (getData() && sensorValue.sensorId == SH2_MAGNETIC_FIELD_CALIBRATED) {
        mX = sensorValue.un.magneticField.x;
        mY = sensorValue.un.magneticField.y;
        mZ = sensorValue.un.magneticField.z;
        return true;
    }
    return false;
}
