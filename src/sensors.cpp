#include "./include/sensors.h"
#include "./include/utils.h"

void Sensors::initialize() {
    // Setup I2C
    Wire.begin(I2C_SDA, I2C_SCL, I2C_CLOCK_SPEED);

    // Initialize sensors
    initializeTofSensors();
    initializeIMU();
    initializeIrSensors();
}

void Sensors::initializeTofSensors() {
    pinMode(LEFT_TOF_RESET_PIN, OUTPUT);
    pinMode(RIGHT_TOF_RESET_PIN, OUTPUT);

    // Hold both sensors in reset for 1 second:
    digitalWrite(LEFT_TOF_RESET_PIN, HIGH);
    digitalWrite(RIGHT_TOF_RESET_PIN, HIGH);
    delay(100);

    // Take right sensor out of reset:
    digitalWrite(RIGHT_TOF_RESET_PIN, LOW); //Right sensor should now be available at default address 0x29

    // TODO 12/25/24: If the ESP is reset (reset button is pressed) while it is doing i2c re-addressing, the Left tof sometimes either:
    // 1. re-addresses to a non 0x29 pin addr, or isn't picked up at all by the i2c line. Best solution i've found has been to unplug the esp and wait until the addreses get reset, or kill power to right tof's avdd/iovdd 
    bool isRegisteredAlready = check_address_on_i2c_line(RIGHT_TOF_ADDRESS);
    byte imager1Addr = DEFAULT_TOF_I2C_ADDRESS;
    if (isRegisteredAlready == true) {
        imager1Addr = RIGHT_TOF_ADDRESS;
    }
    Serial.println(F("Initializing right sensor. This can take up to 10s. Please wait."));
    if (rightTof.sensor.begin(imager1Addr) == false) {
        Serial.println(F("Right sensor not found. Check wiring. Freezing..."));
        while (1);
    }

    if (isRegisteredAlready == false) {
        Serial.print(F("Setting right sensor address to: 0x"));
        Serial.println(RIGHT_TOF_ADDRESS, HEX);

        if (rightTof.sensor.setAddress(RIGHT_TOF_ADDRESS) == false) {
            Serial.println(F("Right sensor failed to set new address. Please try again. Freezing..."));
            while (1);
        }
    }

    digitalWrite(LEFT_TOF_RESET_PIN, LOW); //Release left sensor from reset

    delay(100);
    scanI2C();
    Serial.println(F("Initializing left sensor. This can take up to 10s. Please wait."));
    if (leftTof.sensor.begin() == false) {
        Serial.println(F("Left Sensor not found. Check wiring. Freezing..."));
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

// const TofData& Sensors::getLeftTofData() {
//     return leftTof.getData();
// }

// const TofData& Sensors::getRightTofData() {
//     return rightTof.getData();
// }

const QuaternionData& Sensors::getQuaternion() {
    return imu.getQuaternion();
}

const AccelerometerData& Sensors::getAcceleration() {
    return imu.getAccelerometerData();
}

const GyroscopeData& Sensors::getGyroscope() {
    return imu.getGyroscopeData();
}

const MagnetometerData& Sensors::getMagneticField() {
    return imu.getMagnetometerData();
}

// Raw sensor value access if needed
bool Sensors::getImuData() {
    return imu.getData();
}

const sh2_SensorValue_t& Sensors::getImuSensorValue() const {
    return imu.getSensorValue();
}

void Sensors::initializeIrSensors() {
    Serial.println("Initializing IR Sensors...");

    if (!irSensor.initialize()) {
        Serial.println("IR Sensor initialization failed");
        return;
    }
    Serial.println("IR setup complete");
}

// Raw sensor value access if needed
bool Sensors::getIrData() {
    return irSensor.getData();
}

// Raw sensor value access if needed
void Sensors::sendIrCommand(uint32_t command) {
    return irSensor.sendIRCommand(command);
}
