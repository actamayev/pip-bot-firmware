#pragma once

#include <ArduinoJson.h>

#include "networking/serial_manager.h"
#include "networking/websocket_manager.h"
#include "sensors/color_sensor.h"
#include "sensors/imu.h"
#include "sensors/sensor_data_buffer.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "utils/structs.h"

class SendSensorData : public Singleton<SendSensorData> {
    friend class Singleton<SendSensorData>;
    friend class TaskManager;

  public:
    void set_send_sensor_data(bool enabled) {
        sendSensorData = enabled;
    }
    void set_send_multizone_data(bool enabled) {
        sendMzData = enabled;
    }
    void set_euler_data_enabled(bool enabled) {
        sendEulerData = enabled;
    }
    void set_accel_data_enabled(bool enabled) {
        sendAccelData = enabled;
    }
    void set_gyro_data_enabled(bool enabled) {
        sendGyroData = enabled;
    }
    void set_magnetometer_data_enabled(bool enabled) {
        sendMagnetometerData = enabled;
    }
    void set_multizone_tof_data_enabled(bool enabled) {
        sendMultizoneTofData = enabled;
    }
    void set_side_tof_data_enabled(bool enabled) {
        sendSideTofData = enabled;
    }
    void set_color_sensor_data_enabled(bool enabled) {
        sendColorSensorData = enabled;
    }
    void set_encoder_data_enabled(bool enabled) {
        sendEncoderData = enabled;
    }

  private:
    SendSensorData() = default;
    bool sendSensorData = false;
    bool sendMzData = false;
    bool sendEulerData = false;
    bool sendAccelData = false;
    bool sendGyroData = false;
    bool sendMagnetometerData = false;
    bool sendMultizoneTofData = false;
    bool sendSideTofData = false;
    bool sendColorSensorData = false;
    bool sendEncoderData = false;

    void attach_rpm_data(JsonObject& payload);
    void attach_color_sensor_data(JsonObject& payload);
    void attach_euler_data(JsonObject& payload);
    void attach_accel_data(JsonObject& payload);
    void attach_gyro_data(JsonObject& payload);
    void attach_magnetometer_data(JsonObject& payload);
    void attach_multizone_tof_data(JsonObject& payload);
    void attach_side_tof_data(JsonObject& payload);
    void send_sensor_data_to_server();
    void send_multizone_data();

    uint32_t lastSendTime = 0;
    uint32_t lastMzSendTime = 0;
    const uint32_t SERIAL_SEND_INTERVAL = 50; // Serial: 50ms intervals (20Hz)
    const uint32_t WS_SEND_INTERVAL = 100;    // WebSocket: 100ms intervals (10Hz)
    const uint32_t SERIAL_MZ_INTERVAL = 200;  // Serial MZ: 200ms intervals
    const uint32_t WS_MZ_INTERVAL = 400;      // WebSocket MZ: 400ms intervals
};
