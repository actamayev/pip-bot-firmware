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
        _sendSensorData = enabled;
    }
    void set_send_multizone_data(bool enabled) {
        _sendMzData = enabled;
    }
    void set_euler_data_enabled(bool enabled) {
        _sendEulerData = enabled;
    }
    void set_accel_data_enabled(bool enabled) {
        _sendAccelData = enabled;
    }
    void set_gyro_data_enabled(bool enabled) {
        _sendGyroData = enabled;
    }
    void set_magnetometer_data_enabled(bool enabled) {
        _sendMagnetometerData = enabled;
    }
    void set_multizone_tof_data_enabled(bool enabled) {
        _sendMultizoneTofData = enabled;
    }
    void set_side_tof_data_enabled(bool enabled) {
        _sendSideTofData = enabled;
    }
    void set_color_sensor_data_enabled(bool enabled) {
        _sendColorSensorData = enabled;
    }
    void set_encoder_data_enabled(bool enabled) {
        _sendEncoderData = enabled;
    }

  private:
    SendSensorData() = default;
    bool _sendSensorData = false;
    bool _sendMzData = false;
    bool _sendEulerData = false;
    bool _sendAccelData = false;
    bool _sendGyroData = false;
    bool _sendMagnetometerData = false;
    bool _sendMultizoneTofData = false;
    bool _sendSideTofData = false;
    bool _sendColorSensorData = false;
    bool _sendEncoderData = false;

    static void attach_rpm_data(JsonObject& payload);
    static void attach_color_sensor_data(JsonObject& payload);
    static void attach_euler_data(JsonObject& payload);
    static void attach_accel_data(JsonObject& payload);
    static void attach_gyro_data(JsonObject& payload);
    static void attach_magnetometer_data(JsonObject& payload);
    static void attach_multizone_tof_data(JsonObject& payload);
    static void attach_side_tof_data(JsonObject& payload);
    void send_sensor_data_to_server();
    void send_multizone_data();

    uint32_t _lastSendTime = 0;
    uint32_t _lastMzSendTime = 0;
    const uint32_t SERIAL_SEND_INTERVAL = 50; // Serial: 50ms intervals (20Hz)
    const uint32_t WS_SEND_INTERVAL = 100;    // WebSocket: 100ms intervals (10Hz)
    const uint32_t SERIAL_MZ_INTERVAL = 200;  // Serial MZ: 200ms intervals
    const uint32_t WS_MZ_INTERVAL = 400;      // WebSocket MZ: 400ms intervals
};
