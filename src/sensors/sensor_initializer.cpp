#include "sensor_initializer.h"

#include "utils/utils.h"

// TODO 8/21/25: Consider moving all sensor initialization to the individual sensor level.
// Last time we tried this, it didn't work well, but I think it was because the IMU hogged the line bc of the 84KB firmware it need to flash from the
// ESP to the MZ. We already moved the side tof init to the sensor level
SensorInitializer::SensorInitializer() {
    // Initialize the status array
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensorInitialized[i] = false;
    }

    SerialQueueManager::get_instance().queue_message("Starting centralized sensor initialization...");

    // Initialize sensors sequentially to avoid I2C conflicts
    // Start with the heaviest I2C user first (multizone requires ~84KB data transfer)
    initialize_multizone_tof();
    initialize_imu();
    initialize_color_sensor();

    SerialQueueManager::get_instance().queue_message("Centralized sensor initialization complete");
}

bool SensorInitializer::is_sensor_initialized(SensorType sensor) {
    if (sensor >= 0 && sensor < SENSOR_COUNT) {
        return sensorInitialized[sensor];
    }
    return false;
}

void SensorInitializer::initialize_multizone_tof() {
    SerialQueueManager::get_instance().queue_message("Initializing Multizone sensor...");

    if (!MultizoneTofSensor::get_instance().initialize()) {
        SerialQueueManager::get_instance().queue_message("Multizone sensor initialization failed");
        return;
    }
    SerialQueueManager::get_instance().queue_message("Multizone sensor setup complete");
    sensorInitialized[MULTIZONE_TOF] = true;
}

void SensorInitializer::initialize_imu() {
    SerialQueueManager::get_instance().queue_message("Initializing IMU...");

    if (!ImuSensor::get_instance().initialize()) {
        SerialQueueManager::get_instance().queue_message("IMU initialization failed");
        return;
    }
    SerialQueueManager::get_instance().queue_message("IMU setup complete");
    sensorInitialized[IMU] = true;
}

void SensorInitializer::initialize_color_sensor() {
    SerialQueueManager::get_instance().queue_message("Initializing Color Sensor...");

    if (!ColorSensor::get_instance().initialize()) {
        SerialQueueManager::get_instance().queue_message("Color Sensor initialization failed");
        return;
    }
    SerialQueueManager::get_instance().queue_message("Color Sensor setup complete");
    sensorInitialized[COLOR_SENSOR] = true;
}
