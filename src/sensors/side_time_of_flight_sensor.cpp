#include "side_time_of_flight_sensor.h"

bool SideTimeOfFlightSensor::initialize(const uint8_t TOF_ADDRESS) {
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
        
        // Try to read data to verify initialization (use raw reading since calibration not loaded yet)
        uint16_t testValue = VCNL36828P_GET_PS_DATA(sensorAddress);
        
        // Check if the reading seems valid (this may need to be adjusted based on your sensor)
        if (testValue != 0xFFFF && testValue != 0) {  // Adjust these values based on what indicates failure
            isInitialized = true;
            
            // Load existing calibration or perform new calibration
            loadCalibrationFromPreferences();
            if (!isCalibrated) {
                SerialQueueManager::getInstance().queueMessage("No calibration found - performing auto-calibration");
                performCalibration();
            }
            
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
    
    // Skip rate limiting entirely for maximum performance like performance test
    // unsigned long currentTime = millis();
    // unsigned long elapsedTime = currentTime - _lastUpdateTime;
    // if (elapsedTime < DELAY_BETWEEN_READINGS) return;
    
    // Read current sensor data with no throttling
    uint16_t counts = Read_Proximity_Data();
    // _lastUpdateTime = currentTime;
    
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
    VCNL36828P_SET_PS_IT(sensorAddress, VCNL36828P_PS_IT_1T); // Fastest - matches performance test
    //Set the pulse length "T" for PS_IT
    VCNL36828P_SET_PS_ITB(sensorAddress, VCNL36828P_PS_ITB_25us); // Shortest - matches performance test
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

void SideTimeOfFlightSensor::loadCalibrationFromPreferences() {
    PreferencesManager& prefs = PreferencesManager::getInstance();
    
    if (prefs.hasSideTofCalibration(sensorAddress)) {
        baselineValue = prefs.getSideTofBaseline(sensorAddress);
        useHardwareCalibration = prefs.getSideTofUseHardwareCalibration(sensorAddress);
        isCalibrated = true;
        
        char logMessage[128];
        snprintf(logMessage, sizeof(logMessage), 
                "Loaded calibration for sensor 0x%02X: baseline=%u, hw_calib=%s", 
                sensorAddress, baselineValue, useHardwareCalibration ? "true" : "false");
        SerialQueueManager::getInstance().queueMessage(logMessage);
        
        if (useHardwareCalibration) {
            applyHardwareCalibration(baselineValue);
        }
    } else {
        isCalibrated = false;
        char logMessage[128];
        snprintf(logMessage, sizeof(logMessage), 
                "No stored calibration found for sensor 0x%02X", sensorAddress);
        SerialQueueManager::getInstance().queueMessage(logMessage);
    }
}

bool SideTimeOfFlightSensor::performCalibration() {
    char logMessage[128];
    snprintf(logMessage, sizeof(logMessage), "Calibrating sensor 0x%02X...", sensorAddress);
    SerialQueueManager::getInstance().queueMessage(logMessage);
    
    SerialQueueManager::getInstance().queueMessage("Make sure no obstacles are in front of the sensors!");
    vTaskDelay(pdMS_TO_TICKS(3000)); // Give time to clear obstacles
    
    uint16_t baseline = captureBaselineReading();
    
    snprintf(logMessage, sizeof(logMessage), "Baseline reading: %u", baseline);
    SerialQueueManager::getInstance().queueMessage(logMessage);
    
    baselineValue = baseline;
    
    // Use hardware cancellation if baseline is within valid range (0-4095)
    if (baseline <= 4095) {
        useHardwareCalibration = true;
        applyHardwareCalibration(baseline);
        SerialQueueManager::getInstance().queueMessage("Using hardware calibration");
    } else {
        useHardwareCalibration = false;
        VCNL36828P_SET_PS_CANC(sensorAddress, 0);  // Clear hardware cancellation
        SerialQueueManager::getInstance().queueMessage("Using software calibration (baseline too high for hardware)");
    }
    
    // Store calibration in NVS
    PreferencesManager& prefs = PreferencesManager::getInstance();
    prefs.storeSideTofCalibration(sensorAddress, baselineValue, useHardwareCalibration);
    
    isCalibrated = true;
    
    snprintf(logMessage, sizeof(logMessage), 
            "Sensor 0x%02X calibrated successfully - baseline: %u", sensorAddress, baselineValue);
    SerialQueueManager::getInstance().queueMessage(logMessage);
    
    return true;
}

uint16_t SideTimeOfFlightSensor::captureBaselineReading() {
    const int numSamples = 10;
    uint32_t sum = 0;
    
    // Take multiple readings and average them for better accuracy
    for (int i = 0; i < numSamples; i++) {
        uint16_t reading = VCNL36828P_GET_PS_DATA(sensorAddress);
        sum += reading;
        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay between readings
    }

    return (uint16_t)(sum / numSamples);
}

void SideTimeOfFlightSensor::applyHardwareCalibration(uint16_t baseline) {
    VCNL36828P_SET_PS_CANC(sensorAddress, baseline);
}

uint16_t SideTimeOfFlightSensor::applyCalibration(uint16_t rawReading) {
    if (!isCalibrated) return rawReading; // Return raw reading if not calibrated
    
    // If using hardware calibration, the reading is already calibrated
    if (useHardwareCalibration) {
        return rawReading;
    }
    
    // Apply software calibration (subtract baseline)
    int32_t calibratedReading = (int32_t)rawReading - (int32_t)baselineValue;
    if (calibratedReading < 0) {
        calibratedReading = 0;
    }
    
    return (uint16_t)calibratedReading;
}
