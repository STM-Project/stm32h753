#include "SCREEN_HttpPanels.h"
#include "SCREEN_ReadPanel.h"
#include "SCREEN_TemperaturePk.h"
#include "LCD_BasicGraphics.h"
#include "LCD_fonts_images.h"
#include "LCD_Hardware.h"
#include "http_lcd_panels.h"
#include "wwwPages.h"
#include "touch.h"
#include <stdio.h>
#include <string.h>

extern uint32_t pLcd[];
#define W 800
#define H 480
#define TOP 72
#define VIEW 368
#define SCRATCH ( W * VIEW )
#define CONTENT_X 36
#define COL_W 216
#define COL_STEP 230
#define ROW_W 690
#define SCROLL_X 738
#define SCROLL_W 22
#define BG 0xFF000000U
#define FRAME 0xFF808080U
#define CARD 0xFF505050U
#define GREEN 0xFF9EDF0FU
#define WHITE 0xFFEEEEEEU
#define MUTED 0xFFBBBBBBU
#define VALUE_BLACK 0xFF000000U

_Static_assert ( SCRATCH + ROW_W * 72 <= W * H, "LCD tile scratch exceeds pLcd" );
_Static_assert ( SCRATCH + COL_W * 210 <= W * H, "LCD read scratch exceeds pLcd" );
static struct
{
	int active, settings, font[5], scroll, extent, count, wasDown, capture, startY, startScroll, armed, pressed, moved, dirty, full;
	uint8_t changed[HTTP_LCD_PORTS];
	unsigned pwmPort, pwmValue;
	int pwmSent;
	int outputPressed, modePressed, menuPort, menuY, menuX, menuCount, menuArmed, menuPressed, menuUp;
	uint8_t menuModes[6];
	int savePressed;
	uint32_t sample;
	struct
	{
		int port, x, y, w, h;
	} tiles[HTTP_LCD_PORTS];
	HTTP_LCDPanelData data, latest;
} ui;
/* Local edits survive next/prev navigation, but not a power cycle. */
static struct
{
	int initialized, saveError, saved;
	int8_t draftMode[HTTP_LCD_PORTS];
	uint8_t draftBase[HTTP_LCD_PORTS];
	uint32_t saving;
} edit;
static void SaveCompleted ( const HTTP_LCDPanelData *data )
{
	if ( edit.saving && data->mode_serial == edit.saving )
	{
		edit.saving = 0;
		edit.saveError = data->mode_result != 1;
		edit.saved = !edit.saveError;
		if ( !edit.saveError )
		{
			memset ( edit.draftMode, -1, sizeof edit.draftMode );
			ui.full = ui.dirty = 1;
		}
	}
}

static const char *ModeName ( unsigned mode )
{
	switch ( mode )
	{
	case HTTP_MODE_TEMP:
		return "Temperatura";
	case HTTP_MODE_TEMP_RHU:
		return "Temperatura / RH";
	case HTTP_MODE_PT1000:
		return "PT1000";
	case HTTP_MODE_ADC:
		return "ADC 0-10 V";
	case HTTP_MODE_COUNTER:
		return "Licznik impulsow";
	case HTTP_MODE_INPUT:
		return "Wejscie";
	case HTTP_MODE_EOL:
		return "Wejscie EOL";
	case HTTP_MODE_PWM:
		return "PWM";
	case HTTP_MODE_RELAY:
		return "Przekaznik";
	case HTTP_MODE_IR:
		return "IR";
	case HTTP_MODE_TIMER:
		return "Timer";
	case HTTP_MODE_THERMOSTAT:
		return "Termostat";
	default:
		return "Wylaczony";
	}
}
static unsigned ShownMode ( unsigned port )
{
	return ui.settings && edit.draftMode[port - 1] >= 0 ? (unsigned) edit.draftMode[port - 1] : ui.data.ports[port - 1].mode;
}
/* The exact option sets and ordering used by HTTP_TEMPLATE_ModeInput/Output2. */
static int ModeOptions ( unsigned port, uint8_t out[6] )
{
	static const uint8_t sensor[] = { HTTP_MODE_TEMP, HTTP_MODE_PT1000, HTTP_MODE_TEMP_RHU, HTTP_MODE_INPUT, HTTP_MODE_EOL, HTTP_MODE_DISABLED };
	static const uint8_t adc[] = { HTTP_MODE_ADC, HTTP_MODE_DISABLED };
	static const uint8_t counter[] = { HTTP_MODE_TEMP, HTTP_MODE_TEMP_RHU, HTTP_MODE_INPUT, HTTP_MODE_COUNTER, HTTP_MODE_DISABLED };
	static const uint8_t pwm[] = { HTTP_MODE_RELAY, HTTP_MODE_TIMER, HTTP_MODE_THERMOSTAT, HTTP_MODE_PWM, HTTP_MODE_IR, HTTP_MODE_DISABLED };
	static const uint8_t ir[] = { HTTP_MODE_RELAY, HTTP_MODE_TIMER, HTTP_MODE_THERMOSTAT, HTTP_MODE_IR, HTTP_MODE_DISABLED };
	static const uint8_t relay[] = { HTTP_MODE_RELAY, HTTP_MODE_TIMER, HTTP_MODE_THERMOSTAT, HTTP_MODE_DISABLED };
	const uint8_t *options;
	size_t count;
	if ( port < 1 || port > HTTP_LCD_PORTS )
		return 0;
	if ( port <= 14 )
	{
		options = sensor;
		count = sizeof sensor;
	}
	else if ( port <= 16 )
	{
		options = adc;
		count = sizeof adc;
	}
	else if ( port <= 20 )
	{
		options = counter;
		count = sizeof counter;
	}
	else if ( port <= 23 )
	{
		options = pwm;
		count = sizeof pwm;
	}
	else if ( port <= 27 )
	{
		options = ir;
		count = sizeof ir;
	}
	else
	{
		options = relay;
		count = sizeof relay;
	}
	memcpy ( out, options, count );
	return (int) count;
}
static int Group ( unsigned mode )
{
	switch ( mode )
	{
	case HTTP_MODE_TEMP:
	case HTTP_MODE_TEMP_RHU:
		return 0;
	case HTTP_MODE_COUNTER:
		return 1;
	case HTTP_MODE_INPUT:
		return 2;
	case HTTP_MODE_EOL:
		return 3;
	case HTTP_MODE_ADC:
	case HTTP_MODE_PT1000:
		return 4;
	case HTTP_MODE_PWM:
		return 5;
	case HTTP_MODE_RELAY:
	case HTTP_MODE_IR:
		return 6;
	case HTTP_MODE_TIMER:
	case HTTP_MODE_THERMOSTAT:
		return 7;
	default:
		return 8;
	}
}
static int Maximum ( void )
{
	return ui.extent > VIEW ? ui.extent - VIEW : 0;
}
static int Clamp ( int value, int low, int high )
{
	return value < low ? low : value > high ? high
											: value;
}
static void Layout ( void )
{
	ui.count = 0;
	int row = 0, column = 0, height = 0;
	for ( unsigned group = 0; group < ( ui.settings ? 1u : 8u ); ++group )
		for ( unsigned port = 1; port <= HTTP_LCD_PORTS; ++port )
		{
			unsigned mode = ui.data.ports[port - 1].mode;
			if ( !ui.settings && Group ( mode ) != (int) group )
				continue;
			int index = ui.count++;
			ui.tiles[index].port = (int) port;
			ui.tiles[index].x = CONTENT_X + ( ui.settings ? 0 : column * COL_STEP );
			ui.tiles[index].y = row;
			ui.tiles[index].w = ui.settings ? ROW_W : COL_W;
			ui.tiles[index].h = ui.settings ? 68 : mode == HTTP_MODE_COUNTER ? 210
																			 : 166;
			if ( ui.tiles[index].h > height )
				height = ui.tiles[index].h;
			if ( ui.settings || ++column == 3 )
			{
				row += height + 12;
				column = 0;
				height = 0;
			}
		}
	ui.extent = row + ( column ? height + 12 : 0 );
	ui.scroll = Clamp ( ui.scroll, 0, Maximum () );
}
/* Local tile canvas in the unused tail of pLcd. Text is drawn with the native
 * windowed font engine, then clipped into the viewport, so no negative glyph
 * coordinates or extra full-screen SDRAM allocation are needed. */
static void Rect ( int stride, int x, int y, int w, int h, uint32_t color )
{
	for ( int row = 0; row < h; ++row )
		for ( int col = 0; col < w; ++col )
			pLcd[SCRATCH + ( y + row ) * stride + x + col] = color;
}
static void TileRound ( int stride, int x, int y, int w, int h, int radius, uint32_t top, uint32_t bottom )
{
	for ( int row = 0; row < h; ++row )
	{
		unsigned t = h > 1 ? (unsigned) ( row * 255 / ( h - 1 ) ) : 0;
		uint32_t color = 0xFF000000U;
		for ( unsigned shift = 0; shift <= 16; shift += 8 )
			color |= ( ( ( ( top >> shift ) & 255U ) * ( 255 - t ) + ( ( bottom >> shift ) & 255U ) * t ) / 255U ) << shift;
		for ( int col = 0; col < w; ++col )
		{
			/* Pixel centres in doubled coordinates: even small radii must
			 * remove corner pixels, symmetrically on all four sides. */
			int dx = col < radius ? 2 * ( radius - col ) - 1 : col >= w - radius ? 2 * ( col - ( w - radius ) ) + 1
																				 : 0;
			int dy = row < radius ? 2 * ( radius - row ) - 1 : row >= h - radius ? 2 * ( row - ( h - radius ) ) + 1
																				 : 0;
			if ( dx * dx + dy * dy <= 4 * radius * radius )
				pLcd[SCRATCH + ( y + row ) * stride + x + col] = color;
		}
	}
}
static void Text ( int stride, int height, int font, int x, int y, int width, const char *text, uint32_t color )
{
	char copy[96];
	snprintf ( copy, sizeof copy, "%s", text );
	int truncated = 0;
	while ( copy[0] && LCD_GetWholeStrPxlWidth ( font, copy, 0, NoConstWidth ) > width )
	{
		size_t n = strlen ( copy );
		if ( truncated && n >= 3 )
			n -= 3;
		if ( !n )
			break;
		--n;
		while ( n && ( (unsigned char) copy[n] & 0xC0U ) == 0x80U )
			--n;
		copy[n] = 0;
		if ( n + 3 < sizeof copy )
			strcat ( copy, "..." );
		truncated = 1;
		if ( !n )
		{
			copy[0] = 0;
			break;
		}
	}
	if ( y >= 0 && y + LCD_GetFontHeight ( font ) <= height && x >= 0 && x + width <= stride )
		LCD_StrChangeColorWindow ( SCRATCH, (uint32_t) stride, (uint32_t) height, font, x, y, copy, fullHight, 0, 0, color, 255, NoConstWidth );
}
static void CenterText ( int w, int h, int font, int y, const char *text, uint32_t color )
{
	int width = LCD_GetWholeStrPxlWidth ( font, (char *) text, 0, NoConstWidth );
	if ( width > w - 28 )
		width = w - 28;
	Text ( w, h, font, ( w - width ) / 2, y, w - ( w - width ) / 2 - 14, text, color );
}
/* Wrap port descriptions at word boundaries, with a UTF-8-safe fallback
 * for long words. The third line uses the existing ellipsis clipping. */
static void Description ( int w, int h, int y, const char *name )
{
	char remaining[96], line[96];
	snprintf ( remaining, sizeof remaining, "%s", name );
	int font = ui.font[0], width = w - 28;
	for ( int row = 0; row < 3 && remaining[0]; ++row )
	{
		if ( row == 2 || LCD_GetWholeStrPxlWidth ( font, remaining, 0, NoConstWidth ) <= width )
		{
			CenterText ( w, h, font, y, remaining, GREEN );
			break;
		}
		size_t fit = 0, space = 0;
		for ( size_t next = 0; remaining[next]; )
		{
			++next;
			while ( ( (unsigned char) remaining[next] & 0xC0 ) == 0x80 )
				++next;
			memcpy ( line, remaining, next );
			line[next] = 0;
			if ( LCD_GetWholeStrPxlWidth ( font, line, 0, NoConstWidth ) > width )
				break;
			fit = next;
			if ( remaining[next] == ' ' )
				space = next;
		}
		if ( space )
			fit = space;
		if ( !fit )
			break;
		memcpy ( line, remaining, fit );
		line[fit] = 0;
		CenterText ( w, h, font, y, line, GREEN );
		while ( remaining[fit] == ' ' )
			++fit;
		memmove ( remaining, remaining + fit, strlen ( remaining + fit ) + 1 );
		y += LCD_GetFontHeight ( font ) + 2;
	}
}
/* Draw the degree ring directly: independent of SD font character sets. */
static void Degree ( int stride, int x, int y, uint32_t color )
{
	for ( int row = 0; row < 5; ++row )
		for ( int col = 0; col < 5; ++col )
		{
			int dx = col - 2, dy = row - 2, square = dx * dx + dy * dy;
			if ( square >= 2 && square <= 5 )
				Rect ( stride, x + col, y + row, 1, 1, color );
		}
}
/* Native half-height is measured at the dot/baseline, excluding descenders
 * (the same metric used by LCD's Right_down text positioning). */
static void Measurement ( int w, int h, int x, int y, int width, const char *value, uint32_t color, int font, int align )
{
	char number[32], unit[12] = "";
	snprintf ( number, sizeof number, "%s", value );
	char *split = strrchr ( number, ' ' );
	if ( split )
	{
		*split++ = 0;
		snprintf ( unit, sizeof unit, "%s", split );
	}
	/* Humidity is displayed as whole percent; retain the shared raw value. */
	if ( strcmp ( unit, "%" ) == 0 )
	{
		char *dot = strchr ( number, '.' );
		if ( dot )
			*dot = 0;
	}
	int degree = strcmp ( unit, "C" ) == 0 ? 7 : 0;
	int unit_width = LCD_GetWholeStrPxlWidth ( ui.font[0], unit, 0, NoConstWidth ) + degree;
	int gap = unit[0] ? 4 : 0;
	int available = width - unit_width - gap;
	int number_width = LCD_GetWholeStrPxlWidth ( font, number, 0, NoConstWidth );
	int drawn = number_width < available ? number_width : available;
	int spare = width - drawn - gap - unit_width;
	if ( align == 1 )
		x += spare / 2;
	else if ( align == 2 )
		x += spare;
	Text ( w, h, font, x, y, drawn, number, color );
	if ( unit[0] )
	{
		int unit_y = y + LCD_GetFontHalfHeight ( font ) - LCD_GetFontHalfHeight ( ui.font[0] );
		int unit_x = x + drawn + gap;
		if ( degree && unit_y >= 0 && unit_y + 8 <= h )
			Degree ( w, unit_x, unit_y + 3, color );
		Text ( w, h, ui.font[0], unit_x + degree, unit_y, unit_width - degree, unit, color );
	}
}
static void CopyTile ( int index )
{
	int y = ui.tiles[index].y - ui.scroll;
	int first = y < 0 ? -y : 0;
	int end = ui.tiles[index].h;
	if ( y + end > VIEW )
		end = VIEW - y;
	for ( int row = first; row < end; ++row )
		memcpy ( pLcd + ( y + row ) * W + ui.tiles[index].x,
				 pLcd + SCRATCH + row * ui.tiles[index].w, (size_t) ui.tiles[index].w * sizeof *pLcd );
}
static uint32_t RelayTextColor ( const char *state )
{
	return strcmp ( state, "Stan OFF" ) == 0 ? VALUE_BLACK : WHITE;
}
#include "lpc_thermometer_panel.inc"
static void Tile ( int index )
{
	int port = ui.tiles[index].port;
	const HTTP_LCDPort *p = &ui.data.ports[port - 1];
	int w = ui.tiles[index].w, h = ui.tiles[index].h;
	Rect ( w, 0, 0, w, h, BG );
	TileRound ( w, 0, 0, w, h, 18, FRAME, FRAME );
	int border = 2;
	TileRound ( w, border, border, w - 2 * border, h - 2 * border, 18 - border, CARD, CARD );
	char line[80];
	if ( ui.settings )
	{
		unsigned shown = ShownMode ( (unsigned) port );
		int same = shown == p->mode;
		Text ( w, h, ui.font[1], 14, 10, 198, p->name, WHITE );
		Text ( w, h, ui.font[0], 14, 36, 198, "Nazwa portu", MUTED );
		int selected = ui.modePressed == port || ui.menuPort == port;
		TileRound ( w, 224, 16, 160, 36, 4, selected ? 0xFFAAC765U : 0xFFBBBBBBU, selected ? 0xFFAAC765U : 0xFFBBBBBBU );
		Text ( w, h, ui.font[0], 234, 25, 122, ModeName ( shown ), 0xFF111111U );
		for ( int row = 0; row < 5; ++row )
		{
			int half = selected && ui.menuUp ? row : 4 - row;
			Rect ( w, 368 - half, 31 + row, 2 * half + 1, 1, 0xFF222222U );
		}
		snprintf ( line, sizeof line, "%d", port );
		TileRound ( w, 394, ui.pressed == port ? 19 : 16, 36, 36, 4, ui.pressed == port ? 0xFF99CCFFU : 0xFF468CCFU, ui.pressed == port ? 0xFF99CCFFU : 0xFF468CCFU );
		Text ( w, h, ui.font[1], port < 10 ? 407 : 401, ui.pressed == port ? 26 : 23, port < 10 ? 14 : 26, line, 0xFF172000U );
		if ( shown == HTTP_MODE_PWM )
		{
			unsigned value = ui.capture == 3 && ui.pwmPort == (unsigned) port ? ui.pwmValue : same ? p->pwm
																								   : 0;
			snprintf ( line, sizeof line, "%u %%", value );
			Measurement ( w, h, 442, 22, 74, line, WHITE, ui.font[3], 0 );
			TileRound ( w, 522, 29, 144, 10, 4, 0xFF333333U, 0xFF222222U );
			if ( value )
				TileRound ( w, 522, 29, (int) value * 144 / 100, 10, 3, GREEN, 0xFF88B80CU );
			TileRound ( w, 515 + (int) value * 144 / 100, 21, 14, 26, 5, WHITE, 0xFFBBBBBBU );
		}
		else if ( shown == HTTP_MODE_TEMP_RHU )
		{
			for ( int column = 0; column < 2; ++column )
			{
				int x = 442 + column * 120;
				TileRound ( w, x, 16, 112, 36, 4, WHITE, WHITE );
				Measurement ( w, h, x + 6, 22, 100, same ? p->value[column] : "---", VALUE_BLACK, ui.font[3], 0 );
			}
		}
		else
		{
			int output = shown == HTTP_MODE_RELAY || shown == HTTP_MODE_IR;
			uint32_t fill = output ? ( ui.outputPressed == port ? 0xFF90C0ECU : 0xFF468CCFU ) : WHITE;
			TileRound ( w, 442, ui.outputPressed == port ? 19 : 16, 232, 36, 4, fill, fill );
			const char *value = shown == HTTP_MODE_DISABLED ? "---" : same ? p->value[0]
																		   : "---";
			uint32_t color = output ? RelayTextColor ( value ) : VALUE_BLACK;
			if ( shown == HTTP_MODE_TEMP || shown == HTTP_MODE_PT1000 || shown == HTTP_MODE_ADC || shown == HTTP_MODE_COUNTER )
				Measurement ( w, h, 454, 22, 208, value, color, ui.font[3], 0 );
			else
				Text ( w, h, ui.font[3], 454, ui.outputPressed == port ? 25 : 22, 208, value, color );
		}
	}
	else
	{
		if ( p->mode == HTTP_MODE_COUNTER )
		{
			static const char *labels[4] = { "godzinny", "dobowy", "tygodniowy", "miesieczny" };
			Text ( w, h, ui.font[1], 14, 18, w - 28, p->value[0], p->color[0] );
			for ( int row = 1; row < 5; ++row )
			{
				Text ( w, h, ui.font[0], 14, 23 + row * 25, 118, p->value[row], p->color[row] );
				Text ( w, h, ui.font[0], 137, 23 + row * 25, 65, labels[row - 1], MUTED );
			}
			Description ( w, h, 151, p->name );
		}
		else if ( p->mode == HTTP_MODE_PWM )
		{
			unsigned value = ui.capture == 3 && ui.pwmPort == (unsigned) port ? ui.pwmValue : p->pwm;
			snprintf ( line, sizeof line, "%u %%", value );
			Measurement ( w, h, 14, 30, w - 28, line, WHITE, ui.font[4], 1 );
			TileRound ( w, 18, 88, 180, 12, 5, 0xFF444444U, 0xFF333333U );
			if ( value )
				TileRound ( w, 18, 88, (int) value * 180 / 100, 12, 4, GREEN, 0xFF88B80CU );
			int knob = 18 + (int) value * 180 / 100;
			TileRound ( w, knob - 8, 81, 16, 26, 6, WHITE, 0xFFBBBBBBU );
			Description ( w, h, 108, p->name );
		}
		else
		{
			if ( p->mode == HTTP_MODE_RELAY || p->mode == HTTP_MODE_IR )
				TileRound ( w, 14, 40, w - 28, 43, 5, ui.pressed == port ? 0xFF7BB6EBU : 0xFF468CCFU, 0xFF3579B9U );
			if ( p->mode == HTTP_MODE_TEMP_RHU )
			{
				Measurement ( w, h, 44, 18, w - 88, p->value[0], p->color[0], ui.font[4], 0 );
				Measurement ( w, h, 44, 62, w - 88, p->value[1], p->color[1], ui.font[4], 2 );
			}
			else if ( p->mode == HTTP_MODE_TEMP || p->mode == HTTP_MODE_PT1000 || p->mode == HTTP_MODE_ADC )
				Measurement ( w, h, 14, 30, w - 28, p->value[0], p->color[0], ui.font[4], 1 );
			else
				CenterText ( w, h, ui.font[2], 45, p->value[0], p->mode == HTTP_MODE_RELAY || p->mode == HTTP_MODE_IR ? RelayTextColor ( p->value[0] ) : p->color[0] );
			Description ( w, h, 104, p->name );
		}
	}
	CopyTile ( index );
}
static void Thumb ( int *y, int *h )
{
	*h = ui.extent > VIEW ? VIEW * VIEW / ui.extent : VIEW;
	if ( *h < 32 )
		*h = 32;
	*y = Maximum () ? ui.scroll * ( VIEW - *h ) / Maximum () : 0;
}
static void Header ( void )
{
	if ( ui.settings == 2 )
	{
		NowaHeader ();
		return;
	}
	Rect ( W, 0, 0, W, TOP, BG );
	TileRound ( W, 36, 8, 690, 46, 7, FRAME, 0xFF333333U );
	Text ( W, 64, ui.font[1], 52, 20, ui.settings ? 410 : 480, ui.settings ? "H7 . Ustawienia portow /set" : "H7 . Panel odczytowy /main", WHITE );
	if ( ui.settings )
	{
		uint32_t fill = edit.saving ? 0xFF9A6600U : edit.saveError ? 0xFFAD3333U
												: edit.saved	   ? 0xFF347B3CU
												: ui.savePressed   ? 0xFF285F90U
																   : 0xFF468CCFU;
		TileRound ( W, 538, 14, 174, 34, 4, fill, fill );
		Text ( W, 64, ui.font[1], 550, 21, 150, edit.saving ? "Zapisywanie..." : edit.saveError ? "Blad zapisu"
																			 : edit.saved		? "Zapisano"
																								: "Zapisz",
			   WHITE );
	}
	else
		Text ( W, 64, ui.font[0], 602, 24, 108, ui.data.time, GREEN );
	LCD_DisplayBuff ( 0, 0, W, TOP, pLcd + SCRATCH );
	Rect ( W, 0, 0, W, 40, BG );
	const char *notice = "Pomiar i stany HTTP . przewijaj palcem lub suwakiem";
	uint32_t notice_color = MUTED;
	if ( ui.settings )
	{
		notice = "Nazwa portu                  Tryb               Nr          Odczyt / sterowanie";
		if ( edit.saving )
		{
			notice = "Zapisywanie ustawien na karcie SD...";
			notice_color = 0xFFFFCC55U;
		}
		else if ( edit.saveError )
		{
			notice = "Nie zapisano ustawien. Sprawdz SD lub ponow wybor.";
			notice_color = 0xFFFF8888U;
		}
		else if ( edit.saved )
		{
			notice = "Zapisano na SD. Ustawienia sa dostepne w HTTP.";
			notice_color = GREEN;
		}
	}
	Text ( W, 40, ui.font[0], 36, 12, 690, notice, notice_color );
	LCD_DisplayBuff ( 0, 440, W, 40, pLcd + SCRATCH );
}
#define MENU_W 240
#define MENU_ITEM 32
_Static_assert ( SCRATCH + MENU_W * ( 6 * MENU_ITEM + 4 ) <= W * H, "LCD mode dropdown exceeds pLcd scratch" );
static void DrawMenu ( void )
{
	int h = ui.menuCount * MENU_ITEM + 4;
	Rect ( MENU_W, 0, 0, MENU_W, h, 0xFFB7B7B7U );
	for ( int i = 0; i < ui.menuCount; ++i )
	{
		int y = 2 + i * MENU_ITEM;
		int selected = ui.menuModes[i] == ShownMode ( (unsigned) ui.menuPort );
		uint32_t color = ui.menuPressed == i ? 0xFFD9F5ADU : selected ? 0xFFA1C966U
																	  : 0xFFE0E0E0U;
		Rect ( MENU_W, 2, y, MENU_W - 4, MENU_ITEM - 1, color );
		Text ( MENU_W, h, ui.font[0], 12, y + 8, MENU_W - 42, ModeName ( ui.menuModes[i] ), 0xFF202020U );
		if ( selected )
			Text ( MENU_W, h, ui.font[0], MENU_W - 25, y + 8, 16, "*", 0xFF202020U );
	}
	int y = ui.menuY - TOP;
	for ( int row = 0; row < h; ++row )
		memcpy ( pLcd + ( y + row ) * W + ui.menuX, pLcd + SCRATCH + row * MENU_W, MENU_W * sizeof *pLcd );
}
static void Draw ( void )
{
	if ( ui.menuPort )
		ui.full = 1;
	if ( ui.full )
		for ( int i = 0; i < W * VIEW; ++i )
			pLcd[i] = BG;
	for ( int i = 0; i < ui.count; ++i )
		if ( ( ui.full || ui.changed[ui.tiles[i].port - 1] ) && ui.tiles[i].y + ui.tiles[i].h > ui.scroll && ui.tiles[i].y < ui.scroll + VIEW )
			Tile ( i );
	if ( ui.full )
	{
		int thumb_y, thumb_h;
		Thumb ( &thumb_y, &thumb_h );
		for ( int y = 0; y < VIEW; ++y )
			for ( int x = 0; x < SCROLL_W; ++x )
				pLcd[y * W + SCROLL_X + x] = y >= thumb_y && y < thumb_y + thumb_h ? GREEN : 0xFF333333U;
		if ( ui.menuPort )
			DrawMenu ();
		LCD_DisplayBuff ( 0, TOP, W, VIEW, pLcd );
	}
	else
		for ( int i = 0; i < ui.count; ++i )
			if ( ui.changed[ui.tiles[i].port - 1] )
			{
				int y = ui.tiles[i].y - ui.scroll, bottom = y + ui.tiles[i].h;
				if ( y < 0 )
					y = 0;
				if ( bottom > VIEW )
					bottom = VIEW;
				if ( bottom > y )
					LCD_DisplayBuffStride ( (uint32_t) ui.tiles[i].x, (uint32_t) ( TOP + y ), (uint32_t) ui.tiles[i].w, (uint32_t) ( bottom - y ), pLcd + y * W + ui.tiles[i].x, W );
			}
	memset ( ui.changed, 0, sizeof ui.changed );
	ui.full = 0;
	ui.dirty = 0;
}
static int Hit ( int x, int y )
{
	if ( y < TOP || y >= TOP + VIEW )
		return -1;
	int content_y = y - TOP + ui.scroll;
	for ( int i = 0; i < ui.count; ++i )
		if ( x >= ui.tiles[i].x && x < ui.tiles[i].x + ui.tiles[i].w && content_y >= ui.tiles[i].y && content_y < ui.tiles[i].y + ui.tiles[i].h )
			return i;
	return -1;
}
static int ButtonHit ( int x, int y )
{
	int hit = Hit ( x, y );
	if ( hit < 0 )
		return -1;
	int lx = x - ui.tiles[hit].x, ly = y - TOP + ui.scroll - ui.tiles[hit].y;
	if ( ui.settings )
		return lx >= 394 && lx < 430 && ly >= 16 && ly < 55 ? hit : -1;
	unsigned mode = ui.data.ports[ui.tiles[hit].port - 1].mode;
	return ( mode == HTTP_MODE_RELAY || mode == HTTP_MODE_IR ) && lx >= 14 && lx < COL_W - 14 && ly >= 40 && ly < 83 ? hit : -1;
}
static int SettingsOutputHit ( int x, int y )
{
	if ( !ui.settings )
		return -1;
	int hit = Hit ( x, y );
	if ( hit < 0 )
		return -1;
	unsigned port = (unsigned) ui.tiles[hit].port, mode = ShownMode ( port );
	if ( mode != ui.data.ports[port - 1].mode || ( mode != HTTP_MODE_RELAY && mode != HTTP_MODE_IR ) )
		return -1;
	int lx = x - ui.tiles[hit].x, ly = y - TOP + ui.scroll - ui.tiles[hit].y;
	return lx >= 442 && lx < 674 && ly >= 16 && ly < 55 ? hit : -1;
}
static int ModeHit ( int x, int y )
{
	if ( !ui.settings )
		return -1;
	int hit = Hit ( x, y );
	if ( hit < 0 )
		return -1;
	int lx = x - ui.tiles[hit].x, ly = y - TOP + ui.scroll - ui.tiles[hit].y;
	return lx >= 224 && lx < 384 && ly >= 16 && ly < 52 ? hit : -1;
}
static void OpenMenu ( int index )
{
	ui.menuPort = ui.tiles[index].port;
	ui.menuCount = ModeOptions ( (unsigned) ui.menuPort, ui.menuModes );
	int y = TOP + ui.tiles[index].y - ui.scroll + 16;
	int h = ui.menuCount * MENU_ITEM + 4;
	int below = TOP + VIEW - ( y + 36 ), above = y - TOP;
	ui.menuUp = below < h && above > below;
	ui.menuY = Clamp ( ui.menuUp ? y - h : y + 36, TOP, TOP + VIEW - h );
	ui.menuX = ui.tiles[index].x + 224;
	ui.menuArmed = ui.menuPressed = -1;
	ui.full = ui.dirty = 1;
}
static int MenuHit ( int x, int y )
{
	int row = ( y - ui.menuY - 2 ) / MENU_ITEM;
	return x >= ui.menuX + 2 && x < ui.menuX + MENU_W - 2 && y >= ui.menuY + 2 && y < ui.menuY + 2 + ui.menuCount * MENU_ITEM ? row : -1;
}
static void MenuTouch ( int down, XY_Touch_Struct position )
{
	int hit = MenuHit ( position.x, position.y );
	if ( down && !ui.wasDown )
		ui.menuArmed = hit;
	int pressed = down && ui.menuArmed >= 0 && hit == ui.menuArmed ? hit : -1;
	if ( ui.menuPressed != pressed )
	{
		ui.menuPressed = pressed;
		ui.dirty = 1;
	}
	if ( !down && ui.wasDown )
	{
		if ( hit >= 0 && hit == ui.menuArmed )
		{
			unsigned port = (unsigned) ui.menuPort - 1;
			edit.draftBase[port] = ui.data.ports[port].mode;
			edit.draftMode[port] = ui.menuModes[hit] == ui.data.ports[port].mode ? -1 : (int8_t) ui.menuModes[hit];
			edit.saveError = 0;
			edit.saved = 0;
			Header ();
		}
		ui.menuPort = 0;
		ui.menuPressed = ui.menuArmed = -1;
		ui.full = ui.dirty = 1;
	}
	ui.wasDown = down;
}
static void Pwm ( int x )
{
	int left = 0;
	for ( int i = 0; i < ui.count; ++i )
		if ( ui.tiles[i].port == (int) ui.pwmPort )
			left = ui.tiles[i].x + ( ui.settings ? 522 : 18 );
	unsigned value = (unsigned) Clamp ( ( x - left ) * 100 / ( ui.settings ? 144 : 180 ), 0, 100 );
	if ( ui.pwmSent < 0 || value != ui.pwmValue )
	{
		ui.pwmValue = value;
		if ( HTTP_LCDPanelPwm ( ui.pwmPort, value ) )
		{
			ui.pwmSent = (int) value;
			ui.data.ports[ui.pwmPort - 1].pwm = (uint8_t) value;
			ui.data.ports[ui.pwmPort - 1].pwm_valid = 1;
		}
		ui.changed[ui.pwmPort - 1] = 1;
		ui.dirty = 1;
	}
}
int LCD_HttpPanel ( uint8_t mode, int settings )
{
	if ( mode == LCD_TEMP_EXIT )
	{
		ui.active = 0;
		ui.capture = 0;
		HTTP_LCDPanelActive ( 0 );
		return 0;
	}
	if ( mode > LCD_TEMP_EXIT || settings < 0 || settings > 2 )
		return -1;
	if ( LCD_GetXSize () != W || LCD_GetYSize () != H )
		return -2;
	if ( mode == LCD_TEMP_ENTER || !ui.active || ui.settings != settings )
	{
		SCREEN_ResetAllParameters ();
		LCD_TOUCH_DeleteAllSetTouch ();
		memset ( &ui, 0, sizeof ui );
		ui.settings = settings;
		if ( !edit.initialized )
		{
			memset ( edit.draftMode, -1, sizeof edit.draftMode );
			edit.initialized = 1;
		}
		ui.modePressed = -1;
		ui.outputPressed = -1;
		_Static_assert ( fontID_13 + 4 <= fontID_17, "HTTP panels exceed available font IDs" );
		const int sizes[5] = { FONT_12, FONT_16, FONT_20, FONT_18, FONT_28 };
		for ( int i = 0; i < 5; ++i )
		{
			ui.font[i] = LCD_LoadFont_ChangeColor ( sizes[i], Arial, fontID_13 + i );
			if ( ui.font[i] < 0 )
				return -3;
		}
		HTTP_LCDPanelActive ( 1 );
		for ( unsigned i = 0; i < HTTP_LCD_PORTS; ++i )
			ui.data.ports[i].mode = HTTP_MODE_DISABLED;
		(void) HTTP_LCDPanelRead ( &ui.data );
		SaveCompleted ( &ui.data );
		Layout ();
		ui.active = 1;
		ui.wasDown = LCD_TOUCH_isPress () == press;
		ui.armed = -1;
		ui.pressed = -1;
		ui.sample = HAL_GetTick ();
		Header ();
		ui.full = 1;
		ui.dirty = 1;
	}
	uint32_t now = HAL_GetTick ();
	if ( (uint32_t) ( now - ui.sample ) >= 250U )
	{
		ui.sample = now;
		if ( HTTP_LCDPanelRead ( &ui.latest ) && memcmp ( &ui.data, &ui.latest, sizeof ui.data ) )
		{
			int ports_changed = memcmp ( ui.data.ports, ui.latest.ports, sizeof ui.data.ports );
			if ( ui.settings == 2 && memcmp ( ui.data.thermometers, ui.latest.thermometers, sizeof ui.data.thermometers ) )
				ui.dirty = 1;
			for ( unsigned port = 0; port < HTTP_LCD_PORTS; ++port )
			{
				if ( ui.data.ports[port].mode != ui.latest.ports[port].mode )
					ui.full = 1;
				if ( memcmp ( &ui.data.ports[port], &ui.latest.ports[port], sizeof ui.data.ports[port] ) )
					ui.changed[port] = 1;
			}
			SaveCompleted ( &ui.latest );
			ui.data = ui.latest;
			if ( ports_changed )
			{
				Layout ();
				ui.dirty = 1;
			}
			Header ();
		}
	}
	XY_Touch_Struct position = LCD_TOUCH_GetPos ();
	(void) LCD_TOUCH_GetTypeAndPosition ( &screenTouchPos );
	int down = LCD_TOUCH_isPress () == press;
	if ( ui.settings == 2 )
	{
		NowaTouch ( down, position );
		return 0;
	}
	if ( ui.menuPort )
	{
		MenuTouch ( down, position );
		if ( ui.dirty )
			Draw ();
		return 0;
	}
	int save_hit = ui.settings && position.x >= 538 && position.x < 712 && position.y >= 14 && position.y < 48;
	if ( ( down && !ui.wasDown && save_hit ) || ui.capture == 6 )
	{
		if ( down && !ui.wasDown )
			ui.capture = 6;
		int pressed = down && save_hit && !edit.saving;
		if ( ui.savePressed != pressed )
		{
			ui.savePressed = pressed;
			Header ();
		}
		if ( !down && ui.wasDown )
		{
			if ( save_hit && !edit.saving )
			{
				uint8_t changed[HTTP_LCD_PORTS], modes[HTTP_LCD_PORTS];
				unsigned count = 0;
				for ( unsigned i = 0; i < HTTP_LCD_PORTS; ++i )
				{
					changed[i] = edit.draftMode[i] >= 0;
					modes[i] = changed[i] ? (uint8_t) edit.draftMode[i] : ui.data.ports[i].mode;
					count += changed[i];
				}
				if ( count )
				{
					edit.saving = HTTP_LCDPanelSaveModes ( changed, modes, edit.draftBase );
					edit.saveError = !edit.saving;
					edit.saved = 0;
				}
				Header ();
			}
			ui.capture = 0;
		}
		ui.wasDown = down;
		if ( ui.dirty )
			Draw ();
		return 0;
	}
	int hit = Hit ( position.x, position.y );
	if ( down && !ui.wasDown )
	{
		ui.startY = position.y;
		ui.startScroll = ui.scroll;
		ui.armed = ButtonHit ( position.x, position.y );
		ui.moved = 0;
		ui.capture = position.y >= TOP && position.y < TOP + VIEW ? 1 : 0;
		if ( position.x >= SCROLL_X && position.x < SCROLL_X + SCROLL_W && position.y >= TOP && position.y < TOP + VIEW )
		{
			ui.capture = 2;
			int thumb_y, thumb_h;
			Thumb ( &thumb_y, &thumb_h );
			if ( (int) position.y - TOP < thumb_y || (int) position.y - TOP >= thumb_y + thumb_h )
			{
				ui.scroll = Clamp ( ( (int) position.y - TOP - thumb_h / 2 ) * Maximum () / ( VIEW - thumb_h ? VIEW - thumb_h : 1 ), 0, Maximum () );
				ui.startScroll = ui.scroll;
				ui.full = 1;
				ui.dirty = 1;
			}
		}
		else if ( SettingsOutputHit ( position.x, position.y ) >= 0 )
		{
			ui.capture = 5;
			ui.armed = SettingsOutputHit ( position.x, position.y );
		}
		else if ( !edit.saving && ModeHit ( position.x, position.y ) >= 0 )
		{
			ui.capture = 4;
			ui.armed = ModeHit ( position.x, position.y );
		}
		else if ( hit >= 0 && ui.data.ports[ui.tiles[hit].port - 1].mode == HTTP_MODE_PWM && ( !ui.settings || ShownMode ( (unsigned) ui.tiles[hit].port ) == HTTP_MODE_PWM ) )
		{
			int local_y = position.y - TOP + ui.scroll - ui.tiles[hit].y;
			if ( ui.settings ? local_y >= 16 && local_y < 55 && position.x >= ui.tiles[hit].x + 514 && position.x < ui.tiles[hit].x + 674 : local_y >= 72 && local_y < 113 )
			{
				ui.capture = 3;
				ui.pwmPort = (unsigned) ui.tiles[hit].port;
				ui.pwmSent = -1;
				Pwm ( position.x );
			}
		}
	}
	if ( down && ui.capture == 3 )
		Pwm ( position.x );
	else if ( down && ui.capture )
	{
		int delta = (int) position.y - ui.startY;
		if ( delta > 6 || delta < -6 )
			ui.moved = 1;
		int next = ui.startScroll;
		if ( ui.capture == 2 )
		{
			int thumb_y, thumb_h;
			Thumb ( &thumb_y, &thumb_h );
			(void) thumb_y;
			next += delta * Maximum () / ( VIEW - thumb_h ? VIEW - thumb_h : 1 );
		}
		else if ( ui.moved )
			next -= delta;
		next = Clamp ( next, 0, Maximum () );
		if ( next != ui.scroll )
		{
			ui.scroll = next;
			ui.full = 1;
			ui.dirty = 1;
		}
	}
	int output_hit = SettingsOutputHit ( position.x, position.y );
	int output_pressed = down && !ui.moved && ui.capture == 5 && output_hit == ui.armed && output_hit >= 0 ? ui.tiles[output_hit].port : -1;
	if ( ui.outputPressed != output_pressed )
	{
		if ( ui.outputPressed >= 1 )
			ui.changed[ui.outputPressed - 1] = 1;
		if ( output_pressed >= 1 )
			ui.changed[output_pressed - 1] = 1;
		ui.outputPressed = output_pressed;
		ui.dirty = 1;
	}
	int mode_hit = ModeHit ( position.x, position.y );
	int mode_pressed = down && !ui.moved && ui.capture == 4 && mode_hit == ui.armed && mode_hit >= 0 ? ui.tiles[mode_hit].port : -1;
	if ( ui.modePressed != mode_pressed )
	{
		if ( ui.modePressed >= 1 )
			ui.changed[ui.modePressed - 1] = 1;
		if ( mode_pressed >= 1 )
			ui.changed[mode_pressed - 1] = 1;
		ui.modePressed = mode_pressed;
		ui.dirty = 1;
	}
	int button = ButtonHit ( position.x, position.y );
	int pressed = down && !ui.moved && ui.capture == 1 && button == ui.armed && button >= 0 ? ui.tiles[button].port : -1;
	if ( ui.pressed != pressed )
	{
		if ( ui.pressed >= 1 )
			ui.changed[ui.pressed - 1] = 1;
		if ( pressed >= 1 )
			ui.changed[pressed - 1] = 1;
		ui.pressed = pressed;
		ui.dirty = 1;
	}
	if ( !down && ui.wasDown )
	{
		if ( ui.capture == 5 && !ui.moved && output_hit >= 0 && output_hit == ui.armed )
			(void) HTTP_LCDPanelToggle ( (unsigned) ui.tiles[output_hit].port );
		if ( ui.capture == 4 && !ui.moved && mode_hit >= 0 && mode_hit == ui.armed )
			OpenMenu ( mode_hit );
		if ( !ui.settings && ui.capture == 1 && !ui.moved && button == ui.armed && button >= 0 )
		{
			unsigned port = (unsigned) ui.tiles[button].port;
			unsigned type = ui.data.ports[port - 1].mode;
			if ( type == HTTP_MODE_RELAY || type == HTTP_MODE_IR )
				(void) HTTP_LCDPanelToggle ( port );
		}
		ui.capture = 0;
		ui.armed = -1;
	}
	ui.wasDown = down;
	if ( ui.dirty )
		Draw ();
	return 0;
}
