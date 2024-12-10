#include "./include/sensors.h"

void Sensors::initialize() {
    // Setup I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_CLOCK_SPEED);

    // Initialize sensors
    initializeTofSensors();
    initializeIMU();
}

void Sensors::initializeTofSensors() {
    pinMode(LEFT_TOF_RESET_PIN, OUTPUT);
    pinMode(RIGHT_TOF_RESET_PIN, OUTPUT);

    // Hold Left TOF in reset while we configure Right TOF
    digitalWrite(LEFT_TOF_RESET_PIN, HIGH);

    // Reset and initialize right tof:
    digitalWrite(RIGHT_TOF_RESET_PIN, HIGH);
    delay(100); // TODO: Figure out if we should use delay (use millis instead?)
    digitalWrite(RIGHT_TOF_RESET_PIN, LOW);

    Serial.println("Initializing Right Tof...");
    if (rightTof.sensor.begin() == false) {
        Serial.println("Right TOF not found - check wiring. Freezing...");
        return;
    }

    // Set new address for right TOF
    Serial.printf("Setting right tof address to: 0x%02X\n", RIGHT_TOF_ADDRESS);
    if (rightTof.sensor.setAddress(RIGHT_TOF_ADDRESS) == false) {
        Serial.println("Failed to set right tof address. Freezing...");
        return;
    }

    // Verify new address
    int newAddress = rightTof.sensor.getAddress();
    Serial.printf("Right tof new address confirmed as: 0x%02X\n", newAddress);

    // Initialize left TOF
    digitalWrite(LEFT_TOF_RESET_PIN, LOW);
    delay(100);

    Serial.println("Initializing Left tof");
    if (leftTof.sensor.begin() == false) {
        Serial.println("Left TOF not found - check wiring. Freezing...");
        return;
    }

    // Configure both sensors
    Serial.println("Configuring both sensors...");
    
    rightTof.initialize();
    leftTof.initialize();

    Serial.println("TOF setup complete");
}

void Sensors::initializeIMU() {
    if (!imu.initialize()) {
        Serial.println("IMU initialization failed");
        return;
    }
}

bool Sensors::getTofData(const VL53L5CX_ResultsData** leftData, const VL53L5CX_ResultsData** rightData) {
    bool leftSuccess = leftTof.getData();
    bool rightSuccess = rightTof.getData();
    
    if (leftSuccess) *leftData = &leftTof.getSensorData();
    if (rightSuccess) *rightData = &rightTof.getSensorData();
    
    return leftSuccess && rightSuccess;
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
