#include "SCREEN_TemperaturePk.h"
#include "SCREEN_ReadPanel.h"
#include "LCD_BasicGraphics.h"
#include "LCD_fonts_images.h"
#include "LCD_Hardware.h"
#include "touch.h"
#include <math.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

/* The existing SDRAM drawing buffer, presented by the existing LCD_Show(). */
extern uint32_t pLcd[];

#define PANEL_W 800
#define PANEL_H 480
#define PANEL_TOUCH 61000U
#define PANEL_WHITE 0xFFFFFFFFU
#define PANEL_INK 0xFF15345BU
#define PANEL_MUTED 0xFF657990U

static struct
{
	int active, fonts[4], temperature, previousPk;
	int wasDown, armed, pressed, previousPressed, pulse;
	uint32_t sampleTick, demoTick;
} panel;

uint8_t lcdPanelPkDemo;
volatile int lcdPanelResult;
volatile int lcdPanelTemperature10 = 255;
int ( *lcdPanelReadTemperature10 ) ( void );

static uint32_t PanelMix ( uint32_t a, uint32_t b, unsigned t )
{
	unsigned r = ( ( ( a >> 16 ) & 255 ) * ( 255 - t ) + ( ( b >> 16 ) & 255 ) * t + 127 ) / 255;
	unsigned g = ( ( ( a >> 8 ) & 255 ) * ( 255 - t ) + ( ( b >> 8 ) & 255 ) * t + 127 ) / 255;
	unsigned v = ( ( a & 255 ) * ( 255 - t ) + ( b & 255 ) * t + 127 ) / 255;
	return 0xFF000000U | ( r << 16 ) | ( g << 8 ) | v;
}

static void PanelPixel ( int x, int y, uint32_t color, unsigned alpha )
{
	if ( (unsigned) x >= PANEL_W || (unsigned) y >= PANEL_H || !alpha )
		return;
	LCD_Buffer ( PANEL_W, x, y, alpha == 255 ? color : PanelMix ( pLcd[y * PANEL_W + x], color, alpha ) );
}

/* Rounded vertical gradient with antialiased corners and source-over alpha.
 * It preserves the existing blue background instead of painting corner boxes.
 */
static void PanelRound ( int x, int y, int w, int h, int radius,
						 uint32_t top, uint32_t bottom, unsigned opacity )
{
	if ( w <= 0 || h <= 0 )
		return;
	if ( radius > w / 2 )
		radius = w / 2;
	if ( radius > h / 2 )
		radius = h / 2;
	for ( int j = 0; j < h; ++j )
	{
		uint32_t color = PanelMix ( top, bottom, h > 1 ? (unsigned) ( j * 255 / ( h - 1 ) ) : 0 );
		for ( int i = 0; i < w; ++i )
		{
			float dx = i < radius ? radius - i - 0.5f : ( i >= w - radius ? i - ( w - radius ) + 0.5f : 0 );
			float dy = j < radius ? radius - j - 0.5f : ( j >= h - radius ? j - ( h - radius ) + 0.5f : 0 );
			unsigned alpha = opacity;
			if ( dx > 0 && dy > 0 )
			{
				float coverage = radius + 0.5f - sqrtf ( dx * dx + dy * dy );
				if ( coverage <= 0 )
					continue;
				if ( coverage < 1 )
					alpha = (unsigned) ( opacity * coverage );
			}
			PanelPixel ( x + i, y + j, color, alpha );
		}
	}
}

static void PanelText ( int font, int x, int y, char *text, uint32_t color )
{
	/* bkColor=0 uses the existing framebuffer behind antialiased glyphs. */
	LCD_StrChangeColor ( font, x, y, text, fullHight, 0, 0, color, 255, NoConstWidth );
}

static void PanelCentered ( int font, int center, int y, char *text, uint32_t color )
{
	int width = LCD_GetWholeStrPxlWidth ( font, text, 0, NoConstWidth );
	PanelText ( font, center - width / 2, y, text, color );
}

static void PanelBackground ( void )
{
	for ( int y = 0; y < PANEL_H; ++y )
		for ( int x = 0; x < PANEL_W; ++x )
		{
			unsigned t = (unsigned) ( x * 56 / 799 + y * 199 / 479 );
			int d = ( x - 65 ) * ( x - 65 ) + ( y - 10 ) * ( y - 10 );
			unsigned glow = d < 250000 ? (unsigned) ( ( 250000 - d ) * 64 / 250000 ) : 0;
			uint32_t color = PanelMix ( 0xFF2386F4U, 0xFF061A4BU, t );
			LCD_Buffer ( PANEL_W, x, y, PanelMix ( color, 0xFF7AE4FFU, glow ) );
		}

	PanelRound ( 667, 333, 240, 240, 120, PANEL_WHITE, PANEL_WHITE, 7 );
	PanelRound ( 692, 358, 190, 190, 95, PANEL_WHITE, PANEL_WHITE, 5 );

	PanelRound ( 28, 24, 44, 44, 14, 0xFF72C8FFU, 0xFF2379E9U, 255 );
	PanelRound ( 46, 32, 8, 22, 4, PANEL_WHITE, PANEL_WHITE, 255 );
	PanelRound ( 42, 48, 16, 16, 8, PANEL_WHITE, PANEL_WHITE, 255 );
	PanelRound ( 48, 37, 4, 18, 2, 0xFF1472DBU, 0xFF1472DBU, 255 );

	PanelText ( panel.fonts[1], 86, 22, "Temperatura", PANEL_WHITE );
	PanelText ( panel.fonts[0], 87, 62, "Panel sterowania", 0xFFD7E9FFU );
	PanelRound ( 610, 28, 162, 36, 18, PANEL_WHITE, PANEL_WHITE, 30 );
	PanelCentered ( panel.fonts[0], 705, 34, "TRYB DEMO", PANEL_WHITE );

	PanelRound ( 25, 121, 420, 294, 26, 0xFF00143BU, 0xFF00143BU, 38 );
	PanelRound ( 459, 121, 316, 294, 26, 0xFF00143BU, 0xFF00143BU, 38 );
	PanelRound ( 28, 115, 414, 292, 24, PANEL_WHITE, PANEL_WHITE, 255 );
	PanelRound ( 462, 115, 310, 292, 24, PANEL_WHITE, PANEL_WHITE, 255 );

	PanelText ( panel.fonts[0], 54, 140, "POMIAR TEMPERATURY", PANEL_MUTED );
	PanelCentered ( panel.fonts[0], 235, 367, "Stopnie Celsjusza", PANEL_MUTED );
	PanelCentered ( panel.fonts[0], 617, 140, "PRZEKAZNIK", PANEL_MUTED );

	for ( int x = 28; x < 772; ++x )
		PanelPixel ( x, 434, PANEL_WHITE, 42 );
	PanelText ( panel.fonts[0], 30, 448, "Odczyt co 500 ms", 0xFFC6DAF7U );
	PanelCentered ( panel.fonts[0], 670, 448, "DOTYK  /  PK DEMO", 0xFFC6DAF7U );
}

static void PanelTemperature ( void )
{
	char text[24];
	int value = panel.temperature;
	int valid = value >= -500 && value <= 1500;
	if ( valid )
	{
		unsigned magnitude = (unsigned) ( value < 0 ? -value : value );
		snprintf ( text, sizeof ( text ), "%s%u,%u", value < 0 ? "-" : "", magnitude / 10, magnitude % 10 );
	}
	else
		snprintf ( text, sizeof ( text ), "--,-" );

	LCD_Shape ( 48, 183, LCD_Rectangle, 374, 123, PANEL_WHITE, PANEL_WHITE, PANEL_WHITE );
	int width = LCD_GetWholeStrPxlWidth ( panel.fonts[3], text, 0, NoConstWidth );
	int x = 235 - ( width + 47 ) / 2;
	PanelText ( panel.fonts[3], x, 192, text, PANEL_INK );
	PanelText ( panel.fonts[2], x + width + 18, 215, "C", 0xFF3B79C9U );
	PanelRound ( x + width + 4, 209, 9, 9, 4, 0xFF3B79C9U, 0xFF3B79C9U, 255 );
	PanelRound ( x + width + 6, 211, 5, 5, 2, PANEL_WHITE, PANEL_WHITE, 255 );

	PanelRound ( 72, 326, 328, 8, 4, 0xFFE7EFFCU, 0xFFE7EFFCU, 255 );
	int bounded = value < 0 ? 0 : ( value > 500 ? 500 : value );
	int fill = valid ? 328 * bounded / 500 : 0;
	PanelRound ( 72, 326, fill, 8, 4, 0xFF59BCFFU, 0xFF0B57D0U, 255 );
	PanelRound ( 624, 42, 8, 8, 4, panel.pulse ? 0xFF91F1C0U : 0xFFE8F1FFU,
				 panel.pulse ? 0xFF91F1C0U : 0xFFE8F1FFU, 255 );
}

static void PanelButton ( int pk )
{
	LCD_Shape ( 482, 165, LCD_Rectangle, 270, 238, PANEL_WHITE, PANEL_WHITE, PANEL_WHITE );
	uint32_t top = pk ? 0xFF54A5FFU : 0xFF7094C4U;
	uint32_t bottom = pk ? 0xFF0B57D0U : 0xFF355785U;
	if ( panel.pressed )
	{
		top = PanelMix ( top, 0xFF001E50U, 65 );
		bottom = PanelMix ( bottom, 0xFF001E50U, 65 );
	}
	PanelRound ( 528, 168, 178, 178, 89, pk ? 0xFFDCEBFFU : 0xFFECF1F8U,
				 pk ? 0xFFDCEBFFU : 0xFFECF1F8U, 255 );
	PanelRound ( 537, 181, 160, 160, 80, 0xFF16365FU, 0xFF16365FU, 28 );
	int shift = panel.pressed ? 2 : 0;
	PanelRound ( 537, 177 + shift, 160, 160, 80, top, bottom, 255 );
	int cx = 617, cy = 235 + shift;
	for ( int dy = -32; dy <= 32; ++dy )
		for ( int dx = -32; dx <= 32; ++dx )
		{
			float d = sqrtf ( (float) ( dx * dx + dy * dy ) );
			float coverage = fminf ( 32.0f - d, d - 25.0f );
			if ( dy < -13 && dx > -12 && dx < 12 )
				continue;
			if ( coverage > 0 )
				PanelPixel ( cx + dx, cy + dy, PANEL_WHITE,
							 coverage >= 1 ? 255 : (unsigned) ( coverage * 255 ) );
		}
	PanelRound ( 614, 196 + shift, 6, 29, 3, PANEL_WHITE, PANEL_WHITE, 255 );
	PanelCentered ( panel.fonts[2], 617, 281 + shift, "PK", PANEL_WHITE );
	PanelCentered ( panel.fonts[1], 617, 347, pk ? "WLACZONY" : "WYLACZONY",
					pk ? 0xFF0B57D0U : PANEL_MUTED );
	PanelCentered ( panel.fonts[0], 617, 381, pk ? "Dotknij, aby wylaczyc" : "Dotknij, aby wlaczyc", PANEL_MUTED );
}

static int PanelInside ( XY_Touch_Struct p )
{
	int dx = (int) p.x - 617, dy = (int) p.y - 257;
	return dx * dx + dy * dy < 80 * 80;
}

int LCD_TemperaturePkPanel ( uint8_t mode, int ( *readTemperature10 ) ( void ), uint8_t *pkDemo )
{
	if ( mode == LCD_TEMP_EXIT )
	{
		LCD_TOUCH_DeleteSelectTouch ( PANEL_TOUCH );
		panel.active = 0;
		panel.armed = panel.pressed = 0;
		return 0;
	}
	if ( !pkDemo || mode > LCD_TEMP_EXIT )
		return -1;
	if ( LCD_GetXSize () != PANEL_W || LCD_GetYSize () != PANEL_H )
		return -2;
	uint32_t now = HAL_GetTick ();
	int redrawAll = mode == LCD_TEMP_ENTER || !panel.active;
	int changed = 0, redrawTemperature = 0;

	if ( redrawAll )
	{
		/* Exclusive full-screen view, like the existing screen entry functions. */
		panel.active = 0;
		SCREEN_ResetAllParameters ();
		LCD_TOUCH_DeleteAllSetTouch ();
		const int sizes[4] = { FONT_16, FONT_24, FONT_28_bold, FONT_72 };
		const int ids[4] = { fontID_14, fontID_15, fontID_16, fontID_17 };
		for ( int i = 0; i < 4; ++i )
		{
			panel.fonts[i] = LCD_LoadFont_ChangeColor ( sizes[i], Arial, ids[i] );
			if ( panel.fonts[i] < 0 )
				return -3;
		}
		if ( LCDTOUCH_Set ( 537, 177, 160, 160, ID_TOUCH_POINT, PANEL_TOUCH, pressRelease ) < 0 )
			return -4;
		panel.demoTick = now;
		panel.sampleTick = now - 500U;
		panel.previousPk = -1;
		panel.previousPressed = -1;
		panel.wasDown = LCD_TOUCH_isPress () == press;
		panel.armed = panel.pressed = panel.pulse = 0;
		panel.active = 1;
		PanelBackground ();
	}

	screenTouchState = LCD_TOUCH_GetTypeAndPosition ( &screenTouchPos );
	int down = LCD_TOUCH_isPress () == press;
	int inside = PanelInside ( LCD_TOUCH_GetPos () );
	if ( down && !panel.wasDown )
		panel.armed = inside;
	if ( !down && panel.wasDown )
	{
		if ( panel.armed && inside )
		{
			*pkDemo = !*pkDemo;
			changed = 1;
		}
		panel.armed = 0;
	}
	panel.wasDown = down;
	panel.pressed = down && panel.armed && inside;

	if ( (uint32_t) ( now - panel.sampleTick ) >= 500U )
	{
		panel.sampleTick = now;
		if ( readTemperature10 )
			panel.temperature = readTemperature10 ();
		else
		{
			/* Explicit demo: 25.5 -> 26.5 -> 24.5 -> 25.5 C. */
			unsigned n = (uint32_t) ( now - panel.demoTick ) / 500U % 40U;
			panel.temperature = n <= 10 ? 255 + (int) n : ( n <= 30 ? 275 - (int) n : 215 + (int) n );
		}
		lcdPanelTemperature10 = panel.temperature;
		panel.pulse = !panel.pulse;
		redrawTemperature = 1;
		PanelTemperature ();
	}
	int redrawButton = redrawAll || panel.previousPk != !!*pkDemo || panel.previousPressed != panel.pressed;
	if ( redrawButton )
	{
		PanelButton ( !!*pkDemo );
		panel.previousPk = !!*pkDemo;
		panel.previousPressed = panel.pressed;
	}

	if ( redrawAll )
		LCD_Show ();
	else
	{
		if ( redrawTemperature )
		{
			LCD_DisplayPart ( 0, 48, 183, 374, 155 );
			LCD_DisplayPart ( 0, 624, 42, 8, 8 );
		}
		if ( redrawButton )
			LCD_DisplayPart ( 0, 482, 165, 270, 238 );
	}
	return changed;
}

/* Screen 8: dark dashboard based on the supplied reference, fitted to 800x480.
 * Shares only drawing primitives with screen 7; all state belongs to this view.
 */
#define DASH_BG 0xFF0B1523U
#define DASH_CARD 0xFF172839U
#define DASH_EDGE 0xFF304A60U
#define DASH_TEXT 0xFFE2EEFFU
#define DASH_MUTED 0xFF9DC8F4U
#define DASH_CYAN 0xFF61CBFFU
static struct
{
	int active, fonts[4], temperature, previousPk, wasDown, armed, pressed;
	unsigned count, head;
	int history[120];
	uint32_t historyTick[120];
	int graphArmed, tooltip, touchX, touchY;
	uint32_t sampleTick, demoTick;
} dashboard;
uint8_t lcdPanel2PkDemo = 1;
volatile int lcdPanel2Result;
volatile int lcdPanel2Temperature10 = 255;
int ( *lcdPanel2ReadTemperature10 ) ( void );

static void DashText ( int f, int x, int y, char *s, uint32_t c )
{
	PanelText ( dashboard.fonts[f], x, y, s, c );
}
static void DashCenter ( int f, int x, int y, char *s, uint32_t c )
{
	PanelCentered ( dashboard.fonts[f], x, y, s, c );
}
static void DashBox ( int x, int y, int w, int h, int r, uint32_t edge, uint32_t fill )
{
	PanelRound ( x, y, w, h, r, edge, edge, 255 );
	PanelRound ( x + 1, y + 1, w - 2, h - 2, r - 1,
				 fill == DASH_CARD ? 0xFF284C66U : fill,
				 fill == DASH_CARD ? 0xFF102034U : fill, 255 );
}
/* Restore the same card gradient behind changing text and graph. */
static void DashSurface ( int x, int y, int w, int h )
{
	int origin = y >= 288 ? 289 : 67, height = y >= 288 ? 154 : 208;
	for ( int j = y; j < y + h; j++ )
	{
		int t = ( j - origin ) * 255 / ( height - 1 );
		if ( t < 0 )
			t = 0;
		if ( t > 255 )
			t = 255;
		uint32_t c = PanelMix ( 0xFF284C66U, 0xFF102034U, (unsigned) t );
		for ( int i = x; i < x + w; i++ )
			LCD_Buffer ( 800, i, j, c );
	}
}
static void DashLine ( int x, int y, int xx, int yy, uint32_t color )
{
	int dx = abs ( xx - x ), sx = x < xx ? 1 : -1, dy = -abs ( yy - y ), sy = y < yy ? 1 : -1, e = dx + dy;
	for ( ;; )
	{
		PanelPixel ( x, y, color, 255 );
		if ( x == xx && y == yy )
			break;
		int e2 = 2 * e;
		if ( e2 >= dy )
		{
			e += dy;
			x += sx;
		}
		if ( e2 <= dx )
		{
			e += dx;
			y += sy;
		}
	}
}
/* Two-pixel curve with fractional coverage on both edges.
 * Step along the major axis to handle steep segments without gaps.
 * PanelPixel blends edge pixels with the existing card/grid background.
 */
static void DashCurveAA ( int x0, int y0, int x1, int y1, uint32_t color )
{
	int steep = abs ( y1 - y0 ) > abs ( x1 - x0 );
	if ( steep )
	{
		int t = x0;
		x0 = y0;
		y0 = t;
		t = x1;
		x1 = y1;
		y1 = t;
	}
	if ( x0 > x1 )
	{
		int t = x0;
		x0 = x1;
		x1 = t;
		t = y0;
		y0 = y1;
		y1 = t;
	}
	float slope = x1 != x0 ? (float) ( y1 - y0 ) / (float) ( x1 - x0 ) : 0.0f;
	for ( int x = x0; x <= x1; x++ )
	{
		float y = (float) y0 + (float) ( x - x0 ) * slope;
		int base = (int) floorf ( y );
		for ( int k = -1; k <= 2; k++ )
		{
			int pixel = base + k;
			float coverage = 1.5f - fabsf ( (float) pixel - y );
			if ( coverage <= 0 )
				continue;
			unsigned alpha = coverage >= 1.0f ? 255U : (unsigned) ( coverage * 255.0f + 0.5f );
			if ( steep )
				PanelPixel ( pixel, x, color, alpha );
			else
				PanelPixel ( x, pixel, color, alpha );
		}
	}
}
static int DashHit ( XY_Touch_Struct p )
{
	if ( p.y >= 192 && p.y < 224 )
	{
		if ( p.x >= 539 && p.x < 604 )
			return 1;
		if ( p.x >= 613 && p.x < 687 )
			return 2;
	}
	return 0;
}
static void DashStatic ( void )
{
	for ( int y = 0; y < 480; y++ )
		for ( int x = 0; x < 800; x++ )
			LCD_Buffer ( 800, x, y, PanelMix ( 0xFF207FA7U, 0xFF070D24U, (unsigned) ( y * 185 / 479 + x * 70 / 799 ) ) );
	DashText ( 0, 22, 8, "STM32F769 . PANEL ZDALNY", DASH_CYAN );
	DashText ( 1, 22, 28, "Temperatura i przekaznik", DASH_TEXT );
	DashBox ( 639, 13, 139, 25, 13, DASH_EDGE, 0xFF122334U );
	DashCenter ( 0, 708, 19, "Lokalny tryb DEMO", DASH_MUTED );
	DashBox ( 22, 66, 378, 210, 16, DASH_EDGE, DASH_CARD );
	DashBox ( 416, 66, 362, 210, 16, DASH_EDGE, DASH_CARD );
	DashText ( 1, 40, 80, "Czujnik temperatury . 1", DASH_TEXT );
	DashText ( 1, 435, 80, "Przekaznik . DEMO", DASH_TEXT );
	/* Keep the caption below the temperature repaint rectangle (ends at 210). */
	DashText ( 0, 180, 228, "Ostatni pomiar ze STM32", DASH_MUTED );
	DashText ( 0, 435, 248, "DEMO: stan w pamieci STM32, bez GPIO.", DASH_MUTED );
	DashBox ( 22, 288, 756, 156, 16, DASH_EDGE, DASH_CARD );
	DashText ( 1, 40, 299, "Historia temperatury", DASH_TEXT );
	DashText ( 0, 40, 425, "Ostatnie 120 pomiarow . odczyt co 500 ms", DASH_MUTED );
	DashText ( 0, 22, 458, "Pomiar lokalny STM32F769", DASH_MUTED );
	DashText ( 0, 390, 458, "PK: demonstracja . bez sterowania GPIO", DASH_MUTED );
}
static void DashTemperature ( void )
{
	DashSurface ( 48, 112, 334, 98 );
	PanelRound ( 105, 114, 24, 75, 12, 0xFFBACAD6U, 0xFFBACAD6U, 255 );
	PanelRound ( 108, 117, 18, 72, 9, DASH_BG, DASH_BG, 255 );
	int valid = dashboard.temperature >= -800 && dashboard.temperature <= 1500;
	int bounded = dashboard.temperature;
	if ( bounded < -500 )
		bounded = -500;
	if ( bounded > 900 )
		bounded = 900;
	uint32_t mercury = 0xFF52677CU;
	if ( valid )
	{
		if ( bounded <= 200 )
			mercury = PanelMix ( 0xFF168BFFU, 0xFFFFDF20U, (unsigned) ( ( bounded + 500 ) * 255 / 700 ) );
		else
			mercury = PanelMix ( 0xFFFFDF20U, 0xFFFF1C16U, (unsigned) ( ( bounded - 200 ) * 255 / 700 ) );
	}
	int level = valid ? ( bounded + 500 ) * 66 / 1400 : 0;
	if ( level > 0 )
		PanelRound ( 111, 186 - level, 12, level + 4, 6, mercury, mercury, 255 );
	PanelRound ( 99, 174, 36, 36, 18, 0xFFBACAD6U, 0xFFBACAD6U, 255 );
	PanelRound ( 103, 178, 28, 28, 14, mercury, mercury, 255 );
	/* ConstWidth is the native font API's constant-width attribute.
     * Right-align the integer independently: changing 9.9 -> 10.0 or sign
     * cannot move the decimal point, fractional digit or degree/C symbol.
     */
	char integer[16], fraction[2];
	if ( valid )
	{
		int v = dashboard.temperature;
		unsigned a = (unsigned) ( v < 0 ? -v : v );
		snprintf ( integer, sizeof ( integer ), "%s%u", v < 0 ? "-" : "", a / 10 );
		fraction[0] = (char) ( '0' + a % 10 );
		fraction[1] = 0;
	}
	else
	{
		snprintf ( integer, sizeof ( integer ), "--" );
		fraction[0] = '-';
		fraction[1] = 0;
	}
	int font = dashboard.fonts[2];
	int width = LCD_GetWholeStrPxlWidth ( font, integer, 0, ConstWidth );
	LCD_StrChangeColor ( font, 270 - width, 140, integer, fullHight, 0, 0, DASH_TEXT, 255, ConstWidth );
	LCD_StrChangeColor ( font, 270, 140, ".", fullHight, 0, 0, DASH_TEXT, 255, ConstWidth );
	LCD_StrChangeColor ( font, 290, 140, fraction, fullHight, 0, 0, DASH_TEXT, 255, ConstWidth );
	DashText ( 1, 342, 168, "C", DASH_MUTED );
	PanelRound ( 333, 165, 6, 6, 3, DASH_MUTED, DASH_MUTED, 255 );
	PanelRound ( 335, 167, 2, 2, 1, 0xFF193349U, 0xFF193349U, 255 );
	/* Repaint graph, then connect only valid samples; oldest is on the left. */
	/* Include the FULL height of the time labels at y=410. Otherwise their
     * lower antialiased rows accumulate on every sample/tooltip repaint. */
	DashSurface ( 42, 327, 714, 98 );
	const int low = -500, high = 900;
	for ( int i = 0; i < 4; i++ )
	{
		int y = 331 + i * 23;
		DashLine ( 115, y, 735, y, DASH_EDGE );
		int v = high - ( high - low ) * i / 3;
		unsigned magnitude = (unsigned) ( v < 0 ? -v : v );
		char label[20];
		snprintf ( label, sizeof ( label ), "%s%u.%u", v < 0 ? "-" : "", magnitude / 10, magnitude % 10 );
		DashText ( 0, 48, y - 5, label, DASH_MUTED );
	}
	int px = 0, py = 0, previousValid = 0;
	for ( unsigned i = 0; i < dashboard.count; i++ )
	{
		int v = dashboard.history[( dashboard.head + 120 - dashboard.count + i ) % 120];
		if ( v < -800 || v > 1500 )
		{
			previousValid = 0;
			continue;
		}
		int plotted = v < low ? low : ( v > high ? high : v );
		int x = 115 + (int) i * 620 / 119, y = 400 - ( plotted - low ) * 69 / ( high - low );
		if ( previousValid )
			DashCurveAA ( px, py, x, y, DASH_CYAN );
		px = x;
		py = y;
		previousValid = 1;
	}
	DashText ( 0, 115, 410, "-60 s", DASH_MUTED );
	DashText ( 0, 695, 410, "teraz", DASH_MUTED );
}
/* Temporary overlay: the next graph repaint restores its background. */
static void DashTooltip ( void )
{
	if ( dashboard.tooltip < 0 || (unsigned) dashboard.tooltip >= dashboard.count )
		return;
	unsigned slot = ( dashboard.head + 120 - dashboard.count + (unsigned) dashboard.tooltip ) % 120;
	int value = dashboard.history[slot], valid = value >= -800 && value <= 1500;
	int plotted = value < -500 ? -500 : ( value > 900 ? 900 : value );
	int x = 115 + dashboard.tooltip * 620 / 119;
	int y = 400 - ( plotted + 500 ) * 69 / 1400;
	if ( valid )
	{
		DashLine ( x, 331, x, 400, 0xFF4C7E9CU );
		PanelRound ( x - 4, y - 4, 9, 9, 4, DASH_TEXT, DASH_TEXT, 255 );
		PanelRound ( x - 2, y - 2, 5, 5, 2, DASH_CYAN, DASH_CYAN, 255 );
	}
	int bx = x + 12;
	if ( bx > 490 )
		bx = x - 257;
	if ( bx < 115 )
		bx = 115;
	int by = y > 365 ? 332 : 364;
	DashBox ( bx + 2, by + 2, 245, 50, 7, 0xFF081522U, 0xFF081522U );
	DashBox ( bx, by, 245, 50, 7, DASH_CYAN, 0xFF102C42U );
	char line[80];
	if ( valid )
	{
		unsigned magnitude = (unsigned) ( value < 0 ? -value : value );
		snprintf ( line, sizeof ( line ), "Pomiar %u: %s%u.%u C", (unsigned) dashboard.tooltip + 1,
				   value < 0 ? "-" : "", magnitude / 10, magnitude % 10 );
	}
	else
		snprintf ( line, sizeof ( line ), "Pomiar %u: brak danych", (unsigned) dashboard.tooltip + 1 );
	DashText ( 0, bx + 8, by + 5, line, DASH_TEXT );
	uint32_t age = (uint32_t) ( dashboard.sampleTick - dashboard.historyTick[slot] );
	snprintf ( line, sizeof ( line ), "t: -%lu.%lu s  Punkt: %d,%d",
			   (unsigned long) ( age / 1000U ), (unsigned long) ( age % 1000U / 100U ), x, y );
	DashText ( 0, bx + 8, by + 19, line, DASH_MUTED );
	snprintf ( line, sizeof ( line ), "Dotyk X:%d  Y:%d", dashboard.touchX, dashboard.touchY );
	DashText ( 0, bx + 8, by + 33, line, DASH_MUTED );
}
static void DashRelay ( int pk )
{
	DashSurface ( 435, 100, 322, 143 );
	PanelRound ( 570, 110, 64, 64, 32, 0xFF1C3441U, 0xFF1C3441U, 255 );
	if ( pk )
	{
		/* Soft radial halo, blended over the restored card gradient.
         * Draw only when ON; repainting the region removes it on OFF.
         */
		for ( int dy = -42; dy <= 42; dy++ )
			for ( int dx = -42; dx <= 42; dx++ )
			{
				float distance = sqrtf ( (float) ( dx * dx + dy * dy ) );
				float fade = ( 42.0f - distance ) / 19.0f;
				if ( fade <= 0 )
					continue;
				if ( fade > 1 )
					fade = 1;
				float smooth = fade * fade * ( 3.0f - 2.0f * fade );
				PanelPixel ( 602 + dx, 142 + dy, 0xFF58FFC0U, (unsigned) ( 65.0f * smooth ) );
			}
	}
	PanelRound ( 576, 116, 52, 52, 26, pk ? 0xFF60E4B0U : 0xFF52677CU, pk ? 0xFF60E4B0U : 0xFF52677CU, 255 );
	DashCenter ( 1, 602, 171, pk ? "Wlaczony . ON" : "Wylaczony . OFF", DASH_TEXT );
	DashBox ( 539, 192, 65, 32, 8, 0xFF64CDFFU, dashboard.pressed == 1 ? 0xFF125078U : 0xFF237FA6U );
	DashBox ( 613, 192, 74, 32, 8, 0xFF496C88U, dashboard.pressed == 2 ? 0xFF122A3EU : 0xFF203E56U );
	DashCenter ( 0, 571, 202, "Wlacz", DASH_TEXT );
	DashCenter ( 0, 650, 202, "Wylacz", DASH_TEXT );
	DashCenter ( 0, 602, 231, pk ? "Zadany: ON . lokalny DEMO" : "Zadany: OFF . lokalny DEMO", DASH_MUTED );
}
int LCD_TemperaturePkPanel2 ( uint8_t mode, int ( *readTemperature10 ) ( void ), uint8_t *pkDemo )
{
	if ( mode == LCD_TEMP_EXIT )
	{
		LCD_TOUCH_DeleteSelectTouch ( 61001 );
		LCD_TOUCH_DeleteSelectTouch ( 61002 );
		LCD_TOUCH_DeleteSelectTouch ( 61003 );
		dashboard.active = 0;
		return 0;
	}
	if ( !pkDemo || mode > LCD_TEMP_EXIT )
		return -1;
	if ( LCD_GetXSize () != 800 || LCD_GetYSize () != 480 )
		return -2;
	uint32_t now = HAL_GetTick ();
	int all = mode == LCD_TEMP_ENTER || !dashboard.active, changed = 0, sample = 0;
	if ( all )
	{
		dashboard.active = 0;
		SCREEN_ResetAllParameters ();
		LCD_TOUCH_DeleteAllSetTouch ();
		const int sizes[] = { FONT_9, FONT_14_bold, FONT_36, FONT_16 };
		const int ids[] = { fontID_14, fontID_15, fontID_16, fontID_17 };
		for ( int i = 0; i < 4; i++ )
		{
			dashboard.fonts[i] = LCD_LoadFont_ChangeColor ( sizes[i], Arial, ids[i] );
			if ( dashboard.fonts[i] < 0 )
				return -3;
		}
		if ( LCDTOUCH_Set ( 539, 192, 65, 32, ID_TOUCH_POINT, 61001, pressRelease ) < 0 )
			return -4;
		if ( LCDTOUCH_Set ( 613, 192, 74, 32, ID_TOUCH_POINT, 61002, pressRelease ) < 0 )
		{
			LCD_TOUCH_DeleteSelectTouch ( 61001 );
			return -4;
		}
		if ( LCDTOUCH_Set ( 115, 331, 621, 70, ID_TOUCH_POINT, 61003, pressRelease ) < 0 )
		{
			LCD_TOUCH_DeleteSelectTouch ( 61001 );
			LCD_TOUCH_DeleteSelectTouch ( 61002 );
			return -4;
		}
		dashboard.graphArmed = 0;
		dashboard.tooltip = -1;
		dashboard.count = dashboard.head = 0;
		dashboard.previousPk = -1;
		dashboard.wasDown = LCD_TOUCH_isPress () == press;
		dashboard.armed = dashboard.pressed = 0;
		dashboard.demoTick = now;
		dashboard.sampleTick = now - 500U;
		dashboard.active = 1;
		DashStatic ();
	}
	screenTouchState = LCD_TOUCH_GetTypeAndPosition ( &screenTouchPos );
	XY_Touch_Struct position = LCD_TOUCH_GetPos ();
	int down = LCD_TOUCH_isPress () == press, hit = DashHit ( position );
	int inGraph = position.x >= 115 && position.x <= 735 && position.y >= 331 && position.y <= 400;
	if ( down && !dashboard.wasDown )
		dashboard.graphArmed = inGraph;
	if ( !down )
		dashboard.graphArmed = 0;
	if ( down && !dashboard.wasDown )
		dashboard.armed = hit;
	if ( !down && dashboard.wasDown )
	{
		if ( dashboard.armed && hit == dashboard.armed )
		{
			uint8_t requested = hit == 1;
			changed = !!*pkDemo != requested;
			*pkDemo = requested;
		}
		dashboard.armed = 0;
	}
	dashboard.wasDown = down;
	int pressed = down && hit == dashboard.armed ? hit : 0;
	int relay = all || dashboard.previousPk != !!*pkDemo || dashboard.pressed != pressed;
	dashboard.pressed = pressed;
	if ( (uint32_t) ( now - dashboard.sampleTick ) >= 500U )
	{
		dashboard.sampleTick = now;
		if ( readTemperature10 )
			dashboard.temperature = readTemperature10 ();
		else
		{
			/* -50..+90 C sine wave: 30 s cycle, 15 s min to max.
             * Phase offset starts the demonstration at 25.5 C.
             * Wrapping the elapsed milliseconds preserves precision.
             */
			uint32_t elapsed = (uint32_t) ( now - dashboard.demoTick ) % 30000U;
			float phase = 6.28318530718f * (float) elapsed / 30000.0f + 0.07865249f;
			dashboard.temperature = 200 + (int) lroundf ( 700.0f * sinf ( phase ) );
		}
		lcdPanel2Temperature10 = dashboard.temperature;
		dashboard.history[dashboard.head] = dashboard.temperature;
		dashboard.historyTick[dashboard.head] = now;
		dashboard.head = ( dashboard.head + 1 ) % 120;
		if ( dashboard.count < 120 )
			dashboard.count++;
		sample = 1;
	}
	int selected = -1;
	if ( down && dashboard.graphArmed && inGraph && dashboard.count )
	{
		/* Nearest available sample, including the partly filled history. */
		selected = ( ( (int) position.x - 115 ) * 119 + 310 ) / 620;
		if ( (unsigned) selected >= dashboard.count )
			selected = (int) dashboard.count - 1;
	}
	int overlay = dashboard.tooltip != selected ||
				  ( selected >= 0 && ( dashboard.touchX != position.x || dashboard.touchY != position.y ) );
	dashboard.tooltip = selected;
	dashboard.touchX = position.x;
	dashboard.touchY = position.y;
	if ( sample || overlay )
	{
		DashTemperature ();
		DashTooltip ();
	}
	if ( relay )
	{
		DashRelay ( !!*pkDemo );
		dashboard.previousPk = !!*pkDemo;
	}
	if ( all )
		LCD_Show ();
	else
	{
		if ( sample )
			LCD_DisplayPart ( 0, 48, 112, 334, 98 );
		if ( sample || overlay )
			LCD_DisplayPart ( 0, 42, 327, 714, 98 );
		if ( relay )
			LCD_DisplayPart ( 0, 435, 100, 322, 143 );
	}
	return changed;
}

/* Screen 9: three touch-adjustable COLD/WARM/HOT gauges. */
#define GAUGE_MASK_LARGE ( 160 * 72 )
#define GAUGE_MASK_SMALL ( 80 * 36 )
static struct
{
	int active, wasDown, captured, font[2], previous[3];
	struct
	{
		int width, height, offset;
	} label[6];
	uint8_t mask[3 * GAUGE_MASK_LARGE + 3 * GAUGE_MASK_SMALL];
} gauges;
uint16_t lcdGaugePositions[3] = { 500, 180, 820 };
volatile int lcdPanel3Result;
static const struct
{
	int x, y, r, inner, band, pivot, length, base;
} gaugeGeometry[3] = {
	{ 400, 270, 250, 80, 192, 291, 209, 22 },
	{ 265, 435, 118, 37, 90, 445, 106, 11 },
	{ 535, 435, 118, 37, 90, 445, 106, 11 } };
static void GaugeRect ( int index, int *x, int *y, int *w, int *h )
{
	int r = gaugeGeometry[index].r;
	*x = gaugeGeometry[index].x - r - 2;
	*y = gaugeGeometry[index].y - r - 2;
	*w = 2 * r + 4;
	*h = gaugeGeometry[index].y + ( index ? 27 : 45 ) - *y;
}
static int GaugePrepareLabels ( void )
{
	char *words[] = { "COLD", "WARM", "HOT" };
	for ( int i = 0; i < 6; i++ )
	{
		int font = gauges.font[i / 3], w = LCD_GetWholeStrPxlWidth ( font, words[i % 3], 0, NoConstWidth );
		int h = LCD_GetFontHeight ( font ), maxW = i < 3 ? 160 : 80, maxH = i < 3 ? 72 : 36;
		if ( w <= 0 || h <= 0 || w > maxW || h > maxH )
			return -3;
		int offset = i < 3 ? i * GAUGE_MASK_LARGE : 3 * GAUGE_MASK_LARGE + ( i - 3 ) * GAUGE_MASK_SMALL;
		gauges.label[i].width = w;
		gauges.label[i].height = h;
		gauges.label[i].offset = offset;
		for ( int y = 0; y < h; y++ )
			for ( int x = 0; x < w; x++ )
				LCD_Buffer ( 800, x, y, PANEL_WHITE );
		LCD_StrChangeColor ( font, 0, 0, words[i % 3], fullHight, 0, PANEL_WHITE, 0xFF000000U, 255, NoConstWidth );
		for ( int y = 0; y < h; y++ )
			for ( int x = 0; x < w; x++ )
				gauges.mask[offset + y * w + x] = (uint8_t) ( 255U - ( ( pLcd[y * 800 + x] >> 8 ) & 255U ) );
	}
	return 0;
}
static float GaugeMask ( int id, int x, int y )
{
	if ( x < 0 || y < 0 || x >= gauges.label[id].width || y >= gauges.label[id].height )
		return 0;
	return gauges.mask[gauges.label[id].offset + y * gauges.label[id].width + x];
}
static void GaugeLabel ( int index, int word, int selected )
{
	int id = ( index ? 3 : 0 ) + word;
	float theta = ( word == 0 ? 136.0f : word == 1 ? 90.0f
												   : 44.0f ) *
				  0.01745329252f;
	float rotation = 1.57079632679f - theta, cs = cosf ( rotation ), sn = sinf ( rotation );
	float radius = ( gaugeGeometry[index].r + gaugeGeometry[index].band ) * 0.5f;
	float cx = gaugeGeometry[index].x + radius * cosf ( theta );
	float cy = gaugeGeometry[index].y - radius * sinf ( theta );
	float hw = gauges.label[id].width * 0.5f, hh = gauges.label[id].height * 0.5f;
	/* Bound the rotated glyph instead of scanning a large square. Include
     * the one-pixel bilinear fringe on all four sides. */
	int extentX = (int) ceilf ( fabsf ( cs ) * ( hw + 1 ) + fabsf ( sn ) * ( hh + 1 ) ) + 2;
	int extentY = (int) ceilf ( fabsf ( sn ) * ( hw + 1 ) + fabsf ( cs ) * ( hh + 1 ) ) + 2;
	uint32_t color = word == selected ? 0xFF444442U : 0xFFADADABU;
	/* Inverse mapping and bilinear coverage preserve the native SD font's AA
     * while rotating the text tangentially along the grey arc.
     */
	for ( int y = (int) cy - extentY; y <= (int) cy + extentY; y++ )
		for ( int x = (int) cx - extentX; x <= (int) cx + extentX; x++ )
		{
			float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
			float u = cs * dx + sn * dy + hw - 0.5f, v = -sn * dx + cs * dy + hh - 0.5f;
			int ix = (int) floorf ( u ), iy = (int) floorf ( v );
			float fx = u - ix, fy = v - iy;
			float alpha = ( 1 - fy ) * ( ( 1 - fx ) * GaugeMask ( id, ix, iy ) + fx * GaugeMask ( id, ix + 1, iy ) ) +
						  fy * ( ( 1 - fx ) * GaugeMask ( id, ix, iy + 1 ) + fx * GaugeMask ( id, ix + 1, iy + 1 ) );
			if ( alpha > 0 )
				PanelPixel ( x, y, color, (unsigned) ( alpha + 0.5f ) );
		}
}
static void GaugeNeedle ( int index, int shadow )
{
	float angle = ( 160.0f - (float) lcdGaugePositions[index] * 0.14f ) * 0.01745329252f;
	float cs = cosf ( angle ), sn = sinf ( angle ), length = gaugeGeometry[index].length, base = gaugeGeometry[index].base;
	float norm = sqrtf ( 1.0f + base * base / ( length * length ) );
	int x, y, w, h;
	GaugeRect ( index, &x, &y, &w, &h );
	/* Conservative rotated bounds of the needle and its rounded base.
     * Coverage outside this box is zero; keep the original pixel equations. */
	float reach = base + 1.0f;
	float px = gaugeGeometry[index].x + ( shadow ? 3 : 0 );
	float py = gaugeGeometry[index].pivot + ( shadow ? 2 : 0 );
	int left = (int) floorf ( px + fminf ( -reach * cs, ( length + 1 ) * cs ) - reach * fabsf ( sn ) ) - 1;
	int right = (int) ceilf ( px + fmaxf ( -reach * cs, ( length + 1 ) * cs ) + reach * fabsf ( sn ) ) + 1;
	int top = (int) floorf ( py + fminf ( reach * sn, -( length + 1 ) * sn ) - reach * fabsf ( cs ) ) - 1;
	int bottom = (int) ceilf ( py + fmaxf ( reach * sn, -( length + 1 ) * sn ) + reach * fabsf ( cs ) ) + 1;
	if ( left < x )
		left = x;
	if ( right > x + w )
		right = x + w;
	if ( top < y )
		top = y;
	if ( bottom > y + h )
		bottom = y + h;
	for ( int yy = top; yy < bottom; yy++ )
		for ( int xx = left; xx < right; xx++ )
		{
			float dx = xx + 0.5f - gaugeGeometry[index].x - ( shadow ? 3 : 0 );
			float dy = yy + 0.5f - gaugeGeometry[index].pivot - ( shadow ? 2 : 0 );
			float u = dx * cs - dy * sn, v = dx * sn + dy * cs;
			float coverage;
			if ( u < 0 )
				coverage = base + 0.5f - sqrtf ( u * u + v * v );
			else
				coverage = 0.5f + fminf ( length - u, ( base * ( 1 - u / length ) - fabsf ( v ) ) / norm );
			if ( coverage <= 0 )
				continue;
			unsigned alpha = coverage >= 1 ? 255U : (unsigned) ( coverage * 255 );
			float shade = ( v / base + 1.0f ) * 0.5f;
			if ( shade < 0 )
				shade = 0;
			if ( shade > 1 )
				shade = 1;
			uint32_t color = v < 0 ? PanelMix ( 0xFF797975U, 0xFF464743U, (unsigned) ( shade * 255 ) ) : PanelMix ( 0xFF222321U, 0xFF000000U, (unsigned) ( shade * 255 ) );
			PanelPixel ( xx, yy, shadow ? 0xFF000000U : color, shadow ? alpha * 38 / 255 : alpha );
		}
}
static void GaugeDraw ( int index )
{
	int x, y, w, h;
	GaugeRect ( index, &x, &y, &w, &h );
	for ( int yy = y; yy < y + h; yy++ )
		for ( int xx = x; xx < x + w; xx++ )
		{
			float dx = xx + 0.5f - gaugeGeometry[index].x, dy = yy + 0.5f - gaugeGeometry[index].y;
			float distance2 = dx * dx + dy * dy;
			float outerRadius = gaugeGeometry[index].r + 0.5f;
			if ( distance2 >= outerRadius * outerRadius )
			{
				LCD_Buffer ( 800, xx, yy, PANEL_WHITE );
				continue;
			}
			float radius = sqrtf ( distance2 );
			float angle = atan2f ( -dy, dx ) * 57.2957795131f;
			uint32_t color = PANEL_WHITE;
			float angular = fminf ( angle - 20.0f, 160.0f - angle ) * 0.01745329252f * radius;
			float outer = fminf ( gaugeGeometry[index].r + 0.5f - radius, angular + 0.5f );
			if ( outer > 0 )
			{
				float grey = fminf ( outer, radius - gaugeGeometry[index].band + 0.5f );
				if ( grey > 0 )
					color = PanelMix ( PANEL_WHITE, 0xFFE6E6E5U, grey >= 1 ? 255 : (unsigned) ( grey * 255 ) );
				float band = fminf ( gaugeGeometry[index].band + 0.5f - radius, radius - gaugeGeometry[index].inner + 0.5f );
				float coverage = fminf ( band, angular + 0.5f );
				if ( coverage > 0 )
				{
					float edgeWarm = ( angle - 70.0f ) * 0.01745329252f * radius + 0.5f;
					float edgeCold = ( angle - 110.0f ) * 0.01745329252f * radius + 0.5f;
					if ( edgeWarm < 0 )
						edgeWarm = 0;
					if ( edgeWarm > 1 )
						edgeWarm = 1;
					if ( edgeCold < 0 )
						edgeCold = 0;
					if ( edgeCold > 1 )
						edgeCold = 1;
					uint32_t sector = PanelMix ( 0xFFFF0800U, 0xFFFFB600U, (unsigned) ( edgeWarm * 255 ) );
					sector = PanelMix ( sector, 0xFF00B8F1U, (unsigned) ( edgeCold * 255 ) );
					float f = ( radius - gaugeGeometry[index].inner ) / ( gaugeGeometry[index].band - gaugeGeometry[index].inner );
					if ( f < 0 )
						f = 0;
					if ( f > 1 )
						f = 1;
					/* Dark rim shading fades into bright colour toward the hole. */
					float f2 = f * f, f4 = f2 * f2;
					unsigned shade = (unsigned) ( 85.0f * f4 * f2 * f );
					sector = PanelMix ( sector, 0xFF333323U, shade );
					color = PanelMix ( color, sector, coverage >= 1 ? 255 : (unsigned) ( coverage * 255 ) );
				}
			}
			LCD_Buffer ( 800, xx, yy, color );
		}
	int selected = lcdGaugePositions[index] < 357 ? 0 : lcdGaugePositions[index] > 643 ? 2
																					   : 1;
	for ( int word = 0; word < 3; word++ )
		GaugeLabel ( index, word, selected );
	GaugeNeedle ( index, 1 );
	GaugeNeedle ( index, 0 );
}
static int GaugeHit ( XY_Touch_Struct point )
{
	for ( int i = 0; i < 3; i++ )
	{
		int dx = (int) point.x - gaugeGeometry[i].x, dy = (int) point.y - gaugeGeometry[i].y;
		int r = gaugeGeometry[i].r;
		if ( dy <= 27 && dx * dx + dy * dy <= r * r )
			return i;
	}
	return -1;
}
int LCD_TemperaturePkPanel3 ( uint8_t mode )
{
	if ( mode == LCD_TEMP_EXIT )
	{
		for ( int i = 0; i < 3; i++ )
			LCD_TOUCH_DeleteSelectTouch ( (uint16_t) ( 61004 + i ) );
		gauges.active = 0;
		gauges.captured = -1;
		return 0;
	}
	if ( mode > LCD_TEMP_EXIT )
		return -1;
	if ( LCD_GetXSize () != 800 || LCD_GetYSize () != 480 )
		return -2;
	int all = mode == LCD_TEMP_ENTER || !gauges.active, changed = 0;
	if ( all )
	{
		gauges.active = 0;
		SCREEN_ResetAllParameters ();
		LCD_TOUCH_DeleteAllSetTouch ();
		gauges.font[0] = LCD_LoadFont_ChangeColor ( FONT_36, Arial, fontID_14 );
		gauges.font[1] = LCD_LoadFont_ChangeColor ( FONT_16, Arial, fontID_15 );
		if ( gauges.font[0] < 0 || gauges.font[1] < 0 || GaugePrepareLabels () < 0 )
			return -3;
		for ( int i = 0; i < 3; i++ )
		{
			int x, y, w, h;
			GaugeRect ( i, &x, &y, &w, &h );
			if ( LCDTOUCH_Set ( x, y, w, h, ID_TOUCH_POINT, (uint16_t) ( 61004 + i ), pressRelease ) < 0 )
			{
				for ( int j = 0; j < i; j++ )
					LCD_TOUCH_DeleteSelectTouch ( (uint16_t) ( 61004 + j ) );
				return -4;
			}
			gauges.previous[i] = -1;
		}
		for ( int y = 0; y < 480; y++ )
			for ( int x = 0; x < 800; x++ )
				LCD_Buffer ( 800, x, y, PANEL_WHITE );
		gauges.wasDown = LCD_TOUCH_isPress () == press;
		gauges.captured = -1;
		gauges.active = 1;
	}
	screenTouchState = LCD_TOUCH_GetTypeAndPosition ( &screenTouchPos );
	XY_Touch_Struct position = LCD_TOUCH_GetPos ();
	int down = LCD_TOUCH_isPress () == press;
	if ( down && !gauges.wasDown )
		gauges.captured = GaugeHit ( position );
	if ( !down )
		gauges.captured = -1;
	if ( down && gauges.captured >= 0 )
	{
		int i = gauges.captured;
		float dx = (float) position.x - gaugeGeometry[i].x, dy = (float) position.y - gaugeGeometry[i].pivot;
		if ( dx * dx + dy * dy > 144.0f )
		{
			float angle = atan2f ( -dy, dx ) * 57.2957795131f;
			if ( angle < 0 )
				angle = dx < 0 ? 160.0f : 20.0f;
			if ( angle < 20 )
				angle = 20;
			if ( angle > 160 )
				angle = 160;
			unsigned value = (unsigned) lroundf ( ( 160 - angle ) * 1000.0f / 140.0f );
			if ( value != lcdGaugePositions[i] )
			{
				lcdGaugePositions[i] = (uint16_t) value;
				changed = 1;
			}
		}
	}
	gauges.wasDown = down;
	for ( int i = 0; i < 3; i++ )
	{
		if ( lcdGaugePositions[i] > 1000 )
			lcdGaugePositions[i] = 1000;
		if ( all || gauges.previous[i] != lcdGaugePositions[i] )
		{
			GaugeDraw ( i );
			gauges.previous[i] = lcdGaugePositions[i];
			if ( !all )
			{
				int x, y, w, h;
				GaugeRect ( i, &x, &y, &w, &h );
				LCD_DisplayPart ( 0, x, y, w, h );
			}
		}
	}
	if ( all )
		LCD_Show ();
	return changed;
}
