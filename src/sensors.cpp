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
    bool leftSuccess = leftTof.getTofData();
    bool rightSuccess = rightTof.getTofData();
    Serial.printf("leftSuccess %d", leftSuccess);
    Serial.printf("rightSuccess %d", rightSuccess);

    if (leftSuccess) *leftData = &leftTof.sensorData;
    if (rightSuccess) *rightData = &rightTof.sensorData;

    return leftSuccess && rightSuccess;
}

// const TofData& Sensors::getLeftTofData() {
//     return leftTof.getTofData();
// }

// const TofData& Sensors::getRightTofData() {
//     return rightTof.getTofData();
// }

EulerAngles& Sensors::getEulerAngles() {
    return imu.getEulerAngles();
}

float Sensors::getPitch() {
    return imu.getPitch();
}

float Sensors::getYaw() {
    return imu.getYaw();
}

float Sensors::getRoll() {
    return imu.getRoll();
}

float Sensors::getXAccel() {
    return imu.getPitch();
}

float Sensors::getYAccel() {
    return imu.getYaw();
}

float Sensors::getZAccel() {
    return imu.getRoll();
}

float Sensors::getXRotationRate() {
    return imu.getXRotationRate();
}

float Sensors::getYRotationRate() {
    return imu.getYRotationRate();
}

float Sensors::getZRotationRate() {
    return imu.getZRotationRate();
}

float Sensors::getMagneticFieldX() {
    return imu.getMagneticFieldX();
}

float Sensors::getMagneticFieldY() {
    return imu.getMagneticFieldY();
}

float Sensors::getMagneticFieldZ() {
    return imu.getMagneticFieldZ();
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
    return irSensor.getIrData();
}

// Raw sensor value access if needed
void Sensors::sendIrCommand(uint32_t command) {
    return irSensor.sendIRCommand(command);
}
