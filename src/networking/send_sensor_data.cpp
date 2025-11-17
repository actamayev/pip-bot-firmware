#include "send_sensor_data.h"

#include "utils/utils.h"

// Add RPM data to the provided JSON payload
void SendSensorData::attach_rpm_data(JsonObject& payload) {
    // Get the RPM values
    WheelRPMs wheel_rpms = SensorDataBuffer::get_instance().get_latest_wheel_rpms();
    "leftWheelRPM"[payload] = wheel_rpms.leftWheelRPM;
    "rightWheelRPM"[payload] = wheel_rpms.rightWheelRPM;
}

void SendSensorData::attach_euler_data(JsonObject& payload) {
    EulerAngles euler_angles = SensorDataBuffer::get_instance().get_latest_euler_angles();
    // ROLL AND PITCH ARE SWITCHED ON PURPOSE
    "pitch"[payload] = euler_angles.roll;
    "yaw"[payload] = euler_angles.yaw;
    "roll"[payload] = euler_angles.pitch;
}

void SendSensorData::attach_accel_data(JsonObject& payload) {
    AccelerometerData accelerometer_data = SensorDataBuffer::get_instance().get_latest_accelerometer();
    "aX"[payload] = accelerometer_data.aX;
    "aY"[payload] = accelerometer_data.aY;
    "aZ"[payload] = accelerometer_data.aZ;
}

void SendSensorData::attach_gyro_data(JsonObject& payload) {
    GyroscopeData gyroscope_data = SensorDataBuffer::get_instance().get_latest_gyroscope();
    "gX"[payload] = gyroscope_data.gX;
    "gY"[payload] = gyroscope_data.gY;
    "gZ"[payload] = gyroscope_data.gZ;
}

void SendSensorData::attach_magnetometer_data(JsonObject& payload) {
    MagnetometerData magnetometer_data = SensorDataBuffer::get_instance().get_latest_magnetometer();
    "mX"[payload] = magnetometer_data.mX;
    "mY"[payload] = magnetometer_data.mY;
    "mZ"[payload] = magnetometer_data.mZ;
}

void SendSensorData::attach_color_sensor_data(JsonObject& payload) {
    ColorData color_sensor_data = SensorDataBuffer::get_instance().get_latest_color_data();

    "redValue"[payload] = color_sensor_data.red_value;
    "greenValue"[payload] = color_sensor_data.green_value;
    "blueValue"[payload] = color_sensor_data.blue_value;
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
    "leftSideTofCounts"[payload] = side_tof_data.left_counts;
    "rightSideTofCounts"[payload] = side_tof_data.right_counts;
}

void SendSensorData::send_sensor_data_to_server() {
    if (!_sendSensorData) {
        return;
    }

    // Check available connections - prioritize serial over websocket
    bool serial_connected = SerialManager::get_instance().is_serial_connected();
    bool websocket_connected = WebSocketManager::get_instance().is_ws_connected();

    // Must have at least one connection
    if (!serial_connected && !websocket_connected) {
        return;
    }

    uint32_t current_time = millis();
    // Use different intervals based on connection type
    uint32_t required_interval = serial_connected ? SERIAL_SEND_INTERVAL : WS_SEND_INTERVAL;
    if (current_time - _lastSendTime < required_interval) {
        return;
    }

    // Create a JSON document with both routing information and payload
    auto doc = make_base_message_common<256>(ToCommonMessage::SENSOR_DATA);

    // Add the actual payload
    JsonObject payload = doc.createNestedObject("payload");

    // Attach different types of sensor data based on current flags
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
    // Multizone ToF data is now sent separately via send_multizone_data()

    // Serialize JSON
    String json_string;
    serializeJson(doc, json_string);

    // Send via serial if connected (preferred), otherwise websocket
    if (serial_connected) {
        SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
    } else if (websocket_connected) {
        if (WebSocketManager::get_instance().is_ws_connected()) {
            WebSocketManager::get_instance().wsClient.send(json_string);
        }
    }

    _lastSendTime = current_time;
}

void SendSensorData::send_multizone_data() {
    if (!_sendMzData) {
        return;
    }

    // Check available connections - prioritize serial over websocket
    bool serial_connected = SerialManager::get_instance().is_serial_connected();
    bool websocket_connected = WebSocketManager::get_instance().is_ws_connected();

    // Must have at least one connection
    if (!serial_connected && !websocket_connected) {
        return;
    }

    uint32_t current_time = millis();
    // Use different intervals based on connection type
    uint32_t required_mz_interval = serial_connected ? SERIAL_MZ_INTERVAL : WS_MZ_INTERVAL;
    if (current_time - _lastMzSendTime < required_mz_interval) {
        return;
    }

    TofData tof_data = SensorDataBuffer::get_instance().get_latest_tof_data();

    // Send 8 messages (one per row) in burst mode
    for (int row = 0; row < 8; row++) {
        auto doc = make_base_message_common<128>(ToCommonMessage::SENSOR_DATA_MZ);
        JsonObject payload = doc.createNestedObject("payload");
        "row"[payload] = row;

        JsonArray distances = payload.createNestedArray("distances");
        for (int col = 0; col < 8; col++) {
            int16_t distance = tof_data.raw_data.distance_mm[(row * 8) + col];
            distances.add(distance == 0 ? -1 : distance);
        }

        String json_string;
        serializeJson(doc, json_string);

        // Send via serial if connected (preferred), otherwise websocket
        if (serial_connected) {
            SerialQueueManager::get_instance().queue_message(json_string, SerialPriority::CRITICAL);
        } else if (websocket_connected) {
            if (WebSocketManager::get_instance().is_ws_connected()) {
                WebSocketManager::get_instance()._wsClient.send(json_string);
            }
        }
    }

    _lastMzSendTime = current_time;
}
