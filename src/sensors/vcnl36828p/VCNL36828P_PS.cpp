/*
 * VCNL36828P_PS.cpp
 *
 * Created  : 2 February 2023
 * Modified : 31 May 2023 (original), 6 March 2025 (this version)
 * Author   : HWanyusof (original), Grok 3 (modified)
 * Version  : 1.3
 */

#include "./VCNL36828P_Prototypes.h"
#include "./VCNL36828P.h"
#include "./I2C_Functions.h"
#include "./VCNL36828P_Application_Library.h"

//****************************************************************************************************
//*****************************************Sensor API*************************************************

/*Enable/disable the internal calibration
 *VCNL36828P_SET_PS_CAL(int slaveAddress, Byte ps_cal, int i2cBus)
 *Byte ps_cal - Input Parameter:
 *
 * VCNL36828P_PS_CAL_EN
 * VCNL36828P_PS_CAL_DIS
 */
void VCNL36828P_SET_PS_CAL(int slaveAddress, Byte ps_cal, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_CAL_EN|VCNL36828P_PS_CAL_DIS))|ps_cal;
    VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Switch the sensor on/off
 *VCNL36828P_SET_PS_ON(int slaveAddress, Byte ps_on, int i2cBus)
 *Byte ps_on - Input Parameter:
 *
 * VCNL36828P_PS_ON_EN
 * VCNL36828P_PS_ON_DIS
 */
void VCNL36828P_SET_PS_ON(int slaveAddress, Byte ps_on, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_ON_EN|VCNL36828P_PS_ON_DIS))|ps_on;
    VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable high dynamic range (12 bit/16 bit) ADC output setting
 *VCNL36828P_SET_PS_HD(int slaveAddress, Byte HDBit, int i2cBus)
 *Byte HDBit - Input Parameter:
 *
 * VCNL36828P_PS_HD_16Bits
 * VCNL36828P_PS_HD_12Bits
 */
void VCNL36828P_SET_PS_HD(int slaveAddress, Byte HDBit, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_HD_12Bits|VCNL36828P_PS_HD_16Bits))|HDBit;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable the sunlight protection mode interrupt setting
 *VCNL36828P_SET_PS_SP_INT(int slaveAddress, Byte SP_Bit, int i2cBus)
 *Byte SP_Bit - Input Parameter:
 *
 * VCNL36828P_PS_SP_INT_EN
 * VCNL36828P_PS_SP_INT_DIS
 */
void VCNL36828P_SET_PS_SP_INT(int slaveAddress, Byte SP_Bit, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SP_INT_EN | VCNL36828P_PS_SP_INT_DIS))|SP_Bit;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable the smart persistence setting when the interrupt event is triggered
 *VCNL36828P_SET_PS_SMART_PERS(int slaveAddress, Byte Pers, int i2cBus)
 *Byte Pers - Input Parameter:
 *
 * VCNL36828P_PS_SMART_PERS_EN
 * VCNL36828P_PS_SMART_PERS_DIS
 */
void VCNL36828P_SET_PS_SMART_PERS(int slaveAddress, Byte Pers, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SMART_PERS_DIS|VCNL36828P_PS_SMART_PERS_EN))|Pers;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the amount of consecutive threshold crossing events necessary to trigger interrupt
 *VCNL36828P_SET_PS_PERS(int slaveAddress, Byte Pers, int i2cBus)
 *Byte Pers - Input Parameter:
 *
 * VCNL36828P_PS_PERS_1
 * VCNL36828P_PS_PERS_2
 * VCNL36828P_PS_PERS_3
 * VCNL36828P_PS_PERS_4
 */
void VCNL36828P_SET_PS_PERS(int slaveAddress, Byte Pers, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_PERS_1|VCNL36828P_PS_PERS_2|VCNL36828P_PS_PERS_3|VCNL36828P_PS_PERS_4))|Pers;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the interrupt mode setting
 *VCNL36828P_SET_PS_INT(int slaveAddress, Byte InterruptMode, int i2cBus)
 *Byte InterruptMode - Input Parameter:
 *
 * VCNL36828P_PS_INT_EN
 * VCNL36828P_PS_INT_FIRST_HIGH
 * VCNL36828P_PS_INT_LOGIC_MODE
 * VCNL36828P_PS_INT_DIS
 */
void VCNL36828P_SET_PS_INT(int slaveAddress, Byte InterruptMode, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_INT_EN|VCNL36828P_PS_INT_FIRST_HIGH|VCNL36828P_PS_INT_LOGIC_MODE|VCNL36828P_PS_INT_DIS))|InterruptMode;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the measurement period
 *VCNL36828P_SET_PS_PERIOD(int slaveAddress, Byte Period, int i2cBus)
 *Byte Period - Input Parameter:
 *
 * VCNL36828P_PS_PERIOD_400ms
 * VCNL36828P_PS_PERIOD_200ms
 * VCNL36828P_PS_PERIOD_100ms
 * VCNL36828P_PS_PERIOD_50ms
 */
void VCNL36828P_SET_PS_PERIOD(int slaveAddress, Byte Period, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_PERIOD_400ms|VCNL36828P_PS_PERIOD_200ms|VCNL36828P_PS_PERIOD_100ms|VCNL36828P_PS_PERIOD_50ms))|Period;
    VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the integration time for one measurement; the pulse length “T” is determined by PS_ITB
 *VCNL36828P_SET_PS_IT(int slaveAddress, Byte IntegrationTime, int i2cBus)
 *Byte IntegrationTime - Input Parameter:
 *
 * VCNL36828P_PS_IT_8T
 * VCNL36828P_PS_IT_4T
 * VCNL36828P_PS_IT_2T
 * VCNL36828P_PS_IT_1T
 */
void VCNL36828P_SET_PS_IT(int slaveAddress, Byte IntegrationTime, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_IT_8T|VCNL36828P_PS_IT_4T|VCNL36828P_PS_IT_2T|VCNL36828P_PS_IT_1T))|IntegrationTime;
    VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the number of infrared signal pulses per measurement
 *VCNL36828P_SET_PS_MPS(int slaveAddress, Byte MultiPulse, int i2cBus)
 *Byte MultiPulse - Input Parameter:
 *
 * VCNL36828P_PS_MPS_8
 * VCNL36828P_PS_MPS_4
 * VCNL36828P_PS_MPS_2
 * VCNL36828P_PS_MPS_1
 */
void VCNL36828P_SET_PS_MPS(int slaveAddress, Byte MultiPulse, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_MPS_8|VCNL36828P_PS_MPS_4|VCNL36828P_PS_MPS_2|VCNL36828P_PS_MPS_1))|MultiPulse;
    VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the pulse length “T” for PS_IT
 *VCNL36828P_SET_PS_ITB(int slaveAddress, Byte Itb, int i2cBus)
 *Byte Itb - Input Parameter:
 *
 * VCNL36828P_PS_ITB_50us
 * VCNL36828P_PS_ITB_25us
 */
void VCNL36828P_SET_PS_ITB(int slaveAddress, Byte Itb, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_ITB_50us|VCNL36828P_PS_ITB_25us))|Itb;
    VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the gain of the ADC
 *VCNL36828P_SET_PS_GAIN(int slaveAddress, Byte Gain, int i2cBus)
 *Byte Gain - Input Parameter:
 *
 * VCNL36828P_PS_GAIN_x2
 * VCNL36828P_PS_GAIN_x1
 */
void VCNL36828P_SET_PS_GAIN(int slaveAddress, Byte Gain, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_GAIN_x2|VCNL36828P_PS_GAIN_x1))|Gain;
    VCNL36828P_Data.WData[1] = VCNL36828P_Data.RData[1];
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the sensitivity of the ADC
 *VCNL36828P_SET_PS_SENS(int slaveAddress, Byte Sens, int i2cBus)
 *Byte Sens - Input Parameter:
 *
 * VCNL36828P_PS_SENS_HIGH
 * VCNL36828P_PS_SENS_NORMAL
 */
void VCNL36828P_SET_PS_SENS(int slaveAddress, Byte Sens, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SENS_HIGH|VCNL36828P_PS_SENS_NORMAL))|Sens;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable the internal crosstalk cancellation
 *VCNL36828P_SET_PS_OFFSET(int slaveAddress, Byte Offset, int i2cBus)
 *Byte Offset - Input Parameter:
 *
 * VCNL36828P_PS_OFFSET_EN
 * VCNL36828P_PS_OFFSET_DIS
 */
void VCNL36828P_SET_PS_OFFSET(int slaveAddress, Byte Offset, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_OFFSET_EN|VCNL36828P_PS_OFFSET_DIS))|Offset;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the VCSEL driving current
 *VCNL36828P_SET_I_VCSEL(int slaveAddress, Byte I_Vcsel, int i2cBus)
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
void VCNL36828P_SET_I_VCSEL(int slaveAddress, Byte I_Vcsel, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_2;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = VCNL36828P_Data.RData[0];
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_I_VCSEL_20mA|VCNL36828P_I_VCSEL_19mA|VCNL36828P_I_VCSEL_17mA|VCNL36828P_I_VCSEL_15mA|VCNL36828P_I_VCSEL_12mA|VCNL36828P_I_VCSEL_11mA|VCNL36828P_I_VCSEL_9mA|VCNL36828P_I_VCSEL_7mA))|I_Vcsel;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the active force mode trigger(This bit will be reset to 0 after the measurement cycle)
 *VCNL36828P_SET_PS_TRIG(int slaveAddress, Byte TriggerBit, int i2cBus)
 *Byte TriggerBit - Input Parameter:
 *
 * VCNL36828P_PS_TRIG_EN
 * VCNL36828P_PS_TRIG_DIS
 */
void VCNL36828P_SET_PS_TRIG(int slaveAddress, Byte TriggerBit, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_TRIG_DIS|VCNL36828P_PS_TRIG_EN))|TriggerBit;
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]);
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the measurement mode of the sensor
 *VCNL36828P_SET_PS_MODE(int slaveAddress, Byte Mode, int i2cBus)
 *Byte Mode - Input Parameter:
 *
 * VCNL36828P_PS_MODE_AUTO_MODE
 * VCNL36828P_PS_MODE_AF_MODE
 */
void VCNL36828P_SET_PS_MODE(int slaveAddress, Byte Mode, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]&~(VCNL36828P_PS_MODE_AUTO_MODE|VCNL36828P_PS_MODE_AF_MODE))|Mode;
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]);
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the short measurement period
 *VCNL36828P_SET_PS_SHORT_PERIOD(int slaveAddress, Byte Short_Period, int i2cBus)
 *Byte Short_Period - Input Parameter:
 *
 * VCNL36828P_PS_SHORT_PERIOD_25ms
 * VCNL36828P_PS_SHORT_PERIOD_12_5ms
 * VCNL36828P_PS_SHORT_PERIOD_6_25ms
 * VCNL36828P_PS_SHORT_PERIOD_DIS
 */
void VCNL36828P_SET_PS_SHORT_PERIOD(int slaveAddress, Byte Short_Period, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]);
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SHORT_PERIOD_DIS|VCNL36828P_PS_SHORT_PERIOD_6_25ms|VCNL36828P_PS_SHORT_PERIOD_12_5ms|VCNL36828P_PS_SHORT_PERIOD_25ms))|Short_Period;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Enable/disable the sunlight cancellation
 *VCNL36828P_SET_PS_SC(int slaveAddress, Byte SunlightBit, int i2cBus)
 *Byte SunlightBit - Input Parameter:
 *
 * VCNL36828P_PS_SC_EN
 * VCNL36828P_PS_SC_DIS
 */
void VCNL36828P_SET_PS_SC(int slaveAddress, Byte SunlightBit, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    VCNL36828P_Data.WData[0] = (VCNL36828P_Data.RData[0]);
    VCNL36828P_Data.WData[1] = (VCNL36828P_Data.RData[1]&~(VCNL36828P_PS_SC_DIS|VCNL36828P_PS_SC_EN))|SunlightBit;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the low threshold interrupt value
 *VCNL36828P_SET_PS_THDL(int slaveAddress, Word LowThreshold, int i2cBus)
 *Word LowThreshold - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void VCNL36828P_SET_PS_THDL(int slaveAddress, Word LowThreshold, int i2cBus)
{
    Byte LowByte = LowThreshold;
    Byte HighByte = LowThreshold>>8;
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_THDL;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    VCNL36828P_Data.WData[0] = LowByte;
    VCNL36828P_Data.WData[1] = HighByte;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the high threshold interrupt value
 *VCNL36828P_SET_PS_THDH(int slaveAddress, Word HighThreshold, int i2cBus)
 *Word HighThreshold - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void VCNL36828P_SET_PS_THDH(int slaveAddress, Word HighThreshold, int i2cBus)
{
    Byte LowByte = HighThreshold;
    Byte HighByte = HighThreshold>>8;
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_THDH;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    VCNL36828P_Data.WData[0] = LowByte;
    VCNL36828P_Data.WData[1] = HighByte;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Set the offset count cancellation value
 *VCNL36828P_SET_PS_CANC(int slaveAddress, Word CancelValue, int i2cBus)
 *Word CancelValue - Input Parameter:
 *
 * Value between 0d0 and 0d4095
 */
void VCNL36828P_SET_PS_CANC(int slaveAddress, Word CancelValue, int i2cBus)
{
    Byte LowByte = CancelValue;
    Byte HighByte = CancelValue>>8;
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CANC;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    VCNL36828P_Data.WData[0] = LowByte;
    VCNL36828P_Data.WData[1] = HighByte;
    WriteI2C_Bus(&VCNL36828P_Data);
}

/*Read the proximity output data
 *VCNL36828P_GET_PS_DATA(int slaveAddress, int i2cBus)
 *
 *returns PS Data between 0d0 and 0d65535
 */
Word VCNL36828P_GET_PS_DATA(int slaveAddress, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_DATA;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    return (VCNL36828P_Data.RData[1]<<8|VCNL36828P_Data.RData[0]);
}

/*Read the interrupt flag register
 *VCNL36828P_GET_INT_FLAG(int slaveAddress, int i2cBus) returns interrupt flag status.
 *Please refer to Table 15 on the Datasheet page 16
 */
Word VCNL36828P_GET_INT_FLAG(int slaveAddress, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_INT_FLAG;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    return VCNL36828P_Data.RData[1];
}

/*Read Register value
 *VCNL36828P_READ_REG(int slaveAddress, Byte Reg, int i2cBus)
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
Word VCNL36828P_READ_REG(int slaveAddress, Byte Reg, int i2cBus)
{
    Word RegValue;
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = Reg;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    RegValue = VCNL36828P_Data.RData[1]<<8|VCNL36828P_Data.RData[0];
    return RegValue;
}

/*Read the PS_ON bit
 *VCNL36828P_GET_PS_ON_Bit(int slaveAddress, int i2cBus)
 *returns 1 for Shutdown and 0 for Sensor On
 */
bool VCNL36828P_GET_PS_ON_Bit(int slaveAddress, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_1;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    if ((VCNL36828P_Data.RData[0] & 0x01) == 0x01) {return 1;}
    else
        return 0;
}

/*Read the PS_MODE bit
 *VCNL36828P_GET_PS_MODE_Bit(int slaveAddress, int i2cBus)
 *returns 1 for AF Mode and 0 for Auto Mode
 */
bool VCNL36828P_GET_PS_MODE_Bit(int slaveAddress, int i2cBus)
{
    struct TransferData VCNL36828P_Data;
    VCNL36828P_Data.Slave_Address = slaveAddress;
    VCNL36828P_Data.RegisterAddress = VCNL36828P_PS_CONF_3;
    VCNL36828P_Data.Select_I2C_Bus = i2cBus;
    ReadI2C_Bus(&VCNL36828P_Data);
    if ((VCNL36828P_Data.RData[0] & 0x10) == 0x10) {return 1;}
    else
        return 0;
}

/*Read Device ID
 *VCNL36828P_GET_ID(int slaveAddress, int i2cBus)
 *Returns ID
 */
Word VCNL36828P_GET_ID(int slaveAddress, int i2cBus)
{
	Word RegValue;
	struct TransferData VCNL36828P_Data;
	VCNL36828P_Data.Slave_Address = slaveAddress;
	VCNL36828P_Data.RegisterAddress = VCNL36828P_ID;
	VCNL36828P_Data.Select_I2C_Bus = i2cBus;
	ReadI2C_Bus(&VCNL36828P_Data);
	RegValue = VCNL36828P_Data.RData[1]<<8|VCNL36828P_Data.RData[0];
	return RegValue;
}
