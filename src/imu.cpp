#include "./include/imu.h"

bool ImuSensor::initialize() {
    if (!imu.begin_I2C(IMU_DEFAULT_ADDRESS)) {
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
bool ImuSensor::updateQuaternion() {
    if (!enableGameRotationVector()) return false;

    if (getData() && sensorValue.sensorId == SH2_GAME_ROTATION_VECTOR) {
        currentQuaternion.qX = sensorValue.un.gameRotationVector.i;
        currentQuaternion.qY = sensorValue.un.gameRotationVector.j;
        currentQuaternion.qZ = sensorValue.un.gameRotationVector.k;
        currentQuaternion.qW = sensorValue.un.gameRotationVector.real;
        currentQuaternion.isValid = true;
        return true;
    }
    currentQuaternion.isValid = false;
    return false;
}

bool ImuSensor::updateAccelerometer() {
    if (!enableAccelerometer()) return false;
    
    if (getData() && sensorValue.sensorId == SH2_ACCELEROMETER) {
        currentAccelData.aX = sensorValue.un.accelerometer.x;
        currentAccelData.aY = sensorValue.un.accelerometer.y;
        currentAccelData.aZ = sensorValue.un.accelerometer.z;
        currentAccelData.isValid = true;
        return true;
    }
    currentAccelData.isValid = false;
    return false;
}

bool ImuSensor::updateGyroscope() {
    if (!enableGyroscope()) return false;
    
    if (getData() && sensorValue.sensorId == SH2_GYROSCOPE_CALIBRATED) {
        currentGyroData.gX = sensorValue.un.gyroscope.x;
        currentGyroData.gY = sensorValue.un.gyroscope.y;
        currentGyroData.gZ = sensorValue.un.gyroscope.z;
        currentGyroData.isValid = true;
        return true;
    }
    currentGyroData.isValid = false;
    return false;
}

bool ImuSensor::updateMagnetometer() {
    if (!enableMagneticField()) return false;

    if (getData() && sensorValue.sensorId == SH2_MAGNETIC_FIELD_CALIBRATED) {
        currentMagnetometer.mX = sensorValue.un.magneticField.x;
        currentMagnetometer.mY = sensorValue.un.magneticField.y;
        currentMagnetometer.mZ = sensorValue.un.magneticField.z;
        currentGyroData.isValid = true;
        return true;
    }
    currentMagnetometer.isValid = false;
    return false;
}
