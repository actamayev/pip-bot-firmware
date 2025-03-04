/*
 * VCNL36828P.h
 *
 * Created  : 2 February 2023
 * Modified : 31 May 2023
 * Author   : HWanyusof
 * Version  : 1.2
 */

//Register definitions - Refer to Table 3 on the datasheet
#define VCNL36828P_PS_CONF_1		 0x00
#define VCNL36828P_PS_CONF_2		 0x01
#define VCNL36828P_PS_CONF_3		 0x02
#define VCNL36828P_PS_THDL			 0x03
#define VCNL36828P_PS_THDH			 0x04
#define VCNL36828P_PS_CANC			 0x05
#define VCNL36828P_PS_DATA			 0xF8
#define VCNL36828P_INT_FLAG			 0xF9
#define VCNL36828P_ID				 0xFA

//Register definitions PS_CONF1_L - Refer to Table 4 on the datasheet
#define VCNL36828P_PS_CAL_EN		 0x01<<7
#define VCNL36828P_PS_CAL_DIS		 0x00<<7
#define VCNL36828P_PS_ON_EN			 0x01<<0
#define VCNL36828P_PS_ON_DIS	     0x00<<0

//Register definitions PS_CONF1_H - Refer to Table 5 on the datasheet
#define VCNL36828P_PS_HD_16Bits		 0x01<<6
#define VCNL36828P_PS_HD_12Bits		 0x00<<6
#define VCNL36828P_PS_SP_INT_EN		 0x01<<5
#define VCNL36828P_PS_SP_INT_DIS	 0x00<<5
#define VCNL36828P_PS_SMART_PERS_EN	 0x01<<4
#define VCNL36828P_PS_SMART_PERS_DIS 0x00<<4
#define VCNL36828P_PS_PERS_4		 0x03<<2
#define VCNL36828P_PS_PERS_3		 0x02<<2
#define VCNL36828P_PS_PERS_2		 0x01<<2
#define VCNL36828P_PS_PERS_1		 0x00<<2
#define VCNL36828P_PS_INT_EN		 0x03<<0
#define VCNL36828P_PS_INT_FIRST_HIGH 0x02<<0
#define VCNL36828P_PS_INT_LOGIC_MODE 0x01<<0
#define VCNL36828P_PS_INT_DIS		 0x00<<0


//Register definitions PS_CONF2_L - Refer to Table 6 on the datasheet
#define VCNL36828P_PS_PERIOD_400ms	 0x03<<6
#define VCNL36828P_PS_PERIOD_200ms	 0x02<<6
#define VCNL36828P_PS_PERIOD_100ms	 0x01<<6
#define VCNL36828P_PS_PERIOD_50ms	 0x00<<6
#define VCNL36828P_PS_IT_8T			 0x03<<4
#define VCNL36828P_PS_IT_4T			 0x02<<4
#define VCNL36828P_PS_IT_2T			 0x01<<4
#define VCNL36828P_PS_IT_1T			 0x00<<4
#define VCNL36828P_PS_MPS_8			 0x03<<2
#define VCNL36828P_PS_MPS_4			 0x02<<2
#define VCNL36828P_PS_MPS_2			 0x01<<2
#define VCNL36828P_PS_MPS_1			 0x00<<2
#define VCNL36828P_PS_ITB_50us		 0x01<<1
#define VCNL36828P_PS_ITB_25us		 0x00<<1
#define VCNL36828P_PS_GAIN_x2		 0x01<<0
#define VCNL36828P_PS_GAIN_x1		 0x00<<0

//Register definitions PS_CONF2_H - Refer to Table 7 on the datasheet
#define VCNL36828P_PS_SENS_HIGH		 0x01<<5
#define VCNL36828P_PS_SENS_NORMAL	 0x00<<5
#define VCNL36828P_PS_OFFSET_EN		 0x01<<4
#define VCNL36828P_PS_OFFSET_DIS	 0x00<<4
#define VCNL36828P_I_VCSEL_20mA	 	 0x07<<0
#define VCNL36828P_I_VCSEL_19mA	 	 0x06<<0
#define VCNL36828P_I_VCSEL_17mA		 0x05<<0
#define VCNL36828P_I_VCSEL_15mA	 	 0x04<<0
#define VCNL36828P_I_VCSEL_12mA	 	 0x03<<0
#define VCNL36828P_I_VCSEL_11mA	 	 0x02<<0
#define VCNL36828P_I_VCSEL_9mA	 	 0x01<<0
#define VCNL36828P_I_VCSEL_7mA	 	 0x00<<0

//Register definitions PS_CONF3_L - Refer to Table 9 on the datasheet
#define VCNL36828P_PS_TRIG_EN		 0x01<<5
#define VCNL36828P_PS_TRIG_DIS		 0x00<<5
#define VCNL36828P_PS_MODE_AF_MODE   0x01<<4
#define VCNL36828P_PS_MODE_AUTO_MODE 0x00<<4

//Register definitions PS_CONF3_H - Refer to Table 10 on the datasheet
#define VCNL36828P_PS_SHORT_PERIOD_25ms	 	 0x03<<6
#define VCNL36828P_PS_SHORT_PERIOD_12_5ms	 0x02<<6
#define VCNL36828P_PS_SHORT_PERIOD_6_25ms	 0x01<<6
#define VCNL36828P_PS_SHORT_PERIOD_DIS		 0x00<<6
#define VCNL36828P_PS_SC_EN	 				 0x07<<2
#define VCNL36828P_PS_SC_DIS	 		     0x00<<2



