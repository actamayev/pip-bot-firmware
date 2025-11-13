#pragma once

#include <ArduinoJson.h>
#include "sensors/imu.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "utils/structs.h"
#include "sensors/ir_sensor.h"
#include "sensors/ir_sensor.h"
#include "sensors/color_sensor.h"
#include "sensors/sensor_data_buffer.h"
#include "networking/websocket_manager.h"
#include "networking/serial_manager.h"

class SendSensorData : public Singleton<SendSensorData> {
    friend class Singleton<SendSensorData>;
    friend class TaskManager;

    public:
        void setSendSensorData(bool enabled) { sendSensorData = enabled; }
        void setSendMultizoneData(bool enabled) { sendMzData = enabled; }
        void setEulerDataEnabled(bool enabled) { sendEulerData = enabled; }
        void setAccelDataEnabled(bool enabled) { sendAccelData = enabled; }
        void setGyroDataEnabled(bool enabled) { sendGyroData = enabled; }
        void setMagnetometerDataEnabled(bool enabled) { sendMagnetometerData = enabled; }
        void setMultizoneTofDataEnabled(bool enabled) { sendMultizoneTofData = enabled; }
        void setSideTofDataEnabled(bool enabled) { sendSideTofData = enabled; }
        void setColorSensorDataEnabled(bool enabled) { sendColorSensorData = enabled; }
        void setEncoderDataEnabled(bool enabled) { sendEncoderData = enabled; }

    private:
        SendSensorData() = default;
        bool sendSensorData = false;
        bool sendMzData = false;
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
        void sendMultizoneData();

        unsigned long lastSendTime = 0;
        unsigned long lastMzSendTime = 0;
        const unsigned long SERIAL_SEND_INTERVAL = 50; // Serial: 50ms intervals (20Hz)
        const unsigned long WS_SEND_INTERVAL = 100; // WebSocket: 100ms intervals (10Hz)
        const unsigned long SERIAL_MZ_INTERVAL = 200; // Serial MZ: 200ms intervals
        const unsigned long WS_MZ_INTERVAL = 400; // WebSocket MZ: 400ms intervals
};
