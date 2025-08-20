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

// TOF update method (existing)
void SensorDataBuffer::updateTofData(const TofData& tof) {
    currentTofData = tof;
    markTofDataUpdated();
}

// Side TOF update method (existing)
void SensorDataBuffer::updateSideTofData(const SideTofData& sideTof) {
    currentSideTofData = sideTof;
    markSideTofDataUpdated();
}

// NEW: Color sensor update method
void SensorDataBuffer::updateColorData(const ColorData& color) {
    currentColorData = color;
    markColorDataUpdated();
}

// NEW: IR sensor update method
void SensorDataBuffer::updateIrData(const IrData& ir) {
    currentIrData = ir;
    markIrDataUpdated();
}

// NEW: Encoder update method
void SensorDataBuffer::updateEncoderData(const EncoderData& encoder) {
    currentEncoderData = encoder;
    markEncoderDataUpdated();
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

// TOF Read methods - reset timeouts when called (existing)
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

// Side TOF Read methods - reset timeouts when called (existing)
SideTofData SensorDataBuffer::getLatestSideTofData() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData;
}

uint16_t SensorDataBuffer::getLatestLeftSideTofCounts() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData.leftCounts;
}

uint16_t SensorDataBuffer::getLatestRightSideTofCounts() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData.rightCounts;
}

bool SensorDataBuffer::isLeftSideTofValid() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData.leftValid;
}

bool SensorDataBuffer::isRightSideTofValid() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData.rightValid;
}

// NEW: Color sensor Read methods - reset timeouts when called
ColorData SensorDataBuffer::getLatestColorData() {
    timeouts.color_last_request.store(millis());
    return currentColorData;
}

uint8_t SensorDataBuffer::getLatestRedValue() {
    timeouts.color_last_request.store(millis());
    return currentColorData.redValue;
}

uint8_t SensorDataBuffer::getLatestGreenValue() {
    timeouts.color_last_request.store(millis());
    return currentColorData.greenValue;
}

uint8_t SensorDataBuffer::getLatestBlueValue() {
    timeouts.color_last_request.store(millis());
    return currentColorData.blueValue;
}

bool SensorDataBuffer::isColorDataValid() {
    timeouts.color_last_request.store(millis());
    return currentColorData.isValid;
}

// NEW: IR sensor Read methods - reset timeouts when called
IrData SensorDataBuffer::getLatestIrData() {
    timeouts.ir_last_request.store(millis());
    return currentIrData;
}

float SensorDataBuffer::getLatestIrSensorReading(uint8_t index) {
    timeouts.ir_last_request.store(millis());
    if (index < 5) {
        return currentIrData.sensorReadings[index];
    }
    return 0.0f;
}

float* SensorDataBuffer::getLatestIrSensorReadings() {
    timeouts.ir_last_request.store(millis());
    return currentIrData.sensorReadings;
}

bool SensorDataBuffer::isIrDataValid() {
    timeouts.ir_last_request.store(millis());
    return currentIrData.isValid;
}

// NEW: Encoder Read methods - reset timeouts when called
EncoderData SensorDataBuffer::getLatestEncoderData() {
    timeouts.encoder_last_request.store(millis());
    return currentEncoderData;
}

WheelRPMs SensorDataBuffer::getLatestWheelRPMs() {
    timeouts.encoder_last_request.store(millis());
    WheelRPMs rpms;
    rpms.leftWheelRPM = currentEncoderData.leftWheelRPM;
    rpms.rightWheelRPM = currentEncoderData.rightWheelRPM;
    return rpms;
}

float SensorDataBuffer::getLatestLeftWheelRPM() {
    timeouts.encoder_last_request.store(millis());
    return currentEncoderData.leftWheelRPM;
}

float SensorDataBuffer::getLatestRightWheelRPM() {
    timeouts.encoder_last_request.store(millis());
    return currentEncoderData.rightWheelRPM;
}

float SensorDataBuffer::getLatestDistanceTraveledCm() {
    timeouts.encoder_last_request.store(millis());
    return currentEncoderData.distanceTraveledCm;
}

bool SensorDataBuffer::isEncoderDataValid() {
    timeouts.encoder_last_request.store(millis());
    return currentEncoderData.isValid;
}

// Raw encoder count access methods (for motor driver)
int64_t SensorDataBuffer::getLatestLeftEncoderCount() {
    timeouts.encoder_last_request.store(millis());
    return currentEncoderData.leftEncoderCount;
}

int64_t SensorDataBuffer::getLatestRightEncoderCount() {
    timeouts.encoder_last_request.store(millis());
    return currentEncoderData.rightEncoderCount;
}

std::pair<int64_t, int64_t> SensorDataBuffer::getLatestEncoderCounts() {
    timeouts.encoder_last_request.store(millis());
    return std::make_pair(currentEncoderData.leftEncoderCount, currentEncoderData.rightEncoderCount);
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
    timeouts.tof_last_request.store(currentTime);
    timeouts.side_tof_last_request.store(currentTime);
    timeouts.color_last_request.store(currentTime);  // Include color sensor
    timeouts.ir_last_request.store(currentTime);  // Include IR sensor
    timeouts.encoder_last_request.store(currentTime);  // Include encoder
}


void SensorDataBuffer::markDataUpdated() {
    lastUpdateTime.store(millis());
}

void SensorDataBuffer::markTofDataUpdated() {
    lastTofUpdateTime.store(millis());
}

void SensorDataBuffer::markSideTofDataUpdated() {
    lastSideTofUpdateTime.store(millis());
}

void SensorDataBuffer::markColorDataUpdated() {
    lastColorUpdateTime.store(millis());
}

void SensorDataBuffer::markIrDataUpdated() {
    lastIrUpdateTime.store(millis());
}

void SensorDataBuffer::markEncoderDataUpdated() {
    lastEncoderUpdateTime.store(millis());
}
