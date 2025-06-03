#pragma once

#include "message_processor.h"
#include "utils/config.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "actuators/led/rgb_led.h"
#include "serial_queue_manager.h"

class SerialManager : public Singleton<SerialManager> {
    friend class Singleton<SerialManager>;

    public:
        void pollSerial();
        void sendHandshakeConfirmation();
        bool isConnected = false;
        unsigned long lastActivityTime = 0;
        void sendJsonMessage(const String& route, const String& status);
        void sendPipIdMessage();
        // static void safePrintln(const String& message, SerialPriority priority);

    private:
        SerialManager() = default;  // Make constructor private and implement it
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
        uint8_t receiveBuffer[MAX_PROGRAM_SIZE];
        uint16_t bufferPosition = 0;
        uint16_t expectedPayloadLength = 0;
        bool useLongFormat = false;
        uint8_t currentMessageType = 0;
        bool messageStarted = false;

        void processCompleteMessage();
        const unsigned long SERIAL_CONNECTION_TIMEOUT = 10000;
};
