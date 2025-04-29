#pragma once

#include "../utils/singleton.h"
#include "../utils/structs.h"
#include "../networking/message_processor.h"

class SerialManager : public Singleton<SerialManager> {
    friend class Singleton<SerialManager>;

    public:
        void begin();
        void pollSerial();
        void sendHandshakeConfirmation();
        void sendJsonToSerial(const String& jsonData);

    private:
        bool isConnected = false;
        unsigned long lastActivityTime = 0;
        uint8_t receiveBuffer[256];  // Buffer for incoming data
        size_t bufferPosition = 0;
        bool messageStarted = false;

        SerialManager() = default;
        void processCompleteMessage();
};
