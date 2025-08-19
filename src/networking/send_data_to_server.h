#pragma once

#include <ArduinoJson.h>
#include "sensors/imu.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "sensors/ir_sensor.h"
#include "sensors/ir_sensor.h"
#include "sensors/color_sensor.h"
#include "sensors/encoder_manager.h"
#include "networking/websocket_manager.h"

class SendDataToServer : public Singleton<SendDataToServer> {
    friend class Singleton<SendDataToServer>;

    public:
        void sendSensorDataToServer();
        void sendBytecodeMessage(String message);

    private:
        SendDataToServer() = default;
        bool sendSensorData = false;
        void attachRPMData(JsonObject& payload);
        void attachIRData(JsonObject& payload);
        void attachColorSensorData(JsonObject& payload);
        void attachImuData(JsonObject& payload);

        unsigned long lastSendTime = 0;
        const unsigned long SEND_INTERVAL = 50; // Poll every 50ms for more responsive data
};
