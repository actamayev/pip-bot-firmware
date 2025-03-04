/*
 * VCNL36828P_Application_Library.h
 *
 * Created  : 2 February 2023
 * Modified : 31 May 2023
 * Author   : HWanyusof
 * Version	: 1.2
 */

#include "typedefinition.h"

//Reset the Sensor to the default value
void Reset_Sensor();

//Print the output of the sensor
void Print_Data_Only();

/*Print the variable in DEC for debugging
 *Print_Variable_DEC(Word Var)
 *Word Var - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void Print_Variable_DEC(Word Var);

/*Print the variable in HEX for debugging
 *Print_Variable_HEX(Word Var)
 *Word Var - Input Parameter:
 *
 * Value between 0d0 and 0d65535
 */
void Print_Variable_HEX(Word Var);

//Reverses a string 'str' of length 'len'
void reverse(char* str, int len);

//Converts a given integer x to string str[].
//d is the number of digits required in the output.
//If d is more than the number of digits in x,
//then 0s are added at the beginning.
int intToStr(int x, char str[], int d);

//Converts a floating-point/double number to a string.
void ftoa(float n, char* res, int afterpoint);
