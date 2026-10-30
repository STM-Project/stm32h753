#include "http_lcd_forms.h"
#if defined( __arm__ ) || defined( __thumb__ )
#define FORM_SDRAM __attribute__ ( ( section ( ".sdram" ) ) )
#else
#define FORM_SDRAM
#endif
static HTTP_LCDFormData form_published FORM_SDRAM, form_request FORM_SDRAM;
static char form_target[HTTP_SETTINGS_TARGET_MAX + 1u] FORM_SDRAM;
static unsigned form_operation, form_active, form_port, form_names;
static uint32_t form_serial, form_last;
/* SDRAM is NOLOAD on H7. Readiness must live in zeroed internal BSS, not in
 * form_published, whose serial/revision may survive reset or contain garbage. */
static unsigned form_initialized, form_ready;
static unsigned form_pwm_changed, form_pwm_base;
const HTTP_SettingPage *HTTP_LCDFormRoute ( unsigned port, unsigned mode, int names_only, uint16_t *instance )
{
	if ( !instance || port < 1 || port > 33 )
		return NULL;
	char route[32];
	const char *prefix = "/set";
	unsigned n = 0;
	if ( !names_only )
	{
		switch ( mode )
		{
		case HTTP_MODE_INPUT:
			prefix = "/we";
			n = port <= 14 ? port : port >= 17 && port <= 20 ? port - 2
															 : 0;
			break;
		case HTTP_MODE_EOL:
			prefix = "/eol";
			n = port <= 14 ? port : 0;
			break;
		case HTTP_MODE_ADC:
			prefix = "/nap";
			n = port >= 15 && port <= 16 ? port - 14 : 0;
			break;
		case HTTP_MODE_COUNTER:
			prefix = "/etr";
			n = port >= 17 && port <= 20 ? port - 16 : 0;
			break;
		case HTTP_MODE_RELAY:
		case HTTP_MODE_IR:
		case HTTP_MODE_TIMER:
		case HTTP_MODE_THERMOSTAT:
			prefix = mode == HTTP_MODE_TIMER ? "/tim" : mode == HTTP_MODE_THERMOSTAT ? "/term"
																					 : "/wypk";
			n = port >= 28 ? port - 27 : port >= 24 ? port - 17
									 : port >= 21	? port - 10
													: 0;
			break;
		default:
			break;
		}
	}
	if ( strcmp ( prefix, "/set" ) && !n )
		return NULL;
	if ( n )
		snprintf ( route, sizeof route, "%s%03u", prefix, n );
	else
		snprintf ( route, sizeof route, "%s", prefix );
	return HTTP_SettingsCatalogFind ( route, instance );
}
unsigned HTTP_LCDFormFieldIndex ( const HTTP_LCDFormData *data, unsigned row )
{
	return data->names_only ? data->port - 1 : row;
}
uint32_t HTTP_LCDFormOpen ( unsigned port, int names_only )
{
	if ( port < 1 || port > 33 )
		return 0;
	taskENTER_CRITICAL ();
	uint32_t serial = 0;
	if ( !form_operation )
	{
		if ( !++form_serial )
			++form_serial;
		serial = form_serial;
		form_port = port;
		form_names = !!names_only;
		form_active = 1;
		form_operation = 1;
	}
	taskEXIT_CRITICAL ();
	if ( serial )
		HTTP_SensorLCDWake ();
	return serial;
}
void HTTP_LCDFormClose ( void )
{
	taskENTER_CRITICAL ();
	form_active = 0;
	taskEXIT_CRITICAL ();
}
int HTTP_LCDFormWanted ( void )
{
	taskENTER_CRITICAL ();
	int wanted = form_active || form_operation;
	taskEXIT_CRITICAL ();
	return wanted;
}
int HTTP_LCDFormRead ( HTTP_LCDFormData *out )
{
	if ( !out )
		return 0;
	taskENTER_CRITICAL ();
	uint32_t revision = form_published.revision;
	int ready = form_ready && !( revision & 1u ) && form_published.serial && form_published.count <= HTTP_LCD_FORM_FIELDS;
	if ( ready )
		memcpy ( out, &form_published, offsetof ( HTTP_LCDFormData, values ) );
	taskEXIT_CRITICAL ();
	if ( !ready )
		return 0;
	for ( unsigned i = 0; i < out->count; ++i )
	{
		taskENTER_CRITICAL ();
		memcpy ( out->values[i], form_published.values[i], HTTP_LCD_FORM_VALUE );
		taskEXIT_CRITICAL ();
	}
	taskENTER_CRITICAL ();
	int valid = revision == form_published.revision;
	taskEXIT_CRITICAL ();
	return valid;
}
int HTTP_LCDFormSave ( const HTTP_LCDFormData *base, const HTTP_LCDFormData *draft )
{
	if ( !base || !draft || !base->page || !base->count || base->count > HTTP_LCD_FORM_FIELDS || base->serial != draft->serial ||
		 base->page != draft->page || base->instance != draft->instance || base->port != draft->port || base->mode != draft->mode ||
		 base->names_only != draft->names_only || base->count != draft->count || draft->pwm > 100 )
		return 0;
	for ( unsigned i = 0; i < draft->count; ++i )
		if ( !memchr ( draft->values[i], 0, HTTP_LCD_FORM_VALUE ) )
			return 0;
	taskENTER_CRITICAL ();
	int accepted = !form_operation && form_active && base->serial == form_serial;
	if ( accepted )
	{
		form_request = *draft;
		form_pwm_changed = base->pwm != draft->pwm;
		form_pwm_base = base->pwm;
		form_request.hash = base->hash;
		form_operation = 2;
		form_published.status = LCD_FORM_BUSY;
		form_published.revision += 2;
	}
	taskEXIT_CRITICAL ();
	if ( accepted )
		HTTP_SensorLCDWake ();
	return accepted;
}
static uint32_t FormHash ( const HTTP_LCDFormData *data )
{
	uint32_t hash = 2166136261u;
	for ( unsigned i = 0; i < data->count; ++i )
	{
		const char *value = HTTP_SettingsGet ( data->page, data->instance, &data->page->fields[HTTP_LCDFormFieldIndex ( data, i )] );
		if ( !value )
			value = "";
		do
		{
			hash = ( hash ^ (unsigned char) *value ) * 16777619u;
		} while ( *value++ );
	}
	return hash;
}
static int FormEncode ( size_t *used, const char *text )
{
	static const char hex[] = "0123456789ABCDEF";
	while ( *text )
	{
		unsigned char c = (unsigned char) *text++;
		if ( *used + 4 >= sizeof form_target )
			return 0;
		if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_' || c == '-' || c == '.' )
			form_target[( *used )++] = (char) c;
		else
		{
			form_target[( *used )++] = '%';
			form_target[( *used )++] = hex[c >> 4];
			form_target[( *used )++] = hex[c & 15];
		}
	}
	form_target[*used] = 0;
	return 1;
}
static int FormCommit ( void )
{
	uint16_t instance_now = 0;
	if ( form_request.page != HTTP_LCDFormRoute ( form_request.port, HTTP_SettingsPortMode ( form_request.port ), form_request.names_only, &instance_now ) || instance_now != form_request.instance ||
		 HTTP_SettingsPortMode ( form_request.port ) != form_request.mode || FormHash ( &form_request ) != form_request.hash )
		return LCD_FORM_CONFLICT;
	if ( form_request.mode == HTTP_MODE_PWM && form_pwm_changed )
	{
		const RelayState *state = RELAY_GetState ( form_request.port );
		unsigned current = state && state->pwm_valid ? state->pwm_percent : 0;
		if ( current != form_pwm_base )
			return LCD_FORM_CONFLICT;
		if ( form_request.pwm > 100 )
			return LCD_FORM_INVALID;
	}
	char route[48];
	if ( !HTTP_SettingsCatalogPath ( form_request.page, form_request.instance, route, sizeof route ) )
		return LCD_FORM_INVALID;
	size_t used = (size_t) snprintf ( form_target, sizeof form_target, "/set?h7_page=" );
	if ( !FormEncode ( &used, route ) )
		return LCD_FORM_INVALID;
	const char *suffix = "&h7_form=0&n_109_zapisz=1";
	if ( used + strlen ( suffix ) >= sizeof form_target )
		return LCD_FORM_INVALID;
	strcpy ( form_target + used, suffix );
	used += strlen ( suffix );
	for ( unsigned i = 0; i < form_request.count; ++i )
	{
		const HTTP_SettingField *field = &form_request.page->fields[HTTP_LCDFormFieldIndex ( &form_request, i )];
		if ( field->kind == HTTP_SETTING_CHECKBOX && !strcmp ( form_request.values[i], "0" ) )
			continue;
		char name[HTTP_SETTINGS_FIELD_NAME_MAX + 1];
		if ( !HTTP_SettingsCatalogFieldName ( field, form_request.instance, name, sizeof name ) || used + 2 >= sizeof form_target )
			return LCD_FORM_INVALID;
		form_target[used++] = '&';
		if ( !FormEncode ( &used, name ) )
			return LCD_FORM_INVALID;
		form_target[used++] = '=';
		if ( !FormEncode ( &used, form_request.values[i] ) )
			return LCD_FORM_INVALID;
	}
	const HTTP_SettingPage *page = NULL;
	uint16_t instance = 0;
	int result = HTTP_SettingsApplyTarget ( form_target, used, &page, &instance );
	if ( result == HTTP_SETTINGS_SAVED )
	{
		HTTP_SettingsBindingsCommitted ( page, instance );
		if ( form_request.mode == HTTP_MODE_PWM && form_pwm_changed )
			RELAY_RequestPwm ( form_request.port, form_request.pwm );
		return LCD_FORM_SAVED;
	}
	return result == HTTP_SETTINGS_STORAGE_ERROR ? LCD_FORM_SD_ERROR : LCD_FORM_INVALID;
}
void HTTP_LCDFormService ( uint32_t now )
{
	taskENTER_CRITICAL ();
	unsigned operation = form_operation, active_form = form_active;
	taskEXIT_CRITICAL ();
	if ( !operation && ( !active_form || (uint32_t) ( now - form_last ) < 250 ) )
		return;
	form_last = now;
	if ( !form_initialized )
	{
		/* Initialize only the header; every published value is overwritten
		 * below before form_ready allows the GUI to read it. */
		memset ( &form_published, 0, offsetof ( HTTP_LCDFormData, values ) );
		form_initialized = 1;
	}
	int status = operation == 2 ? FormCommit () : LCD_FORM_READY;
	taskENTER_CRITICAL ();
	++form_published.revision;
	taskEXIT_CRITICAL ();
	if ( operation == 1 )
	{
		form_published.serial = form_serial;
		form_published.port = form_port;
		form_published.mode = HTTP_SettingsPortMode ( form_port );
		form_published.names_only = form_names;
		form_published.page = HTTP_LCDFormRoute ( form_port, form_published.mode, form_names, &form_published.instance );
		if ( form_published.page && !strcmp ( form_published.page->route, "/set" ) )
			form_published.names_only = 1;
		form_published.count = !form_published.page ? 0 : form_published.names_only ? 1
																					: form_published.page->field_count;
		if ( form_published.count > HTTP_LCD_FORM_FIELDS )
			form_published.count = 0;
	}
	if ( !form_published.page || !form_published.count )
		status = LCD_FORM_INVALID;
	else
	{
		for ( unsigned i = 0; i < form_published.count; ++i )
		{
			const char *value = HTTP_SettingsGet ( form_published.page, form_published.instance, &form_published.page->fields[HTTP_LCDFormFieldIndex ( &form_published, i )] );
			taskENTER_CRITICAL ();
			snprintf ( form_published.values[i], HTTP_LCD_FORM_VALUE, "%s", value ? value : "" );
			taskEXIT_CRITICAL ();
		}
		form_published.hash = FormHash ( &form_published );
		const RelayState *state = RELAY_GetState ( form_published.port );
		form_published.pwm = state && state->pwm_valid ? state->pwm_percent : 0;
	}
	form_published.status = operation ? status : form_published.status;
	taskENTER_CRITICAL ();
	++form_published.revision;
	form_ready = 1;
	if ( operation )
		form_operation = 0;
	taskEXIT_CRITICAL ();
}
