#ifndef HTTP_SENSOR_LCD_H
#define HTTP_SENSOR_LCD_H
#include "http_sensor_settings.h"
/* Cross-task snapshots/mailbox. The WiFi task exclusively owns the model/SD. */
enum
{
	SENSOR_LCD_IDLE = 0,
	SENSOR_LCD_SAVED = 1,
	SENSOR_LCD_PENDING = 2,
	SENSOR_LCD_STORAGE_ERROR = -2,
	SENSOR_LCD_CONFLICT = -3
};
int HTTP_SensorLCDRead ( unsigned port, HttpSensorSettings values[2] );
int HTTP_SensorLCDSubmit ( unsigned port, const HttpSensorSettings base[2],
						   const HttpSensorSettings values[2] );
int HTTP_SensorLCDResult ( void );
/* Call after model initialization and after each WiFi iteration, before waiting. */
void HTTP_SensorLCDService ( void );
/* Platform hook: wakes WiFi when LCD submits a save. */
void HTTP_SensorLCDWake ( void );
#endif
