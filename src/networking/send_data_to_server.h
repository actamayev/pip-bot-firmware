#pragma once

#include <ArduinoJson.h>
#include "sensors/imu.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "sensors/ir_sensor.h"
#include "sensors/ir_sensor.h"
#include "sensors/color_sensor.h"
#include "sensors/sensor_data_buffer.h"
#include "networking/websocket_manager.h"
#include "networking/serial_queue_manager.h"
#include "networking/network_state_manager.h"

class SendDataToServer : public Singleton<SendDataToServer> {
    friend class Singleton<SendDataToServer>;
    friend class TaskManager;

    public:
        void setSendSensorData(bool enabled) { sendSensorData = enabled; }
        void setEulerDataEnabled(bool enabled) { sendEulerData = enabled; }
        void setAccelDataEnabled(bool enabled) { sendAccelData = enabled; }
        void setGyroDataEnabled(bool enabled) { sendGyroData = enabled; }
        void setMagnetometerDataEnabled(bool enabled) { sendMagnetometerData = enabled; }
        void setMultizoneTofDataEnabled(bool enabled) { sendMultizoneTofData = enabled; }
        void setSideTofDataEnabled(bool enabled) { sendSideTofData = enabled; }
        void setColorSensorDataEnabled(bool enabled) { sendColorSensorData = enabled; }
        void setEncoderDataEnabled(bool enabled) { sendEncoderData = enabled; }

    private:
        SendDataToServer() = default;
        bool sendSensorData = false;
        bool sendEulerData = false;
        bool sendAccelData = false;
        bool sendGyroData = false;
        bool sendMagnetometerData = false;
        bool sendMultizoneTofData = false;
        bool sendSideTofData = false;
        bool sendColorSensorData = false;
        bool sendEncoderData = false;
        
        void attachRPMData(JsonObject& payload);
        void attachIRData(JsonObject& payload);
        void attachColorSensorData(JsonObject& payload);
        void attachEulerData(JsonObject& payload);
        void attachAccelData(JsonObject& payload);
        void attachGyroData(JsonObject& payload);
        void attachMagnetometerData(JsonObject& payload);
        void attachMultizoneTofData(JsonObject& payload);
        void attachSideTofData(JsonObject& payload);
        void sendSensorDataToServer();

        unsigned long lastSendTime = 0;
        const unsigned long SEND_INTERVAL = 50; // Poll every 50ms for more responsive data
};
