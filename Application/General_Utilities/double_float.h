/*
 * double_float.h
 *
 *  Created on: 02.04.2021
 *      Author: Elektronika RM
 */

#ifndef GENERAL_UTILITIES_DOUBLE_FLOAT_H_
#define GENERAL_UTILITIES_DOUBLE_FLOAT_H_

char* dbl2stri		  	  (char *buffer, double dbl, unsigned int dec_digits);
void  float2stri		  (char *buffer, float dbl, unsigned int dec_digits);
float GetNewfloatValue (float value, int precision);


#endif /* GENERAL_UTILITIES_DOUBLE_FLOAT_H_ */
