#include "./include/config.h"
#include "./include/ir_sensor.h"
#include "./include/sensors.h"
#include "./include/encoder_manager.h"
#include "./include/websocket_manager.h"
#include "./include/send_data_to_server.h"

// Add RPM data to the provided JSON payload
void SendDataToServer::attachRPMData(JsonObject& payload) {
    // Get the RPM values
    WheelRPMs wheelRpms = EncoderManager::getInstance().getBothWheelRPMs();
    payload["leftWheelRPM"] = wheelRpms.leftWheelRPM;
    payload["rightWheelRPM"] = wheelRpms.rightWheelRPM;
}

// Add IR sensor data to the provided JSON payload
void SendDataToServer::attachIRData(JsonObject& payload) {
    float* irSensorData = irSensor.getSensorData();

    // Create a JSON array for the sensor readings
    JsonArray irArray = payload.createNestedArray("irSensorData");
    
    // Add each sensor reading to the array
    for (int i = 0; i < 5; i++) {
        irArray.add(irSensorData[i]);
    }
}

void SendDataToServer::attachColorSensorData(JsonObject& payload) {
    ColorSensorData colorSensorData = Sensors::getInstance().getColorSensorData();

    payload["redValue"] = colorSensorData.redValue;
    payload["greenValue"] = colorSensorData.greenValue;
    payload["blueValue"] = colorSensorData.blueValue;
}

void SendDataToServer::sendSensorDataToServer() {
    if (!sendSensorData) return;

    // Create a JSON document with both routing information and payload
    StaticJsonDocument<256> doc;

    // Add routing information
    doc["route"] = "/sensor-data";

    // Add the actual payload
    JsonObject payload = doc.createNestedObject("payload");

    // Attach different types of sensor data
    attachRPMData(payload);
    attachIRData(payload);
    attachColorSensorData(payload);

    // Serialize and send
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send the JSON string to the WebSocket
    WebSocketManager::getInstance().wsClient.send(jsonString);
}
