#pragma once
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "./config.h"

class IrSensor {
    public:
        // Constructor should initialize the IRsend and IRrecv objects
        IrSensor() : irsend(IR_SEND_PIN), irrecv(IR_RECV_PIN) {}

        bool initialize();
        bool getData();
        void sendIRCommand(uint32_t command);

        decode_results results;

    private: 
        IRsend irsend;
        IRrecv irrecv;
};
