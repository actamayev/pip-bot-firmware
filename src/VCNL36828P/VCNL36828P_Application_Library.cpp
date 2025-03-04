/*
 * VCNL36828P_Application_Library.cpp
 *
 * Created  : 2 February 2023
 * Modified : 31 May 2023
 * Author   : HWanyusof
 * Version	: 1.2
 */

#include "./include/VCNL36828P_Application_Library.h"
#include "./include/VCNL36828P_Prototypes.h"
#include "./include/VCNL36828P.h"
#include "./include/I2C_Functions.h"

extern int I2C_Bus;
extern int VCNL36828P_SlaveAddress;

//****************************************************************************************************
//***************************************Application API**********************************************

//Reset the Sensor to the default value
void Reset_Sensor()
{
	struct TransferData VCNL36828P_Data;

	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	VCNL36828P_Data.WData[0] = 0x00;
	VCNL36828P_Data.WData[1] = 0x00;
	WriteI2C_Bus(&VCNL36828P_Data);

	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	VCNL36828P_Data.WData[0] = 0x00;
	VCNL36828P_Data.WData[1] = 0x00;
	WriteI2C_Bus(&VCNL36828P_Data);

	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	VCNL36828P_Data.WData[0] = 0x00;
	VCNL36828P_Data.WData[1] = 0x00;
	WriteI2C_Bus(&VCNL36828P_Data);

	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_THDL;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	VCNL36828P_Data.WData[0] = 0x00;
	VCNL36828P_Data.WData[1] = 0x00;
	WriteI2C_Bus(&VCNL36828P_Data);

	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_THDH;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	VCNL36828P_Data.WData[0] = 0x00;
	VCNL36828P_Data.WData[1] = 0x00;
	WriteI2C_Bus(&VCNL36828P_Data);

    VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CANC;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	VCNL36828P_Data.WData[0] = 0x00;
	VCNL36828P_Data.WData[1] = 0x00;
	WriteI2C_Bus(&VCNL36828P_Data);

}

//Print the output of the sensor
void Print_Data_Only()
{
	Word value;

	#define TRANSMIT_BUFFER_SIZE  128
	char   TransmitBuffer[TRANSMIT_BUFFER_SIZE];
	char   TransmitBuffer2[TRANSMIT_BUFFER_SIZE];

	#ifdef Arduino

		Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>PS<<<<<<<<<<<<<<<<<<<<<<<<<");
		delay(50);

		//Print Proximity Data
		//Set Trigger for the AF Mode + Associated delay due to IT
		if(VCNL36828P_GET_PS_MODE_Bit() == 1)
		{
			//Set trigger to start a measurement
			VCNL36828P_SET_PS_TRIG(VCNL36828P_PS_TRIG_EN);

			//Delay of PS Measurement + other Circuit Delay
			delay(50);
		}

		//Delay for Auto Mode + Associated delay due to IT
		if(VCNL36828P_GET_PS_MODE_Bit() == 0)
		{
			//Delay of PS Measurement + other Circuit Delay
			delay(50);
		}

		if(VCNL36828P_SlaveAddress == 0x60)
		{
			Serial.println(">>>>>>>>>>>>>>>>>Sensor 1 - 0x60<<<<<<<<<<<<<<<<<<<");
			delay(50);
		}

		if(VCNL36828P_SlaveAddress == 0x51)
		{
			Serial.println(">>>>>>>>>>>>>>>>>Sensor 2 - 0x51<<<<<<<<<<<<<<<<<<<");
			delay(50);
		}

		value = VCNL36828P_GET_ID();
		Serial.print(">>>>>>>ID : 0x");
		Serial.print(value,HEX);
		Serial.println("<<<<<<<<");
		delay(50);

		value = VCNL36828P_READ_REG(VCNL36828P_PS_DATA);
		Serial.print(">>>>>>>Proximity Data : ");
		Serial.print(value,DEC);
		Serial.println(" Counts<<<<<<<<");
		delay(50);

		//Print the Interrupt Flag
		Serial.println("***************************************************");
		delay(50);

		//Print the Interrupt Flag
		value = VCNL36828P_GET_INT_FLAG();
		Serial.print(">>>>>>>Interrupt Flag : 0x");
		Serial.print(value,HEX);
		Serial.println("<<<<<<<<");
		delay(50);

		Serial.println("***************************************************");
		delay(50);

		Serial.println(" ");
		delay(50);

		Serial.println(" ");

		delay(1000);

	#endif
}


/*Print the variable in DEC for debugging
 *Print_Variable_DEC(Word Var)
 *Word Var - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void Print_Variable_DEC(Word Var)
{

	#define TRANSMIT_BUFFER_SIZE  128
	char   TransmitBuffer[TRANSMIT_BUFFER_SIZE];
	char   TransmitBuffer2[TRANSMIT_BUFFER_SIZE];

	#ifdef Arduino

		Serial.println("***************************************************");
		delay(50);

		Serial.print(">>>>>>>Variable : 0d");
		Serial.println(Var,DEC);
		delay(50);

		Serial.println("***************************************************");

		delay(2000);

	#endif


}

/*Print the variable in HEX for debugging
 *Print_Variable_HEX(Word Var)
 *Word Var - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void Print_Variable_HEX(Word Var)
{

	#define TRANSMIT_BUFFER_SIZE  128
	char   TransmitBuffer[TRANSMIT_BUFFER_SIZE];
	char   TransmitBuffer2[TRANSMIT_BUFFER_SIZE];

	#ifdef Arduino

		Serial.println("***************************************************");
		delay(50);

		Serial.print(">>>>>>>Variable : 0d");
		Serial.println(Var,DEC);
		delay(50);

		Serial.println("***************************************************");

		delay(2000);

	#endif

}

//Reverses a string 'str' of length 'len'
void reverse(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

//Converts a given integer x to string str[].
//d is the number of digits required in the output.
//If d is more than the number of digits in x,
//then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;

    while (x)
	{
		//Store and convert int to char (Valid for single digit)
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }

    //If number of digits required is more, then
    //add 0s at the beginning
    while (i < d) str[i++] = '0';

	//Reverse the string characters in the array str
    reverse(str, i);

	//Place the null character at the end of the array
    str[i] = '\0';

	//Return the position i
    return i;
}

//Converts a floating-point/double number to a string.
void ftoa(float n, char* res, int afterpoint)
{
    //Extract integer part
    int ipart = (int)n;

    //Extract decimal part
    float fpart = n - (float)ipart;

    //Convert integer part to string and the function returns the position after the interger
    int i = intToStr(ipart, res, 0);

    //Check for display option after point
    if (afterpoint != 0)
	{
		//Add dot after the integer part
        res[i] = '.';

        //Multiply decimal part by 10^decimal point
        fpart = fpart* pow(10, afterpoint);

		//Convert decimal part to string
        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}
