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
    Serial.println("IR signal received:");
    Serial.println(resultToSourceCode(&results));
    irrecv.resume();
    return true;
}

void IrSensor::sendIRCommand(uint32_t command) {
    irsend.sendNEC(0x0102, command, 0);
    Serial.print("Sent IR command with decimal value: ");
    Serial.println(command);
}
