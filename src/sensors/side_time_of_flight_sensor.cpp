#include "side_time_of_flight_sensor.h"

bool SideTimeOfFlightSensor::canRetryInitialization() const {
    if (isInitialized) return false;

    unsigned long currentTime = millis();
    if (currentTime - lastInitAttempt < INIT_RETRY_INTERVAL) {
        return false; // Too soon to retry
    }

    if (initRetryCount >= MAX_INIT_RETRIES) {
        return false; // Too many retries
    }

    return true;
}

bool SideTimeOfFlightSensor::initialize(const uint8_t TOF_ADDRESS) {
    lastInitAttempt = millis();
    initRetryCount++;
    
    // Save the sensor address to the class member variable
    sensorAddress = TOF_ADDRESS;
    
    // Add a delay before trying to initialize
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        // Reset and initialize sensor
        Reset_Specific_Sensor();
        
        // Initialize the sensor in auto mode
        Basic_Initialization_Auto_Mode();
        
        // Try to read data to verify initialization
        uint16_t testValue = Read_Proximity_Data();
        
        // Check if the reading seems valid (this may need to be adjusted based on your sensor)
        if (testValue != 0xFFFF && testValue != 0) {  // Adjust these values based on what indicates failure
            isInitialized = true;
            return true;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // Delay between attempts
    }
    
    return false;
}

// NEW: Buffer-based sensor data update method
void SideTimeOfFlightSensor::updateSensorData() {
    // Only try to read if sensor is initialized
    if (!isInitialized) return;
    
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _lastUpdateTime;
    
    // Only update if enough time has passed (rate limiting)
    if (elapsedTime < DELAY_BETWEEN_READINGS) {
        return;
    }
    
    // Read current sensor data
    uint16_t counts = Read_Proximity_Data();
    _lastUpdateTime = currentTime;
    
    // Note: This method is called by SideTofManager which will combine
    // both left and right readings and write to buffer
    // We don't write to buffer directly here since we need both sensors
}

uint16_t SideTimeOfFlightSensor::getCurrentCounts() {
    // Only try to read if sensor is initialized
    if (!isInitialized) return 0;
    
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _lastUpdateTime;
    
    // Only update if enough time has passed
    if (elapsedTime >= DELAY_BETWEEN_READINGS) {
        _lastUpdateTime = currentTime;
    }
    
    return Read_Proximity_Data();
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
    vTaskDelay(pdMS_TO_TICKS(100));
}
