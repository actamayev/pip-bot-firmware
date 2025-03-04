/*
 * VCNL36828P_PS.cpp
 *
 * Created  : 2 February 2023
 * Modified : 31 May 2023
 * Author   : HWanyusof
 * Version	: 1.2
 */

#include "./include/VCNL36828P_Prototypes.h"
#include "./include/VCNL36828P.h"
#include "./include/I2C_Functions.h"
#include "./include/VCNL36828P_Application_Library.h"
extern int I2C_Bus;
extern int VCNL36828P_SlaveAddress;

//****************************************************************************************************
//*****************************************Sensor API*************************************************

/*Enable/disable the internal calibration
 *VCNL36828P_SET_PS_CAL(Byte ps_cal)
 *Byte ps_cal - Input Parameter:
 *
 * VCNL36828P_PS_CAL_EN
 * VCNL36828P_PS_CAL_DIS
 */
void VCNL36828P_SET_PS_CAL(Byte ps_cal)
{
    struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_CAL_EN|VCNL36828P_PS_CAL_DIS))|ps_cal;
	VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
	WriteI2C_Bus(&VCNL36828P_Data);
}


/*Switch the sensor on/off
 *VCNL36828P_SET_PS_ON(Byte ps_on)
 *Byte ps_on - Input Parameter:
 *
 * VCNL36828P_PS_ON_EN
 * VCNL36828P_PS_ON_DIS
 */
void VCNL36828P_SET_PS_ON(Byte ps_on)
{
    struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_ON_EN|VCNL36828P_PS_ON_DIS))|ps_on;
	VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable high dynamic range (12 bit/16 bit) ADC output setting
 *VCNL36828P_SET_PS_HD(Byte HDBit)
 *Byte HDBit - Input Parameter:
 *
 * VCNL36828P_PS_HD_16Bits
 * VCNL36828P_PS_HD_12Bits
 */
void VCNL36828P_SET_PS_HD(Byte HDBit)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_HD_12Bits|VCNL36828P_PS_HD_16Bits))|HDBit;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable the sunlight protection mode interrupt setting
 *VCNL36828P_SET_PS_SP_INT(Byte SP_Bit)
 *Byte SP_Bit - Input Parameter:
 *
 * VCNL36828P_PS_SP_INT_EN
 * VCNL36828P_PS_SP_INT_DIS
 */
void VCNL36828P_SET_PS_SP_INT(Byte SP_Bit)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SP_INT_EN | VCNL36828P_PS_SP_INT_DIS))|SP_Bit;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable the smart persistence setting when the interrupt event is triggered
 *VCNL36828P_SET_PS_SMART_PERS(Byte Pers)
 *Byte Pers - Input Parameter:
 *
 * VCNL36828P_PS_SMART_PERS_EN
 * VCNL36828P_PS_SMART_PERS_DIS
 */
void VCNL36828P_SET_PS_SMART_PERS(Byte Pers)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SMART_PERS_DIS|VCNL36828P_PS_SMART_PERS_EN))|Pers;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the amount of consecutive threshold crossing events necessary to trigger interrupt
 *VCNL36828P_SET_PS_PERS(Byte Pers)
 *Byte Pers - Input Parameter:
 *
 * VCNL36828P_PS_PERS_1
 * VCNL36828P_PS_PERS_2
 * VCNL36828P_PS_PERS_3
 * VCNL36828P_PS_PERS_4
 */
void VCNL36828P_SET_PS_PERS(Byte Pers)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_PERS_1|VCNL36828P_PS_PERS_2|VCNL36828P_PS_PERS_3|VCNL36828P_PS_PERS_4))|Pers;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the interrupt mode setting
 *VCNL36828P_SET_PS_INT(Byte InterruptMode)
 *Byte InterruptMode - Input Parameter:
 *
 * VCNL36828P_PS_INT_EN
 * VCNL36828P_PS_INT_FIRST_HIGH
 * VCNL36828P_PS_INT_LOGIC_MODE
 * VCNL36828P_PS_INT_DIS
 */
void VCNL36828P_SET_PS_INT(Byte InterruptMode)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_INT_EN|VCNL36828P_PS_INT_FIRST_HIGH|VCNL36828P_PS_INT_LOGIC_MODE|VCNL36828P_PS_INT_DIS))|InterruptMode;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the measurement period
 *VCNL36828P_SET_PS_PERIOD(Byte Period)
 *Byte Period - Input Parameter:
 *
 * VCNL36828P_PS_PERIOD_400ms
 * VCNL36828P_PS_PERIOD_200ms
 * VCNL36828P_PS_PERIOD_100ms
 * VCNL36828P_PS_PERIOD_50ms
 */
void VCNL36828P_SET_PS_PERIOD(Byte Period)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_PERIOD_400ms|VCNL36828P_PS_PERIOD_200ms|VCNL36828P_PS_PERIOD_100ms|VCNL36828P_PS_PERIOD_50ms))|Period;
	VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the integration time for one measurement; the pulse length “T” is determined by PS_ITB
 *VCNL36828P_SET_PS_IT(Byte IntegrationTime)
 *Byte IntegrationTime - Input Parameter:
 *
 * VCNL36828P_PS_IT_8T
 * VCNL36828P_PS_IT_4T
 * VCNL36828P_PS_IT_2T
 * VCNL36828P_PS_IT_1T
 */
void VCNL36828P_SET_PS_IT(Byte IntegrationTime)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_IT_8T|VCNL36828P_PS_IT_4T|VCNL36828P_PS_IT_2T|VCNL36828P_PS_IT_1T))|IntegrationTime;
	VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the number of infrared signal pulses per measurement
 *VCNL36828P_SET_PS_MPS(Byte MultiPulse)
 *Byte MultiPulse - Input Parameter:
 *
 * VCNL36828P_PS_MPS_8
 * VCNL36828P_PS_MPS_4
 * VCNL36828P_PS_MPS_2
 * VCNL36828P_PS_MPS_1
 */
void VCNL36828P_SET_PS_MPS(Byte MultiPulse)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_MPS_8|VCNL36828P_PS_MPS_4|VCNL36828P_PS_MPS_2|VCNL36828P_PS_MPS_1))|MultiPulse;
	VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the pulse length “T” for PS_IT
 *VCNL36828P_SET_PS_ITB(Byte Itb)
 *Byte Itb - Input Parameter:
 *
 * VCNL36828P_PS_ITB_50us
 * VCNL36828P_PS_ITB_25us
 */
void VCNL36828P_SET_PS_ITB(Byte Itb)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_ITB_50us|VCNL36828P_PS_ITB_25us))|Itb;
	VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the gain of the ADC
 *VCNL36828P_SET_PS_GAIN(Byte Gain)
 *Byte Gain - Input Parameter:
 *
 * VCNL36828P_PS_GAIN_x2
 * VCNL36828P_PS_GAIN_x1
 */
void VCNL36828P_SET_PS_GAIN(Byte Gain)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_GAIN_x2|VCNL36828P_PS_GAIN_x1))|Gain;
	VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the sensitivity of the ADC
 *VCNL36828P_SET_PS_SENS(Byte Sens)
 *Byte Sens - Input Parameter:
 *
 * VCNL36828P_PS_SENS_HIGH
 * VCNL36828P_PS_SENS_NORMAL
 */
void VCNL36828P_SET_PS_SENS(Byte Sens)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SENS_HIGH|VCNL36828P_PS_SENS_NORMAL))|Sens;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable the internal crosstalk cancellation
 *VCNL36828P_SET_PS_OFFSET(Byte Offset)
 *Byte Offset - Input Parameter:
 *
 * VCNL36828P_PS_OFFSET_EN
 * VCNL36828P_PS_OFFSET_DIS
 */
void VCNL36828P_SET_PS_OFFSET(Byte Offset)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_OFFSET_EN|VCNL36828P_PS_OFFSET_DIS))|Offset;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the VCSEL driving current
 *VCNL36828P_SET_I_VCSEL(Byte I_Vcsel)
 *Byte I_Vcsel - Input Parameter:
 *
 * VCNL36828P_I_VCSEL_20mA
 * VCNL36828P_I_VCSEL_19mA
 * VCNL36828P_I_VCSEL_17mA
 * VCNL36828P_I_VCSEL_15mA
 * VCNL36828P_I_VCSEL_12mA
 * VCNL36828P_I_VCSEL_11mA
 * VCNL36828P_I_VCSEL_9mA
 * VCNL36828P_I_VCSEL_7mA
 */
void VCNL36828P_SET_I_VCSEL(Byte I_Vcsel)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_I_VCSEL_20mA|VCNL36828P_I_VCSEL_19mA|VCNL36828P_I_VCSEL_17mA|VCNL36828P_I_VCSEL_15mA|VCNL36828P_I_VCSEL_12mA|VCNL36828P_I_VCSEL_11mA|VCNL36828P_I_VCSEL_9mA|VCNL36828P_I_VCSEL_7mA))|I_Vcsel;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the active force mode trigger(This bit will be reset to 0 after the measurement cycle)
 *VCNL36828P_SET_PS_TRIG(Byte TriggerBit)
 *Byte TriggerBit - Input Parameter:
 *
 * VCNL36828P_PS_TRIG_EN
 * VCNL36828P_PS_TRIG_DIS
 */
void VCNL36828P_SET_PS_TRIG(Byte TriggerBit)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_TRIG_DIS|VCNL36828P_PS_TRIG_EN))|TriggerBit;
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]);
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the measurement mode of the sensor
 *VCNL36828P_SET_PS_MODE(Byte Mode)
 *Byte Mode - Input Parameter:
 *
 * VCNL36828P_PS_MODE_AUTO_MODE
 * VCNL36828P_PS_MODE_AF_MODE
 */
void VCNL36828P_SET_PS_MODE(Byte Mode)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_MODE_AUTO_MODE|VCNL36828P_PS_MODE_AF_MODE))|Mode;
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]);
	WriteI2C_Bus(&VCNL36828P_Data);
}


/*Set the short measurement period
 *VCNL36828P_SET_PS_SHORT_PERIOD(Byte Short_Period)
 *Byte Short_Period - Input Parameter:
 *
 * VCNL36828P_PS_SHORT_PERIOD_25ms
 * VCNL36828P_PS_SHORT_PERIOD_12_5ms
 * VCNL36828P_PS_SHORT_PERIOD_6_25ms
 * VCNL36828P_PS_SHORT_PERIOD_DIS
 */
void VCNL36828P_SET_PS_SHORT_PERIOD(Byte Short_Period)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]);
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SHORT_PERIOD_DIS|VCNL36828P_PS_SHORT_PERIOD_6_25ms|VCNL36828P_PS_SHORT_PERIOD_12_5ms|VCNL36828P_PS_SHORT_PERIOD_25ms))|Short_Period;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable the sunlight cancellation
 *VCNL36828P_SET_PS_SC(Byte SunlightBit)
 *Byte SunlightBit - Input Parameter:
 *
 * VCNL36828P_PS_SC_EN
 * VCNL36828P_PS_SC_DIS
 */
void VCNL36828P_SET_PS_SC(Byte SunlightBit)
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]);
	VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SC_DIS|VCNL36828P_PS_SC_EN))|SunlightBit;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the low threshold interrupt value
 *VCNL36828P_SET_PS_THDL(Word LowThreshold)
 *Word LowThreshold - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void VCNL36828P_SET_PS_THDL(Word LowThreshold)
{
	Byte LowByte = LowThreshold;
	Byte HighByte = LowThreshold>>8;
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_THDL;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	VCNL36828P_Data.WData[0] = LowByte;
	VCNL36828P_Data.WData[1] = HighByte;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the high threshold interrupt value
 *VCNL36828P_SET_PS_THDH(Word HighThreshold)
 *Word HighThreshold - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void VCNL36828P_SET_PS_THDH(Word HighThreshold)
{
	Byte LowByte = HighThreshold;
	Byte HighByte = HighThreshold>>8;
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_THDH;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	VCNL36828P_Data.WData[0] = LowByte;
	VCNL36828P_Data.WData[1] = HighByte;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the offset count cancellation value
 *VCNL36828P_SET_PS_CANC(Word CancelValue)
 *Word CancelValue - Input Parameter:
 *
 * Value between 0d0 and 0d4095
 */
void VCNL36828P_SET_PS_CANC(Word CancelValue)
{
	Byte LowByte = CancelValue;
	Byte HighByte = CancelValue>>8;
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CANC;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	VCNL36828P_Data.WData[0] = LowByte;
	VCNL36828P_Data.WData[1] = HighByte;
	WriteI2C_Bus(&VCNL36828P_Data);
}

/*Read the proximity output data
 *VCNL36828P_GET_PS_DATA()
 *
 *returns PS Data between 0d0 and 0d65535
 */
Word VCNL36828P_GET_PS_DATA()
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_DATA;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	return (VCNL36828P_Data.RData[1]<<8|VCNL36828P_Data.RData[0]);
}

/*Read the interrupt flag register
 *VCNL36828P_GET_INT_FLAG() returns interrupt flag status.
 *Please refer to Table 15 on the Datasheet page 16
 */
Word VCNL36828P_GET_INT_FLAG()
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_INT_FLAG;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	return VCNL36828P_Data.RData[1];
}

/*Read Register value
 *VCNL36828P_READ_REG(Byte Reg)
 *Byte Reg - Input Parameter:
 *
 * VCNL36828P_PS_CONF_1
 * VCNL36828P_PS_CONF_2
 * VCNL36828P_PS_CONF_3
 * VCNL36828P_PS_THDL
 * VCNL36828P_PS_THDH
 * VCNL36828P_PS_CANC
 * VCNL36828P_PS_DATA
 * VCNL36828P_INT_FLAG
 * VCNL36828P_ID
 *
 *returns Register Value between 0d0/0x00 and 0d65535/0xFFFF
 */
Word VCNL36828P_READ_REG(Byte Reg)
{
	Word RegValue;
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = Reg;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	RegValue = VCNL36828P_Data.RData[1]<<8|VCNL36828P_Data.RData[0];
	return RegValue;
}

/*Read the PS_ON bit
*returns 1 for Shutdown and 0 for Sensor On
*/
bool VCNL36828P_GET_PS_ON_Bit()
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	if ((VCNL36828P_Data.RData[0] & 0x01) == 0x01) {return 1;}
    else
    return 0;
}

/*Read the PS_MODE bit
 *returns 1 for AF Mode and 0 for Auto Mode
 */
bool VCNL36828P_GET_PS_MODE_Bit()
{
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	if ((VCNL36828P_Data.RData[0] & 0x10) == 0x10) {return 1;}
    else
    return 0;
}

/*Read Device ID
 *Returns ID
 */
Word VCNL36828P_GET_ID()
{
	Word RegValue;
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = VCNL36828P_SlaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_ID;
	VCNL36828P_Data.Select_I2C_Bus = I2C_Bus;
	ReadI2C_Bus(&VCNL36828P_Data);
	RegValue = VCNL36828P_Data.RData[1]<<8|VCNL36828P_Data.RData[0];
	return RegValue;
}


