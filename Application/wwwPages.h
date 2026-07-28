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

extern char HttpRefr[];
extern const char HttpStyle[];
extern const char HttpMainReadPanel[];
extern const char HttpMainReadPanel2[];
extern const char HttpMainSettings[];

DATA_TO_SEND* GetPageWWW(char *getHttpRequest);

#endif /* HTTP_WWWPAGES_H_ */
