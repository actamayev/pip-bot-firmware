#pragma once
#include <Arduino.h>
#include "VCNL36828P.h"
#include "VCNL36828P_Prototypes.h"
#include "typedefinition.h"
#include "VCNL36828P_Application_Library.h"
#include "./config.h"
#include "./structs.h"

class SideTimeOfFlightSensors {
    public:
        SideTimeOfFlightSensors() = default;

        bool initialize();
        SideTofDistances getBothDistances();
    private:
        int VCNL36828P_SlaveAddress = 0x60;
        int I2C_Bus = 1;
            // Reset a specific sensor by address
        void Reset_Specific_Sensor(int sensorAddress) {
            VCNL36828P_SlaveAddress = sensorAddress;
            Reset_Sensor();
            delay(100);
        }

        // Read proximity data from a specific sensor
        uint16_t Read_Proximity_Data(int sensorAddress) {
            VCNL36828P_SlaveAddress = sensorAddress;
            return VCNL36828P_GET_PS_DATA();
        }

        uint16_t _leftTofDistance;
        uint16_t _rightTofDistance;

        // Simplified initialization function for a specific sensor
        void Basic_Initialization_Auto_Mode(int sensorAddress) {
            // Set the current sensor address
            VCNL36828P_SlaveAddress = sensorAddress;
            
            //1.) Initialization
            //Switch the sensor off
            VCNL36828P_SET_PS_ON(VCNL36828P_PS_ON_DIS);
            
            //2.) Setting up PS
            //PS_CONF1_H
            //Enable/disable high dynamic range (12 bit/16 bit) ADC output setting
            VCNL36828P_SET_PS_HD(VCNL36828P_PS_HD_16Bits);
            
            //PS_CONF2_L
            //Set the integration time for one measurement; the pulse length "T" is determined by PS_ITB
            VCNL36828P_SET_PS_IT(VCNL36828P_PS_IT_8T);
            //Set the pulse length "T" for PS_IT
            VCNL36828P_SET_PS_ITB(VCNL36828P_PS_ITB_50us);
            //PS_CONF2_H
            //Set the VCSEL driving current
            VCNL36828P_SET_I_VCSEL(VCNL36828P_I_VCSEL_20mA);
            
            //PS_CONF3_L
            //Set the measurement mode of the sensor
            VCNL36828P_SET_PS_MODE(VCNL36828P_PS_MODE_AUTO_MODE);
            
            //3.) Switch On the sensor
            //Enable the internal calibration
            VCNL36828P_SET_PS_CAL(VCNL36828P_PS_CAL_EN);
            //Switch the sensor on
            VCNL36828P_SET_PS_ON(VCNL36828P_PS_ON_EN);
            
            //Delay needs to be changed depending on the API of the µ-controller
            delay(1000);
        }

        unsigned long _lastUpdateTime;
        static constexpr unsigned long DELAY_BETWEEN_READINGS = 100; //ms

        void update();
};
