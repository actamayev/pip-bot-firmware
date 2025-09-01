#include "imu.h"

bool ImuSensor::initialize() {
    // Add a delay before trying to initialize
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        if (imu.begin_I2C(IMU_DEFAULT_ADDRESS, &Wire1)) {
            SerialQueueManager::getInstance().queueMessage("BNO08x Found!");
            isInitialized = true;
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    
    SerialQueueManager::getInstance().queueMessage("Failed to find BNO08x chip");
    // scanI2C();
    return false;
}

void ImuSensor::updateEnabledReports() {
    if (!isInitialized) return;
    
    ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
    
    // Enable/disable quaternion reports based on timeout
    bool shouldEnableQuat = timeouts.shouldEnableQuaternion();
    if (shouldEnableQuat && !enabledReports.gameRotationVector) {
        enableGameRotationVector();
    } else if (!shouldEnableQuat && enabledReports.gameRotationVector) {
        disableGameRotationVector();
    }
    
    // Enable/disable accelerometer reports
    bool shouldEnableAccel = timeouts.shouldEnableAccelerometer();
    if (shouldEnableAccel && !enabledReports.accelerometer) {
        enableAccelerometer();
    } else if (!shouldEnableAccel && enabledReports.accelerometer) {
        disableAccelerometer();
    }
    
    // Enable/disable gyroscope reports
    bool shouldEnableGyro = timeouts.shouldEnableGyroscope();
    if (shouldEnableGyro && !enabledReports.gyroscope) {
        enableGyroscope();
    } else if (!shouldEnableGyro && enabledReports.gyroscope) {
        disableGyroscope();
    }
    
    // Enable/disable magnetometer reports
    bool shouldEnableMag = timeouts.shouldEnableMagnetometer();
    if (shouldEnableMag && !enabledReports.magneticField) {
        enableMagneticField();
    } else if (!shouldEnableMag && enabledReports.magneticField) {
        disableMagneticField();
    }
}

void ImuSensor::enableGameRotationVector() {
    if (!isInitialized || enabledReports.gameRotationVector) return;

    if (!imu.enableReport(SH2_GAME_ROTATION_VECTOR, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::getInstance().queueMessage("Could not enable game rotation vector");
        return;
    }
    
    enabledReports.gameRotationVector = true;
}

void ImuSensor::enableAccelerometer() {
    if (!isInitialized || enabledReports.accelerometer) return;

    if (!imu.enableReport(SH2_ACCELEROMETER, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::getInstance().queueMessage("Could not enable accelerometer");
        return;
    }
    
    enabledReports.accelerometer = true;
    return;
}

void ImuSensor::enableGyroscope() {
    if (!isInitialized || enabledReports.gyroscope) return;

    if (!imu.enableReport(SH2_GYROSCOPE_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::getInstance().queueMessage("Could not enable gyroscope");
        return;
    }
    
    enabledReports.gyroscope = true;
    return;
}

void ImuSensor::enableMagneticField() {
    if (!isInitialized || enabledReports.magneticField) return;

    if (!imu.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::getInstance().queueMessage("Could not enable magnetic field");
        return;
    }
    
    enabledReports.magneticField = true;
    return;
}

void ImuSensor::disableGameRotationVector() {
    // Note: BNO08x doesn't have a clean disable method, so we just mark as disabled
    enabledReports.gameRotationVector = false;
}

void ImuSensor::disableAccelerometer() {
    enabledReports.accelerometer = false;
}

void ImuSensor::disableGyroscope() {
    enabledReports.gyroscope = false;
}

void ImuSensor::disableMagneticField() {
    enabledReports.magneticField = false;
}

bool ImuSensor::shouldBePolling() const {
    if (!isInitialized) return false;
    
    ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
    
    // Should poll if any report type is within timeout window
    return timeouts.shouldEnableQuaternion() || 
           timeouts.shouldEnableAccelerometer() || 
           timeouts.shouldEnableGyroscope() || 
           timeouts.shouldEnableMagnetometer();
}

// New simplified update method - replaces old updateAllSensorData
void ImuSensor::updateSensorData() {
    if (!isInitialized) return;

    // Update enabled reports based on timeouts
    updateEnabledReports();
    
    // Single read attempt - no more 8X loop!
    if (!imu.getSensorEvent(&sensorValue)) return;
    SensorDataBuffer& buffer = SensorDataBuffer::getInstance();
    
    switch (sensorValue.sensorId) {
        case SH2_GAME_ROTATION_VECTOR: {
            QuaternionData quaternion;
            quaternion.qX = sensorValue.un.gameRotationVector.i;
            quaternion.qY = sensorValue.un.gameRotationVector.j;
            quaternion.qZ = sensorValue.un.gameRotationVector.k;
            quaternion.qW = sensorValue.un.gameRotationVector.real;
            quaternion.isValid = true;
            
            buffer.updateQuaternion(quaternion);
            break;
        }
        
        case SH2_ACCELEROMETER: {
            AccelerometerData accel;
            accel.aX = sensorValue.un.accelerometer.x;
            accel.aY = sensorValue.un.accelerometer.y;
            accel.aZ = sensorValue.un.accelerometer.z;
            accel.isValid = true;
            
            buffer.updateAccelerometer(accel);
            break;
        }
        
        case SH2_GYROSCOPE_CALIBRATED: {
            GyroscopeData gyro;
            gyro.gX = sensorValue.un.gyroscope.x;
            gyro.gY = sensorValue.un.gyroscope.y;
            gyro.gZ = sensorValue.un.gyroscope.z;
            gyro.isValid = true;
            
            buffer.updateGyroscope(gyro);
            break;
        }
        
        case SH2_MAGNETIC_FIELD_CALIBRATED: {
            MagnetometerData mag;
            mag.mX = sensorValue.un.magneticField.x;
            mag.mY = sensorValue.un.magneticField.y;
            mag.mZ = sensorValue.un.magneticField.z;
            mag.isValid = true;
            
            buffer.updateMagnetometer(mag);
            break;
        }
    }
}

void ImuSensor::turnOff() {
    // Disable all reports
    enabledReports.accelerometer = false;
    enabledReports.gyroscope = false;
    enabledReports.magneticField = false;
    enabledReports.gameRotationVector = false;
}
