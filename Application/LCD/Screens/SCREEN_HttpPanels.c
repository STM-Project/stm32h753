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

_Static_assert ( SCRATCH + ROW_W * 72 <= W * H, "LCD tile scratch exceeds pLcd" );
_Static_assert ( SCRATCH + COL_W * 210 <= W * H, "LCD read scratch exceeds pLcd" );
static struct
{
	int active, settings, font[3], scroll, extent, count, wasDown, capture, startY, startScroll, armed, pressed, moved, dirty, full;
	uint8_t changed[HTTP_LCD_PORTS];
	unsigned pwmPort, pwmValue;
	int pwmSent;
	uint32_t sample;
	struct
	{
		int port, x, y, w, h;
	} tiles[HTTP_LCD_PORTS];
	HTTP_LCDPanelData data, latest;
} ui;

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
			int dx = col < radius ? radius - col - 1 : col >= w - radius ? col - ( w - radius )
																		 : 0;
			int dy = row < radius ? radius - row - 1 : row >= h - radius ? row - ( h - radius )
																		 : 0;
			if ( dx * dx + dy * dy <= radius * radius )
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
static void Tile ( int index )
{
	int port = ui.tiles[index].port;
	const HTTP_LCDPort *p = &ui.data.ports[port - 1];
	int w = ui.tiles[index].w, h = ui.tiles[index].h;
	Rect ( w, 0, 0, w, h, BG );
	TileRound ( w, 0, 0, w, h, 18, FRAME, FRAME );
	TileRound ( w, 5, 5, w - 10, h - 10, 14, CARD, CARD );
	char line[80];
	if ( ui.settings )
	{
		snprintf ( line, sizeof line, "%02d", port );
		TileRound ( w, 12, 15, 38, 38, 5, 0xFFBBBBBBU, 0xFF999999U );
		Text ( w, h, ui.font[1], 17, 21, 30, line, 0xFF111111U );
		Text ( w, h, ui.font[1], 62, 10, 252, p->name, WHITE );
		Text ( w, h, ui.font[0], 62, 36, 252, "Nazwa portu", 0xFFDDDDDDU );
		TileRound ( w, 328, 16, 176, 36, 4, 0xFFBBBBBBU, 0xFF999999U );
		Text ( w, h, ui.font[0], 338, 25, 156, ModeName ( p->mode ), 0xFF111111U );
		uint32_t color = ui.pressed == port ? 0xFFCAFF4CU : GREEN;
		TileRound ( w, 520, ui.pressed == port ? 19 : 16, 154, 36, 5, color, ui.pressed == port ? 0xFF81BA04U : 0xFF8ABB0CU );
		Text ( w, h, ui.font[1], 532, ui.pressed == port ? 26 : 23, 130, "Ustawienia", 0xFF172000U );
	}
	else
	{
		snprintf ( line, sizeof line, "%s  %02d", ModeName ( p->mode ), port );
		Text ( w, h, ui.font[0], 14, 12, w - 28, line, MUTED );
		if ( p->mode == HTTP_MODE_COUNTER )
		{
			static const char *labels[4] = { "godzinny", "dobowy", "tygodniowy", "miesieczny" };
			Text ( w, h, ui.font[1], 14, 38, w - 28, p->value[0], p->color[0] );
			for ( int row = 1; row < 5; ++row )
			{
				Text ( w, h, ui.font[0], 14, 39 + row * 27, 118, p->value[row], p->color[row] );
				Text ( w, h, ui.font[0], 137, 39 + row * 27, 65, labels[row - 1], MUTED );
			}
			Text ( w, h, ui.font[0], 14, 185, w - 28, p->name, GREEN );
		}
		else if ( p->mode == HTTP_MODE_PWM )
		{
			unsigned value = ui.capture == 3 && ui.pwmPort == (unsigned) port ? ui.pwmValue : p->pwm;
			snprintf ( line, sizeof line, "%u %%", value );
			Text ( w, h, ui.font[2], 14, 38, w - 28, line, WHITE );
			TileRound ( w, 18, 88, 180, 12, 5, 0xFF444444U, 0xFF333333U );
			if ( value )
				TileRound ( w, 18, 88, (int) value * 180 / 100, 12, 4, GREEN, 0xFF88B80CU );
			int knob = 18 + (int) value * 180 / 100;
			TileRound ( w, knob - 8, 81, 16, 26, 6, WHITE, 0xFFBBBBBBU );
			CenterText ( w, h, ui.font[1], 121, p->name, GREEN );
		}
		else
		{
			if ( p->mode == HTTP_MODE_RELAY || p->mode == HTTP_MODE_IR )
				TileRound ( w, 14, 40, w - 28, 43, 5, ui.pressed == port ? 0xFF7BB6EBU : 0xFF468CCFU, 0xFF3579B9U );
			CenterText ( w, h, ui.font[2], 42, p->value[0], p->mode == HTTP_MODE_RELAY || p->mode == HTTP_MODE_IR ? 0xFF111111U : p->color[0] );
			if ( p->mode == HTTP_MODE_TEMP_RHU )
				Text ( w, h, ui.font[1], 18, 85, w - 36, p->value[1], p->color[1] );
			CenterText ( w, h, ui.font[1], 122, p->name, GREEN );
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
	Rect ( W, 0, 0, W, TOP, BG );
	TileRound ( W, 36, 8, 690, 46, 7, FRAME, 0xFF333333U );
	Text ( W, 64, ui.font[1], 52, 20, 480, ui.settings ? "H7 . Ustawienia portow /set" : "H7 . Panel odczytowy /main", WHITE );
	Text ( W, 64, ui.font[0], 602, 24, 108, ui.data.time, GREEN );
	LCD_DisplayBuff ( 0, 0, W, TOP, pLcd + SCRATCH );
	Rect ( W, 0, 0, W, 40, BG );
	Text ( W, 40, ui.font[0], 36, 12, 690, ui.settings ? "Nazwa portu                         Tryb                         Ustawienia" : "Pomiar i stany HTTP . przewijaj palcem lub suwakiem", MUTED );
	LCD_DisplayBuff ( 0, 440, W, 40, pLcd + SCRATCH );
}
static void Draw ( void )
{
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
		return lx >= 520 && lx < 674 && ly >= 16 && ly < 55 ? hit : -1;
	unsigned mode = ui.data.ports[ui.tiles[hit].port - 1].mode;
	return ( mode == HTTP_MODE_RELAY || mode == HTTP_MODE_IR ) && lx >= 14 && lx < COL_W - 14 && ly >= 40 && ly < 83 ? hit : -1;
}
static void Pwm ( int x )
{
	int left = 0;
	for ( int i = 0; i < ui.count; ++i )
		if ( ui.tiles[i].port == (int) ui.pwmPort )
			left = ui.tiles[i].x + 18;
	unsigned value = (unsigned) Clamp ( ( x - left ) * 100 / 180, 0, 100 );
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
	if ( mode > LCD_TEMP_EXIT || settings < 0 || settings > 1 )
		return -1;
	if ( LCD_GetXSize () != W || LCD_GetYSize () != H )
		return -2;
	if ( mode == LCD_TEMP_ENTER || !ui.active || ui.settings != settings )
	{
		SCREEN_ResetAllParameters ();
		LCD_TOUCH_DeleteAllSetTouch ();
		memset ( &ui, 0, sizeof ui );
		ui.settings = settings;
		const int sizes[3] = { FONT_12, FONT_16, FONT_28_bold };
		for ( int i = 0; i < 3; ++i )
		{
			ui.font[i] = LCD_LoadFont_ChangeColor ( sizes[i], Arial, fontID_14 + i );
			if ( ui.font[i] < 0 )
				return -3;
		}
		HTTP_LCDPanelActive ( 1 );
		for ( unsigned i = 0; i < HTTP_LCD_PORTS; ++i )
			ui.data.ports[i].mode = HTTP_MODE_DISABLED;
		(void) HTTP_LCDPanelRead ( &ui.data );
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
			for ( unsigned port = 0; port < HTTP_LCD_PORTS; ++port )
			{
				if ( ui.data.ports[port].mode != ui.latest.ports[port].mode )
					ui.full = 1;
				if ( memcmp ( &ui.data.ports[port], &ui.latest.ports[port], sizeof ui.data.ports[port] ) )
					ui.changed[port] = 1;
			}
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
		else if ( !ui.settings && hit >= 0 && ui.data.ports[ui.tiles[hit].port - 1].mode == HTTP_MODE_PWM )
		{
			int local_y = position.y - TOP + ui.scroll - ui.tiles[hit].y;
			if ( local_y >= 72 && local_y < 113 )
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
