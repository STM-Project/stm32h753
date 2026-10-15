/*
 * wwwPages.h
 *
 *  Created on: 23.12.2020
 *      Author: RafalMar
 */

#ifndef HTTP_WWWPAGES_H_
#define HTTP_WWWPAGES_H_

#include "stm32h7xx_hal.h"
#include "common.h"

#define HTTP_BUFFER_SIZE 		262144
#define HTTP_BUFFER_SIZE_PRE 	262144

typedef enum{
	TEMPL_None,
	TEMPL_Temp,
	TEMPL_TempRhu,
	TEMPL_Ptt,
	TEMPL_Adc,
	TEMPL_Impl,
	TEMPL_In,
	TEMPL_Eol,
	TEMPL_Pk,
	TEMPL_Te,
	TEMPL_Ti,
	TEMPL_Pwm,
}HTTP_TEMPALTE_TYPE;

extern char HttpRefr[];
extern char HttpRefr_sdram[];
extern const char HttpStyle[];
extern const char HttpStructMainReadPanel[];
extern const char HttpMainMenu[];
extern const char HttpMainReadPanel[];
extern const char HttpMainReadPanel2[];
extern const char HttpMainSettings[];

char* HTTP_TEMPLATE_AdcTempRhu(char* buff, u8 type,u8 nr, float val1,float val2, u32 color1,u32 color2, char* name1,char* name2);
char* HTTP_TEMPLATE_Impl	  (char* buff, u8 type,u8 nr, uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4,uint32_t val5, u32 color,char* name);
char* HTTP_TEMPLATE_In		  (char* buff, u8 type,u8 nr, char* name1, u32 color, char* name2);
char* HTTP_TEMPLATE_Out		  (char* buff, u8 type,u8 nr, float val, char* name1, char* name2);
char* HTTP_TEMPLATE_TestUsun(char* buff);

/* HttpMainReadPanel2: each function returns the next free buffer position. */
char* HTTP_TEMPLATE_AdcTempRhu2(char* buff, u8 type,u8 nr, float val1,float val2, u32 color1,u32 color2, char* name1,char* name2);
char* HTTP_TEMPLATE_In2(char* buff, u8 type, u8 nr, char* name1, u32 color, char* name2);
char* HTTP_TEMPLATE_Out2(char* buff, u8 type,u8 nr, float val, char* name1, char* name2);
/* After rendering a port, emit its inactive IDs exactly once, e.g.:
 * ptr = HTTP_TEMPLATE_In2(ptr,TEMPL_In,8,"Stan OFF",0xeee,"Wejscie 8");
 * ptr = HTTP_TEMPLATE_Hidden2(ptr,TEMPL_In,8);
 * Use TEMPL_None for a port without a visible tile. No buffer capacity check
 * is performed, as in the original HTTP_TEMPLATE functions.
 */
char* HTTP_TEMPLATE_Hidden2(char* buff, u8 type, u8 nr);

#endif /* HTTP_WWWPAGES_H_ */
