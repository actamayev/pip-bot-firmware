#pragma once
#include <freertos/FreeRTOS.h>  // Must be first!
#include <ArduinoJson.h>
#include "message_processor.h"
#include "utils/config.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "actuators/led/rgb_led.h"
#include "serial_queue_manager.h"
#include "sensors/battery_monitor.h"

class SerialManager : public Singleton<SerialManager> {
    friend class Singleton<SerialManager>;

    public:
        void pollSerial();
        void sendHandshakeConfirmation();
        bool isConnected = false;
        unsigned long lastActivityTime = 0;
        void sendJsonMessage(RouteType route, const String& status);
        void sendPipIdMessage();
        void sendSavedNetworksResponse(const std::vector<WiFiCredentials>& networks);
        void sendScanResultsResponse(const std::vector<WiFiNetworkInfo>& networks);
        void sendScanStartedMessage();
        void sendBatteryMonitorData(const BatteryState& batteryState);
        void sendBatteryDataItem(const String& key, int value);
        void sendBatteryDataItem(const String& key, unsigned int value);
        void sendBatteryDataItem(const String& key, float value);
        void sendBatteryDataItem(const String& key, bool value);

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

        const unsigned long SERIAL_CONNECTION_TIMEOUT = 400;
};
