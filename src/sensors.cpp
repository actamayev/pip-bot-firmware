#include "./include/sensors.h"

void Sensors::initialize() {
    // Setup I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_CLOCK_SPEED);

    // Initialize sensors
    initializeTofSensors();
    // initializeIMU();
}

void Sensors::initializeTofSensors() {
    // Configure reset pins
    pinMode(LEFT_TOF_RESET_PIN, OUTPUT);
    pinMode(RIGHT_TOF_RESET_PIN, OUTPUT);

    // Hold left sensor in reset while we configure right sensor
    digitalWrite(LEFT_TOF_RESET_PIN, HIGH);

    // Reset and initialize right sensor
    digitalWrite(RIGHT_TOF_RESET_PIN, HIGH);
    delay(100);
    digitalWrite(RIGHT_TOF_RESET_PIN, LOW);
    delay(100);  // Add delay after releasing from reset

    Serial.println("Initializing right sensor...");
    if (!rightTof.sensor.begin()) {
        Serial.println("Right sensor not found - check wiring. Freezing...");
        return;
    }

    // Set new address for right sensor
    Serial.printf("Setting right sensor address to: 0x%02X\n", RIGHT_TOF_ADDRESS);
    if (!rightTof.sensor.setAddress(RIGHT_TOF_ADDRESS)) {
        Serial.println("Failed to set right sensor address. Freezing...");
        return;
    }

    // Verify new address
    int newAddress = rightTof.sensor.getAddress();
    Serial.printf("Right sensor new address confirmed as: 0x%02X\n", newAddress);

    // Initialize left sensor
    digitalWrite(LEFT_TOF_RESET_PIN, LOW);
    delay(100);  // Add delay after releasing from reset

    Serial.println("Initializing left sensor...");
    if (!leftTof.sensor.begin()) {
        Serial.println("Left sensor not found - check wiring. Freezing...");
        return;
    }

    // Configure both sensors
    Serial.println("Configuring sensors...");

    // Initialize both sensors
    if (!leftTof.initialize()) {
        Serial.println("Left sensor not initializd...");
        return;
    }

    if (!rightTof.initialize()) {
        Serial.println("Right sensor not initializd...");
        return;
    }

    Serial.println("TOF setup complete");
}

void Sensors::initializeIMU() {
    Serial.println("Initializing IMU...");

    if (!imu.initialize()) {
        Serial.println("IMU initialization failed");
        return;
    }
    Serial.println("IMU setup complete");
}

bool Sensors::getTofData(const VL53L5CX_ResultsData** leftData, const VL53L5CX_ResultsData** rightData) {
    bool leftSuccess = leftTof.getData();
    bool rightSuccess = rightTof.getData();
    
    if (leftSuccess) *leftData = &leftTof.sensorData;
    if (rightSuccess) *rightData = &rightTof.sensorData;

    return leftSuccess;
}

bool Sensors::getQuaternion(float& qX, float& qY, float& qZ, float& qW) {
    return imu.getQuaternion(qX, qY, qZ, qW);
}

bool Sensors::getAcceleration(float& aX, float& aY, float& aZ) {
    return imu.getAcceleration(aX, aY, aZ);
}

bool Sensors::getGyroscope(float& gX, float& gY, float& gZ) {
    return imu.getGyroscope(gX, gY, gZ);
}

bool Sensors::getMagneticField(float& mX, float& mY, float& mZ) {
    return imu.getMagneticField(mX, mY, mZ);
}

// Raw sensor value access if needed
bool Sensors::getImuData() {
    return imu.getData();
}

const sh2_SensorValue_t& Sensors::getImuSensorValue() const {
    return imu.getSensorValue();
}
