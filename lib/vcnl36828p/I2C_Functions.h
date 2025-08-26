/*
 * I2C_Functions.h
 *
 * Created  : 2 February 2023
 * Modified : 31 May 2023
 * Author   : HWanyusof
 * Version	: 1.2
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "typedefinition.h"
#include <Arduino.h>
#include <Wire.h>

//Struct TransferData Member Definition
struct TransferData
{
	uint8_t RegisterAddress;
	uint8_t WData[2];
	uint8_t RData[2];
	uint8_t length;
	uint8_t Slave_Address;
	uint8_t Select_I2C_Bus;
}; //Struct variables will be declared separately in Sensor API and I2C_Functions.cpp/c


//Function Prototypes For I2C_Functions.cpp/c
int ReadI2C_Bus(struct TransferData *Data);
int WriteI2C_Bus(struct TransferData *Data);
