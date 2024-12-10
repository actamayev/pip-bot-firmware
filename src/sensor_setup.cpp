#include "./include/config.h"
#include "./include/sensor_setup.h"

SparkFun_VL53L5CX rightTof;
VL53L5CX_ResultsData rightTofData;

SparkFun_VL53L5CX leftTof;
VL53L5CX_ResultsData leftTofData;

SensorSetup::SensorSetup() {
    sensor_setup();
}

void SensorSetup::sensor_setup() {
    i2c_setup();
    // pinMode(DIGITAL_IR_PIN_1, INPUT); // GPIO 32
    // pinMode(DIGITAL_IR_PIN_3, INPUT); // GPIO 34

    // Serial.println("Starting VL53L1X multiple sensors test...");

    // if (!setupTofSensors()) {
    //     Serial.println("Failed to initialize sensors!");
    //     while (1) delay(10);  // Halt if setup failed
    // }

    // Serial.println("All sensors initialized successfully!");
}

void SensorSetup::i2c_setup() {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400 * 1000); // 400kHz
}

void SensorSetup::setup_tof_sensors() {
    pinMode(LEFT_TOF_RESET_PIN, OUTPUT);
    pinMode(RIGHT_TOF_RESET_PIN, OUTPUT);

    // Hold Left TOF in reset while we configure Right TOF
    digitalWrite(LEFT_TOF_RESET_PIN, HIGH);

    // Reset and initialize right tof:
    digitalWrite(RIGHT_TOF_RESET_PIN, HIGH);
    delay(100); // TODO: Figure out if we should use delay (use millis instead?)
    digitalWrite(RIGHT_TOF_RESET_PIN, LOW);

    Serial.println("Initializing Right Tof...");
    if (rightTof.begin() == false) {
        Serial.println("Right TOF not found - check wiring. Freezing...");
        while (1);
    }

    // Set new address for right TOF
    Serial.printf("Setting right tof address to: 0x%02X\n", RIGHT_TOF_ADDRESS);
    if (rightTof.setAddress(RIGHT_TOF_ADDRESS) == false) {
        Serial.println("Failed to set right tof address. Freezing...");
        while (1);
    }

    // Verify new address
    int newAddress = rightTof.getAddress();
    Serial.printf("Right tof new address confirmed as: 0x%02X\n", newAddress);

    // Initialize left TOF
    digitalWrite(LEFT_TOF_RESET_PIN, LOW);
    delay(100);

    Serial.println("Initializing left tof");
    if (leftTof.begin() == false) {
        Serial.println("Left TOF not found - check wiring. Freezing...");
        while (1);
    }

    // Configure both sensors
    Serial.println("Configuring both sensors...");
    
    // Set resolution for both sensors
    rightTof.setResolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);
    rightTof.setResolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);

    // Set ranging frequency
    rightTof.setRangingFrequency(TOF_RANGING_FREQUENCY);
    leftTof.setRangingFrequency(TOF_RANGING_FREQUENCY);

    Serial.println("TOF setup complete");
    // Start ranging
    // rightTof.startRanging();
    // leftTof.startRanging();
}

// bool SensorSetup::setupTofSensors() {
//     // Initialize I2C
//     Wire.begin(TIME_OF_FLIGHT_SDA, TIME_OF_FLIGHT_SCL);

//     // Set all XSHUT pins as outputs
//     pinMode(TIME_OF_FLIGHT_XSHUT_1, OUTPUT);
//     pinMode(TIME_OF_FLIGHT_XSHUT_2, OUTPUT);
//     pinMode(TIME_OF_FLIGHT_XSHUT_3, OUTPUT);

//     // Disable all sensors
//     digitalWrite(TIME_OF_FLIGHT_XSHUT_1, LOW);
//     digitalWrite(TIME_OF_FLIGHT_XSHUT_2, LOW);
//     digitalWrite(TIME_OF_FLIGHT_XSHUT_3, LOW);
//     delay(10);

//     // Initialize sensor 1 (keep default address)
//     digitalWrite(TIME_OF_FLIGHT_XSHUT_1, HIGH);
//     delay(10);
//     if (!sensor1.begin(TOF_SENSOR1_ADDRESS, &Wire)) {
//         Serial.println("Failed to boot first VL53L1X");
//         return false;
//     }
//     sensor1.startRanging();

//     // Initialize sensor 2
//     digitalWrite(TIME_OF_FLIGHT_XSHUT_2, HIGH);
//     // delay(10);
//     // if (!sensor2.begin(TOF_SENSOR2_ADDRESS, &Wire)) {
//     //     Serial.println("Failed to boot second VL53L1X");
//     //     return false;
//     // }
//     // sensor2.startRanging();

//     // // Initialize sensor 3
//     digitalWrite(TIME_OF_FLIGHT_XSHUT_3, HIGH);
//     // delay(10);
//     // if (!sensor3.begin(TOF_SENSOR3_ADDRESS, &Wire)) {
//     //     Serial.println("Failed to boot third VL53L1X");
//     //     return false;
//     // }
//     // sensor3.startRanging();

//     return true;
// }
