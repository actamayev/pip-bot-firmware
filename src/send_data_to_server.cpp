#include "./include/config.h"
#include "./include/encoder_manager.h"
#include "./include/websocket_manager.h"
#include "./include/send_data_to_server.h"

// TODO: Put this function into the network loop.
// Change the network loop to be every 20ms
// Have a socket receive a message for when to set sendSensorData to true.
void SendDataToServer::sendSensorDataToServer() {
    if (!sendSensorData) return;
    
    // Create a JSON document with both routing information and payload
    StaticJsonDocument<256> doc;
    
    // Add routing information
    doc["route"] = "/sensor-data";
    
    // Add the actual payload
    JsonObject payload = doc.createNestedObject("payload");
    
    // Get the RPM values
    WheelRPMs wheelRpms = EncoderManager::getInstance().getBothWheelRPMs();
    payload["leftWheelRPM"] = wheelRpms.leftWheelRPM;
    payload["rightWheelRPM"] = wheelRpms.rightWheelRPM;
    
    // Serialize and send
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send the JSON string to the WebSocket
    WebSocketManager::getInstance().wsClient.send(jsonString);
}
