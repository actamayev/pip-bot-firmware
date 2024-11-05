#include <Wire.h>
#include <Adafruit_VL53L1X.h>
#include "config.h"
#include "sensor_setup.h"

SensorSetup sensorSetup;

Adafruit_VL53L1X sensor1 = Adafruit_VL53L1X();
Adafruit_VL53L1X sensor2 = Adafruit_VL53L1X();
Adafruit_VL53L1X sensor3 = Adafruit_VL53L1X();

void SensorSetup::sensor_setup() {
    // Setup onboard LED
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW); // Turn off LED initially

    pinMode(DIGITAL_IR_PIN_1, INPUT); // GPIO 32
    pinMode(DIGITAL_IR_PIN_3, INPUT); // GPIO 34

    // Serial.println("Starting VL53L1X multiple sensors test...");

    // if (!setupTofSensors()) {
    //     Serial.println("Failed to initialize sensors!");
    //     while (1) delay(10);  // Halt if setup failed
    // }

    // Serial.println("All sensors initialized successfully!");
}

bool SensorSetup::setupTofSensors() {
    // Initialize I2C
    Wire.begin(TIME_OF_FLIGHT_SDA, TIME_OF_FLIGHT_SCL);

    // Set all XSHUT pins as outputs
    pinMode(TIME_OF_FLIGHT_XSHUT_1, OUTPUT);
    pinMode(TIME_OF_FLIGHT_XSHUT_2, OUTPUT);
    pinMode(TIME_OF_FLIGHT_XSHUT_3, OUTPUT);

    // Disable all sensors
    digitalWrite(TIME_OF_FLIGHT_XSHUT_1, LOW);
    digitalWrite(TIME_OF_FLIGHT_XSHUT_2, LOW);
    digitalWrite(TIME_OF_FLIGHT_XSHUT_3, LOW);
    delay(10);

    // Initialize sensor 1 (keep default address)
    digitalWrite(TIME_OF_FLIGHT_XSHUT_1, HIGH);
    delay(10);
    if (!sensor1.begin(TOF_SENSOR1_ADDRESS, &Wire)) {
        Serial.println("Failed to boot first VL53L1X");
        return false;
    }
    sensor1.startRanging();

    // Initialize sensor 2
    digitalWrite(TIME_OF_FLIGHT_XSHUT_2, HIGH);
    // delay(10);
    // if (!sensor2.begin(TOF_SENSOR2_ADDRESS, &Wire)) {
    //     Serial.println("Failed to boot second VL53L1X");
    //     return false;
    // }
    // sensor2.startRanging();

    // // Initialize sensor 3
    digitalWrite(TIME_OF_FLIGHT_XSHUT_3, HIGH);
    // delay(10);
    // if (!sensor3.begin(TOF_SENSOR3_ADDRESS, &Wire)) {
    //     Serial.println("Failed to boot third VL53L1X");
    //     return false;
    // }
    // sensor3.startRanging();

    return true;
}
