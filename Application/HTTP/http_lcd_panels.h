#ifndef HTTP_LCD_PANELS_H
#define HTTP_LCD_PANELS_H
#include <stddef.h>
#include <stdint.h>
#define HTTP_LCD_PORTS 33u
#define HTTP_LCD_ROWS 5u
/* Immutable GUI snapshot, published only by the HTTP/WiFi owner task. */
typedef struct
{
	uint8_t port, mode, pwm, pwm_valid;
	char name[31];
	char value[HTTP_LCD_ROWS][32];
	uint32_t color[HTTP_LCD_ROWS];
} HTTP_LCDPort;
typedef struct
{
	uint32_t revision;
	int mode_result;
	uint32_t mode_serial;
	char time[25];
	HTTP_LCDPort ports[HTTP_LCD_PORTS];
} HTTP_LCDPanelData;
void HTTP_LCDPanelActive ( int active );
int HTTP_LCDPanelWanted ( void );
int HTTP_LCDPanelRead ( HTTP_LCDPanelData *out );
int HTTP_LCDPanelPwm ( unsigned port, unsigned percent );
int HTTP_LCDPanelToggle ( unsigned port );
/* One atomic form commit. Unchanged ports use the latest HTTP values.
 * Returns request id, or zero when another save is pending / arguments invalid. */
uint32_t HTTP_LCDPanelSaveModes ( const uint8_t changed[HTTP_LCD_PORTS], const uint8_t modes[HTTP_LCD_PORTS], const uint8_t expected[HTTP_LCD_PORTS] );
/* Only call from WiFi, after HTTP model init and before its notification wait. */
void HTTP_LCDPanelService ( const char *refresh, size_t length, uint32_t now );
#endif
