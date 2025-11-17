#include "imu.h"

bool ImuSensor::initialize() {
    // Add a delay before trying to initialize
    vTaskDelay(pdMS_TO_TICKS(50));

    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        if (imu.begin_I2C(IMU_DEFAULT_ADDRESS, &Wire1)) {
            SerialQueueManager::get_instance().queueMessage("BNO08x Found!");
            isInitialized = true;
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    SerialQueueManager::get_instance().queueMessage("Failed to find BNO08x chip");
    // scanI2C();
    return false;
}

void ImuSensor::update_enabled_reports() {
    if (!isInitialized) return;

    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();

    // Enable/disable quaternion reports based on extended conditions
    bool shouldEnableQuat = SensorDataBuffer::get_instance().should_enable_quaternion_extended();
    if (shouldEnableQuat && !enabledReports.gameRotationVector) {
        enable_game_rotation_vector();
    } else if (!shouldEnableQuat && enabledReports.gameRotationVector) {
        disable_game_rotation_vector();
    }

    // Enable/disable accelerometer reports (unchanged)
    bool shouldEnableAccel = timeouts.shouldEnableAccelerometer();
    if (shouldEnableAccel && !enabledReports.accelerometer) {
        enable_accelerometer();
    } else if (!shouldEnableAccel && enabledReports.accelerometer) {
        disable_accelerometer();
    }

    // Enable/disable gyroscope reports (unchanged)
    bool shouldEnableGyro = timeouts.shouldEnableGyroscope();
    if (shouldEnableGyro && !enabledReports.gyroscope) {
        enable_gyroscope();
    } else if (!shouldEnableGyro && enabledReports.gyroscope) {
        disable_gyroscope();
    }

    // Enable/disable magnetometer reports (unchanged)
    bool shouldEnableMag = timeouts.shouldEnableMagnetometer();
    if (shouldEnableMag && !enabledReports.magneticField) {
        enable_magnetic_field();
    } else if (!shouldEnableMag && enabledReports.magneticField) {
        disable_magnetic_field();
    }
}

void ImuSensor::enable_game_rotation_vector() {
    if (!isInitialized || enabledReports.gameRotationVector) return;

    if (!imu.enableReport(SH2_GAME_ROTATION_VECTOR, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::get_instance().queueMessage("Could not enable game rotation vector");
        return;
    }

    enabledReports.gameRotationVector = true;
}

void ImuSensor::enable_accelerometer() {
    if (!isInitialized || enabledReports.accelerometer) return;

    if (!imu.enableReport(SH2_ACCELEROMETER, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::get_instance().queueMessage("Could not enable accelerometer");
        return;
    }

    enabledReports.accelerometer = true;
    return;
}

void ImuSensor::enable_gyroscope() {
    if (!isInitialized || enabledReports.gyroscope) return;

    if (!imu.enableReport(SH2_GYROSCOPE_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::get_instance().queueMessage("Could not enable gyroscope");
        return;
    }

    enabledReports.gyroscope = true;
    return;
}

void ImuSensor::enable_magnetic_field() {
    if (!isInitialized || enabledReports.magneticField) return;

    if (!imu.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::get_instance().queueMessage("Could not enable magnetic field");
        return;
    }

    enabledReports.magneticField = true;
    return;
}

void ImuSensor::disable_game_rotation_vector() {
    // Note: BNO08x doesn't have a clean disable method, so we just mark as disabled
    enabledReports.gameRotationVector = false;
}

void ImuSensor::disable_accelerometer() {
    enabledReports.accelerometer = false;
}

void ImuSensor::disable_gyroscope() {
    enabledReports.gyroscope = false;
}

void ImuSensor::disable_magnetic_field() {
    enabledReports.magneticField = false;
}

bool ImuSensor::should_be_polling() const {
    if (!isInitialized) return false;

    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();

    // Should poll if any report type is within timeout window, using extended logic for quaternion
    return SensorDataBuffer::get_instance().should_enable_quaternion_extended() || timeouts.shouldEnableAccelerometer() ||
           timeouts.shouldEnableGyroscope() || timeouts.shouldEnableMagnetometer();
}

// New simplified update method - replaces old updateAllSensorData
void ImuSensor::update_sensor_data() {
    if (!isInitialized) return;

    // Update enabled reports based on timeouts
    update_enabled_reports();

    // Single read attempt - no more 8X loop!
    if (!imu.getSensorEvent(&sensorValue)) return;
    SensorDataBuffer& buffer = SensorDataBuffer::get_instance();

    switch (sensorValue.sensorId) {
        case SH2_GAME_ROTATION_VECTOR: {
            QuaternionData quaternion;
            quaternion.qX = sensorValue.un.gameRotationVector.i;
            quaternion.qY = sensorValue.un.gameRotationVector.j;
            quaternion.qZ = sensorValue.un.gameRotationVector.k;
            quaternion.qW = sensorValue.un.gameRotationVector.real;
            quaternion.isValid = true;

            buffer.update_quaternion(quaternion);
            break;
        }

        case SH2_ACCELEROMETER: {
            AccelerometerData accel;
            accel.aX = sensorValue.un.accelerometer.x;
            accel.aY = sensorValue.un.accelerometer.y;
            accel.aZ = sensorValue.un.accelerometer.z;
            accel.isValid = true;

            buffer.update_accelerometer(accel);
            break;
        }

        case SH2_GYROSCOPE_CALIBRATED: {
            GyroscopeData gyro;
            gyro.gX = sensorValue.un.gyroscope.x;
            gyro.gY = sensorValue.un.gyroscope.y;
            gyro.gZ = sensorValue.un.gyroscope.z;
            gyro.isValid = true;

            buffer.update_gyroscope(gyro);
            break;
        }

        case SH2_MAGNETIC_FIELD_CALIBRATED: {
            MagnetometerData mag;
            mag.mX = sensorValue.un.magneticField.x;
            mag.mY = sensorValue.un.magneticField.y;
            mag.mZ = sensorValue.un.magneticField.z;
            mag.isValid = true;

            buffer.update_magnetometer(mag);
            break;
        }
    }
}

void ImuSensor::turn_off() {
    // Disable all reports
    enabledReports.accelerometer = false;
    enabledReports.gyroscope = false;
    enabledReports.magneticField = false;
    enabledReports.gameRotationVector = false;
}
