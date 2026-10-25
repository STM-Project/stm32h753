#include "http_sensor_lcd.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

static HttpSensorSettings published[HTTP_SENSOR_PORT_COUNT][2];
static int ready, state;
static struct
{
	unsigned port;
	HttpSensorSettings base[2], values[2];
} request;

__attribute__ ( ( weak ) ) void HTTP_SensorLCDWake ( void )
{
}

int HTTP_SensorLCDRead ( unsigned port, HttpSensorSettings values[2] )
{
	if ( !values || port < 1 || port > HTTP_SENSOR_PORT_COUNT )
		return 0;
	taskENTER_CRITICAL ();
	int valid = ready;
	if ( valid )
		memcpy ( values, published[port - 1], sizeof published[0] );
	taskEXIT_CRITICAL ();
	return valid;
}
int HTTP_SensorLCDSubmit ( unsigned port, const HttpSensorSettings base[2],
						   const HttpSensorSettings values[2] )
{
	if ( !base || !values || port < 1 || port > HTTP_SENSOR_PORT_COUNT )
		return 0;
	taskENTER_CRITICAL ();
	int accepted = ready && state != SENSOR_LCD_PENDING;
	if ( accepted )
	{
		request.port = port;
		memcpy ( request.base, base, sizeof request.base );
		memcpy ( request.values, values, sizeof request.values );
		state = SENSOR_LCD_PENDING;
	}
	taskEXIT_CRITICAL ();
	if ( accepted )
		HTTP_SensorLCDWake ();
	return accepted;
}
int HTTP_SensorLCDResult ( void )
{
	taskENTER_CRITICAL ();
	int result = state;
	taskEXIT_CRITICAL ();
	return result;
}
static int same_sensor ( const HttpSensorSettings *a, const HttpSensorSettings *b )
{
	/* Compare values, not structure padding or bytes after a name terminator. */
	return strcmp ( a->name, b->name ) == 0 &&
		   a->max1 == b->max1 && a->max2 == b->max2 &&
		   a->min1 == b->min1 && a->min2 == b->min2 &&
		   a->hysteresis == b->hysteresis && a->correction == b->correction &&
		   a->delay_seconds == b->delay_seconds;
}
void HTTP_SensorLCDService ( void )
{
	/* Only WiFi calls Service; no producer may replace a pending request. */
	int result = HTTP_SensorLCDResult ();
	int processing = result == SENSOR_LCD_PENDING;
	if ( processing )
	{
		const HttpSensorSettings *t = HTTP_SensorSettingsGet ( request.port, 0 );
		const HttpSensorSettings *h = HTTP_SensorSettingsGet ( request.port, 1 );
		if ( !same_sensor ( t, &request.base[0] ) || !same_sensor ( h, &request.base[1] ) )
			result = SENSOR_LCD_CONFLICT;
		else
			result = HTTP_SensorSettingsSavePort ( request.port, request.values );
	}
	/* Publish before completion so GUI never sees success with stale values. */
	for ( unsigned port = 1; port <= HTTP_SENSOR_PORT_COUNT; ++port )
	{
		taskENTER_CRITICAL ();
		published[port - 1][0] = *HTTP_SensorSettingsGet ( port, 0 );
		published[port - 1][1] = *HTTP_SensorSettingsGet ( port, 1 );
		taskEXIT_CRITICAL ();
	}
	taskENTER_CRITICAL ();
	ready = 1;
	if ( processing )
		state = result;
	taskEXIT_CRITICAL ();
}
