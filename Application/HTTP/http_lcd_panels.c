#include "http_lcd_panels.h"
#include "http_sensor_lcd.h"
#include "wwwPages.h"
#include "relay_states.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define NORMAL 0xFFEEEEEEU
#define ALARM 0xFFFF8888U
#define COLD 0xFF88CCFFU
static HTTP_LCDPanelData published, pending;
static int active, ready;
static uint32_t last_update;
/* Slider samples coalesce per PWM channel; toggles are counted, never replaced. */
static uint8_t pwm_dirty[3], pwm_value[3], toggles[13];

void HTTP_LCDPanelActive ( int enabled )
{
	taskENTER_CRITICAL ();
	active = enabled;
	taskEXIT_CRITICAL ();
	if ( enabled )
		HTTP_SensorLCDWake ();
}
int HTTP_LCDPanelWanted ( void )
{
	taskENTER_CRITICAL ();
	int wanted = active;
	taskEXIT_CRITICAL ();
	return wanted;
}
int HTTP_LCDPanelRead ( HTTP_LCDPanelData *out )
{
	if ( !out )
		return 0;
	taskENTER_CRITICAL ();
	int valid = ready;
	if ( valid )
		memcpy ( out, &published, sizeof *out );
	taskEXIT_CRITICAL ();
	return valid;
}
int HTTP_LCDPanelPwm ( unsigned port, unsigned percent )
{
	if ( port < 21 || port > 23 || percent > 100 )
		return 0;
	taskENTER_CRITICAL ();
	pwm_value[port - 21] = (uint8_t) percent;
	pwm_dirty[port - 21] = 1;
	taskEXIT_CRITICAL ();
	HTTP_SensorLCDWake ();
	return 1;
}
int HTTP_LCDPanelToggle ( unsigned port )
{
	if ( port < 21 || port > 33 )
		return 0;
	taskENTER_CRITICAL ();
	int accepted = toggles[port - 21] < 255;
	if ( accepted )
		++toggles[port - 21];
	taskEXIT_CRITICAL ();
	if ( accepted )
		HTTP_SensorLCDWake ();
	return accepted;
}
static void Field ( char *out, size_t capacity, const char *data, size_t length, size_t offset, size_t size )
{
	if ( !data || offset > length || size > length - offset || size >= capacity )
	{
		snprintf ( out, capacity, "---" );
		return;
	}
	memcpy ( out, data + offset, size );
	out[size] = 0;
	/* Trim the protocol's padding, but never interpret missing fields as zero. */
	while ( size && out[size - 1] == ' ' )
		out[--size] = 0;
	char *first = out;
	while ( *first == ' ' )
		++first;
	if ( first != out )
		memmove ( out, first, strlen ( first ) + 1 );
	if ( !out[0] )
		snprintf ( out, capacity, "---" );
}
static int Code ( const char *data, size_t length, size_t offset )
{
	return data && offset < length && data[offset] >= '0' && data[offset] <= '6' ? data[offset] - '0' : -1;
}
static void Value ( HTTP_LCDPort *p, unsigned row, const char *data, size_t length, size_t offset, size_t size, const char *unit )
{
	char text[24];
	Field ( text, sizeof text, data, length, offset, size );
	snprintf ( p->value[row], sizeof p->value[row], "%s%s", text, unit );
}
void HTTP_LCDPanelService ( const char *data, size_t length, uint32_t now )
{
	int command = 0;
	for ( unsigned i = 0; i < 13; ++i )
	{
		taskENTER_CRITICAL ();
		unsigned count = toggles[i];
		toggles[i] = 0;
		unsigned dirty = i < 3 ? pwm_dirty[i] : 0;
		unsigned value = i < 3 ? pwm_value[i] : 0;
		if ( i < 3 )
			pwm_dirty[i] = 0;
		taskEXIT_CRITICAL ();
		if ( !count && !dirty )
			continue;
		unsigned mode = HTTP_SettingsPortMode ( i + 21 );
		if ( dirty && mode == HTTP_MODE_PWM )
		{
			RELAY_RequestPwm ( i + 21, value );
			command = 1;
		}
		if ( count && ( mode == HTTP_MODE_RELAY || mode == HTTP_MODE_IR ) )
		{
			const RelayStates *all = RELAY_GetStates ();
			for ( unsigned j = 0; j < RELAY_OUTPUT_COUNT; ++j )
				if ( all->outputs[j].port == i + 21 )
				{
					int code = Code ( data, length, 25u + j );
					if ( code >= 0 && code <= 3 )
						RELAY_UpdateState ( i + 21, (unsigned) code & 1u );
				}
			while ( count-- )
			{
				RELAY_RequestToggle ( i + 21 );
				command = 1;
			}
		}
	}
	taskENTER_CRITICAL ();
	int interested = active;
	taskEXIT_CRITICAL ();
	if ( !interested && !command )
		return;
	if ( ready && !command && (uint32_t) ( now - last_update ) < 250U )
		return;
	last_update = now;
	memset ( &pending, 0, sizeof pending );
	Field ( pending.time, sizeof pending.time, data, length, 0, 8 );
	for ( unsigned i = 0; i < HTTP_LCD_PORTS; ++i )
	{
		HTTP_LCDPort *p = &pending.ports[i];
		unsigned port = i + 1;
		p->port = (uint8_t) port;
		p->mode = (uint8_t) HTTP_SettingsPortMode ( port );
		const char *name = HTTP_SettingsPortName ( port, NULL );
		snprintf ( p->name, sizeof p->name, "%s", name ? name : "" );
		for ( unsigned row = 0; row < HTTP_LCD_ROWS; ++row )
			p->color[row] = NORMAL;
		if ( p->mode == HTTP_MODE_TEMP || p->mode == HTTP_MODE_TEMP_RHU )
		{
			size_t offset = 73u + i * 12u;
			Value ( p, 0, data, length, offset, 5, " C" );
			int code = Code ( data, length, offset + 5 );
			p->color[0] = code == 1 ? ALARM : code == 2 ? COLD
														: NORMAL;
			if ( p->mode == HTTP_MODE_TEMP_RHU )
			{
				char humidity[8];
				Field ( humidity, sizeof humidity, data, length, offset + 7, 3 );
				if ( humidity[0] >= '0' && humidity[0] <= '9' && strlen ( humidity ) == 3 )
					snprintf ( p->value[1], sizeof p->value[1], "%c%c.%c %%", humidity[0], humidity[1], humidity[2] );
				else
					snprintf ( p->value[1], sizeof p->value[1], "%s %%", humidity );
				code = Code ( data, length, offset + 10 );
				p->color[1] = code == 1 ? ALARM : code == 2 ? COLD
															: NORMAL;
			}
		}
		else if ( p->mode == HTTP_MODE_PT1000 )
		{
			size_t offset = 311u + i * 10u;
			Value ( p, 0, data, length, offset, 9, " C" );
			int code = Code ( data, length, offset + 9 );
			p->color[0] = code == 1 ? ALARM : code == 2 ? COLD
														: NORMAL;
		}
		else if ( p->mode == HTTP_MODE_ADC )
		{
			char unit[8];
			Field ( unit, sizeof unit, data, length, 452u + ( port - 15u ) * 6u, 5 );
			char text[24];
			Field ( text, sizeof text, data, length, 290u + ( port - 15u ) * 10u, 9 );
			snprintf ( p->value[0], sizeof p->value[0], "%s %s", text, strcmp ( unit, "---" ) ? unit : "V" );
		}
		else if ( p->mode == HTTP_MODE_COUNTER )
		{
			size_t offset = 465u + ( port - 17u ) * 95u;
			Value ( p, 0, data, length, offset, 21, "" );
			while ( p->value[0][0] == '0' && p->value[0][1] >= '0' && p->value[0][1] <= '9' )
				memmove ( p->value[0], p->value[0] + 1, strlen ( p->value[0] ) );
			for ( unsigned row = 1; row < 5; ++row )
			{
				Value ( p, row, data, length, offset + 23u + ( row - 1u ) * 18u, 14, "" );
				while ( p->value[row][0] == '0' && p->value[row][1] >= '0' && p->value[row][1] <= '9' )
					memmove ( p->value[row], p->value[row] + 1, strlen ( p->value[row] ) );
				p->color[row] = Code ( data, length, offset + 23u + ( row - 1u ) * 18u + 15u ) == 1 ? ALARM : NORMAL;
			}
		}
		else if ( p->mode == HTTP_MODE_INPUT || p->mode == HTTP_MODE_EOL )
		{
			int code = Code ( data, length, ( p->mode == HTTP_MODE_EOL ? 58u : 39u ) + i );
			const char *text = code < 0 ? "---" : code >= 5				   ? "Uszkodzona linia"
											  : ( code == 0 || code == 2 ) ? "Stan ON"
																		   : "Stan OFF";
			snprintf ( p->value[0], sizeof p->value[0], "%s", text );
			p->color[0] = code >= 0 && ( code >= 5 || code == 0 || code == 2 ) ? ALARM : NORMAL;
		}
		else
		{
			const RelayState *state = RELAY_GetState ( port );
			p->pwm_valid = state && state->pwm_valid;
			p->pwm = p->pwm_valid ? state->pwm_percent : 0;
			int on = RELAY_GetDisplayState ( port );
			if ( on < 0 && state )
			{
				const RelayStates *all = RELAY_GetStates ();
				for ( unsigned j = 0; j < RELAY_OUTPUT_COUNT; ++j )
					if ( all->outputs[j].port == port )
					{
						int code = Code ( data, length, 25u + j );
						if ( code >= 0 && code <= 3 )
							on = code & 1;
					}
			}
			snprintf ( p->value[0], sizeof p->value[0], "%s", on < 0 ? "---" : on ? "Stan ON"
																				  : "Stan OFF" );
		}
	}
	/* Revision changes only when display content changes, not each polling tick. */
	taskENTER_CRITICAL ();
	if ( !ready || memcmp ( &pending.time, &published.time, sizeof pending - offsetof ( HTTP_LCDPanelData, time ) ) )
	{
		pending.revision = published.revision + 1U;
		published = pending;
	}
	ready = 1;
	taskEXIT_CRITICAL ();
}
