#pragma once

#include <ArduinoJson.h>
#include "./singleton.h"

class SendDataToServer : public Singleton<SendDataToServer> {
    friend class Singleton<SendDataToServer>;

    public:
        SendDataToServer() = default;
        bool sendSensorData = true;
        void sendSensorDataToServer();
};
