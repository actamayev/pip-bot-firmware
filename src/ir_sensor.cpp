#include "./include/config.h"
#include "./include/ir_sensor.h"

IrSensor irSensor;

IrSensor::IrSensor() {
    channels[0] = {"Y6", LOW, HIGH, HIGH};    // First IR (y6)
    channels[1] = {"Y4", LOW, LOW, HIGH};     // Second IR (y4)
    channels[2] = {"Y0", LOW, LOW, LOW};      // Third IR (y0)
    channels[3] = {"Y1", HIGH, LOW, LOW};     // Fourth IR (y1)
    channels[4] = {"Y2", LOW, HIGH, LOW};     // Fifth IR (y2)

    // Initialize motor pins
    pinMode(PIN_MUX_C, OUTPUT);
    pinMode(PIN_MUX_B, OUTPUT);
    pinMode(PIN_MUX_A, OUTPUT);
    
    // Configure multiplexer output pin as analog input
    pinMode(PIN_MUX_OUT, INPUT);
    analogReadResolution(12);
    
    // Configure and enable IR sensor
    pinMode(PIN_IR_EN, OUTPUT);
    digitalWrite(PIN_IR_EN, HIGH);
}

void IrSensor::read_ir_sensor() {
    for (int i = 0; i < 5; i++) {
        // Set multiplexer to current channel
        setMuxChannel(channels[i].A, channels[i].B, channels[i].C);
        
        // Read sensor value and convert to voltage
        int rawValue = analogRead(PIN_MUX_OUT);
        sensorReadings[i] = (rawValue * 3.3) / 4095.0;
    }
}

void IrSensor::setMuxChannel(bool A, bool B, bool C) {
    digitalWrite(PIN_MUX_A, A);
    digitalWrite(PIN_MUX_B, B);
    digitalWrite(PIN_MUX_C, C);
    delay(3);  // Small delay for multiplexer to settle
}

float* IrSensor::getSensorData() {
    read_ir_sensor();
    return sensorReadings;
}
