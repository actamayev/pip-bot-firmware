#pragma once
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h> // Must be first!

#include "actuators/led/rgb_led.h"
#include "message_processor.h"
#include "sensors/battery_monitor.h"
#include "serial_queue_manager.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "utils/structs.h"

class SerialManager : public Singleton<SerialManager> {
    friend class Singleton<SerialManager>;
    friend class MessageProcessor;

  public:
    void poll_serial();
    bool is_serial_connected() const {
        return isConnected;
    }
    uint32_t lastActivityTime = 0;
    void send_json_message(ToSerialMessage route, const String& status);
    void send_pip_id_message();
    void send_saved_networks_response(const std::vector<WiFiCredentials>& networks);
    void send_scan_results_response(const std::vector<WiFiNetworkInfo>& networks);
    void send_scan_started_message();
    void send_battery_monitor_data();
    void send_dino_score(int score);
    void send_network_deleted_response(bool success);
    void send_pip_turning_off();

  private:
    SerialManager() = default; // Make constructor private and implement it
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

    const uint32_t SERIAL_CONNECTION_TIMEOUT = 400;
    bool isConnected = false;

    void send_battery_data_item(const String& key, int value);
    void send_battery_data_item(const String& key, uint32_t value);
    void send_battery_data_item(const String& key, float value);
    void send_battery_data_item(const String& key, bool value);
};
