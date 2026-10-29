/* Procedural LCD /nowa thermometer: no stored frames or bitmap caches. */
#include <math.h>
#define THERMO_W 112
#define THERMO_H 280
static float ThermometerClamp ( float value )
{
	return value < 0 ? 0 : value > 1 ? 1
									 : value;
}
static uint32_t ThermometerBlend ( uint32_t background, uint32_t foreground, float alpha )
{
	unsigned a = (unsigned) ( ThermometerClamp ( alpha ) * 255 + 0.5f );
	uint32_t color = 0xFF000000U;
	for ( unsigned shift = 0; shift <= 16; shift += 8 )
		color |= ( ( ( ( background >> shift ) & 255U ) * ( 255 - a ) + ( ( foreground >> shift ) & 255U ) * a + 127 ) / 255 ) << shift;
	return color;
}
/* The same blue -> violet -> red ramp as HTTP panel.js. */
static uint32_t ThermometerLiquid ( unsigned permille )
{
	if ( permille > 1000 )
		permille = 1000;
	unsigned red = ( 255 * permille + 500 ) / 1000;
	unsigned green = ( 128 * ( 1000 - permille ) + 500 ) / 1000;
	unsigned blue = ( 255 * ( 1000 - permille ) + 500 ) / 1000;
	return 0xFF000000U | red << 16 | green << 8 | blue;
}
/* Geometry follows the original SVG at 112x280. Coverage, radial highlights
 * and glass layers are evaluated directly into the existing tile scratch. */
static void ThermometerDraw ( int stride, int left, int top, unsigned permille, int valid )
{
	if ( permille > 1000 )
		permille = 1000;
	uint32_t liquid = ThermometerLiquid ( permille );
	float level_y = 185.2f - 149.8f * permille / 1000;
	for ( int y = 0; y < THERMO_H; ++y )
		for ( int x = 0; x < THERMO_W; ++x )
		{
			float fx = x + 0.5f, fy = y + 0.5f, dx = fx - 56;
			uint32_t color = 0xFFFFFFFFU;
			/* Rounded glass stem: radius 16, top 5, joins the bowl. */
			if ( fx >= 39 && fx <= 73 && fy >= 4 && fy <= 237 )
			{
				float dy = fy < 21 ? fy - 21 : fy > 216 ? fy - 216
														: 0;
				float distance = sqrtf ( dx * dx + dy * dy );
				float coverage = ThermometerClamp ( 16.5f - distance );
				color = ThermometerBlend ( color, 0xFFE3F8FCU, coverage * 0.6f );
				if ( fy >= level_y )
					color = ThermometerBlend ( color, liquid, coverage * 0.75f );
				float cap_y = ( fy - level_y ) / 4.8f;
				if ( dx * dx / 256 + cap_y * cap_y < 1 )
					color = ThermometerBlend ( color, 0xFF111820U, coverage * 0.82f );
				float stripe = ThermometerClamp ( 1 - fabsf ( fx - 51 ) / 7 );
				if ( fy > 16 && fy < 215 )
					color = ThermometerBlend ( color, 0xFFFFFFFFU, stripe * 0.36f * coverage );
				float rim = coverage * ThermometerClamp ( distance - 14.5f );
				color = ThermometerBlend ( color, 0xFF92B6BFU, rim * 0.6f );
			}
			/* Liquid bulb with radial light, darkened edge and glass glints. */
			float by = fy - 235.1f;
			if ( dx * dx + by * by < 42 * 42 )
			{
				float distance = sqrtf ( dx * dx + by * by );
				float coverage = ThermometerClamp ( 40.5f - distance );
				uint32_t bulb = ThermometerBlend ( 0xFFFFFFFFU, liquid, 0.75f );
				float radial = ThermometerClamp ( 1 - ( ( dx + 12 ) * ( dx + 12 ) + ( by + 15 ) * ( by + 15 ) ) / 1900 );
				bulb = ThermometerBlend ( bulb, 0xFFFFFFFFU, radial * 0.42f );
				float shadow = ThermometerClamp ( ( distance - 26 ) / 14 );
				bulb = ThermometerBlend ( bulb, liquid, shadow * 0.45f );
				float glint = ThermometerClamp ( 1 - ( ( dx + 15 ) * ( dx + 15 ) / 145 + ( by + 20 ) * ( by + 20 ) / 75 ) );
				bulb = ThermometerBlend ( bulb, 0xFFFFFFFFU, glint * 0.52f );
				float rim = ThermometerClamp ( distance - 38 );
				bulb = ThermometerBlend ( bulb, liquid, rim * 0.5f );
				color = ThermometerBlend ( color, bulb, coverage );
			}
			/* Scale marks use the SVG's faint grey and original spacing. */
			for ( int mark = 0; mark < 9; ++mark )
			{
				int tick_y = 34 + mark * 18, length = mark % 4 == 0 ? 32 : 18;
				if ( y >= tick_y && y < tick_y + 4 && x >= 79 && x < 79 + length )
					color = 0xFFDDDDDDU;
			}
			if ( !valid )
			{
				unsigned grey = ( ( ( color >> 16 ) & 255 ) + ( ( color >> 8 ) & 255 ) + ( color & 255 ) ) / 3;
				grey = ( grey + 510 ) / 3;
				color = 0xFF000000U | grey * 0x010101U;
			}
			pLcd[SCRATCH + ( top + y ) * stride + left + x] = color;
		}
}
static void NowaHeader ( void )
{
	Rect ( W, 0, 0, W, TOP, 0xFFFFFFFFU );
	Text ( W, TOP, ui.font[1], 36, 20, 510, "LPC /nowa . Termometry", VALUE_BLACK );
	Text ( W, TOP, ui.font[0], 610, 24, 120, ui.data.time, 0xFF555555U );
	LCD_DisplayBuff ( 0, 0, W, TOP, pLcd + SCRATCH );
	Rect ( W, 0, 0, W, 40, 0xFFFFFFFFU );
	Text ( W, 40, ui.font[0], 36, 12, 690, "Pomiary i przekazniki wspolne z HTTP /nowa", 0xFF555555U );
	LCD_DisplayBuff ( 0, 440, W, 40, pLcd + SCRATCH );
}
static void ThermometerValue ( char *out, size_t size, int tenths )
{
	unsigned absolute = (unsigned) ( tenths < 0 ? -tenths : tenths );
	snprintf ( out, size, "%s%u.%u C", tenths < 0 ? "-" : "", absolute / 10, absolute % 10 );
}
static void NowaDraw ( void )
{
	for ( unsigned i = 0; i < W * VIEW; ++i )
		pLcd[i] = 0xFFFFFFFFU;
	for ( unsigned i = 0; i < 3; ++i )
	{
		const HTTP_LpcThermometer *t = &ui.data.thermometers[i];
		int lo = t->minimum, hi = t->maximum;
		if ( hi <= lo )
		{
			lo = -100;
			hi = 500;
		}
		unsigned permille = t->valid ? (unsigned) Clamp ( ( t->tenths - lo ) * 1000 / ( hi - lo ), 0, 1000 ) : 0;
		const int stride = 236, height = 310, origin = 36 + (int) i * 244;
		Rect ( stride, 0, 0, stride, height, 0xFFFFFFFFU );
		ThermometerDraw ( stride, 8, 28, permille, t->valid );
		Text ( stride, height, ui.font[1], 4, 0, 228, t->name, VALUE_BLACK );
		char text[40];
		ThermometerValue ( text, sizeof text, hi );
		Measurement ( stride, height, 128, 38, 104, text, 0xFF555555U, ui.font[0], 0 );
		if ( t->valid )
			ThermometerValue ( text, sizeof text, t->tenths );
		else
			snprintf ( text, sizeof text, "---" );
		Measurement ( stride, height, 124, 130, 108, text, VALUE_BLACK, ui.font[2], 0 );
		ThermometerValue ( text, sizeof text, lo );
		Measurement ( stride, height, 128, 263, 104, text, 0xFF555555U, ui.font[0], 0 );
		for ( int y = 0; y < height; ++y )
			memcpy ( pLcd + y * W + origin, pLcd + SCRATCH + y * stride, stride * sizeof *pLcd );
	}
	for ( unsigned i = 0; i < 4; ++i )
	{
		const HTTP_LCDPort *p = &ui.data.ports[27 + i];
		int enabled = p->mode == HTTP_MODE_RELAY || p->mode == HTTP_MODE_IR;
		int pressed = ui.pressed == (int) i + 28;
		Rect ( 176, 0, 0, 176, 50, 0xFFFFFFFFU );
		TileRound ( 176, 0, 0, 176, 48, 5, !enabled ? 0xFFDDDDDDU : pressed ? 0xFF87B8E3U
																			: 0xFF468CCFU,
					!enabled ? 0xFFDDDDDDU : pressed ? 0xFF87B8E3U
													 : 0xFF3579B9U );
		Text ( 176, 50, ui.font[0], 8, 4, 160, p->name, VALUE_BLACK );
		Text ( 176, 50, ui.font[1], 8, 23, 160, p->value[0], enabled ? RelayTextColor ( p->value[0] ) : 0xFF666666U );
		for ( unsigned y = 0; y < 50; ++y )
			memcpy ( pLcd + ( y + 318 ) * W + 36 + i * 184, pLcd + SCRATCH + y * 176, 176 * sizeof *pLcd );
	}
	LCD_DisplayBuff ( 0, TOP, W, VIEW, pLcd );
	ui.full = ui.dirty = 0;
}
static void NowaTouch ( int down, XY_Touch_Struct position )
{
	int hit = -1;
	if ( position.y >= TOP + 318 && position.y < TOP + 366 )
		for ( int i = 0; i < 4; ++i )
			if ( position.x >= 36 + i * 184 && position.x < 212 + i * 184 &&
				 ( ui.data.ports[27 + i].mode == HTTP_MODE_RELAY || ui.data.ports[27 + i].mode == HTTP_MODE_IR ) )
				hit = 28 + i;
	if ( down && !ui.wasDown )
		ui.armed = hit;
	int pressed = down && hit >= 0 && hit == ui.armed ? hit : -1;
	if ( pressed != ui.pressed )
	{
		ui.pressed = pressed;
		ui.dirty = 1;
	}
	if ( !down && ui.wasDown )
	{
		if ( hit >= 0 && hit == ui.armed )
			(void) HTTP_LCDPanelToggle ( (unsigned) hit );
		ui.armed = -1;
	}
	ui.wasDown = down;
	if ( ui.dirty || ui.full )
		NowaDraw ();
}
