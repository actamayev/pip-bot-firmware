#include "send_sensor_data.h"
#include "utils/utils.h"

// Add RPM data to the provided JSON payload
void SendSensorData::attachRPMData(JsonObject& payload) {
    // Get the RPM values
    WheelRPMs wheelRpms = SensorDataBuffer::getInstance().getLatestWheelRPMs();
    payload["leftWheelRPM"] = wheelRpms.leftWheelRPM;
    payload["rightWheelRPM"] = wheelRpms.rightWheelRPM;
}

// Add IR sensor data to the provided JSON payload
void SendSensorData::attachIRData(JsonObject& payload) {
    SensorDataBuffer& buffer = SensorDataBuffer::getInstance();
    
    // Get IR sensor data (this will reset IR timeout)
    IrData irData = buffer.getLatestIrData();

    // Create a JSON array for the sensor readings
    JsonArray irArray = payload.createNestedArray("irSensorData");
    
    // Add each sensor reading to the array
    for (uint8_t i = 0; i < 5; i++) {
        irArray.add(irData.sensorReadings[i]);
    }
}

void SendSensorData::attachEulerData(JsonObject& payload) {
    EulerAngles eulerAngles = SensorDataBuffer::getInstance().getLatestEulerAngles();
    //ROLL AND PITCH ARE SWITCHED ON PURPOSE
    payload["pitch"] = eulerAngles.roll;
    payload["yaw"] = eulerAngles.yaw;
    payload["roll"] = eulerAngles.pitch;
}

void SendSensorData::attachAccelData(JsonObject& payload) {
    AccelerometerData accelerometerData = SensorDataBuffer::getInstance().getLatestAccelerometer();
    payload["aX"] = accelerometerData.aX;
    payload["aY"] = accelerometerData.aY;
    payload["aZ"] = accelerometerData.aZ;
}

void SendSensorData::attachGyroData(JsonObject& payload) {
    GyroscopeData gyroscopeData = SensorDataBuffer::getInstance().getLatestGyroscope();
    payload["gX"] = gyroscopeData.gX;
    payload["gY"] = gyroscopeData.gY;
    payload["gZ"] = gyroscopeData.gZ;
}

void SendSensorData::attachMagnetometerData(JsonObject& payload) {
    MagnetometerData magnetometerData = SensorDataBuffer::getInstance().getLatestMagnetometer();
    payload["mX"] = magnetometerData.mX;
    payload["mY"] = magnetometerData.mY;
    payload["mZ"] = magnetometerData.mZ;
}

void SendSensorData::attachColorSensorData(JsonObject& payload) {
    ColorData colorSensorData = SensorDataBuffer::getInstance().getLatestColorData();

    payload["redValue"] = colorSensorData.redValue;
    payload["greenValue"] = colorSensorData.greenValue;
    payload["blueValue"] = colorSensorData.blueValue;
}

void SendSensorData::attachMultizoneTofData(JsonObject& payload) {
    TofData tofData = SensorDataBuffer::getInstance().getLatestTofData();    
    JsonArray distanceArray = payload.createNestedArray("distanceGrid");
    for (int i = 0; i < 64; i++) {
        distanceArray.add(tofData.rawData.distance_mm[i]);
    }
}

void SendSensorData::attachSideTofData(JsonObject& payload) {
    SideTofData sideTofData = SensorDataBuffer::getInstance().getLatestSideTofData();
    payload["leftSideTofCounts"] = sideTofData.leftCounts;
    payload["rightSideTofCounts"] = sideTofData.rightCounts;
}

void SendSensorData::sendSensorDataToServer() {
    if (!sendSensorData) return;

    // Check available connections - prioritize serial over websocket
    bool serialConnected = SerialManager::getInstance().isSerialConnected();
    bool websocketConnected = WebSocketManager::getInstance().isWsConnected();
    
    // Must have at least one connection
    if (!serialConnected && !websocketConnected) return;

    unsigned long currentTime = millis();
    if (currentTime - lastSendTime < SEND_INTERVAL) return;

    // Create a JSON document with both routing information and payload
    StaticJsonDocument<256> doc;

    // Add routing information
    doc["route"] = routeToString(RouteType::SENSOR_DATA);

    // Add the actual payload
    JsonObject payload = doc.createNestedObject("payload");

    // Attach different types of sensor data based on current flags
    if (sendEncoderData) attachRPMData(payload);
    if (sendColorSensorData) attachColorSensorData(payload);
    if (sendEulerData) attachEulerData(payload);
    if (sendAccelData) attachAccelData(payload);
    if (sendGyroData) attachGyroData(payload);
    if (sendMagnetometerData) attachMagnetometerData(payload);
    if (sendSideTofData) attachSideTofData(payload);
    // Multizone ToF data is now sent separately via sendMultizoneData()

    // Serialize JSON
    String jsonString;
    serializeJson(doc, jsonString);

    // Send via serial if connected (preferred), otherwise websocket
    if (serialConnected) {
        SerialQueueManager::getInstance().queueMessage(jsonString, SerialPriority::CRITICAL);
    } else if (websocketConnected) {
        WebSocketManager::getInstance().wsClient.send(jsonString);
    }

    lastSendTime = currentTime;
}

void SendSensorData::sendMultizoneData() {
    if (!sendMzData) return;

    // Check available connections - prioritize serial over websocket
    bool serialConnected = SerialManager::getInstance().isSerialConnected();
    bool websocketConnected = WebSocketManager::getInstance().isWsConnected();
    
    // Must have at least one connection
    if (!serialConnected && !websocketConnected) return;

    unsigned long currentTime = millis();
    if (currentTime - lastMzSendTime < MZ_SEND_INTERVAL) return;

    TofData tofData = SensorDataBuffer::getInstance().getLatestTofData();
    
    // Send 8 messages (one per row) in burst mode
    for (int row = 0; row < 8; row++) {
        StaticJsonDocument<128> doc;
        doc["route"] = routeToString(RouteType::SENSOR_DATA_MZ);
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
            SerialQueueManager::getInstance().queueMessage(jsonString, SerialPriority::CRITICAL);
        } else if (websocketConnected) {
            WebSocketManager::getInstance().wsClient.send(jsonString);
        }
    }

    lastMzSendTime = currentTime;
}
