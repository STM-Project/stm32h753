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
}HTTP_TEMPALTE_TYPE;

extern char HttpRefr[];
extern const char HttpStyle[];
extern const char HttpStructMainReadPanel[];
extern const char HttpMainMenu[];
extern const char HttpMainReadPanel[];
extern const char HttpMainReadPanel2[];
extern const char HttpMainSettings[];

void HTTP_TEMPLATE_TempRhu(char* buff, u8 type,u8 nr, float val1,float val2, char* name1,char* name2);


#endif /* HTTP_WWWPAGES_H_ */
