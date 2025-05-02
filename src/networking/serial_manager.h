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
        SerialManager() = default;
        enum class ParseState {
            WAITING_FOR_START,
            READING_MESSAGE_TYPE,
            READING_FORMAT_FLAG,
            READING_LENGTH_BYTE1,
            READING_LENGTH_BYTE2,
            READING_PAYLOAD,
            WAITING_FOR_END
        };
        ParseState parseState = ParseState::WAITING_FOR_START;
        uint8_t receiveBuffer[8192];  // Buffer for incoming data
        uint16_t bufferPosition = 0;
        uint16_t expectedPayloadLength = 0;
        bool useLongFormat = false;
        uint8_t currentMessageType = 0;
        bool messageStarted = false;

        void processCompleteMessage();
        const unsigned long SERIAL_CONNECTION_TIMEOUT = 10000;
};
