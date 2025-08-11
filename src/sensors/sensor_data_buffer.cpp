#include "sensor_data_buffer.h"

// IMU update methods (existing)
void SensorDataBuffer::updateQuaternion(const QuaternionData& quaternion) {
    currentSample.quaternion = quaternion;
    // Update derived Euler angles if quaternion is valid
    if (quaternion.isValid) {
        quaternionToEuler(
            quaternion.qW, quaternion.qX, quaternion.qY, quaternion.qZ,
            currentSample.eulerAngles.yaw, 
            currentSample.eulerAngles.pitch, 
            currentSample.eulerAngles.roll
        );
        currentSample.eulerAngles.isValid = true;
    }
    markDataUpdated();
}

void SensorDataBuffer::updateAccelerometer(const AccelerometerData& accel) {
    currentSample.accelerometer = accel;
    markDataUpdated();
}

void SensorDataBuffer::updateGyroscope(const GyroscopeData& gyro) {
    currentSample.gyroscope = gyro;
    markDataUpdated();
}

void SensorDataBuffer::updateMagnetometer(const MagnetometerData& mag) {
    currentSample.magnetometer = mag;
    markDataUpdated();
}

// NEW: TOF update method
void SensorDataBuffer::updateTofData(const TofData& tof) {
    currentTofData = tof;
    markTofDataUpdated();
}

// IMU Read methods - reset timeouts when called (existing)
EulerAngles SensorDataBuffer::getLatestEulerAngles() {
    timeouts.quaternion_last_request.store(millis());
    return currentSample.eulerAngles;
}

QuaternionData SensorDataBuffer::getLatestQuaternion() {
    timeouts.quaternion_last_request.store(millis());
    return currentSample.quaternion;
}

AccelerometerData SensorDataBuffer::getLatestAccelerometer() {
    timeouts.accelerometer_last_request.store(millis());
    return currentSample.accelerometer;
}

GyroscopeData SensorDataBuffer::getLatestGyroscope() {
    timeouts.gyroscope_last_request.store(millis());
    return currentSample.gyroscope;
}

MagnetometerData SensorDataBuffer::getLatestMagnetometer() {
    timeouts.magnetometer_last_request.store(millis());
    return currentSample.magnetometer;
}

// NEW: TOF Read methods - reset timeouts when called
TofData SensorDataBuffer::getLatestTofData() {
    timeouts.tof_last_request.store(millis());
    return currentTofData;
}

VL53L7CX_ResultsData SensorDataBuffer::getLatestTofRawData() {
    timeouts.tof_last_request.store(millis());
    return currentTofData.rawData;
}

bool SensorDataBuffer::isObjectDetectedTof() {
    timeouts.tof_last_request.store(millis());
    return currentTofData.isObjectDetected && currentTofData.isValid;
}

// Convenience methods for individual values (existing)
float SensorDataBuffer::getLatestPitch() {
    return getLatestEulerAngles().roll;  // Note: roll maps to pitch in your system
}

float SensorDataBuffer::getLatestYaw() {
    return getLatestEulerAngles().yaw;
}

float SensorDataBuffer::getLatestRoll() {
    return getLatestEulerAngles().pitch;  // Note: pitch maps to roll in your system
}

float SensorDataBuffer::getLatestXAccel() {
    return getLatestAccelerometer().aX;
}

float SensorDataBuffer::getLatestYAccel() {
    return getLatestAccelerometer().aY;
}

float SensorDataBuffer::getLatestZAccel() {
    return getLatestAccelerometer().aZ;
}

float SensorDataBuffer::getLatestXRotationRate() {
    return getLatestGyroscope().gX;
}

float SensorDataBuffer::getLatestYRotationRate() {
    return getLatestGyroscope().gY;
}

float SensorDataBuffer::getLatestZRotationRate() {
    return getLatestGyroscope().gZ;
}

double SensorDataBuffer::getLatestAccelMagnitude() {
    return sqrt(pow(getLatestXAccel(), 2) + pow(getLatestYAccel(), 2) + pow(getLatestZAccel(), 2));
}

float SensorDataBuffer::getLatestMagneticFieldX() {
    return getLatestMagnetometer().mX;
}

float SensorDataBuffer::getLatestMagneticFieldY() {
    return getLatestMagnetometer().mY;
}

float SensorDataBuffer::getLatestMagneticFieldZ() {
    return getLatestMagnetometer().mZ;
}

ImuSample SensorDataBuffer::getLatestImuSample() {
    // Mark all timeouts as accessed
    uint32_t currentTime = millis();
    timeouts.quaternion_last_request.store(currentTime);
    timeouts.accelerometer_last_request.store(currentTime);
    timeouts.gyroscope_last_request.store(currentTime);
    timeouts.magnetometer_last_request.store(currentTime);
    
    return currentSample;
}

// Helper methods for bulk polling control
void SensorDataBuffer::startPollingAllSensors() {
    uint32_t currentTime = millis();
    timeouts.quaternion_last_request.store(currentTime);
    timeouts.accelerometer_last_request.store(currentTime);
    timeouts.gyroscope_last_request.store(currentTime);
    timeouts.magnetometer_last_request.store(currentTime);
    timeouts.tof_last_request.store(currentTime);  // Include TOF
}

void SensorDataBuffer::stopPollingAllSensors() {
    // Set timeouts to expired (more than 1 minute ago)
    uint32_t expiredTime = millis() - (ReportTimeouts::TIMEOUT_MS + 1000);
    timeouts.quaternion_last_request.store(expiredTime);
    timeouts.accelerometer_last_request.store(expiredTime);
    timeouts.gyroscope_last_request.store(expiredTime);
    timeouts.magnetometer_last_request.store(expiredTime);
    timeouts.tof_last_request.store(expiredTime);  // Include TOF
}

void SensorDataBuffer::markDataUpdated() {
    lastUpdateTime.store(millis());
}

void SensorDataBuffer::markTofDataUpdated() {
    lastTofUpdateTime.store(millis());
}
