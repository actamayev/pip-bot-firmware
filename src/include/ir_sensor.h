#pragma once
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "./config.h"

class IrSensor {
    public:
        IrSensor() : irsend(IR_SEND_PIN), irrecv(IR_RECV_PIN) {}
        
        bool initialize();
        bool getIrData();  // Returns true if new data was received
        void sendIRCommand(uint32_t command);
        
        // Add getter for the last received value
        uint32_t getLastReceivedValue() const { return lastReceivedValue; }
        
    private: 
        IRsend irsend;
        IRrecv irrecv;
        decode_results results;
        uint32_t lastReceivedValue = 0;  // Store the last received value
};
