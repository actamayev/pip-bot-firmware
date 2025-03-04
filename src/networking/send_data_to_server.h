#pragma once

#include <ArduinoJson.h>
#include "../utils/singleton.h"

class SendDataToServer : public Singleton<SendDataToServer> {
    friend class Singleton<SendDataToServer>;

    public:
        SendDataToServer() = default;
        // 2/26/25 TODO: In the future, set this to false, and only set it to true when we actually need to send data
        // Have the client send a message when the user is on a particular page (ie. demo page)
        bool sendSensorData = true;
        void sendSensorDataToServer();
    
    private:
        void attachRPMData(JsonObject& payload);
        void attachIRData(JsonObject& payload);
        void attachColorSensorData(JsonObject& payload);

        unsigned long lastSendTime = 0;
        const unsigned long SEND_INTERVAL = 200; // Poll every 50ms
};
