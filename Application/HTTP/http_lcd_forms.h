#ifndef HTTP_LCD_FORMS_H
#define HTTP_LCD_FORMS_H
#include "http_settings_model.h"
#define HTTP_LCD_FORM_FIELDS 301u
#define HTTP_LCD_FORM_VALUE ( HTTP_SETTINGS_VALUE_MAX + 1u )
enum
{
	LCD_FORM_READY = 1,
	LCD_FORM_BUSY = 2,
	LCD_FORM_SAVED = 3,
	LCD_FORM_INVALID = -1,
	LCD_FORM_SD_ERROR = -2,
	LCD_FORM_CONFLICT = -4
};
typedef struct
{
	uint32_t serial, revision, hash;
	int status;
	unsigned port, mode, names_only, count, pwm;
	uint16_t instance;
	const HTTP_SettingPage *page;
	char values[HTTP_LCD_FORM_FIELDS][HTTP_LCD_FORM_VALUE];
} HTTP_LCDFormData;
/* GUI only passes messages/copies; model and SD remain in the WiFi owner. */
uint32_t HTTP_LCDFormOpen ( unsigned port, int names_only );
void HTTP_LCDFormClose ( void );
int HTTP_LCDFormRead ( HTTP_LCDFormData *out );
int HTTP_LCDFormSave ( const HTTP_LCDFormData *base, const HTTP_LCDFormData *draft );
int HTTP_LCDFormWanted ( void );
void HTTP_LCDFormService ( uint32_t now );
/* Pure route lookup, same physical-port -> HTTP-instance map as /set. */
const HTTP_SettingPage *HTTP_LCDFormRoute ( unsigned port, unsigned mode, int names_only, uint16_t *instance );
unsigned HTTP_LCDFormFieldIndex ( const HTTP_LCDFormData *data, unsigned row );
#endif
