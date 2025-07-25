#include "utils/config.h"
#include "ir_sensor.h"

IrSensor::IrSensor() {
    // Initialize motor pins
    pinMode(PIN_MUX_A0, OUTPUT);
    pinMode(PIN_MUX_A1, OUTPUT);
    pinMode(PIN_MUX_A2, OUTPUT);
    
    // Configure multiplexer output pin as analog input
    pinMode(PIN_MUX_OUT, INPUT);
    analogReadResolution(12);
    
    // Configure and enable IR sensor
    pinMode(PIN_IR_EN, OUTPUT);
    analogWrite(PIN_IR_EN, 200); // 51/255 = 20% duty cycle
}

void IrSensor::read_ir_sensor() {
    for (uint8_t i = 0; i < 5; i++) {
        // Set multiplexer to current channel
        setMuxChannel(channels[i].A0, channels[i].A1, channels[i].A2);
        
        // Read sensor value and convert to voltage
        uint16_t rawValue = analogRead(PIN_MUX_OUT);
        sensorReadings[i] = (rawValue * 3.3) / 4095.0;
    }
}

void IrSensor::setMuxChannel(bool A0, bool A1, bool A2) {
    digitalWrite(PIN_MUX_A0, A0);
    digitalWrite(PIN_MUX_A1, A1);
    digitalWrite(PIN_MUX_A2, A2);
    vTaskDelay(pdMS_TO_TICKS(3));  // Small delay for multiplexer to settle
}

float* IrSensor::getSensorData() {
    read_ir_sensor();
    return sensorReadings;
}
