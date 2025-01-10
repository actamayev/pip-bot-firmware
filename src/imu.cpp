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

bool ImuSensor::enableGameRotationVector() {
    if (!isInitialized) return false;

    if (enabledReports.gameRotationVector == true) return true;

    if (!imu.enableReport(SH2_GAME_ROTATION_VECTOR, IMU_UPDATE_FREQ_MICROSECS)) {
        Serial.println("Could not enable game rotation vector");
        return false;
    }
    
    enabledReports.gameRotationVector = true;
    return true;
}

bool ImuSensor::enableAccelerometer() {
    if (!isInitialized) return false;
    
    if (enabledReports.accelerometer == true) return true;

    if (!imu.enableReport(SH2_ACCELEROMETER, IMU_UPDATE_FREQ_MICROSECS)) {
        Serial.println("Could not enable accelerometer");
        return false;
    }
    
    enabledReports.accelerometer = true;
    return true;
}

bool ImuSensor::enableGyroscope() {
    if (!isInitialized) return false;
    
    if (enabledReports.gyroscope == true) return true;

    if (!imu.enableReport(SH2_GYROSCOPE_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
        Serial.println("Could not enable gyroscope");
        return false;
    }
    
    enabledReports.gyroscope = true;
    return true;
}

bool ImuSensor::enableMagneticField() {
    if (!isInitialized) return false;
    
    if (enabledReports.magneticField == true) return true;

    if (!imu.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
        Serial.println("Could not enable magnetic field");
        return false;
    }
    
    enabledReports.magneticField = true;
    return true;
}

bool ImuSensor::getImuData() {
    if (!isInitialized) return false;
    return imu.getSensorEvent(&sensorValue);
}

const QuaternionData& ImuSensor::getQuaternion() {
    updateQuaternion();
    return currentQuaternion;
}

// Convenience methods to get specific data types
bool ImuSensor::updateQuaternion() {
    if (!enableGameRotationVector()) return false;

    if (!getImuData() || sensorValue.sensorId != SH2_GAME_ROTATION_VECTOR) {
        currentQuaternion.isValid = false;
        return false;
    }
    currentQuaternion.qX = sensorValue.un.gameRotationVector.i;
    currentQuaternion.qY = sensorValue.un.gameRotationVector.j;
    currentQuaternion.qZ = sensorValue.un.gameRotationVector.k;
    currentQuaternion.qW = sensorValue.un.gameRotationVector.real;
    currentQuaternion.isValid = true;
    return true;
}

EulerAngles& ImuSensor::getEulerAngles() {
    static EulerAngles angles = {0, 0, 0, false}; // Use static for return reference validity

    // Use getQuaternion to ensure the quaternion is updated
    const QuaternionData& quaternion = getQuaternion();

    if (!quaternion.isValid) {
        angles.isValid = false;
    } else {
        quaternionToEuler(
            quaternion.qW, 
            quaternion.qX, 
            quaternion.qY, 
            quaternion.qZ,
            angles.yaw, angles.pitch, angles.roll
        );
        angles.isValid = true;
    }

    return angles;
}

float ImuSensor::getPitch() {
    return getEulerAngles().pitch;
}

float ImuSensor::getYaw() {
    return getEulerAngles().yaw;
}

float ImuSensor::getRoll() {
    return getEulerAngles().roll;
}

const AccelerometerData& ImuSensor::getAccelerometerData() {
    updateAccelerometer();
    return currentAccelData;
}

bool ImuSensor::updateAccelerometer() {
    if (!enableAccelerometer()) return false;

    if (!getImuData() || sensorValue.sensorId != SH2_ACCELEROMETER) {
        currentAccelData.isValid = false;
        return false;
    }
    currentAccelData.aX = sensorValue.un.accelerometer.x;
    currentAccelData.aY = sensorValue.un.accelerometer.y;
    currentAccelData.aZ = sensorValue.un.accelerometer.z;
    currentAccelData.isValid = true;
    return true;
}

float ImuSensor::getXAccel() {
    return currentAccelData.aX;
}

float ImuSensor::getYAccel() {
    return currentAccelData.aY;
}

float ImuSensor::getZAccel() {
    return currentAccelData.aZ;
}

double ImuSensor::getAccelMagnitude() {
    return sqrt(pow(getXAccel(), 2) + pow(getYAccel(), 2) + pow(getZAccel(), 2));
}

const GyroscopeData& ImuSensor::getGyroscopeData() {
    updateGyroscope();
    return currentGyroData;
}

bool ImuSensor::updateGyroscope() {
    if (!enableGyroscope()) return false;
    
    if (!getImuData() || sensorValue.sensorId != SH2_GYROSCOPE_CALIBRATED) {
        currentGyroData.isValid = false;
        return false;
    }
    currentGyroData.gX = sensorValue.un.gyroscope.x;
    currentGyroData.gY = sensorValue.un.gyroscope.y;
    currentGyroData.gZ = sensorValue.un.gyroscope.z;
    currentGyroData.isValid = true;
    return true;
}

float ImuSensor::getXRotationRate() {
    return currentGyroData.gX;
}

float ImuSensor::getYRotationRate() {
    return currentGyroData.gY;
}

float ImuSensor::getZRotationRate() {
    return currentGyroData.gZ;
}

const MagnetometerData& ImuSensor::getMagnetometerData() {
    updateMagnetometer();
    return currentMagnetometer;
}

bool ImuSensor::updateMagnetometer() {
    if (!enableMagneticField()) return false;

    if (!getImuData() || sensorValue.sensorId != SH2_MAGNETIC_FIELD_CALIBRATED) {
        currentMagnetometer.isValid = false;
        return false;
    }
    currentMagnetometer.mX = sensorValue.un.magneticField.x;
    currentMagnetometer.mY = sensorValue.un.magneticField.y;
    currentMagnetometer.mZ = sensorValue.un.magneticField.z;
    currentGyroData.isValid = true;
    return true;
}

float ImuSensor::getMagneticFieldX() {
    return currentMagnetometer.mX;
}

float ImuSensor::getMagneticFieldY() {
    return currentMagnetometer.mY;
}

float ImuSensor::getMagneticFieldZ() {
    return currentMagnetometer.mZ;
}
