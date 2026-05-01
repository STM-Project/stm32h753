/*
 * double_float.c
 *
 *  Created on: 02.04.2021
 *      Author: Elektronika RM
 */

#include "double_float.h"
#include <stdint.h>
#include <math.h>


const double round_nums[8] =
{ 0.5, 0.05, 0.005, 0.0005, 0.00005,0.000005,0.0000005,0.00000005};

char* dbl2stri(char *buffer, double value, unsigned int dec_digits)
{
	int idx;
	int64_t dbl_int, dbl_frac;
	int64_t mult = 1;
	char *output = buffer;
	char tbfr[40];

	if (isfinite(value))
	{
		if ((value <= -99999999999999) || (value >= 99999999999999))
		{
			*output++ = '-';
			*output++ = '-';
			*output++ = '-';
			*output++ = '-';
			*output++ = '-';
		}
		else
		{
			if (value < 0.0)
			{
				*output++ = '-';
				value *= -1.0;
			}

			if (dec_digits <8)
			{
				value += round_nums[dec_digits];
				for (idx = 0; idx < dec_digits; idx++)
					mult *= 10;
			}
			else
			{
				dec_digits = 7;
				value += round_nums[7];
				mult = 10000000;
			}

			dbl_int = (int64_t) value;
			dbl_frac = (int64_t) ((value - (double) dbl_int) * (double) mult);

			idx = 0;
			while (dbl_int != 0)
			{
				tbfr[idx++] = '0' + (dbl_int % 10);
				dbl_int /= 10;
			}

			if (idx == 0)
				*output++ = '0';
			else
			{
				while (idx > 0)
				{
					*output++ = tbfr[idx - 1];
					idx--;
				}
			}

			if (dec_digits > 0)
			{
				*output++ = '.';

				idx = 0;
				while (dbl_frac != 0)
				{
					tbfr[idx++] = '0' + (dbl_frac % 10);
					dbl_frac /= 10;
				}
				while (idx < dec_digits)
					tbfr[idx++] = '0';

				if (idx == 0)
					*output++ = '0';
				else
				{
					while (idx > 0)
					{
						*output++ = tbfr[idx - 1];
						idx--;
					}
				}
			}
		}
	}
	else
	{
		*output++ = '-';
		*output++ = '-';
		*output++ = '-';
		*output++ = '-';
		*output++ = '-';

	}
	*output = 0;
	return buffer;
}

void float2stri(char *buffer, float value, unsigned int dec_digits)
{
	int idx;
	int64_t dbl_int, dbl_frac;
	int64_t mult = 1;
	char *output = buffer;
	char tbfr[40];

	if (isfinite(value))
	{
		if ((value <= -99999999999999) || (value >= 99999999999999))
		{
			*output++ = '-';
			*output++ = '-';
			*output++ = '-';
			*output++ = '-';
			*output++ = '-';
		}
		else
		{
			if (value < 0.0)
			{
				*output++ = '-';
				value *= -1.0;
			}

			if (dec_digits <8)
			{
				value += round_nums[dec_digits];
				for (idx = 0; idx < dec_digits; idx++)
					mult *= 10;
			}
			else
			{
				dec_digits = 7;
				value += round_nums[7];
				mult = 10000000;
			}

			dbl_int = (int64_t) value;
			dbl_frac = (int64_t) ((value - (float) dbl_int) * (float) mult);

			idx = 0;
			while (dbl_int != 0)
			{
				tbfr[idx++] = '0' + (dbl_int % 10);
				dbl_int /= 10;
			}

			if (idx == 0)
				*output++ = '0';
			else
			{
				while (idx > 0)
				{
					*output++ = tbfr[idx - 1];
					idx--;
				}
			}

			if (dec_digits > 0)
			{
				*output++ = '.';

				idx = 0;
				while (dbl_frac != 0)
				{
					tbfr[idx++] = '0' + (dbl_frac % 10);
					dbl_frac /= 10;
				}
				while (idx < dec_digits)
					tbfr[idx++] = '0';

				if (idx == 0)
					*output++ = '0';
				else
				{
					while (idx > 0)
					{
						*output++ = tbfr[idx - 1];
						idx--;
					}
				}
			}
		}
	}
	else
	{
		*output++ = '-';
		*output++ = '-';
		*output++ = '-';
		*output++ = '-';
		*output++ = '-';
	}
	*output = 0;
}

float GetNewfloatValue(float value, int precision)
{
	float coeff = pow(10, precision);
	return  round(value * coeff) / coeff;
}

