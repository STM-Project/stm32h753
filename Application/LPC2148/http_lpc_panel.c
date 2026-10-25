#include "http_lpc_panel.h"
#include "lpc_pages.h"
#include "../HTTP/http_sensor_settings.h"
#include "../HTTP/http_settings_model.h"
#include "../relay_states.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

const uint8_t HTTP_LpcTemperaturePorts[3] = { 1, 2, 3 };
const uint8_t HTTP_LpcInputPorts[7] = { 4, 5, 6, 7, 8, 9, 10 };
const uint8_t HTTP_LpcOutputPorts[4] = { 28, 29, 30, 31 };
HTTP_LpcMeasurements HTTP_LpcMeasurementsData;

void HTTP_LpcResetMeasurements ( void )
{
	memset ( &HTTP_LpcMeasurementsData, 0, sizeof HTTP_LpcMeasurementsData );
}
int HTTP_LpcSetTemperature ( unsigned port, int tenths, int valid )
{
	if ( valid && ( tenths < -550 || tenths > 1500 ) )
		return 0;
	for ( unsigned i = 0; i < HTTP_LPC_TEMPERATURE_COUNT; i++ )
		if ( HTTP_LpcTemperaturePorts[i] == port )
		{
			HTTP_LpcMeasurementsData.temperature_tenths[i] = (int16_t) ( valid ? tenths : 0 );
			HTTP_LpcMeasurementsData.temperature_valid[i] = (uint8_t) !!valid;
			return 1;
		}
	return 0;
}
int HTTP_LpcSetInput ( unsigned port, int on, int valid )
{
	if ( valid && on != 0 && on != 1 )
		return 0;
	for ( unsigned i = 0; i < HTTP_LPC_INPUT_COUNT; i++ )
		if ( HTTP_LpcInputPorts[i] == port )
		{
			HTTP_LpcMeasurementsData.input_on[i] = (uint8_t) ( valid ? on : 0 );
			HTTP_LpcMeasurementsData.input_valid[i] = (uint8_t) !!valid;
			return 1;
		}
	return 0;
}
typedef struct
{
	char *out;
	size_t size, used;
	int failed;
} Writer;
static void append ( Writer *w, const char *format, ... )
{
	if ( w->failed )
		return;
	va_list args;
	va_start ( args, format );
	int n = vsnprintf ( w->out + w->used, w->size - w->used, format, args );
	va_end ( args );
	if ( n < 0 || (size_t) n >= w->size - w->used )
		w->failed = 1;
	else
		w->used += (size_t) n;
}
static void string ( Writer *w, const char *s )
{
	append ( w, "\"" );
	for ( ; s && *s; s++ )
	{
		unsigned char c = (unsigned char) *s;
		if ( c == '"' || c == '\\' )
			append ( w, "\\%c", c );
		else if ( c < 32 )
			append ( w, "\\u%04x", c );
		else
			append ( w, "%c", c );
	}
	append ( w, "\"" );
}
static const char *port_field ( unsigned port, int mode )
{
	uint16_t instance = 0;
	char name[40];
	const HTTP_SettingPage *page = HTTP_SettingsCatalogFind ( "/set", &instance );
	snprintf ( name, sizeof name, mode ? "n_GPIO_val_%03u" : "n_s_Port_nazwa%u_001", port );
	return HTTP_SettingsGetByName ( page, instance, name );
}
/* Fixed-width /TME.txt fields: outputs at 25, inputs at 39,
 * and 12 bytes per sensor port starting at 73. */
static int refresh_state ( const char *data, size_t length, size_t offset, int input )
{
	if ( !data || offset >= length || data[offset] < '0' || data[offset] > '3' )
		return -1;
	int on = ( data[offset] - '0' ) & 1;
	return input ? !on : on;
}
int HTTP_LpcOutputStateFromRefresh ( const char *data, size_t length, unsigned port )
{
	const RelayStates *states = RELAY_GetStates ();
	for ( unsigned i = 0; i < RELAY_OUTPUT_COUNT; i++ )
		if ( states->outputs[i].port == port )
			return refresh_state ( data, length, 25u + i, 0 );
	return -1;
}
static int refresh_temperature ( const char *data, size_t length, unsigned port, int *value )
{
	size_t offset = 73u + ( port - 1u ) * 12u;
	if ( !data || offset > length || length - offset < 5u )
		return 0;
	const char *p = data + offset;
	unsigned i = 0, digits = 0;
	int sign = 1, number = 0;
	while ( i < 3u && p[i] == ' ' )
		i++;
	if ( i < 3u && ( p[i] == '-' || p[i] == '+' ) )
	{
		if ( p[i] == '-' )
			sign = -1;
		i++;
	}
	for ( ; i < 3u; i++ )
	{
		if ( p[i] < '0' || p[i] > '9' )
			return 0;
		number = number * 10 + p[i] - '0';
		digits++;
	}
	if ( !digits || p[3] != '.' || p[4] < '0' || p[4] > '9' )
		return 0;
	number = sign * ( number * 10 + p[4] - '0' );
	*value = number;
	return 1;
}
static size_t panel_json ( char *out, size_t capacity, const char *refresh, size_t length, int all )
{
	if ( !out || !capacity )
		return 0;
	Writer w = { out, capacity, 0, 0 };
	out[0] = 0;
	append ( &w, "{\"temperatures\":[" );
	for ( unsigned i = 0; i < ( all ? 20u : HTTP_LPC_TEMPERATURE_COUNT ); i++ )
	{
		unsigned port = all ? i + 1u : HTTP_LpcTemperaturePorts[i];
		const HttpSensorSettings *s = HTTP_SensorSettingsGet ( port, 0 );
		char key[48];
		snprintf ( key, sizeof key, "n_txtval%03u", port );
		const char *saved_name = LPC_GetSavedSetting ( key );
		append ( &w, "%s{\"port\":%u,\"name\":", i ? "," : "", port );
		string ( &w, saved_name ? saved_name : s ? s->name
												 : "Temperatura" );
		append ( &w, ",\"tenths\":" );
		int measured = 0;
		int valid = refresh ? refresh_temperature ( refresh, length, port, &measured ) : HTTP_LpcMeasurementsData.temperature_valid[i];
		if ( !refresh )
			measured = HTTP_LpcMeasurementsData.temperature_tenths[i] + ( s ? s->correction : 0 );
		if ( valid )
			append ( &w, "%d", measured );
		else
			append ( &w, "null" );
		append ( &w, ",\"min\":%d,\"max\":%d}", s ? s->min1 : -100, s ? s->max1 : 500 );
	}
	append ( &w, "],\"inputs\":[" );
	static const uint8_t inputs[] = { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20 };
	for ( unsigned i = 0; i < ( all ? sizeof ( inputs ) : HTTP_LPC_INPUT_COUNT ); i++ )
	{
		unsigned port = all ? inputs[i] : HTTP_LpcInputPorts[i];
		char key[48];
		snprintf ( key, sizeof key, "n_txtwe%03u", i + 1u );
		const char *label = LPC_GetSavedSetting ( key );
		append ( &w, "%s{\"port\":%u,\"name\":", i ? "," : "", port );
		string ( &w, label ? label : port_field ( port, 0 ) );
		int state = refresh ? refresh_state ( refresh, length, 39u + ( port <= 14u ? port - 1u : port - 3u ), 1 ) : !HTTP_LpcMeasurementsData.input_valid[i] ? -1
																																							 : HTTP_LpcMeasurementsData.input_on[i];
		append ( &w, ",\"state\":%s}", state < 0 ? "null" : state ? "true"
																  : "false" );
	}
	append ( &w, "],\"outputs\":[" );
	for ( unsigned i = 0; i < ( all ? RELAY_OUTPUT_COUNT : HTTP_LPC_OUTPUT_COUNT ); i++ )
	{
		unsigned port = all ? RELAY_GetStates ()->outputs[i].port : HTTP_LpcOutputPorts[i];
		const RelayState *s = RELAY_GetState ( port );
		const char *mode = port_field ( port, 1 );
		int enabled = mode && ( strtoul ( mode, NULL, 10 ) == 0 || strtoul ( mode, NULL, 10 ) == 8 );
		char key[48];
		snprintf ( key, sizeof key, "n_pktxt%03u", i + 1u );
		const char *label = LPC_GetSavedSetting ( key );
		append ( &w, "%s{\"port\":%u,\"command\":%u,\"name\":", i ? "," : "", port, i + 1 );
		string ( &w, label ? label : port_field ( port, 0 ) );
		int state = refresh ? HTTP_LpcOutputStateFromRefresh ( refresh, length, port ) : s && s->valid ? s->on
																									   : -1;
		append ( &w, ",\"state\":%s,\"requested\":%s,\"enabled\":%s}",
				 state < 0 ? "null" : state ? "true"
											: "false",
				 !s || !s->request_valid ? "null" : s->requested_on ? "true"
																	: "false",
				 enabled ? "true" : "false" );
	}
	append ( &w, "]}" );
	if ( w.failed )
	{
		out[0] = 0;
		return 0;
	}
	return w.used;
}

size_t HTTP_LpcPanelJson ( char *out, size_t capacity )
{
	return panel_json ( out, capacity, NULL, 0, 0 );
}
size_t HTTP_LpcPanelJsonFromRefresh ( char *out, size_t capacity, const char *refresh, size_t length )
{
	/* A missing refresh snapshot must not silently select the separate
     * direct-publication data source. */
	return panel_json ( out, capacity, refresh ? refresh : "", refresh ? length : 0, 0 );
}

size_t HTTP_LpcLegacyJson ( char *out, size_t capacity, const char *refresh, size_t length )
{
	return panel_json ( out, capacity, refresh ? refresh : "", refresh ? length : 0, 1 );
}
