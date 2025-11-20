#include "send_sensor_data.h"

#include "utils/utils.h"

// Add RPM data to the provided JSON payload
void SendSensorData::attach_rpm_data(JsonObject& payload) {
    // Get the RPM values
    WheelRPMs wheel_rpms = SensorDataBuffer::get_instance().get_latest_wheel_rpms();
    payload["leftWheelRPM"] = wheel_rpms.leftWheelRPM;
    payload["rightWheelRPM"] = wheel_rpms.rightWheelRPM;
}

void SendSensorData::attach_euler_data(JsonObject& payload) {
    EulerAngles euler_angles = SensorDataBuffer::get_instance().get_latest_euler_angles();
    // ROLL AND PITCH ARE SWITCHED ON PURPOSE
    payload["pitch"] = euler_angles.roll;
    payload["yaw"] = euler_angles.yaw;
    payload["roll"] = euler_angles.pitch;
}

void SendSensorData::attach_accel_data(JsonObject& payload) {
    AccelerometerData accelerometer_data = SensorDataBuffer::get_instance().get_latest_accelerometer();
    payload["aX"] = accelerometer_data.aX;
    payload["aY"] = accelerometer_data.aY;
    payload["aZ"] = accelerometer_data.aZ;
}

void SendSensorData::attach_gyro_data(JsonObject& payload) {
    GyroscopeData gyroscope_data = SensorDataBuffer::get_instance().get_latest_gyroscope();
    payload["gX"] = gyroscope_data.gX;
    payload["gY"] = gyroscope_data.gY;
    payload["gZ"] = gyroscope_data.gZ;
}

void SendSensorData::attach_magnetometer_data(JsonObject& payload) {
    MagnetometerData magnetometer_data = SensorDataBuffer::get_instance().get_latest_magnetometer();
    payload["mX"] = magnetometer_data.mX;
    payload["mY"] = magnetometer_data.mY;
    payload["mZ"] = magnetometer_data.mZ;
}

void SendSensorData::attach_color_sensor_data(JsonObject& payload) {
    ColorData color_sensor_data = SensorDataBuffer::get_instance().get_latest_color_data();

    payload["redValue"] = color_sensor_data.red_value;
    payload["greenValue"] = color_sensor_data.green_value;
    payload["blueValue"] = color_sensor_data.blue_value;
}

void SendSensorData::attach_multizone_tof_data(JsonObject& payload) {
    TofData tof_data = SensorDataBuffer::get_instance().get_latest_tof_data();
    JsonArray distance_array = payload.createNestedArray("distanceGrid");
    for (short& i : tof_data.raw_data.distance_mm) {
        distance_array.add(i);
    }
}

void SendSensorData::attach_side_tof_data(JsonObject& payload) {
    SideTofData side_tof_data = SensorDataBuffer::get_instance().get_latest_side_tof_data();
    payload["leftSideTofCounts"] = side_tof_data.left_counts;
    payload["rightSideTofCounts"] = side_tof_data.right_counts;
}

void SendSensorData::send_sensor_data_to_server() {
    if (!_sendSensorData) {
        return;
    }

    bool serial_connected = SerialManager::get_instance().is_serial_connected();
    bool websocket_connected = SensorWebSocketManager::get_instance().is_ws_connected(); // CHANGED

    if (!serial_connected && !websocket_connected) {
        return;
    }

    const uint32_t CURRENT_TIME = millis();
    uint32_t required_interval = serial_connected ? SERIAL_SEND_INTERVAL : WS_SEND_INTERVAL;
    if (CURRENT_TIME - _lastSendTime < required_interval) {
        return;
    }

    auto doc = make_base_message_common<256>(ToCommonMessage::SENSOR_DATA);
    JsonObject payload = doc.createNestedObject("payload");

    if (_sendEncoderData) {
        attach_rpm_data(payload);
    }
    if (_sendColorSensorData) {
        attach_color_sensor_data(payload);
    }
    if (_sendEulerData) {
        attach_euler_data(payload);
    }
    if (_sendAccelData) {
        attach_accel_data(payload);
    }
    if (_sendGyroData) {
        attach_gyro_data(payload);
    }
    if (_sendMagnetometerData) {
        attach_magnetometer_data(payload);
    }
    if (_sendSideTofData) {
        attach_side_tof_data(payload);
    }

    String json_string;
    serializeJson(doc, json_string);

    if (serial_connected) {
        SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
    } else if (websocket_connected) {
        SensorWebSocketManager::get_instance()._wsClient.send(json_string); // CHANGED
    }

    _lastSendTime = CURRENT_TIME;
}

void SendSensorData::send_multizone_data() {
    if (!_sendMzData) {
        return;
    }

    bool serial_connected = SerialManager::get_instance().is_serial_connected();
    bool websocket_connected = SensorWebSocketManager::get_instance().is_ws_connected(); // CHANGED

    if (!serial_connected && !websocket_connected) {
        return;
    }

    const uint32_t CURRENT_TIME = millis();
    uint32_t required_mz_interval = serial_connected ? SERIAL_MZ_INTERVAL : WS_MZ_INTERVAL;
    if (CURRENT_TIME - _lastMzSendTime < required_mz_interval) {
        return;
    }

    TofData tof_data = SensorDataBuffer::get_instance().get_latest_tof_data();

    for (int row = 0; row < 8; row++) {
        auto doc = make_base_message_common<128>(ToCommonMessage::SENSOR_DATA_MZ);
        JsonObject payload = doc.createNestedObject("payload");
        payload["row"] = row;

        JsonArray distances = payload.createNestedArray("distances");
        for (int col = 0; col < 8; col++) {
            int16_t distance = tof_data.raw_data.distance_mm[(row * 8) + col];
            distances.add(distance == 0 ? -1 : distance);
        }

        String json_string;
        serializeJson(doc, json_string);

        if (serial_connected) {
            SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
        } else if (websocket_connected) {
            SensorWebSocketManager::get_instance()._wsClient.send(json_string); // CHANGED
        }
    }

    _lastMzSendTime = CURRENT_TIME;
}
