#include "imu.h"

bool ImuSensor::initialize() {
    // Add a delay before trying to initialize
    vTaskDelay(pdMS_TO_TICKS(50));

    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        if (_imu.begin_I2C(IMU_DEFAULT_ADDRESS, &Wire1)) {
            SerialQueueManager::get_instance().queue_message("BNO08x Found!");
            _isInitialized = true;
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    SerialQueueManager::get_instance().queue_message("Failed to find BNO08x chip");
    // scan_i2_c();
    return false;
}

void ImuSensor::update_enabled_reports() {
    if (!_isInitialized) {
        return;
    }

    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();

    // Enable/disable quaternion reports based on extended conditions
    bool should_enable_quat = SensorDataBuffer::get_instance().should_enable_quaternion_extended();
    if (should_enable_quat && !_enabledReports.gameRotationVector) {
        enable_game_rotation_vector();
    } else if (!should_enable_quat && _enabledReports.gameRotationVector) {
        disable_game_rotation_vector();
    }

    // Enable/disable accelerometer reports (unchanged)
    bool should_enable_accel = ReportTimeouts::should_enable_accelerometer();
    if (should_enable_accel && !_enabledReports.accelerometer) {
        enable_accelerometer();
    } else if (!should_enable_accel && _enabledReports.accelerometer) {
        disable_accelerometer();
    }

    // Enable/disable gyroscope reports (unchanged)
    bool should_enable_gyro = ReportTimeouts::should_enable_gyroscope();
    if (should_enable_gyro && !_enabledReports.gyroscope) {
        enable_gyroscope();
    } else if (!should_enable_gyro && _enabledReports.gyroscope) {
        disable_gyroscope();
    }

    // Enable/disable magnetometer reports (unchanged)
    bool should_enable_mag = ReportTimeouts::should_enable_magnetometer();
    if (should_enable_mag && !_enabledReports.magneticField) {
        enable_magnetic_field();
    } else if (!should_enable_mag && _enabledReports.magneticField) {
        disable_magnetic_field();
    }
}

void ImuSensor::enable_game_rotation_vector() {
    if (!_isInitialized || _enabledReports.gameRotationVector) {
        return;
    }

    if (!_imu.enableReport(SH2_GAME_ROTATION_VECTOR, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::get_instance().queue_message("Could not enable game rotation vector");
        return;
    }

    _enabledReports.gameRotationVector = true;
}

void ImuSensor::enable_accelerometer() {
    if (!_isInitialized || _enabledReports.accelerometer) {
        return;
    }

    if (!_imu.enableReport(SH2_ACCELEROMETER, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::get_instance().queue_message("Could not enable accelerometer");
        return;
    }

    _enabledReports.accelerometer = true;
}

void ImuSensor::enable_gyroscope() {
    if (!_isInitialized || _enabledReports.gyroscope) {
        return;
    }

    if (!_imu.enableReport(SH2_GYROSCOPE_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::get_instance().queue_message("Could not enable gyroscope");
        return;
    }

    _enabledReports.gyroscope = true;
}

void ImuSensor::enable_magnetic_field() {
    if (!_isInitialized || _enabledReports.magneticField) {
        return;
    }

    if (!_imu.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
        SerialQueueManager::get_instance().queue_message("Could not enable magnetic field");
        return;
    }

    _enabledReports.magneticField = true;
}

void ImuSensor::disable_game_rotation_vector() {
    // Note: BNO08x doesn't have a clean disable method, so we just mark as disabled
    _enabledReports.gameRotationVector = false;
}

void ImuSensor::disable_accelerometer() {
    _enabledReports.accelerometer = false;
}

void ImuSensor::disable_gyroscope() {
    _enabledReports.gyroscope = false;
}

void ImuSensor::disable_magnetic_field() {
    _enabledReports.magneticField = false;
}

bool ImuSensor::should_be_polling() const {
    if (!_isInitialized) {
        return false;
    }

    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();

    // Should poll if any report type is within timeout window, using extended logic for quaternion
    return SensorDataBuffer::get_instance().should_enable_quaternion_extended() || timeouts.should_enable_accelerometer() ||
           timeouts.should_enable_gyroscope() || timeouts.should_enable_magnetometer();
}

// New simplified update method - replaces old updateAllSensorData
void ImuSensor::update_sensor_data() {
    if (!_isInitialized) {
        return;
    }

    // Update enabled reports based on timeouts
    update_enabled_reports();

    // Single read attempt - no more 8X loop!
    if (!_imu.getSensorEvent(&_sensorValue)) {
        return;
    }
    SensorDataBuffer& buffer = SensorDataBuffer::get_instance();

    switch (_sensorValue.sensorId) {
        case SH2_GAME_ROTATION_VECTOR: {
            QuaternionData quaternion;
            quaternion.qX = _sensorValue.un.gameRotationVector.i;
            quaternion.qY = _sensorValue.un.gameRotationVector.j;
            quaternion.qZ = _sensorValue.un.gameRotationVector.k;
            quaternion.qW = _sensorValue.un.gameRotationVector.real;
            quaternion.isValid = true;

            buffer.update_quaternion(quaternion);
            break;
        }

        case SH2_ACCELEROMETER: {
            AccelerometerData accel;
            accel.aX = _sensorValue.un.accelerometer.x;
            accel.aY = _sensorValue.un.accelerometer.y;
            accel.aZ = _sensorValue.un.accelerometer.z;
            accel.isValid = true;

            buffer.update_accelerometer(accel);
            break;
        }

        case SH2_GYROSCOPE_CALIBRATED: {
            GyroscopeData gyro;
            gyro.gX = _sensorValue.un.gyroscope.x;
            gyro.gY = _sensorValue.un.gyroscope.y;
            gyro.gZ = _sensorValue.un.gyroscope.z;
            gyro.isValid = true;

            buffer.update_gyroscope(gyro);
            break;
        }

        case SH2_MAGNETIC_FIELD_CALIBRATED: {
            MagnetometerData mag;
            mag.mX = _sensorValue.un.magneticField.x;
            mag.mY = _sensorValue.un.magneticField.y;
            mag.mZ = _sensorValue.un.magneticField.z;
            mag.isValid = true;

            buffer.update_magnetometer(mag);
            break;
        }
    }
}

void ImuSensor::turn_off() {
    // Disable all reports
    _enabledReports.accelerometer = false;
    _enabledReports.gyroscope = false;
    _enabledReports.magneticField = false;
    _enabledReports.gameRotationVector = false;
}
