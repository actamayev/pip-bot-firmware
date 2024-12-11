// #include "./include/config.h"
// #include "./include/sensor_setup.h"

// // TOF:
// SparkFun_VL53L5CX rightTof;
// VL53L5CX_ResultsData rightTofData;

// SparkFun_VL53L5CX leftTof;
// VL53L5CX_ResultsData leftTofData;

// // IMU:
// Adafruit_BNO08x imu;
// sh2_SensorValue_t imu_sensor_value;

// SensorSetup::SensorSetup() {
//     sensor_setup();
// }

// void SensorSetup::sensor_setup() {
//     i2c_setup();
//     // TODO: Later on, call these setup functions conditionally. Have a is_tof_setup flag that initializes as false, and have a checker run the setup function before using it to ensure it's properly setup
//     setup_tof_sensors();
//     setup_imu();
// }

// void SensorSetup::i2c_setup() {
//     Wire.begin(I2C_SDA, I2C_SCL);
//     Wire.setClock(I2C_CLOCK_SPEED);
// }

// void SensorSetup::setup_tof_sensors() {
//     pinMode(LEFT_TOF_RESET_PIN, OUTPUT);
//     pinMode(RIGHT_TOF_RESET_PIN, OUTPUT);

//     // Hold Left TOF in reset while we configure Right TOF
//     digitalWrite(LEFT_TOF_RESET_PIN, HIGH);

//     // Reset and initialize right tof:
//     digitalWrite(RIGHT_TOF_RESET_PIN, HIGH);
//     delay(100); // TODO: Figure out if we should use delay (use millis instead?)
//     digitalWrite(RIGHT_TOF_RESET_PIN, LOW);

//     Serial.println("Initializing Right Tof...");
//     if (rightTof.begin() == false) {
//         Serial.println("Right TOF not found - check wiring. Freezing...");
//         return;
//     }

//     // Set new address for right TOF
//     Serial.printf("Setting right tof address to: 0x%02X\n", RIGHT_TOF_ADDRESS);
//     if (rightTof.setAddress(RIGHT_TOF_ADDRESS) == false) {
//         Serial.println("Failed to set right tof address. Freezing...");
//         return;
//     }

//     // Verify new address
//     int newAddress = rightTof.getAddress();
//     Serial.printf("Right tof new address confirmed as: 0x%02X\n", newAddress);

//     // Initialize left TOF
//     digitalWrite(LEFT_TOF_RESET_PIN, LOW);
//     delay(100);

//     Serial.println("Initializing left tof");
//     if (leftTof.begin() == false) {
//         Serial.println("Left TOF not found - check wiring. Freezing...");
//         return;
//     }

//     // Configure both sensors
//     Serial.println("Configuring both sensors...");
    
//     // Set resolution for both sensors
//     rightTof.setResolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);
//     rightTof.setResolution(TOF_IMAGE_RESOLUTION * TOF_IMAGE_RESOLUTION);
//     rightTof.setRangingFrequency(TOF_RANGING_FREQUENCY);
//     leftTof.setRangingFrequency(TOF_RANGING_FREQUENCY);

//     Serial.println("TOF setup complete");
//     // Start ranging
//     // rightTof.startRanging();
//     // leftTof.startRanging();
// }

// void SensorSetup::setup_imu() {
//     if (!imu.begin_I2C()) {
//         Serial.println("Failed to find BNO08x chip");
//         while (1) { delay(10); }
//     }
//     Serial.println("BNO08x Found!");

//     // Enable the game rotation vector
//     setReports();

//     Serial.println("Reading events");
// }

// void SensorSetup::setReports() {
//     Serial.println("Setting desired reports");

//     // Enable Game Rotation Vector with update rate of 5ms (200Hz)
//     if (!imu.enableReport(SH2_GAME_ROTATION_VECTOR, IMU_UPDATE_FREQ_MICROSECS)) {
//         Serial.println("Could not enable game vector");
//     }

//     // Enable Accelerometer
//     // if (!imu.enableReport(SH2_ACCELEROMETER, IMU_UPDATE_FREQ_MICROSECS)) {
//     //     Serial.println("Could not enable accelerometer");
//     // }

//     // // Enable Gyroscope
//     // if (!imu.enableReport(SH2_GYROSCOPE_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
//     //     Serial.println("Could not enable gyroscope");
//     // }

//     // // Enable Magnetic Field
//     // if (!imu.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, IMU_UPDATE_FREQ_MICROSECS)) {
//     //     Serial.println("Could not enable magnetic field");
//     // }
// }
