#include "send_sensor_data.h"

#include "utils/utils.h"

// Add RPM data to the provided JSON payload
void SendSensorData::attach_rpm_data(JsonObject& payload) {
    // Get the RPM values
    WheelRPMs wheelRpms = SensorDataBuffer::get_instance().get_latest_wheel_rpms();
    payload["leftWheelRPM"] = wheelRpms.leftWheelRPM;
    payload["rightWheelRPM"] = wheelRpms.rightWheelRPM;
}

void SendSensorData::attach_euler_data(JsonObject& payload) {
    EulerAngles eulerAngles = SensorDataBuffer::get_instance().get_latest_euler_angles();
    // ROLL AND PITCH ARE SWITCHED ON PURPOSE
    payload["pitch"] = eulerAngles.roll;
    payload["yaw"] = eulerAngles.yaw;
    payload["roll"] = eulerAngles.pitch;
}

void SendSensorData::attach_accel_data(JsonObject& payload) {
    AccelerometerData accelerometerData = SensorDataBuffer::get_instance().get_latest_accelerometer();
    payload["aX"] = accelerometerData.aX;
    payload["aY"] = accelerometerData.aY;
    payload["aZ"] = accelerometerData.aZ;
}

void SendSensorData::attach_gyro_data(JsonObject& payload) {
    GyroscopeData gyroscopeData = SensorDataBuffer::get_instance().get_latest_gyroscope();
    payload["gX"] = gyroscopeData.gX;
    payload["gY"] = gyroscopeData.gY;
    payload["gZ"] = gyroscopeData.gZ;
}

void SendSensorData::attach_magnetometer_data(JsonObject& payload) {
    MagnetometerData magnetometerData = SensorDataBuffer::get_instance().get_latest_magnetometer();
    payload["mX"] = magnetometerData.mX;
    payload["mY"] = magnetometerData.mY;
    payload["mZ"] = magnetometerData.mZ;
}

void SendSensorData::attach_color_sensor_data(JsonObject& payload) {
    ColorData colorSensorData = SensorDataBuffer::get_instance().get_latest_color_data();

    payload["redValue"] = colorSensorData.redValue;
    payload["greenValue"] = colorSensorData.greenValue;
    payload["blueValue"] = colorSensorData.blueValue;
}

void SendSensorData::attach_multizone_tof_data(JsonObject& payload) {
    TofData tofData = SensorDataBuffer::get_instance().get_latest_tof_data();
    JsonArray distanceArray = payload.createNestedArray("distanceGrid");
    for (int i = 0; i < 64; i++) {
        distanceArray.add(tofData.rawData.distance_mm[i]);
    }
}

void SendSensorData::attach_side_tof_data(JsonObject& payload) {
    SideTofData sideTofData = SensorDataBuffer::get_instance().get_latest_side_tof_data();
    payload["leftSideTofCounts"] = sideTofData.leftCounts;
    payload["rightSideTofCounts"] = sideTofData.rightCounts;
}

void SendSensorData::send_sensor_data_to_server() {
    if (!sendSensorData) return;

    // Check available connections - prioritize serial over websocket
    bool serialConnected = SerialManager::get_instance().is_serial_connected();
    bool websocketConnected = WebSocketManager::get_instance().is_ws_connected();

    // Must have at least one connection
    if (!serialConnected && !websocketConnected) return;

    unsigned long current_time = millis();
    // Use different intervals based on connection type
    unsigned long requiredInterval = serialConnected ? SERIAL_SEND_INTERVAL : WS_SEND_INTERVAL;
    if (current_time - lastSendTime < requiredInterval) return;

    // Create a JSON document with both routing information and payload
    auto doc = makeBaseMessageCommon<256>(ToCommonMessage::SENSOR_DATA);

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
    String jsonString;
    serializeJson(doc, jsonString);

    // Send via serial if connected (preferred), otherwise websocket
    if (serialConnected) {
        SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
    } else if (websocketConnected) {
        if (WebSocketManager::get_instance().is_ws_connected()) {
            WebSocketManager::get_instance().wsClient.send(jsonString);
        }
    }

    lastSendTime = current_time;
}

void SendSensorData::send_multizone_data() {
    if (!sendMzData) return;

    // Check available connections - prioritize serial over websocket
    bool serialConnected = SerialManager::get_instance().is_serial_connected();
    bool websocketConnected = WebSocketManager::get_instance().is_ws_connected();

    // Must have at least one connection
    if (!serialConnected && !websocketConnected) return;

    unsigned long current_time = millis();
    // Use different intervals based on connection type
    unsigned long requiredMzInterval = serialConnected ? SERIAL_MZ_INTERVAL : WS_MZ_INTERVAL;
    if (current_time - lastMzSendTime < requiredMzInterval) return;

    TofData tofData = SensorDataBuffer::get_instance().get_latest_tof_data();

    // Send 8 messages (one per row) in burst mode
    for (int row = 0; row < 8; row++) {
        auto doc = makeBaseMessageCommon<128>(ToCommonMessage::SENSOR_DATA_MZ);
        JsonObject payload = doc.createNestedObject("payload");
        payload["row"] = row;

        JsonArray distances = payload.createNestedArray("distances");
        for (int col = 0; col < 8; col++) {
            int16_t distance = tofData.rawData.distance_mm[row * 8 + col];
            distances.add(distance == 0 ? -1 : distance);
        }

        String jsonString;
        serializeJson(doc, jsonString);

        // Send via serial if connected (preferred), otherwise websocket
        if (serialConnected) {
            SerialQueueManager::get_instance().queue_message(jsonString, SerialPriority::CRITICAL);
        } else if (websocketConnected) {
            if (WebSocketManager::get_instance().is_ws_connected()) {
                WebSocketManager::get_instance().wsClient.send(jsonString);
            }
        }
    }

    lastMzSendTime = current_time;
}
