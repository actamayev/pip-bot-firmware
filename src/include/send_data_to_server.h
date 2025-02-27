#pragma once

#include <ArduinoJson.h>
#include "./singleton.h"

class SendDataToServer : public Singleton<SendDataToServer> {
    friend class Singleton<SendDataToServer>;

    public:
        SendDataToServer() = default;
        // TODO: In the future, set this to false, and only set it to true when we actually need to send data
        // Have the client send a message when the user is on a particular page (ie. demo page)
        bool sendSensorData = true;
        void sendSensorDataToServer();
    
    private:
        void attachRPMData(JsonObject& payload);
        void attachIRData(JsonObject& payload);
        void attachColorSensorData(JsonObject& payload);
};
