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
    for (int i = 0; i < 64; i++) {
        distance_array.add(tof_data.raw_data.distance_mm[i]);
    }
}

void SendSensorData::attach_side_tof_data(JsonObject& payload) {
    SideTofData side_tof_data = SensorDataBuffer::get_instance().get_latest_side_tof_data();
    payload["leftSideTofCounts"] = side_tof_data.left_counts;
    payload["rightSideTofCounts"] = side_tof_data.right_counts;
}

void SendSensorData::send_sensor_data_to_server() {
    if (!sendSensorData) return;

    // Check available connections - prioritize serial over websocket
    bool serial_connected = SerialManager::get_instance().is_serial_connected();
    bool websocket_connected = WebSocketManager::get_instance().is_ws_connected();

    // Must have at least one connection
    if (!serial_connected && !websocket_connected) return;

    uint32_t current_time = millis();
    // Use different intervals based on connection type
    uint32_t required_interval = serial_connected ? SERIAL_SEND_INTERVAL : WS_SEND_INTERVAL;
    if (current_time - lastSendTime < required_interval) return;

    // Create a JSON document with both routing information and payload
    auto doc = make_base_message_common<256>(ToCommonMessage::SENSOR_DATA);

    // Add the actual payload
    JsonObject payload = doc.createNestedObject("payload");

    // Attach different types of sensor data based on current flags
    if (sendEncoderData) attach_rpm_data(payload);
    if (sendColorSensorData) attach_color_sensor_data(payload);
    if (sendEulerData) attach_euler_data(payload);
    if (sendAccelData) attach_accel_data(payload);
    if (sendGyroData) attach_gyro_data(payload);
    if (sendMagnetometerData) attach_magnetometer_data(payload);
    if (sendSideTofData) attach_side_tof_data(payload);
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

    lastSendTime = current_time;
}

void SendSensorData::send_multizone_data() {
    if (!sendMzData) return;

    // Check available connections - prioritize serial over websocket
    bool serial_connected = SerialManager::get_instance().is_serial_connected();
    bool websocket_connected = WebSocketManager::get_instance().is_ws_connected();

    // Must have at least one connection
    if (!serial_connected && !websocket_connected) return;

    uint32_t current_time = millis();
    // Use different intervals based on connection type
    uint32_t required_mz_interval = serial_connected ? SERIAL_MZ_INTERVAL : WS_MZ_INTERVAL;
    if (current_time - lastMzSendTime < required_mz_interval) return;

    TofData tof_data = SensorDataBuffer::get_instance().get_latest_tof_data();

    // Send 8 messages (one per row) in burst mode
    for (int row = 0; row < 8; row++) {
        auto doc = make_base_message_common<128>(ToCommonMessage::SENSOR_DATA_MZ);
        JsonObject payload = doc.createNestedObject("payload");
        payload["row"] = row;

        JsonArray distances = payload.createNestedArray("distances");
        for (int col = 0; col < 8; col++) {
            int16_t distance = tof_data.raw_data.distance_mm[row * 8 + col];
            distances.add(distance == 0 ? -1 : distance);
        }

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
    }

    lastMzSendTime = current_time;
}
