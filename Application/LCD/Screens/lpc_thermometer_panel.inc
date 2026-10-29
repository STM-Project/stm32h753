/* LCD /nowa: original HTTP SVG frames, shared live snapshot and relay mailbox. */
#include "lpc_thermometer_frames.inc"
#define THERMO_W 112
#define THERMO_H 280
#define THERMO_BYTES ( THERMO_W * THERMO_H * 2 )
static uint8_t thermometerCache[3][THERMO_BYTES] __attribute__ ( ( section ( ".sdram" ), aligned ( 32 ) ) );
static int thermometerLevel[3] = { -1, -1, -1 };
_Static_assert ( THERMO_BYTES <= ( W * H - SCRATCH ) * 4, "Thermometer decode scratch overflow" );
/* Bounded LZ4 block reader. Generated assets only; no heap or external decoder. */
static int ThermometerBlock ( unsigned level, uint8_t *out )
{
	const uint8_t *p = thermoFrames + thermoOffsets[level], *end = thermoFrames + thermoOffsets[level + 1];
	size_t used = 0;
	while ( p < end )
	{
		unsigned token = *p++;
		size_t literal = token >> 4;
		if ( literal == 15 )
		{
			unsigned n;
			do
			{
				if ( p == end )
					return 0;
				n = *p++;
				literal += n;
			} while ( n == 255 );
		}
		if ( literal > (size_t) ( end - p ) || literal > THERMO_BYTES - used )
			return 0;
		memcpy ( out + used, p, literal );
		used += literal;
		p += literal;
		if ( p == end )
			break;
		if ( end - p < 2 )
			return 0;
		unsigned offset = p[0] | (unsigned) p[1] << 8;
		p += 2;
		if ( !offset || offset > used )
			return 0;
		size_t match = ( token & 15 ) + 4;
		if ( ( token & 15 ) == 15 )
		{
			unsigned n;
			do
			{
				if ( p == end )
					return 0;
				n = *p++;
				match += n;
			} while ( n == 255 );
		}
		if ( match > THERMO_BYTES - used )
			return 0;
		while ( match-- )
		{
			out[used] = out[used - offset];
			++used;
		}
	}
	return used == THERMO_BYTES;
}
static int ThermometerImage ( unsigned slot, unsigned level )
{
	if ( slot >= 3 || level > 100 )
		return 0;
	uint8_t *delta = (uint8_t *) ( pLcd + SCRATCH );
	int current = thermometerLevel[slot];
	if ( current < 0 || level < (unsigned) current || level - (unsigned) current > 10 )
	{
		unsigned key = level / 10 * 10;
		if ( !ThermometerBlock ( key, thermometerCache[slot] ) )
			return 0;
		current = (int) key;
	}
	while ( current < (int) level )
	{
		++current;
		if ( !ThermometerBlock ( (unsigned) current, delta ) )
			return 0;
		if ( current % 10 == 0 )
			memcpy ( thermometerCache[slot], delta, THERMO_BYTES );
		else
			for ( unsigned i = 0; i < THERMO_BYTES; ++i )
				thermometerCache[slot][i] += delta[i];
	}
	thermometerLevel[slot] = current;
	return 1;
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
		unsigned level = t->valid ? (unsigned) Clamp ( ( t->tenths - lo ) * 100 / ( hi - lo ), 0, 100 ) : 0;
		int loaded = ThermometerImage ( i, level );
		const int stride = 236, height = 310, origin = 36 + (int) i * 244;
		Rect ( stride, 0, 0, stride, height, 0xFFFFFFFFU );
		if ( loaded )
			for ( unsigned y = 0; y < THERMO_H; ++y )
				for ( unsigned x = 0; x < THERMO_W; ++x )
				{
					unsigned n = 2 * ( y * THERMO_W + x );
					uint16_t c = (uint16_t) ( thermometerCache[i][n] | thermometerCache[i][n + 1] << 8 );
					unsigned r = c >> 11, g = ( c >> 5 ) & 63, b = c & 31;
					uint32_t rgb = 0xFF000000U | ( ( r * 255 / 31 ) << 16 ) | ( ( g * 255 / 63 ) << 8 ) | b * 255 / 31;
					if ( !t->valid )
					{
						unsigned grey = ( ( ( rgb >> 16 ) & 255 ) + ( ( rgb >> 8 ) & 255 ) + ( rgb & 255 ) ) / 3;
						grey = ( grey + 510 ) / 3;
						rgb = 0xFF000000U | grey * 0x010101U;
					}
					pLcd[SCRATCH + ( y + 28 ) * stride + x + 8] = rgb;
				}
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
