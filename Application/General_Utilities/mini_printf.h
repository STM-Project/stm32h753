/*
 * mini_printf.h
 *
 *  Created on: 17.07.2020
 *      Author: Elektronika RM
 */

#ifndef MINI_PRINTF_H_
#define MINI_PRINTF_H_

#include <stdarg.h>

unsigned int mini_strlen(const char *s);
int mini_vsnprintf(char *buffer, unsigned int buffer_len, const char *fmt, va_list va);
int mini_snprintf(char* buffer, unsigned int buffer_len, const char *fmt, ...);

#endif /* MINI_PRINTF_H_ */
