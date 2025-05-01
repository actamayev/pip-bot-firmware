#include "./side_time_of_flight_sensor.h"

bool SideTimeOfFlightSensor::initialize(const uint8_t TOF_ADDRESS) {
    // Save the sensor address to the class member variable
    sensorAddress = TOF_ADDRESS;
    
    // Reset and initialize sensor
    Serial.print("Resetting sensor (");
    Serial.print(sensorAddress, HEX);
    Serial.println(")...");
    Reset_Specific_Sensor();
    Serial.println("Sensor reset complete");

    // Initialize the sensor in auto mode
    Serial.print("Initializing sensor (");
    Serial.print(sensorAddress, HEX);
    Serial.println(")...");
    Basic_Initialization_Auto_Mode();
    Serial.println("Sensor initialization complete");
    
    return true;
}

void SideTimeOfFlightSensor::update() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _lastUpdateTime;
    
    // Only update if enough time has passed
    if (elapsedTime >= DELAY_BETWEEN_READINGS) {
        _tofCounts = Read_Proximity_Data();
        _lastUpdateTime = currentTime;
    }
}

uint16_t SideTimeOfFlightSensor::getCounts() {
    update();
    return _tofCounts;
}

void SideTimeOfFlightSensor::Basic_Initialization_Auto_Mode() {
    //1.) Initialization
    //Switch the sensor off
    VCNL36828P_SET_PS_ON(sensorAddress, VCNL36828P_PS_ON_DIS);
    
    //2.) Setting up PS
    //PS_CONF1_H
    //Enable/disable high dynamic range (12 bit/16 bit) ADC output setting
    VCNL36828P_SET_PS_HD(sensorAddress, VCNL36828P_PS_HD_16Bits);
    
    //PS_CONF2_L
    //Set the integration time for one measurement; the pulse length "T" is determined by PS_ITB
    VCNL36828P_SET_PS_IT(sensorAddress, VCNL36828P_PS_IT_8T);
    //Set the pulse length "T" for PS_IT
    VCNL36828P_SET_PS_ITB(sensorAddress, VCNL36828P_PS_ITB_50us);
    //PS_CONF2_H
    //Set the VCSEL driving current
    VCNL36828P_SET_I_VCSEL(sensorAddress, VCNL36828P_I_VCSEL_20mA);
    
    //PS_CONF3_L
    //Set the measurement mode of the sensor
    VCNL36828P_SET_PS_MODE(sensorAddress, VCNL36828P_PS_MODE_AUTO_MODE);
    
    //3.) Switch On the sensor
    //Enable the internal calibration
    VCNL36828P_SET_PS_CAL(sensorAddress, VCNL36828P_PS_CAL_EN);
    //Switch the sensor on
    VCNL36828P_SET_PS_ON(sensorAddress, VCNL36828P_PS_ON_EN);
    
    //Delay needs to be changed depending on the API of the µ-controller
    delay(100);
}
