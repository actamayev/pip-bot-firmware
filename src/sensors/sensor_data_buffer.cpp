#include "sensor_data_buffer.h"
#include "networking/serial_manager.h"
#include "networking/websocket_manager.h"
#include "custom_interpreter/bytecode_vm.h"

// Use ColorType from ColorTypes namespace
using ColorTypes::ColorType;

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
    markImuDataUpdated();
}

void SensorDataBuffer::updateAccelerometer(const AccelerometerData& accel) {
    currentSample.accelerometer = accel;
    markImuDataUpdated();
}

void SensorDataBuffer::updateGyroscope(const GyroscopeData& gyro) {
    currentSample.gyroscope = gyro;
    markImuDataUpdated();
}

void SensorDataBuffer::updateMagnetometer(const MagnetometerData& mag) {
    currentSample.magnetometer = mag;
    markImuDataUpdated();
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
    return currentEncoderData;
}

WheelRPMs SensorDataBuffer::getLatestWheelRPMs() {
    WheelRPMs rpms;
    rpms.leftWheelRPM = currentEncoderData.leftWheelRPM;
    rpms.rightWheelRPM = currentEncoderData.rightWheelRPM;
    return rpms;
}

float SensorDataBuffer::getLatestLeftWheelRPM() {
    return currentEncoderData.leftWheelRPM;
}

float SensorDataBuffer::getLatestRightWheelRPM() {
    return currentEncoderData.rightWheelRPM;
}

float SensorDataBuffer::getLatestDistanceTraveledIn() {
    return currentEncoderData.distanceTraveledIn;
}

bool SensorDataBuffer::isEncoderDataValid() {
    return currentEncoderData.isValid;
}

// Raw encoder count access methods (for motor driver)
int64_t SensorDataBuffer::getLatestLeftEncoderCount() {
    return currentEncoderData.leftEncoderCount;
}

int64_t SensorDataBuffer::getLatestRightEncoderCount() {
    return currentEncoderData.rightEncoderCount;
}

std::pair<int64_t, int64_t> SensorDataBuffer::getLatestEncoderCounts() {
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

void SensorDataBuffer::stopPollingAllSensors() {
    stopPollingSensor(SensorType::QUATERNION);
    stopPollingSensor(SensorType::ACCELEROMETER);
    stopPollingSensor(SensorType::GYROSCOPE);
    stopPollingSensor(SensorType::MAGNETOMETER);
    stopPollingSensor(SensorType::MULTIZONE_TOF);
    stopPollingSensor(SensorType::SIDE_TOF);
    stopPollingSensor(SensorType::COLOR);
    stopPollingSensor(SensorType::IR);
}

void SensorDataBuffer::stopPollingSensor(SensorType sensorType) {
    switch (sensorType) {
        case SensorType::QUATERNION:
            timeouts.quaternion_last_request.store(0);
            break;
        case SensorType::ACCELEROMETER:
            timeouts.accelerometer_last_request.store(0);
            break;
        case SensorType::GYROSCOPE:
            timeouts.gyroscope_last_request.store(0);
            break;
        case SensorType::MAGNETOMETER:
            timeouts.magnetometer_last_request.store(0);
            break;
        case SensorType::MULTIZONE_TOF:
            timeouts.tof_last_request.store(0);
            break;
        case SensorType::SIDE_TOF:
            timeouts.side_tof_last_request.store(0);
            break;
        case SensorType::COLOR:
            timeouts.color_last_request.store(0);
            break;
        case SensorType::IR:
            timeouts.ir_last_request.store(0);
            break;
    }
}

void SensorDataBuffer::markImuDataUpdated() {
    lastImuUpdateTime.store(millis());
    imuUpdateCount.fetch_add(1);  // Increment frequency counter
}

void SensorDataBuffer::markTofDataUpdated() {
    lastTofUpdateTime.store(millis());
    multizoneTofUpdateCount.fetch_add(1);  // Increment frequency counter
}

void SensorDataBuffer::markSideTofDataUpdated() {
    lastSideTofUpdateTime.store(millis());
    sideTofUpdateCount.fetch_add(1);  // Increment frequency counter
}

void SensorDataBuffer::markColorDataUpdated() {
    lastColorUpdateTime.store(millis());
    colorSensorUpdateCount.fetch_add(1);  // Increment frequency counter
}

void SensorDataBuffer::markIrDataUpdated() {
    lastIrUpdateTime.store(millis());
}

void SensorDataBuffer::markEncoderDataUpdated() {
    lastEncoderUpdateTime.store(millis());
}

float SensorDataBuffer::getImuFrequency() {
    static float lastFrequency = 0.0f;
    static uint32_t lastUpdateCount = 0;
    
    uint32_t currentTime = millis();
    uint32_t lastCalcTime = lastImuFrequencyCalcTime.load();
    uint32_t currentUpdateCount = imuUpdateCount.load();
    
    // Initialize on first call
    if (lastCalcTime == 0) {
        lastImuFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        return 0.0f;
    }
    
    uint32_t timeDelta = currentTime - lastCalcTime;
    
    // Calculate frequency every second (1000ms)
    if (timeDelta >= 1000) {
        uint32_t updateDelta = currentUpdateCount - lastUpdateCount;
        lastFrequency = (float)updateDelta * 1000.0f / (float)timeDelta;
        
        // Debug logging
        char debugBuffer[128];
        snprintf(debugBuffer, sizeof(debugBuffer), "DEBUG: updateDelta=%u, timeDelta=%u, freq=%.1f", 
                 updateDelta, timeDelta, lastFrequency);
        SerialQueueManager::getInstance().queueMessage(debugBuffer);
        
        // Update tracking variables
        lastImuFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        
        return lastFrequency;
    }
    
    // Return last calculated frequency if not time to update yet
    return lastFrequency;
}

float SensorDataBuffer::getMultizoneTofFrequency() {
    static float lastFrequency = 0.0f;
    static uint32_t lastUpdateCount = 0;
    
    uint32_t currentTime = millis();
    uint32_t lastCalcTime = lastMultizoneTofFrequencyCalcTime.load();
    uint32_t currentUpdateCount = multizoneTofUpdateCount.load();
    
    // Initialize on first call
    if (lastCalcTime == 0) {
        lastMultizoneTofFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        return 0.0f;
    }
    
    uint32_t timeDelta = currentTime - lastCalcTime;
    
    // Calculate frequency every second (1000ms)
    if (timeDelta >= 1000) {
        uint32_t updateDelta = currentUpdateCount - lastUpdateCount;
        lastFrequency = (float)updateDelta * 1000.0f / (float)timeDelta;
        
        // Update tracking variables
        lastMultizoneTofFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        
        return lastFrequency;
    }
    
    return lastFrequency;
}

float SensorDataBuffer::getSideTofFrequency() {
    static float lastFrequency = 0.0f;
    static uint32_t lastUpdateCount = 0;
    
    uint32_t currentTime = millis();
    uint32_t lastCalcTime = lastSideTofFrequencyCalcTime.load();
    uint32_t currentUpdateCount = sideTofUpdateCount.load();
    
    // Initialize on first call
    if (lastCalcTime == 0) {
        lastSideTofFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        return 0.0f;
    }
    
    uint32_t timeDelta = currentTime - lastCalcTime;
    
    // Calculate frequency every second (1000ms)
    if (timeDelta >= 1000) {
        uint32_t updateDelta = currentUpdateCount - lastUpdateCount;
        lastFrequency = (float)updateDelta * 1000.0f / (float)timeDelta;
        
        // Update tracking variables
        lastSideTofFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        
        return lastFrequency;
    }
    
    return lastFrequency;
}

float SensorDataBuffer::getColorSensorFrequency() {
    static float lastFrequency = 0.0f;
    static uint32_t lastUpdateCount = 0;
    
    uint32_t currentTime = millis();
    uint32_t lastCalcTime = lastColorSensorFrequencyCalcTime.load();
    uint32_t currentUpdateCount = colorSensorUpdateCount.load();
    
    // Initialize on first call
    if (lastCalcTime == 0) {
        lastColorSensorFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        return 0.0f;
    }
    
    uint32_t timeDelta = currentTime - lastCalcTime;
    
    // Calculate frequency every second (1000ms)
    if (timeDelta >= 1000) {
        uint32_t updateDelta = currentUpdateCount - lastUpdateCount;
        lastFrequency = (float)updateDelta * 1000.0f / (float)timeDelta;
        
        // Update tracking variables
        lastColorSensorFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        
        return lastFrequency;
    }
    
    return lastFrequency;
}

bool SensorDataBuffer::shouldEnableQuaternionExtended() const {
    // Check if within timeout window (original condition)
    bool withinTimeout = timeouts.shouldEnableQuaternion();
    
    // Check if serial is connected
    bool serialConnected = SerialManager::getInstance().isSerialConnected();
    
    // Check if bytecode program is loaded (including paused)
    bool programLoaded = BytecodeVM::getInstance().isProgramLoaded();
    
    // Check if user is connected via websocket
    bool userConnected = WebSocketManager::getInstance().isUserConnectedToThisPip();
    
    return withinTimeout || serialConnected || programLoaded || userConnected;
}

// Add these new methods

ColorType SensorDataBuffer::classifyCurrentColor() {
    uint8_t r = currentColorData.redValue;
    uint8_t g = currentColorData.greenValue;
    uint8_t b = currentColorData.blueValue;
    
    if (!currentColorData.isValid) return ColorType::COLOR_NONE;
    
    // White: All components bright
    if (r > 130 && g > 130 && b > 130) {
        return ColorType::COLOR_WHITE;
    }

    // Yellow: Red and Green strong, Blue weak
    if (r > 120 && g > 120 && b < 100) {
        return ColorType::COLOR_YELLOW;
    }

    // Red: R dominant and bright enough
    if (r > 80 && r > (g + 30) && r > (b + 30)) {
        return ColorType::COLOR_RED;
    }
    
    // Green: G dominant and bright enough  
    if (g > 65 && g > (r + 20) && g > (b + 20)) {
        return ColorType::COLOR_GREEN;
    }
    
    // Blue: B dominant and bright enough
    if (b > 65 && b > (r + 20) && b > (g + 20)) {
        return ColorType::COLOR_BLUE;
    }
    
    // Black: All components dark
    if (r < 60 && g < 60 && b < 60) {
        return ColorType::COLOR_BLACK;
    }
    
    return ColorType::COLOR_NONE;
}

void SensorDataBuffer::updateColorHistory(ColorType color) {
    colorHistory[colorHistoryIndex] = color;
    colorHistoryIndex = (colorHistoryIndex + 1) % 5;
}

bool SensorDataBuffer::checkColorConsistency(ColorType targetColor) {
    // Count how many of the last 5 classifications match the target
    uint8_t matchCount = 0;
    for (int i = 0; i < 5; i++) {
        if (colorHistory[i] == targetColor) {
            matchCount++;
        }
    }
    return matchCount >= 4; // Need 4 out of 5 to confirm
}

bool SensorDataBuffer::isObjectRed() {
    timeouts.color_last_request.store(millis());
    updateColorHistory(classifyCurrentColor());
    return checkColorConsistency(ColorType::COLOR_RED);
}

bool SensorDataBuffer::isObjectGreen() {
    timeouts.color_last_request.store(millis());
    updateColorHistory(classifyCurrentColor());
    return checkColorConsistency(ColorType::COLOR_GREEN);
}

bool SensorDataBuffer::isObjectBlue() {
    timeouts.color_last_request.store(millis());
    updateColorHistory(classifyCurrentColor());
    return checkColorConsistency(ColorType::COLOR_BLUE);
}

bool SensorDataBuffer::isObjectWhite() {
    timeouts.color_last_request.store(millis());
    updateColorHistory(classifyCurrentColor());
    return checkColorConsistency(ColorType::COLOR_WHITE);
}

bool SensorDataBuffer::isObjectBlack() {
    timeouts.color_last_request.store(millis());
    updateColorHistory(classifyCurrentColor());
    return checkColorConsistency(ColorType::COLOR_BLACK);
}

bool SensorDataBuffer::isObjectYellow() {
    timeouts.color_last_request.store(millis());
    updateColorHistory(classifyCurrentColor());
    return checkColorConsistency(ColorType::COLOR_YELLOW);
}
