#include "send_data_to_server.h"

// Add RPM data to the provided JSON payload
void SendDataToServer::attachRPMData(JsonObject& payload) {
    // Get the RPM values
    WheelRPMs wheelRpms = encoderManager.getBothWheelRPMs();
    payload["leftWheelRPM"] = wheelRpms.leftWheelRPM;
    payload["rightWheelRPM"] = wheelRpms.rightWheelRPM;
}

// Add IR sensor data to the provided JSON payload
void SendDataToServer::attachIRData(JsonObject& payload) {
    float* irSensorData = IrSensor::getInstance().getSensorData();

    // Create a JSON array for the sensor readings
    JsonArray irArray = payload.createNestedArray("irSensorData");
    
    // Add each sensor reading to the array
    for (uint8_t i = 0; i < 5; i++) {
        irArray.add(irSensorData[i]);
    }
}

void SendDataToServer::attachImuData(JsonObject& payload) {
    const EulerAngles& eulerAngles = ImuSensor::getInstance().getEulerAngles();
    //ROLL AND PITCH ARE SWITCHED ON PURPOSE
    payload["pitch"] = eulerAngles.roll;
    payload["yaw"] = eulerAngles.yaw;
    payload["roll"] = eulerAngles.pitch;

    const AccelerometerData& accelerometerData = ImuSensor::getInstance().getAccelerometerData();
    payload["aX"] = accelerometerData.aX;
    payload["aY"] = accelerometerData.aY;
    payload["aZ"] = accelerometerData.aZ;

    const GyroscopeData& gyroscopeData = ImuSensor::getInstance().getGyroscopeData();
    payload["gX"] = gyroscopeData.gX;
    payload["gY"] = gyroscopeData.gY;
    payload["gZ"] = gyroscopeData.gZ;

    const MagnetometerData& magnetometerData = ImuSensor::getInstance().getMagnetometerData();
    payload["mX"] = magnetometerData.mX;
    payload["mY"] = magnetometerData.mY;
    payload["mZ"] = magnetometerData.mZ;
}

void SendDataToServer::attachColorSensorData(JsonObject& payload) {
    ColorSensorData colorSensorData = ColorSensor::getInstance().getSensorData();

    payload["redValue"] = colorSensorData.redValue;
    payload["greenValue"] = colorSensorData.greenValue;
    payload["blueValue"] = colorSensorData.blueValue;
}

void SendDataToServer::sendSensorDataToServer() {
    if (!sendSensorData) return;

    unsigned long currentTime = millis();
    if (currentTime - lastSendTime < SEND_INTERVAL) return;

    // Create a JSON document with both routing information and payload
    StaticJsonDocument<256> doc;

    // Add routing information
    doc["route"] = routeToString(RouteType::SENSOR_DATA);

    // Add the actual payload
    JsonObject payload = doc.createNestedObject("payload");

    // Attach different types of sensor data
    attachRPMData(payload);
    // attachIRData(payload);
    attachColorSensorData(payload);
    attachImuData(payload);

    // Serialize and send
    String jsonString;
    serializeJson(doc, jsonString);

    // Send the JSON string to the WebSocket
    WebSocketManager::getInstance().wsClient.send(jsonString);

    lastSendTime = currentTime;
}

void SendDataToServer::sendBytecodeMessage(String message) {
    StaticJsonDocument<256> doc;

    // Add routing information
    doc["route"] = routeToString(RouteType::BYTECODE_STATUS);

    JsonObject payload = doc.createNestedObject("payload");

    payload["message"] = message;
    // Serialize and send
    String jsonString;
    serializeJson(doc, jsonString);

    // Send the JSON string to the WebSocket
    WebSocketManager::getInstance().wsClient.send(jsonString);
}
