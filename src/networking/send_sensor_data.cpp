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
    
    payload["irValid"] = irData.isValid;
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

    NetworkMode mode = NetworkStateManager::getInstance().getCurrentMode();
    
    // Check if we can transmit based on current mode
    bool canTransmit = false;
    if (mode == NetworkMode::WIFI_MODE && WebSocketManager::getInstance().isConnected()) {
        canTransmit = true;
    } else if (mode == NetworkMode::SERIAL_MODE) {
        canTransmit = true;
    }
    
    if (!canTransmit) return;

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

    // Send based on current mode
    if (mode == NetworkMode::WIFI_MODE) {
        WebSocketManager::getInstance().wsClient.send(jsonString);
    } else if (mode == NetworkMode::SERIAL_MODE) {
        // Send with CRITICAL priority for proper serial communication
        SerialQueueManager::getInstance().queueMessage(jsonString, SerialPriority::CRITICAL);
    }

    lastSendTime = currentTime;
}

void SendSensorData::sendMultizoneData() {
    if (!sendMzData) return;

    NetworkMode mode = NetworkStateManager::getInstance().getCurrentMode();
    
    // Check if we can transmit based on current mode
    bool canTransmit = false;
    if (mode == NetworkMode::WIFI_MODE && WebSocketManager::getInstance().isConnected()) {
        canTransmit = true;
    } else if (mode == NetworkMode::SERIAL_MODE) {
        canTransmit = true;
    }
    
    if (!canTransmit) return;

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
        
        // Send based on current mode
        if (mode == NetworkMode::WIFI_MODE) {
            WebSocketManager::getInstance().wsClient.send(jsonString);
        } else if (mode == NetworkMode::SERIAL_MODE) {
            SerialQueueManager::getInstance().queueMessage(jsonString, SerialPriority::CRITICAL);
        }
    }

    lastMzSendTime = currentTime;
}
