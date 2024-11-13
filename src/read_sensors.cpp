#include "config.h"
#include "read_sensors.h"

ReadSensors readSensors;

void ReadSensors::read_digital_ir() {
    int sensorValue = digitalRead(DIGITAL_IR_PIN_3);

    if (sensorValue == LOW) {
        Serial.println("No object detected");
    } else {
        Serial.println("Object detected");
    }
}

void ReadSensors::read_analog_ir() {
    int sensorValue = analogRead(DIGITAL_IR_PIN_1);

    // Convert the analog reading (0 - 4095) to a voltage (0 - 3.3V)
    float voltage = sensorValue * (3.3 / 4095.0);

    // Print the sensor value and voltage to the Serial Monitor
    Serial.print("Analog reading: ");
    Serial.print(sensorValue);
    Serial.print(" | Voltage: ");
    Serial.println(voltage, 2);  // Print the voltage with 2 decimal points
}

// Function to read from a single sensor
SensorReading ReadSensors::readSensor(Adafruit_VL53L1X& sensor, const char* sensorName) {
    SensorReading reading = {0, false};

    if (sensor.dataReady()) {
        reading.distance = sensor.distance();
        reading.valid = (reading.distance != -1);

        if (reading.valid) {
            Serial.print(sensorName);
            Serial.print(": ");
            Serial.print(reading.distance);
            Serial.print("mm  ");
        } else {
            Serial.print(sensorName);
            Serial.print(": Error  ");
        }

        sensor.clearInterrupt();
    }

    return reading;
}
