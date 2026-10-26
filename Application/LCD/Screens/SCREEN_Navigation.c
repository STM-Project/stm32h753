#include "SCREEN_Navigation.h"
#include "LCD_Hardware.h"
#include "LCD_PortH7.h"
#include "timer.h"
#include <string.h>

#define NAV_W 30U
#define NAV_H 60U
#define NAV_Y ( ( LCD_H7_HEIGHT - NAV_H ) / 2U )
#define NAV_PIXELS ( NAV_W * NAV_H )
#define NAV_TIME_MS 5000U

static struct
{
	uint32_t entered;
	int wasDown, armed, blocked, painted;
} navigation;
/* Separate from pLcd: legacy font renderers also use pLcd as scratch memory. */
static uint32_t background[2][NAV_PIXELS] __attribute__ ( ( section ( ".sdram" ), aligned ( 32 ) ) );
static uint32_t patch[NAV_PIXELS] __attribute__ ( ( section ( ".sdram" ), aligned ( 32 ) ) );

int SCREEN_NavigationNext ( int current, int direction )
{
	/* All implemented screens, including the temperature dashboards, gauges
	 * and shared HTTP/LCD threshold editor. Debug '=' uses this same order. */
	if ( current < 0 || current > 10 )
		return 0;
	return direction < 0 ? ( current == 0 ? 10 : current - 1 ) : ( current == 10 ? 0 : current + 1 );
}

void SCREEN_NavigationEnter ( uint32_t now, int down )
{
	navigation.entered = now;
	navigation.wasDown = down;
	navigation.armed = 0;
	navigation.blocked = down;
	navigation.painted = 0;
}

int SCREEN_NavigationInput ( int down, unsigned x, unsigned y, int *direction )
{
	int hit = y >= NAV_Y && y < NAV_Y + NAV_H ? ( x < NAV_W ? -1 : ( x >= LCD_H7_WIDTH - NAV_W && x < LCD_H7_WIDTH ? 1 : 0 ) ) : 0;
	*direction = 0;
	if ( down && !navigation.wasDown )
		navigation.armed = hit;
	int consumed = navigation.blocked || navigation.armed;
	if ( !down && navigation.wasDown )
	{
		if ( !navigation.blocked && hit == navigation.armed )
			*direction = navigation.armed;
		navigation.armed = 0;
		navigation.blocked = 0;
	}
	navigation.wasDown = down;
	return consumed != 0;
}

void SCREEN_NavigationRestore ( void )
{
	if ( !navigation.painted )
		return;
	for ( unsigned side = 0; side < 2; ++side )
		LCD_DisplayBuff ( side ? LCD_H7_WIDTH - NAV_W : 0, NAV_Y, NAV_W, NAV_H, background[side] );
	navigation.painted = 0;
}

static int Capture ( unsigned side )
{
	if ( osMutexWait ( osDeviceMutex, osWaitForever ) != osOK )
		return 0;
	if ( !TakeMutex ( Semphr_sdram, osWaitForever ) )
	{
		osMutexRelease ( osDeviceMutex );
		return 0;
	}
	unsigned x = side ? LCD_H7_WIDTH - NAV_W : 0;
	for ( unsigned y = 0; y < NAV_H; ++y )
	{
		uintptr_t address = LCD_H7_SDRAM_BASE + 3U * ( ( NAV_Y + y ) * LCD_H7_WIDTH + x );
		uintptr_t aligned = address & ~(uintptr_t) 31U;
		SCB_InvalidateDCache_by_Addr ( (uint32_t *) aligned, (int32_t) ( ( address - aligned + NAV_W * 3U + 31U ) & ~31U ) );
		__DSB ();
		const uint8_t *pixel = (const uint8_t *) address;
		for ( unsigned i = 0; i < NAV_W; ++i, pixel += 3 )
			background[side][y * NAV_W + i] = 0xFF000000U | ( (uint32_t) pixel[2] << 16 ) | ( (uint32_t) pixel[1] << 8 ) | pixel[0];
	}
	GiveMutex ( Semphr_sdram );
	osMutexRelease ( osDeviceMutex );
	return 1;
}

static void Pixel ( unsigned x, unsigned y, uint32_t color )
{
	if ( x < NAV_W && y < NAV_H )
		patch[y * NAV_W + x] = color;
}

static void Label ( unsigned side )
{
	/* Tiny built-in 3x5 glyphs avoid loading or changing screen-owned fonts. */
	static const uint16_t next[4] = { 0x6B6D, 0x79A7, 0x5AAD, 0x7492 };
	static const uint16_t prev[4] = { 0x6BA4, 0x6BAD, 0x79A7, 0x5B6A };
	const uint16_t *word = side ? next : prev;
	for ( unsigned letter = 0; letter < 4; ++letter )
		for ( unsigned y = 0; y < 5; ++y )
			for ( unsigned x = 0; x < 3; ++x )
				if ( word[letter] & ( 1U << ( 14U - y * 3U - x ) ) )
					Pixel ( 7U + letter * 4U + x, 45U + y, 0xFFFFFFFFU );
}

void SCREEN_NavigationDraw ( uint32_t now )
{
	if ( (uint32_t) ( now - navigation.entered ) >= NAV_TIME_MS )
		return;
	if ( !lcdH7Status.initialized || LCD_GetXSize () != LCD_H7_WIDTH || LCD_GetYSize () != LCD_H7_HEIGHT )
		return;
	/* Capture both patches before painting: a failure cannot leave half an overlay. */
	if ( !Capture ( 0 ) || !Capture ( 1 ) )
		return;
	for ( unsigned side = 0; side < 2; ++side )
	{
		for ( unsigned i = 0; i < NAV_PIXELS; ++i )
		{
			uint32_t b = background[side][i];
			patch[i] = 0xFF000000U | ( ( ( ( b >> 16 ) & 255U ) / 3U + 10U ) << 16 ) |
					   ( ( ( ( b >> 8 ) & 255U ) / 3U + 25U ) << 8 ) | ( ( b & 255U ) / 3U + 45U );
		}
		for ( unsigned y = 0; y < 25; ++y )
		{
			unsigned distance = y < 12 ? 12 - y : y - 12;
			unsigned x = side ? 22 - distance : 7 + distance;
			for ( unsigned width = 0; width < 4; ++width )
				Pixel ( x + width, 8 + y, 0xFFFFFFFFU );
		}
		Label ( side );
		LCD_DisplayBuff ( side ? LCD_H7_WIDTH - NAV_W : 0, NAV_Y, NAV_W, NAV_H, patch );
	}
	navigation.painted = 1;
}
