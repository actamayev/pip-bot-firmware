#pragma once

#include "../utils/structs.h"
#include "../utils/singleton.h"
#include "../actuators/led/rgb_led.h"
#include "../networking/message_processor.h"

class SerialManager : public Singleton<SerialManager> {
    friend class Singleton<SerialManager>;

    public:
        void pollSerial();
        void sendHandshakeConfirmation();
        void sendJsonToSerial(const String& jsonData);
        bool isConnected = false;
        unsigned long lastActivityTime = 0;

    private:
        uint8_t receiveBuffer[256];  // Buffer for incoming data
        size_t bufferPosition = 0;
        bool messageStarted = false;

        SerialManager() = default;
        void processCompleteMessage();
        const unsigned long SERIAL_CONNECTION_TIMEOUT = 10000;
};
