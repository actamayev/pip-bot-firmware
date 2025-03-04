/*
 *VCNL36828P_Prototypes.h
 *
 * Created  : 2 February 2023
 * Modified : 31 May 2023
 * Author   : HWanyusof
 * Version	: 1.2
 */

#include "typedefinition.h"


//****************************************************************************************************
//*****************************************Sensor API*************************************************

/*Enable/disable the internal calibration
 *VCNL36828P_SET_PS_CAL(Byte ps_cal)
 *Byte ps_cal - Input Parameter:
 *
 * VCNL36828P_PS_CAL_EN
 * VCNL36828P_PS_CAL_DIS
 */
void VCNL36828P_SET_PS_CAL(Byte ps_cal);

/*Switch the sensor on/off
 *VCNL36828P_SET_PS_ON(Byte ps_on)
 *Byte ps_on - Input Parameter:
 *
 * VCNL36828P_PS_ON_EN
 * VCNL36828P_PS_ON_DIS
 */
void VCNL36828P_SET_PS_ON(Byte ps_on);

/*Enable/disable high dynamic range (12 bit/16 bit) ADC output setting
 *VCNL36828P_SET_PS_HD(Byte HDBit)
 *Byte HDBit - Input Parameter:
 *
 * VCNL36828P_PS_HD_16Bits
 * VCNL36828P_PS_HD_12Bits
 */
void VCNL36828P_SET_PS_HD(Byte HDBit);

/*Enable/disable the sunlight protection mode interrupt setting
 *VCNL36828P_SET_PS_SP_INT(Byte SP_Bit)
 *Byte SP_Bit - Input Parameter:
 *
 * VCNL36828P_PS_SP_INT_EN
 * VCNL36828P_PS_SP_INT_DIS
 */
void VCNL36828P_SET_PS_SP_INT(Byte SP_Bit);

/*Enable/disable the smart persistence setting when the interrupt event is triggered
 *VCNL36828P_SET_PS_SMART_PERS(Byte Pers)
 *Byte Pers - Input Parameter:
 *
 * VCNL36828P_PS_SMART_PERS_EN
 * VCNL36828P_PS_SMART_PERS_DIS
 */
void VCNL36828P_SET_PS_SMART_PERS(Byte Pers);

/*Set the amount of consecutive threshold crossing events necessary to trigger interrupt
 *VCNL36828P_SET_PS_PERS(Byte Pers)
 *Byte Pers - Input Parameter:
 *
 * VCNL36828P_PS_PERS_1
 * VCNL36828P_PS_PERS_2
 * VCNL36828P_PS_PERS_3
 * VCNL36828P_PS_PERS_4
 */
void VCNL36828P_SET_PS_PERS(Byte Pers);

/*Set the interrupt mode setting
 *VCNL36828P_SET_PS_INT(Byte InterruptMode)
 *Byte InterruptMode - Input Parameter:
 *
 * VCNL36828P_PS_INT_EN
 * VCNL36828P_PS_INT_FIRST_HIGH
 * VCNL36828P_PS_INT_LOGIC_MODE
 * VCNL36828P_PS_INT_DIS
 */
void VCNL36828P_SET_PS_INT(Byte InterruptMode);

/*Set the measurement period
 *VCNL36828P_SET_PS_PERIOD(Byte Period)
 *Byte Period - Input Parameter:
 *
 * VCNL36828P_PS_PERIOD_400ms
 * VCNL36828P_PS_PERIOD_200ms
 * VCNL36828P_PS_PERIOD_100ms
 * VCNL36828P_PS_PERIOD_50ms
 */
void VCNL36828P_SET_PS_PERIOD(Byte Period);

/*Set the integration time for one measurement; the pulse length “T” is determined by PS_ITB
 *VCNL36828P_SET_PS_IT(Byte IntegrationTime)
 *Byte IntegrationTime - Input Parameter:
 *
 * VCNL36828P_PS_IT_8T
 * VCNL36828P_PS_IT_4T
 * VCNL36828P_PS_IT_2T
 * VCNL36828P_PS_IT_1T
 */
void VCNL36828P_SET_PS_IT(Byte IntegrationTime);

/*Set the number of infrared signal pulses per measurement
 *VCNL36828P_SET_PS_MPS(Byte MultiPulse)
 *Byte MultiPulse - Input Parameter:
 *
 * VCNL36828P_PS_MPS_8
 * VCNL36828P_PS_MPS_4
 * VCNL36828P_PS_MPS_2
 * VCNL36828P_PS_MPS_1
 */
void VCNL36828P_SET_PS_MPS(Byte MultiPulse);

/*Set the pulse length “T” for PS_IT
 *VCNL36828P_SET_PS_ITB(Byte Itb)
 *Byte Itb - Input Parameter:
 *
 * VCNL36828P_PS_ITB_50us
 * VCNL36828P_PS_ITB_25us
 */
void VCNL36828P_SET_PS_ITB(Byte Itb);

/*Set the gain of the ADC
 *VCNL36828P_SET_PS_GAIN(Byte Gain)
 *Byte Gain - Input Parameter:
 *
 * VCNL36828P_PS_GAIN_x2
 * VCNL36828P_PS_GAIN_x1
 */
void VCNL36828P_SET_PS_GAIN(Byte Gain);

/*Set the sensitivity of the ADC
 *VCNL36828P_SET_PS_SENS(Byte Sens)
 *Byte Sens - Input Parameter:
 *
 * VCNL36828P_PS_SENS_HIGH
 * VCNL36828P_PS_SENS_NORMAL
 */
void VCNL36828P_SET_PS_SENS(Byte Sens);

/*Enable/disable the internal crosstalk cancellation
 *VCNL36828P_SET_PS_OFFSET(Byte Offset)
 *Byte Offset - Input Parameter:
 *
 * VCNL36828P_PS_OFFSET_EN
 * VCNL36828P_PS_OFFSET_DIS
 */
void VCNL36828P_SET_PS_OFFSET(Byte Offset);

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
void VCNL36828P_SET_I_VCSEL(Byte I_Vcsel);

/*Set the active force mode trigger(This bit will be reset to 0 after the measurement cycle)
 *VCNL36828P_SET_PS_TRIG(Byte TriggerBit)
 *Byte TriggerBit - Input Parameter:
 *
 * VCNL36828P_PS_TRIG_EN
 * VCNL36828P_PS_TRIG_DIS
 */
void VCNL36828P_SET_PS_TRIG(Byte TriggerBit);

/*Set the measurement mode of the sensor
 *VCNL36828P_SET_PS_MODE(Byte Mode)
 *Byte Mode - Input Parameter:
 *
 * VCNL36828P_PS_MODE_AUTO_MODE
 * VCNL36828P_PS_MODE_AF_MODE
 */
void VCNL36828P_SET_PS_MODE(Byte Mode);

/*Set the short measurement period
 *VCNL36828P_SET_PS_SHORT_PERIOD(Byte Short_Period)
 *Byte Short_Period - Input Parameter:
 *
 * VCNL36828P_PS_SHORT_PERIOD_25ms
 * VCNL36828P_PS_SHORT_PERIOD_12_5ms
 * VCNL36828P_PS_SHORT_PERIOD_6_25ms
 * VCNL36828P_PS_SHORT_PERIOD_DIS
 */
void VCNL36828P_SET_PS_SHORT_PERIOD(Byte Short_Period);

/*Enable/disable the sunlight cancellation
 *VCNL36828P_SET_PS_SC(Byte SunlightBit)
 *Byte SunlightBit - Input Parameter:
 *
 * VCNL36828P_PS_SC_EN
 * VCNL36828P_PS_SC_DIS
 */
void VCNL36828P_SET_PS_SC(Byte SunlightBit);

/*Set the low threshold interrupt value
 *VCNL36828P_SET_PS_THDL(Word LowThreshold)
 *Word LowThreshold - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void VCNL36828P_SET_PS_THDL(Word LowThreshold);

/*Set the high threshold interrupt value
 *VCNL36828P_SET_PS_THDH(Word HighThreshold)
 *Word HighThreshold - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void VCNL36828P_SET_PS_THDH(Word HighThreshold);

/*Set the offset count cancellation value
 *VCNL36828P_SET_PS_CANC(Word CancelValue)
 *Word CancelValue - Input Parameter:
 *
 * Value between 0d0 and 0d4095
 */
void VCNL36828P_SET_PS_CANC(Word CancelValue);

/*Read the proximity output data
 *VCNL36828P_GET_PS_DATA()
 *
 *returns PS Data between 0d0 and 0d65535
 */
Word VCNL36828P_GET_PS_DATA();

/*Read the interrupt flag register
 *VCNL36828P_GET_INT_FLAG() returns interrupt flag status.
 *Please refer to Table 15 on the Datasheet page 16
 */
Word VCNL36828P_GET_INT_FLAG();

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
Word VCNL36828P_READ_REG(Byte Reg);

/*Read the PS_ON bit
*returns 1 for Shutdown and 0 for Sensor On
*/
bool VCNL36828P_GET_PS_ON_Bit();

/*Read the PS_MODE bit
 *returns 1 for AF Mode and 0 for Auto Mode
 */
bool VCNL36828P_GET_PS_MODE_Bit();

/*Read Device ID
 *Returns ID
 */
Word VCNL36828P_GET_ID();


