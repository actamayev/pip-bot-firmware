/*
 * VCNL36828P_Prototypes.h
 *
 * Created  : 2 February 2023
 * Modified : 6 March 2025
 * Author   : HWanyusof (original), Grok 3 (modified)
 * Version  : 1.3
 */

#include "typedefinition.h"

//****************************************************************************************************
//*****************************************Sensor API*************************************************

/*Enable/disable the internal calibration
 *VCNL36828P_SET_PS_CAL(int slaveAddress, Byte ps_cal, int i2cBus = 1)
 *Byte ps_cal - Input Parameter:
 *
 * VCNL36828P_PS_CAL_EN
 * VCNL36828P_PS_CAL_DIS
 */
void VCNL36828P_SET_PS_CAL(int slaveAddress, Byte ps_cal, int i2cBus = 1);

/*Switch the sensor on/off
 *VCNL36828P_SET_PS_ON(int slaveAddress, Byte ps_on, int i2cBus = 1)
 *Byte ps_on - Input Parameter:
 *
 * VCNL36828P_PS_ON_EN
 * VCNL36828P_PS_ON_DIS
 */
void VCNL36828P_SET_PS_ON(int slaveAddress, Byte ps_on, int i2cBus = 1);

/*Enable/disable high dynamic range (12 bit/16 bit) ADC output setting
 *VCNL36828P_SET_PS_HD(int slaveAddress, Byte HDBit, int i2cBus = 1)
 *Byte HDBit - Input Parameter:
 *
 * VCNL36828P_PS_HD_16Bits
 * VCNL36828P_PS_HD_12Bits
 */
void VCNL36828P_SET_PS_HD(int slaveAddress, Byte HDBit, int i2cBus = 1);

/*Enable/disable the sunlight protection mode interrupt setting
 *VCNL36828P_SET_PS_SP_INT(int slaveAddress, Byte SP_Bit, int i2cBus = 1)
 *Byte SP_Bit - Input Parameter:
 *
 * VCNL36828P_PS_SP_INT_EN
 * VCNL36828P_PS_SP_INT_DIS
 */
void VCNL36828P_SET_PS_SP_INT(int slaveAddress, Byte SP_Bit, int i2cBus = 1);

/*Enable/disable the smart persistence setting when the interrupt event is triggered
 *VCNL36828P_SET_PS_SMART_PERS(int slaveAddress, Byte Pers, int i2cBus = 1)
 *Byte Pers - Input Parameter:
 *
 * VCNL36828P_PS_SMART_PERS_EN
 * VCNL36828P_PS_SMART_PERS_DIS
 */
void VCNL36828P_SET_PS_SMART_PERS(int slaveAddress, Byte Pers, int i2cBus = 1);

/*Set the amount of consecutive threshold crossing events necessary to trigger interrupt
 *VCNL36828P_SET_PS_PERS(int slaveAddress, Byte Pers, int i2cBus = 1)
 *Byte Pers - Input Parameter:
 *
 * VCNL36828P_PS_PERS_1
 * VCNL36828P_PS_PERS_2
 * VCNL36828P_PS_PERS_3
 * VCNL36828P_PS_PERS_4
 */
void VCNL36828P_SET_PS_PERS(int slaveAddress, Byte Pers, int i2cBus = 1);

/*Set the interrupt mode setting
 *VCNL36828P_SET_PS_INT(int slaveAddress, Byte InterruptMode, int i2cBus = 1)
 *Byte InterruptMode - Input Parameter:
 *
 * VCNL36828P_PS_INT_EN
 * VCNL36828P_PS_INT_FIRST_HIGH
 * VCNL36828P_PS_INT_LOGIC_MODE
 * VCNL36828P_PS_INT_DIS
 */
void VCNL36828P_SET_PS_INT(int slaveAddress, Byte InterruptMode, int i2cBus = 1);

/*Set the measurement period
 *VCNL36828P_SET_PS_PERIOD(int slaveAddress, Byte Period, int i2cBus = 1)
 *Byte Period - Input Parameter:
 *
 * VCNL36828P_PS_PERIOD_400ms
 * VCNL36828P_PS_PERIOD_200ms
 * VCNL36828P_PS_PERIOD_100ms
 * VCNL36828P_PS_PERIOD_50ms
 */
void VCNL36828P_SET_PS_PERIOD(int slaveAddress, Byte Period, int i2cBus = 1);

/*Set the integration time for one measurement; the pulse length “T” is determined by PS_ITB
 *VCNL36828P_SET_PS_IT(int slaveAddress, Byte IntegrationTime, int i2cBus = 1)
 *Byte IntegrationTime - Input Parameter:
 *
 * VCNL36828P_PS_IT_8T
 * VCNL36828P_PS_IT_4T
 * VCNL36828P_PS_IT_2T
 * VCNL36828P_PS_IT_1T
 */
void VCNL36828P_SET_PS_IT(int slaveAddress, Byte IntegrationTime, int i2cBus = 1);

/*Set the number of infrared signal pulses per measurement
 *VCNL36828P_SET_PS_MPS(int slaveAddress, Byte MultiPulse, int i2cBus = 1)
 *Byte MultiPulse - Input Parameter:
 *
 * VCNL36828P_PS_MPS_8
 * VCNL36828P_PS_MPS_4
 * VCNL36828P_PS_MPS_2
 * VCNL36828P_PS_MPS_1
 */
void VCNL36828P_SET_PS_MPS(int slaveAddress, Byte MultiPulse, int i2cBus = 1);

/*Set the pulse length “T” for PS_IT
 *VCNL36828P_SET_PS_ITB(int slaveAddress, Byte Itb, int i2cBus = 1)
 *Byte Itb - Input Parameter:
 *
 * VCNL36828P_PS_ITB_50us
 * VCNL36828P_PS_ITB_25us
 */
void VCNL36828P_SET_PS_ITB(int slaveAddress, Byte Itb, int i2cBus = 1);

/*Set the gain of the ADC
 *VCNL36828P_SET_PS_GAIN(int slaveAddress, Byte Gain, int i2cBus = 1)
 *Byte Gain - Input Parameter:
 *
 * VCNL36828P_PS_GAIN_x2
 * VCNL36828P_PS_GAIN_x1
 */
void VCNL36828P_SET_PS_GAIN(int slaveAddress, Byte Gain, int i2cBus = 1);

/*Set the sensitivity of the ADC
 *VCNL36828P_SET_PS_SENS(int slaveAddress, Byte Sens, int i2cBus = 1)
 *Byte Sens - Input Parameter:
 *
 * VCNL36828P_PS_SENS_HIGH
 * VCNL36828P_PS_SENS_NORMAL
 */
void VCNL36828P_SET_PS_SENS(int slaveAddress, Byte Sens, int i2cBus = 1);

/*Enable/disable the internal crosstalk cancellation
 *VCNL36828P_SET_PS_OFFSET(int slaveAddress, Byte Offset, int i2cBus = 1)
 *Byte Offset - Input Parameter:
 *
 * VCNL36828P_PS_OFFSET_EN
 * VCNL36828P_PS_OFFSET_DIS
 */
void VCNL36828P_SET_PS_OFFSET(int slaveAddress, Byte Offset, int i2cBus = 1);

/*Set the VCSEL driving current
 *VCNL36828P_SET_I_VCSEL(int slaveAddress, Byte I_Vcsel, int i2cBus = 1)
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
void VCNL36828P_SET_I_VCSEL(int slaveAddress, Byte I_Vcsel, int i2cBus = 1);

/*Set the active force mode trigger(This bit will be reset to 0 after the measurement cycle)
 *VCNL36828P_SET_PS_TRIG(int slaveAddress, Byte TriggerBit, int i2cBus = 1)
 *Byte TriggerBit - Input Parameter:
 *
 * VCNL36828P_PS_TRIG_EN
 * VCNL36828P_PS_TRIG_DIS
 */
void VCNL36828P_SET_PS_TRIG(int slaveAddress, Byte TriggerBit, int i2cBus = 1);

/*Set the measurement mode of the sensor
 *VCNL36828P_SET_PS_MODE(int slaveAddress, Byte Mode, int i2cBus = 1)
 *Byte Mode - Input Parameter:
 *
 * VCNL36828P_PS_MODE_AUTO_MODE
 * VCNL36828P_PS_MODE_AF_MODE
 */
void VCNL36828P_SET_PS_MODE(int slaveAddress, Byte Mode, int i2cBus = 1);

/*Set the short measurement period
 *VCNL36828P_SET_PS_SHORT_PERIOD(int slaveAddress, Byte Short_Period, int i2cBus = 1)
 *Byte Short_Period - Input Parameter:
 *
 * VCNL36828P_PS_SHORT_PERIOD_25ms
 * VCNL36828P_PS_SHORT_PERIOD_12_5ms
 * VCNL36828P_PS_SHORT_PERIOD_6_25ms
 * VCNL36828P_PS_SHORT_PERIOD_DIS
 */
void VCNL36828P_SET_PS_SHORT_PERIOD(int slaveAddress, Byte Short_Period, int i2cBus = 1);

/*Enable/disable the sunlight cancellation
 *VCNL36828P_SET_PS_SC(int slaveAddress, Byte SunlightBit, int i2cBus = 1)
 *Byte SunlightBit - Input Parameter:
 *
 * VCNL36828P_PS_SC_EN
 * VCNL36828P_PS_SC_DIS
 */
void VCNL36828P_SET_PS_SC(int slaveAddress, Byte SunlightBit, int i2cBus = 1);

/*Set the low threshold interrupt value
 *VCNL36828P_SET_PS_THDL(int slaveAddress, Word LowThreshold, int i2cBus = 1)
 *Word LowThreshold - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void VCNL36828P_SET_PS_THDL(int slaveAddress, Word LowThreshold, int i2cBus = 1);

/*Set the high threshold interrupt value
 *VCNL36828P_SET_PS_THDH(int slaveAddress, Word HighThreshold, int i2cBus = 1)
 *Word HighThreshold - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void VCNL36828P_SET_PS_THDH(int slaveAddress, Word HighThreshold, int i2cBus = 1);

/*Set the offset count cancellation value
 *VCNL36828P_SET_PS_CANC(int slaveAddress, Word CancelValue, int i2cBus = 1)
 *Word CancelValue - Input Parameter:
 *
 * Value between 0d0 and 0d4095
 */
void VCNL36828P_SET_PS_CANC(int slaveAddress, Word CancelValue, int i2cBus = 1);

/*Read the proximity output data
 *VCNL36828P_GET_PS_DATA(int slaveAddress, int i2cBus = 1)
 *
 *returns PS Data between 0d0 and 0d65535
 */
Word VCNL36828P_GET_PS_DATA(int slaveAddress, int i2cBus = 1);

/*Read the interrupt flag register
 *VCNL36828P_GET_INT_FLAG(int slaveAddress, int i2cBus = 1) returns interrupt flag status.
 *Please refer to Table 15 on the Datasheet page 16
 */
Word VCNL36828P_GET_INT_FLAG(int slaveAddress, int i2cBus = 1);

/*Read Register value
 *VCNL36828P_READ_REG(int slaveAddress, Byte Reg, int i2cBus = 1)
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
Word VCNL36828P_READ_REG(int slaveAddress, Byte Reg, int i2cBus = 1);

/*Read the PS_ON bit
 *VCNL36828P_GET_PS_ON_Bit(int slaveAddress, int i2cBus = 1)
 *returns 1 for Shutdown and 0 for Sensor On
 */
bool VCNL36828P_GET_PS_ON_Bit(int slaveAddress, int i2cBus = 1);

/*Read the PS_MODE bit
 *VCNL36828P_GET_PS_MODE_Bit(int slaveAddress, int i2cBus = 1)
 *returns 1 for AF Mode and 0 for Auto Mode
 */
bool VCNL36828P_GET_PS_MODE_Bit(int slaveAddress, int i2cBus = 1);

/*Read Device ID
 *VCNL36828P_GET_ID(int slaveAddress, int i2cBus = 1)
 *Returns ID
 */
Word VCNL36828P_GET_ID(int slaveAddress, int i2cBus = 1);
