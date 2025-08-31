#include "send_data_to_server.h"

// Add RPM data to the provided JSON payload
void SendDataToServer::attachRPMData(JsonObject& payload) {
    // Get the RPM values
    WheelRPMs wheelRpms = SensorDataBuffer::getInstance().getLatestWheelRPMs();
    payload["leftWheelRPM"] = wheelRpms.leftWheelRPM;
    payload["rightWheelRPM"] = wheelRpms.rightWheelRPM;
}

// Add IR sensor data to the provided JSON payload
void SendDataToServer::attachIRData(JsonObject& payload) {
    SensorDataBuffer& buffer = SensorDataBuffer::getInstance();
    
    // Get IR sensor data (this will reset IR timeout)
    IrData irData = buffer.getLatestIrData();

    // Create a JSON array for the sensor readings
    JsonArray irArray = payload.createNestedArray("irSensorData");
    
    // Add each sensor reading to the array
    for (uint8_t i = 0; i < 5; i++) {
        irArray.add(irData.sensorReadings[i]);
    }
    
    payload["irValid"] = irData.isValid;
}

void SendDataToServer::attachEulerData(JsonObject& payload) {
    EulerAngles eulerAngles = SensorDataBuffer::getInstance().getLatestEulerAngles();
    //ROLL AND PITCH ARE SWITCHED ON PURPOSE
    payload["pitch"] = eulerAngles.roll;
    payload["yaw"] = eulerAngles.yaw;
    payload["roll"] = eulerAngles.pitch;
}

void SendDataToServer::attachAccelData(JsonObject& payload) {
    AccelerometerData accelerometerData = SensorDataBuffer::getInstance().getLatestAccelerometer();
    payload["aX"] = accelerometerData.aX;
    payload["aY"] = accelerometerData.aY;
    payload["aZ"] = accelerometerData.aZ;
}

void SendDataToServer::attachGyroData(JsonObject& payload) {
    GyroscopeData gyroscopeData = SensorDataBuffer::getInstance().getLatestGyroscope();
    payload["gX"] = gyroscopeData.gX;
    payload["gY"] = gyroscopeData.gY;
    payload["gZ"] = gyroscopeData.gZ;
}

void SendDataToServer::attachMagnetometerData(JsonObject& payload) {
    MagnetometerData magnetometerData = SensorDataBuffer::getInstance().getLatestMagnetometer();
    payload["mX"] = magnetometerData.mX;
    payload["mY"] = magnetometerData.mY;
    payload["mZ"] = magnetometerData.mZ;
}

void SendDataToServer::attachColorSensorData(JsonObject& payload) {
    ColorData colorSensorData = SensorDataBuffer::getInstance().getLatestColorData();

    payload["redValue"] = colorSensorData.redValue;
    payload["greenValue"] = colorSensorData.greenValue;
    payload["blueValue"] = colorSensorData.blueValue;
}

void SendDataToServer::attachMultizoneTofData(JsonObject& payload) {
    TofData tofData = SensorDataBuffer::getInstance().getLatestTofData();
    payload["multizoneTofValid"] = tofData.isValid;
    payload["objectDetected"] = tofData.isObjectDetected;
}

void SendDataToServer::attachSideTofData(JsonObject& payload) {
    SideTofData sideTofData = SensorDataBuffer::getInstance().getLatestSideTofData();
    payload["leftSideTofCounts"] = sideTofData.leftCounts;
    payload["rightSideTofCounts"] = sideTofData.rightCounts;
    payload["leftSideTofValid"] = sideTofData.leftValid;
    payload["rightSideTofValid"] = sideTofData.rightValid;
}

void SendDataToServer::sendSensorDataToServer() {
    if (
        !sendSensorData ||
        !WebSocketManager::getInstance().isConnected()
    ) return;

    unsigned long currentTime = millis();
    if (currentTime - lastSendTime < SEND_INTERVAL) return;

    // Create a JSON document with both routing information and payload
    StaticJsonDocument<256> doc;

    // Add routing information
    doc["route"] = routeToString(RouteType::SENSOR_DATA);

    // Add the actual payload
    JsonObject payload = doc.createNestedObject("payload");

    // Attach different types of sensor data based on current flags
    if (sendEulerData) {
        attachEulerData(payload);
    }
    if (sendAccelData) {
        attachAccelData(payload);
    }
    if (sendGyroData) {
        attachGyroData(payload);
    }
    if (sendMagnetometerData) {
        attachMagnetometerData(payload);
    }
    if (sendMultizoneTofData) {
        attachMultizoneTofData(payload);
    }
    if (sendSideTofData) {
        attachSideTofData(payload);
    }
    if (sendColorSensorData) {
        attachColorSensorData(payload);
    }
    if (sendEncoderData) {
        attachRPMData(payload);
    }

    // Serialize and send
    String jsonString;
    serializeJson(doc, jsonString);

    // Send the JSON string to the WebSocket
    WebSocketManager::getInstance().wsClient.send(jsonString);

    lastSendTime = currentTime;
}
