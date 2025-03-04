#include "./side_time_of_flight_sensors.h"

bool SideTimeOfFlightSensors::initialize() {
    // Reset and initialize both sensors
    Serial.println("Resetting sensor 1 (0x60)...");
    Reset_Specific_Sensor(LEFT_TOF_ADDRESS);
    Serial.println("Sensor 1 reset complete");
    
    Serial.println("Resetting sensor 2 (0x51)...");
    Reset_Specific_Sensor(RIGHT_TOF_ADDRESS);
    Serial.println("Sensor 2 reset complete");
    
    // Initialize the sensors in auto mode
    Serial.println("Initializing sensor 1 (0x60)...");
    Basic_Initialization_Auto_Mode(LEFT_TOF_ADDRESS);
    Serial.println("Sensor 1 initialization complete");
    
    Serial.println("Initializing sensor 2 (0x51)...");
    Basic_Initialization_Auto_Mode(RIGHT_TOF_ADDRESS);
    Serial.println("Sensor 2 initialization complete");
    
    Serial.println("Starting distance readings from both sensors...");
    return true;
}

void SideTimeOfFlightSensors::update() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _lastUpdateTime;
    
    // Only update if enough time has passed
    if (elapsedTime >= DELAY_BETWEEN_READINGS) {
        _leftTofDistance = Read_Proximity_Data(LEFT_TOF_ADDRESS);
        _rightTofDistance = Read_Proximity_Data(RIGHT_TOF_ADDRESS);

        _lastUpdateTime = currentTime;
    }
}

SideTofDistances SideTimeOfFlightSensors::getBothDistances() {
    update();
    return {
        leftDistance: _leftTofDistance,
        rightDistance: _rightTofDistance,
    };
}
