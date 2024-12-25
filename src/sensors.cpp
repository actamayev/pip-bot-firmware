#include "./include/sensors.h"
#include "./include/utils.h"

void Sensors::initialize() {
    // Setup I2C
    Wire.begin(I2C_SDA, I2C_SCL, I2C_CLOCK_SPEED);

    // Initialize sensors
    initializeTofSensors();
    // initializeIMU();
}

void Sensors::initializeTofSensors() {
    pinMode(LEFT_TOF_RESET_PIN, OUTPUT);
    digitalWrite(LEFT_TOF_RESET_PIN, HIGH); //Hold sensor 2 in reset while we configure sensor 1

    pinMode(RIGHT_TOF_RESET_PIN, OUTPUT);
    digitalWrite(RIGHT_TOF_RESET_PIN, HIGH); //Reset sensor 1
    delay(100);
    digitalWrite(RIGHT_TOF_RESET_PIN, LOW); //Sensor 1 should now be available at default address 0x29

    bool isRegisteredAlready = check_address_on_i2c_line(RIGHT_TOF_ADDRESS);
    byte imager1Addr = DEFAULT_TOF_I2C_ADDRESS;
    if (isRegisteredAlready == true) {
        imager1Addr = RIGHT_TOF_ADDRESS;
    }
    Serial.println(F("Initializing sensor 1. This can take up to 10s. Please wait."));
    if (rightTof.sensor.begin(imager1Addr) == false) {
        Serial.println(F("Sensor 1 not found. Check wiring. Freezing..."));
        while (1);
    }

    Serial.print(F("Setting sensor 1 address to: 0x"));
    Serial.println(RIGHT_TOF_ADDRESS, HEX);

    if (rightTof.sensor.setAddress(RIGHT_TOF_ADDRESS) == false) {
        Serial.println(F("Sensor 1 failed to set new address. Please try again. Freezing..."));
        while (1);
    }

    int newAddress = rightTof.sensor.getAddress();

    Serial.print(F("New address of sensor 1 is: 0x"));
    Serial.println(newAddress, HEX);

    digitalWrite(LEFT_TOF_RESET_PIN, LOW); //Release sensor 2 from reset

    Serial.println(F("Initializing sensor 2. This can take up to 10s. Please wait."));
    if (leftTof.sensor.begin() == false) {
        Serial.println(F("Sensor 2 not found. Check wiring. Freezing..."));
        while (1);
    }

    rightTof.initialize();
    leftTof.initialize();
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
