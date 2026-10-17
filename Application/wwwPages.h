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
extern const char HttpStructMainReadPanel2[];

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

/* CGI mode numbers used by n_GPIO_val_NNN (distinct from TEMPL_*). */
typedef enum{
	HTTP_MODE_RELAY=0,
	HTTP_MODE_PWM=1,
	HTTP_MODE_TEMP=2,
	HTTP_MODE_PT1000=3,
	HTTP_MODE_INPUT=4,
	HTTP_MODE_ADC=5,
	HTTP_MODE_DISABLED=6,
	HTTP_MODE_COUNTER=7,
	HTTP_MODE_IR=8,
	HTTP_MODE_TEMP_RHU=9,
	HTTP_MODE_TIMER=10,
	HTTP_MODE_THERMOSTAT=11,
	HTTP_MODE_EOL=13
}HTTP_PORT_MODE;

/* Render one form per port. Invalid port: no output; unsupported mode: disabled.
 * Append ModeScript2 once after the forms, before </body>, and ModeSave2 once.
 * Existing NNpNNN sections are shown/hidden; these functions do not create them.
 * Example:
 * ptr = HTTP_TEMPLATE_ModeInput2(ptr,5,HTTP_MODE_TEMP);
 * ptr = HTTP_TEMPLATE_ModeOutput2(ptr,22,HTTP_MODE_PWM);
 * ptr = HTTP_TEMPLATE_ModeSave2(ptr);
 * ptr = HTTP_TEMPLATE_ModeScript2(ptr);
 * Buffer capacity is the caller's responsibility, as for other templates.
 */
char* HTTP_TEMPLATE_ModeInput2(char* buff, u8 nr, u8 mode);
char* HTTP_TEMPLATE_ModeOutput2(char* buff, u8 nr, u8 mode);
char* HTTP_TEMPLATE_ModeScript2(char* buff);
char* HTTP_TEMPLATE_ModeSave2(char* buff);

/* WWW_MAIN_SETTING: CGI mode codes (HTTP_MODE_*), not TEMPL_*, are used here.
 * route is the numeric settings-link index, which may differ from nr.
 * value1/value2 contain HTML; NULL selects the original placeholder.
 * pwm is 0..100. Call each supported mode once per port, then Hidden once.
 * Name escapes HTML attribute characters. Buffer capacity remains caller-owned.
 */
char* HTTP_TEMPLATE_2_AdcTempRhu(char* buff, u8 type, u8 nr, u8 route, const char* value1, const char* value2, u8 pwm);
char* HTTP_TEMPLATE_2_In(char* buff, u8 type, u8 nr, u8 route, const char* value1, const char* value2, u8 pwm);
char* HTTP_TEMPLATE_2_Impl(char* buff, u8 type, u8 nr, u8 route, const char* value1, const char* value2, u8 pwm);
char* HTTP_TEMPLATE_2_Out(char* buff, u8 type, u8 nr, u8 route, const char* value1, const char* value2, u8 pwm);
char* HTTP_TEMPLATE_2_NamesBegin(char* buff);
char* HTTP_TEMPLATE_2_NamesEnd(char* buff);
char* HTTP_TEMPLATE_2_ModesBegin(char* buff);
char* HTTP_TEMPLATE_2_ModesEnd(char* buff);
char* HTTP_TEMPLATE_2_PanelBegin(char* buff);
char* HTTP_TEMPLATE_2_PanelEnd(char* buff);
char* HTTP_TEMPLATE_2_Name(char* buff, u8 nr, const char* name);
unsigned HTTP_SettingsPortMode(unsigned port);
const char* HTTP_SettingsPortName(unsigned port, const char* fallback);
char* HTTP_TEMPLATE_2_Mode(char* buff, u8 nr, u8 mode);
char* HTTP_TEMPLATE_2_Hidden(char* buff, u8 nr);

#endif /* HTTP_WWWPAGES_H_ */
