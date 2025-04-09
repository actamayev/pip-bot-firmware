#include "./imu.h"

bool ImuSensor::canRetryInitialization() const {
    if (isInitialized) return false;

    unsigned long currentTime = millis();
    if (currentTime - lastInitAttempt < INIT_RETRY_INTERVAL) {
        return false; // Too soon to retry
    }

    if (initRetryCount >= MAX_INIT_RETRIES) {
        return false; // Too many retries
    }

    return true;
}

bool ImuSensor::initialize() {
    lastInitAttempt = millis();
    initRetryCount++;
    
    // Add a delay before trying to initialize
    delay(50);
    
    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        if (imu.begin_I2C(IMU_DEFAULT_ADDRESS)) {
            Serial.println("BNO08x Found!");
            isInitialized = true;
            return true;
        }
        delay(20);
    }
    
    Serial.printf("Failed to find BNO08x chip (retry %d of %d)\n", 
                 initRetryCount, MAX_INIT_RETRIES);
    scanI2C();
    return false;
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

const EulerAngles& ImuSensor::getEulerAngles() {
    updateAllSensorData(); // Non-blocking update
    return currentEulerAngles;
}

const AccelerometerData& ImuSensor::getAccelerometerData() {
    updateAllSensorData(); // Non-blocking update
    return currentAccelData;
}

const GyroscopeData& ImuSensor::getGyroscopeData() {
    updateAllSensorData(); // Non-blocking update
    return currentGyroData;
}

const MagnetometerData& ImuSensor::getMagnetometerData() {
    updateAllSensorData(); // Non-blocking update
    return currentMagnetometer;
}

float ImuSensor::getPitch() {
    return getEulerAngles().roll;
}

float ImuSensor::getYaw() {
    return getEulerAngles().yaw;
}

float ImuSensor::getRoll() {
    return getEulerAngles().pitch;
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

float ImuSensor::getXRotationRate() {
    return currentGyroData.gX;
}

float ImuSensor::getYRotationRate() {
    return currentGyroData.gY;
}

float ImuSensor::getZRotationRate() {
    return currentGyroData.gZ;
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

// Add this implementation to imu.cpp:
bool ImuSensor::updateAllSensorData() {
    if (!isInitialized) return false;

    // Enable all reports we need
    enableGameRotationVector();
    enableAccelerometer();
    enableGyroscope();
    enableMagneticField();

    // Update each data type
    bool updated = false;
    
    // Increased attempts to ensure we get all data types in one call
    // Since we're now skipping the internal rate limiting, this is more important
    for (int i = 0; i < 8; i++) {
        if (getImuData()) {
            switch (sensorValue.sensorId) {
                case SH2_GAME_ROTATION_VECTOR:
                    currentQuaternion.qX = sensorValue.un.gameRotationVector.i;
                    currentQuaternion.qY = sensorValue.un.gameRotationVector.j;
                    currentQuaternion.qZ = sensorValue.un.gameRotationVector.k;
                    currentQuaternion.qW = sensorValue.un.gameRotationVector.real;
                    currentQuaternion.isValid = true;
                    
                    // Update Euler angles
                    quaternionToEuler(
                        currentQuaternion.qW, 
                        currentQuaternion.qX, 
                        currentQuaternion.qY, 
                        currentQuaternion.qZ,
                        currentEulerAngles.yaw, 
                        currentEulerAngles.pitch, 
                        currentEulerAngles.roll
                    );
                    currentEulerAngles.isValid = true;
                    updated = true;
                    break;
                    
                case SH2_ACCELEROMETER:
                    currentAccelData.aX = sensorValue.un.accelerometer.x;
                    currentAccelData.aY = sensorValue.un.accelerometer.y;
                    currentAccelData.aZ = sensorValue.un.accelerometer.z;
                    currentAccelData.isValid = true;
                    updated = true;
                    break;
                    
                case SH2_GYROSCOPE_CALIBRATED:
                    currentGyroData.gX = sensorValue.un.gyroscope.x;
                    currentGyroData.gY = sensorValue.un.gyroscope.y;
                    currentGyroData.gZ = sensorValue.un.gyroscope.z;
                    currentGyroData.isValid = true;
                    updated = true;
                    break;
                    
                case SH2_MAGNETIC_FIELD_CALIBRATED:
                    currentMagnetometer.mX = sensorValue.un.magneticField.x;
                    currentMagnetometer.mY = sensorValue.un.magneticField.y;
                    currentMagnetometer.mZ = sensorValue.un.magneticField.z;
                    currentMagnetometer.isValid = true;
                    updated = true;
                    break;
            }
        }
    }
    
    return updated;
}
