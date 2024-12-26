#include "./include/sensors.h"
#include "./include/utils.h"

void Sensors::initialize() {
    // Setup I2C
    Wire.begin(I2C_SDA, I2C_SCL, I2C_CLOCK_SPEED);

    // Initialize sensors
    initializeTofSensors();
    // initializeIMU();
}

void scanI2C() {
  byte error, address;
  int devicesFound = 0;
  
  Serial.println("Scanning I2C bus...");
  
  for (address = 1; address < 128; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("Device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      devicesFound++;
    }
  }
  
  if (devicesFound == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.printf("Found %d device(s)\n", devicesFound);
  }
  Serial.println();
}

void Sensors::initializeTofSensors() {
    pinMode(LEFT_TOF_RESET_PIN, OUTPUT);
    digitalWrite(LEFT_TOF_RESET_PIN, HIGH); //Hold left sensor in reset while we configure right sensor

    pinMode(RIGHT_TOF_RESET_PIN, OUTPUT);
    digitalWrite(RIGHT_TOF_RESET_PIN, HIGH); //Reset right sensor
    delay(100);
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

    delay(1000);
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
