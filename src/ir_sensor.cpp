#include "./include/ir_sensor.h"

bool IrSensor::initialize() {
    irsend.begin();
    Serial.println("IR Transmitter ready!");

    irrecv.enableIRIn();
    Serial.println("IR Receiver ready!");
    return true;
}

bool IrSensor::getData() {
    if (!irrecv.decode(&results)) return false;
    // Store the received value
    if (results.decode_type == decode_type_t::NEC) {
        lastReceivedValue = results.value;
        Serial.printf("IR Received: 0x%08X\n", results.value);
    }
    
    irrecv.resume();
    return true;
}

void IrSensor::sendIRCommand(uint32_t command) {
    irsend.sendNEC(0x0102, command, 0);
    Serial.printf("IR Sent: 0x%08X\n", command);
}
