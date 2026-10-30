#include "http_lcd_forms.h"
#if defined( __arm__ ) || defined( __thumb__ )
#define LCD_FORM_SDRAM __attribute__ ( ( section ( ".sdram" ) ) )
#else
#define LCD_FORM_SDRAM
#endif
static HTTP_LCDFormData lcdFormBase LCD_FORM_SDRAM, lcdFormDraft LCD_FORM_SDRAM, lcdFormLatest LCD_FORM_SDRAM;
static struct
{
	int active, dirty, pending, scroll, startScroll, startY, wasDown, armed, moved, drag, editing, selection, upper, polish, keyPressed;
	uint32_t serial, sample;
	char text[HTTP_LCD_FORM_VALUE], notice[96];
} lcdForm;
static int FormLoading ( void )
{
	return lcdFormBase.serial != lcdForm.serial;
}
/* Section cards follow the HTTP forms, with two columns on an 800x480 LCD.
 * Geometry is calculated from the schema; no bitmap or framebuffer is added. */
typedef struct
{
	int x, y, w, h;
	const char *title;
} FormCard;
typedef struct
{
	int x, y, w, h;
} FormControl;
static int FormRouteIs ( const char *route )
{
	return lcdFormDraft.page && !strcmp ( lcdFormDraft.page->route, route );
}
static int FormCardAt ( unsigned index, FormCard *card )
{
	if ( !lcdFormDraft.page )
		return 0;
	*card = (FormCard) { 36, 8, 690, 132, "Nazwa portu" };
	if ( lcdFormDraft.names_only )
	{
		if ( !index )
			return 1;
		if ( index == 1 && lcdFormDraft.mode == HTTP_MODE_PWM )
		{
			*card = (FormCard) { 36, 156, 690, 140, "Sterowanie PWM" };
			return 1;
		}
		return 0;
	}
	if ( FormRouteIs ( "/wypk" ) )
	{
		if ( !index )
			return 1;
		if ( index > 2 )
			return 0;
		*card = (FormCard) { index == 1 ? 36 : 388, 156, 338, 164, index == 1 ? "Stan ON" : "Stan OFF" };
		return 1;
	}
	if ( FormRouteIs ( "/term" ) || FormRouteIs ( "/we" ) || FormRouteIs ( "/eol" ) )
	{
		if ( index > 1 )
			return 0;
		*card = (FormCard) { index ? 388 : 36, 8, 338, 350, FormRouteIs ( "/term" ) ? ( index ? "Nazwy i stany wyjscia" : "Termostat / higrostat" ) : ( index ? "Reakcja i przekaznik" : "Nazwa i stany wejscia" ) };
		return 1;
	}
	if ( FormRouteIs ( "/tim" ) )
	{
		static const FormCard cards[] = {
			{ 36, 8, 690, 326, "Timer . godziny ON / OFF" },
			{ 36, 350, 690, 258, "Dni tygodnia i aktywacja" },
			{ 36, 624, 690, 236, "Nazwa i stany wyjscia" } };
		if ( index >= sizeof cards / sizeof *cards )
			return 0;
		*card = cards[index];
		return 1;
	}
	if ( FormRouteIs ( "/etr" ) )
	{
		static const char *periods[] = { "Godzina", "Doba", "Tydzien", "Miesiac" };
		if ( index > 4 )
			return 0;
		if ( !index )
			*card = (FormCard) { 36, 8, 690, 166, "Licznik impulsow . przelicznik" };
		else
			*card = (FormCard) { index % 2 ? 36 : 388, 190 + (int) ( ( index - 1 ) / 2 ) * 236, 338, 220, periods[index - 1] };
		return 1;
	}
	if ( FormRouteIs ( "/nap" ) )
	{
		if ( index > 1 )
			return 0;
		*card = index ? (FormCard) { 36, 6488, 690, 132, "Jednostka pomiaru" } : (FormCard) { 36, 8, 690, 6464, "Tabela translacji . 100 punktow" };
		return 1;
	}
	if ( index >= lcdFormDraft.count )
		return 0;
	*card = (FormCard) { index % 2 ? 388 : 36, 8 + (int) ( index / 2 ) * 148, 338, 132, "Ustawienia portu" };
	return 1;
}
static int FormControlAt ( unsigned field, FormControl *box )
{
	if ( !lcdFormDraft.page || field >= lcdFormDraft.count + ( lcdFormDraft.mode == HTTP_MODE_PWM ) )
		return 0;
	*box = (FormControl) { 54, 62, 654, 64 };
	if ( lcdFormDraft.names_only )
	{
		if ( field )
			box->y = 210;
	}
	else if ( FormRouteIs ( "/wypk" ) )
	{
		if ( field )
			*box = (FormControl) { field == 1 ? 54 : 406, 210, 302, 64 };
	}
	else if ( FormRouteIs ( "/term" ) )
	{
		if ( field >= 4 )
			*box = (FormControl) { 406, 62 + (int) ( field - 4 ) * 80, 302, 64 };
		else if ( field == 1 || field == 2 )
			*box = (FormControl) { field == 1 ? 54 : 213, 142, 143, 64 };
		else
			*box = (FormControl) { 54, field ? 222 : 62, 302, 64 };
	}
	else if ( FormRouteIs ( "/we" ) || FormRouteIs ( "/eol" ) )
	{
		unsigned names = FormRouteIs ( "/eol" ) ? 4 : 3;
		*box = (FormControl) { field < names ? 54 : 406, 62 + (int) ( field < names ? field : field - names ) * 64, 302, 64 };
	}
	else if ( FormRouteIs ( "/tim" ) )
	{
		if ( field < 8 )
			*box = (FormControl) { field < 4 ? 54 : 388, 62 + (int) ( field % 4 ) * 64, 320, 64 };
		else if ( field < 15 )
			*box = (FormControl) { 54 + (int) ( ( field - 8 ) % 3 ) * 224, 404 + (int) ( ( field - 8 ) / 3 ) * 64, 206, 64 };
		else if ( field == 15 )
			*box = (FormControl) { 278, 532, 430, 64 };
		else if ( field == 16 )
			*box = (FormControl) { 54, 678, 654, 64 };
		else
			*box = (FormControl) { field == 17 ? 54 : 388, 750, 320, 64 };
	}
	else if ( FormRouteIs ( "/etr" ) )
	{
		if ( field < 3 )
			*box = (FormControl) { 54 + (int) field * 224, 62, 206, 64 };
		else
		{
			unsigned period = ( field - 3 ) / 4, part = ( field - 3 ) % 4;
			*box = (FormControl) { ( period % 2 ? 406 : 54 ) + (int) ( part % 2 ) * 159, 244 + (int) ( period / 2 ) * 236 + (int) ( part / 2 ) * 80, 143, 64 };
		}
	}
	else if ( FormRouteIs ( "/nap" ) )
	{
		if ( field == 300 )
			box->y = 6542;
		else
		{
			static const int positions[] = { 54, 160, 438 }, widths[] = { 90, 260, 270 };
			*box = (FormControl) { positions[field % 3], 62 + (int) ( field / 3 ) * 64, widths[field % 3], 64 };
		}
	}
	else
		*box = (FormControl) { field % 2 ? 406 : 54, 62 + (int) ( field / 2 ) * 148, 302, 64 };
	return 1;
}
static int FormContentHeight ( void )
{
	int height = 0;
	FormCard card;
	for ( unsigned i = 0; FormCardAt ( i, &card ); ++i )
		if ( card.y + card.h + 8 > height )
			height = card.y + card.h + 8;
	return height;
}

static int FormMaximum ( void )
{
	int height = FormContentHeight () - VIEW;
	return height > 0 ? height : 0;
}
static const HTTP_SettingField *FormField ( unsigned index )
{
	return &lcdFormDraft.page->fields[HTTP_LCDFormFieldIndex ( &lcdFormDraft, index )];
}
static void FormLabel ( unsigned index, char *out, size_t size )
{
	const char *name = FormField ( index )->name;
	if ( !strcmp ( lcdFormDraft.page->route, "/nap" ) )
	{
		if ( index == 300 )
			snprintf ( out, size, "Jednostka pomiaru" );
		else
			snprintf ( out, size, "Punkt %u . %s", index / 3, index % 3 == 0 ? "Znak" : index % 3 == 1 ? "Calkowite"
																									   : "Setne" );
		return;
	}
	const char *label = NULL;
	if ( lcdFormDraft.names_only )
		label = "Nazwa portu";
	else if ( strstr ( name, "nazwa_zw" ) )
		label = "Opis stanu zwartego";
	else if ( strstr ( name, "nazwa_ro" ) )
		label = "Opis stanu rozwartego";
	else if ( strstr ( name, "nazwa_uszk" ) )
		label = "Opis uszkodzenia linii";
	else if ( strstr ( name, "nazwa_on" ) )
		label = "Opis stanu ON";
	else if ( strstr ( name, "nazwa_of" ) )
		label = "Opis stanu OFF";
	else if ( strstr ( name, "nazwa" ) )
		label = "Nazwa portu";
	else if ( strstr ( name, "czas_reak" ) )
		label = "Czas reakcji (mm:ss)";
	else if ( strstr ( name, "czas_przyt" ) )
		label = "Czas przytrzymania (mm:ss)";
	else if ( strstr ( name, "param1" ) )
		label = "Tryb wejscia";
	else if ( strstr ( name, "selautowe" ) )
		label = "Sterowane wyjscie";
	else if ( strstr ( name, "seltempwilg" ) )
		label = "Czujnik temperatury / wilgotnosci";
	else if ( strstr ( name, "Term_czas" ) )
		label = "Czas reakcji (mm:ss)";
	else if ( strstr ( name, "Term_prog" ) )
		label = "Prog termostatu";
	else if ( strstr ( name, "Term_hist" ) )
		label = "Histereza";
	else if ( strstr ( name, "Imp_licz" ) )
		label = "Liczba impulsow";
	else if ( strstr ( name, "Imp_wart" ) )
		label = "Wartosc odpowiadajaca impulsom";
	else if ( strstr ( name, "Imp_jedn" ) )
		label = "Jednostka licznika";
	if ( label )
		snprintf ( out, size, "%s", label );
	else if ( strstr ( name, "Tim_godzON" ) )
		snprintf ( out, size, "Timer ON %c (hh:mm:ss)", name[strlen ( "n_s_Tim_godzON" )] );
	else if ( strstr ( name, "Tim_godzOF" ) )
		snprintf ( out, size, "Timer OFF %c (hh:mm:ss)", name[strlen ( "n_s_Tim_godzOF" )] );
	else if ( strstr ( name, "Tim_akt" ) )
	{
		static const char *days[] = { "Poniedzialek", "Wtorek", "Sroda", "Czwartek", "Piatek", "Sobota", "Niedziela", "Aktywacja timera" };
		unsigned day = (unsigned) ( name[strlen ( "n_s_Tim_akt" )] - '1' );
		snprintf ( out, size, "%s", day < 8 ? days[day] : "Aktywacja timera" );
	}
	else if ( strstr ( name, "pEtr" ) )
	{
		const char *part = name + strlen ( "n_s_pEtr" );
		snprintf ( out, size, "Prog %c . %s", part[2], part[1] == 'd' ? "calkowite" : "setne" );
	}
	else
		snprintf ( out, size, "%s", name );
}
static unsigned FormOptionCount ( const HTTP_SettingField *field )
{
	if ( !field->choices[0] )
		return 0;
	unsigned count = 1;
	for ( const char *p = field->choices; *p; ++p )
		if ( *p == '|' )
			++count;
	return count;
}
static void FormOption ( const HTTP_SettingField *field, unsigned index, char *out, size_t size )
{
	const char *p = field->choices;
	while ( index-- && *p )
	{
		while ( *p && *p != '|' )
			++p;
		if ( *p )
			++p;
	}
	size_t n = strcspn ( p, "|" );
	if ( n >= size )
		n = size - 1;
	memcpy ( out, p, n );
	out[n] = 0;
}
/* Reuse the displayed option names from the immutable HTTP page template;
 * stored values remain its original codes (T01, P28, 0/1/2, ...). */
static void FormOptionLabel ( const HTTP_SettingField *field, const char *code, char *out, size_t size )
{
	snprintf ( out, size, "%s", code );
	const char *start = strstr ( lcdFormDraft.page->html, field->name );
	if ( !start )
		return;
	const char *end = strstr ( start, "</select>" );
	char search[HTTP_LCD_FORM_VALUE + 12];
	snprintf ( search, sizeof search, "value=\"%s\"", code );
	const char *option = strstr ( start, search );
	if ( !end || !option || option >= end )
		return;
	option = strchr ( option, '>' );
	if ( !option || ++option >= end )
		return;
	size_t n = 0;
	while ( *option && option < end && *option != '<' && n + 1 < size )
	{
		if ( !strncmp ( option, "&nbsp;", 6 ) )
		{
			out[n++] = ' ';
			option += 6;
		}
		else if ( !strncmp ( option, "&amp;", 5 ) )
		{
			out[n++] = '&';
			option += 5;
		}
		else
			out[n++] = *option++;
	}
	while ( n && out[n - 1] == ' ' )
		--n;
	out[n] = 0;
}
static void FormButton ( int x, int width, const char *text, int pressed )
{
	TileRound ( W, x, 14, width, 36, 4, pressed ? 0xFF285F90U : 0xFF468CCFU, pressed ? 0xFF285F90U : 0xFF468CCFU );
	Text ( W, TOP, ui.font[0], x + 8, 23, width - 16, text, WHITE );
}
static void FormHeader ( void )
{
	Rect ( W, 0, 0, W, TOP, BG );
	FormButton ( 36, 108, lcdForm.editing >= 0 ? "Anuluj" : "Wroc", lcdForm.armed == -10 );
	char title[64];
	if ( lcdForm.editing >= 0 && (unsigned) lcdForm.editing < lcdFormDraft.count )
		FormLabel ( (unsigned) lcdForm.editing, title, sizeof title );
	else
		snprintf ( title, sizeof title, "%s . port %u", lcdFormDraft.page ? lcdFormDraft.page->title : "Wczytywanie", lcdFormDraft.port );
	Text ( W, TOP, ui.font[1], 164, 22, 264, title, WHITE );
	FormButton ( 436, 120, lcdForm.editing >= 0 ? ( lcdForm.selection >= 0 ? "Poczatek" : "Wyczysc" ) : "Odrzuc", lcdForm.armed == -11 );
	FormButton ( 588, 136, lcdForm.editing >= 0 ? "Gotowe" : FormLoading () ? "Wczytuje..."
														 : lcdForm.pending	? "Zapis..."
																			: "Zapisz",
				 lcdForm.armed == -12 );
	LCD_DisplayBuff ( 0, 0, W, TOP, pLcd + SCRATCH );
	Rect ( W, 0, 0, W, 40, BG );
	Text ( W, 40, ui.font[0], 36, 12, 690, lcdForm.notice, GREEN );
	LCD_DisplayBuff ( 0, 440, W, 40, pLcd + SCRATCH );
}
static void FormValueBox ( int x, int y, int width, unsigned field )
{
	TileRound ( W, x, y, width, 40, 4, WHITE, WHITE );
	if ( FormLoading () )
	{
		Text ( W, 64, ui.font[1], x + 8, y + 9, width - 16, "...", VALUE_BLACK );
		return;
	}
	const char *value = lcdFormDraft.values[field];
	char checkbox[8], option[HTTP_LCD_FORM_VALUE];
	if ( FormField ( field )->kind == HTTP_SETTING_SELECT )
	{
		FormOptionLabel ( FormField ( field ), value, option, sizeof option );
		value = option;
	}
	if ( FormField ( field )->kind == HTTP_SETTING_CHECKBOX )
	{
		snprintf ( checkbox, sizeof checkbox, "%s", !strcmp ( value, "0" ) ? "[ ]" : "[X]" );
		value = checkbox;
	}
	if ( FormRouteIs ( "/term" ) && ( field == 1 || field == 2 ) )
	{
		char measurement[HTTP_LCD_FORM_VALUE + 8];
		snprintf ( measurement, sizeof measurement, "%s %s", value, lcdFormDraft.values[0][0] == 'W' ? "%RH" : "C" );
		Measurement ( W, 64, x + 8, y + 9, width - 16, measurement, VALUE_BLACK, ui.font[3], 0 );
	}
	else
		Text ( W, 64, ui.font[1], x + 8, y + 9, width - ( FormField ( field )->kind == HTTP_SETTING_SELECT ? 36 : 16 ), value, VALUE_BLACK );
	if ( FormField ( field )->kind == HTTP_SETTING_SELECT )
		for ( int row = 0; row < 5; ++row )
			Rect ( W, x + width - 24 + row, y + 17 + row, 9 - 2 * row, 1, VALUE_BLACK );
}
/* Outer cards are clipped directly into the existing viewport. The tail of
 * pLcd remains a single 64-row canvas for titles/controls and native fonts. */
static void FormRoundViewport ( int x, int y, int w, int h, int radius, uint32_t color )
{
	int top = y < 0 ? -y : 0, bottom = y + h > VIEW ? VIEW - y : h;
	for ( int row = top; row < bottom; ++row )
		for ( int col = 0; col < w; ++col )
		{
			int dx = col < radius ? 2 * ( radius - col ) - 1 : col >= w - radius ? 2 * ( col - ( w - radius ) ) + 1
																				 : 0;
			int dy = row < radius ? 2 * ( radius - row ) - 1 : row >= h - radius ? 2 * ( row - ( h - radius ) ) + 1
																				 : 0;
			if ( dx * dx + dy * dy <= 4 * radius * radius )
				pLcd[( y + row ) * W + x + col] = color;
		}
}
static void FormCopyControl ( int x, int content_y, int width, int height )
{
	int y = content_y - lcdForm.scroll;
	int first = y < 0 ? -y : 0, end = y + height > VIEW ? VIEW - y : height;
	for ( int row = first; row < end; ++row )
		memcpy ( pLcd + ( y + row ) * W + x, pLcd + SCRATCH + row * W + x, (size_t) width * sizeof *pLcd );
}
static void FormDraw ( void )
{
	for ( unsigned i = 0; i < W * VIEW; ++i )
		pLcd[i] = BG;
	FormCard card;
	for ( unsigned index = 0; FormCardAt ( index, &card ); ++index )
	{
		int y = card.y - lcdForm.scroll;
		if ( y >= VIEW || y + card.h <= 0 )
			continue;
		FormRoundViewport ( card.x, y, card.w, card.h, 28, FRAME );
		FormRoundViewport ( card.x + 3, y + 3, card.w - 6, card.h - 6, 25, CARD );
		Rect ( W, card.x + 18, 0, card.w - 36, 32, CARD );
		Text ( W, 32, ui.font[1], card.x + 18, 3, card.w - 36, card.title, GREEN );
		FormCopyControl ( card.x + 18, card.y + 12, card.w - 36, 32 );
	}
	for ( unsigned field = 0; field < lcdFormDraft.count + ( lcdFormDraft.mode == HTTP_MODE_PWM ); ++field )
	{
		FormControl box;
		if ( !FormControlAt ( field, &box ) || box.y - lcdForm.scroll >= VIEW || box.y + box.h <= lcdForm.scroll )
			continue;
		Rect ( W, box.x, 0, box.w, box.h, CARD );
		char label[96];
		if ( field == lcdFormDraft.count )
			snprintf ( label, sizeof label, "PWM (%%)" );
		else
			FormLabel ( field, label, sizeof label );
		Text ( W, 64, ui.font[0], box.x + 2, 2, box.w - 4, label, GREEN );
		if ( field == lcdFormDraft.count )
		{
			TileRound ( W, box.x, 24, box.w, 40, 4, WHITE, WHITE );
			snprintf ( label, sizeof label, "%u", lcdFormDraft.pwm );
			Text ( W, 64, ui.font[1], box.x + 8, 33, box.w - 16, FormLoading () ? "..." : label, VALUE_BLACK );
		}
		else
			FormValueBox ( box.x, 24, box.w, field );
		FormCopyControl ( box.x, box.y, box.w, box.h );
	}
	int height = FormContentHeight (), maximum = FormMaximum ();
	int thumb = height > VIEW ? VIEW * VIEW / height : VIEW;
	if ( thumb < 32 )
		thumb = 32;
	int top = maximum ? lcdForm.scroll * ( VIEW - thumb ) / maximum : 0;
	if ( maximum )
		for ( int y = 0; y < VIEW; ++y )
			for ( int x = SCROLL_X; x < SCROLL_X + SCROLL_W; ++x )
				pLcd[y * W + x] = y >= top && y < top + thumb ? GREEN : 0xFF333333U;
	LCD_DisplayBuff ( 0, TOP, W, VIEW, pLcd );
	FormHeader ();
}
static int FormControlHit ( int x, int content_y )
{
	FormControl box;
	for ( unsigned field = 0; FormControlAt ( field, &box ); ++field )
		if ( x >= box.x && x < box.x + box.w && content_y >= box.y + 24 && content_y < box.y + box.h )
			return (int) field;
	return -1;
}

static const char *FormKey ( unsigned key, char out[8] )
{
	static const char *polish[] = { "ą", "ć", "ę", "ł", "ń", "ó", "ś", "ź", "ż", "?" };
	static const char *capitals[] = { "Ą", "Ć", "Ę", "Ł", "Ń", "Ó", "Ś", "Ź", "Ż", "!" };
	static const char keys[] = "1234567890qwertyuiopasdfghjkl-zxcvbnm.,:";
	if ( lcdForm.polish && key >= 10 && key < 20 )
		return lcdForm.upper ? capitals[key - 10] : polish[key - 10];
	out[0] = keys[key];
	if ( lcdForm.upper && out[0] >= 'a' && out[0] <= 'z' )
		out[0] -= 'a' - 'A';
	out[1] = 0;
	return out;
}
static void FormKeyboardDraw ( void )
{
	for ( unsigned i = 0; i < W * VIEW; ++i )
		pLcd[i] = BG;
	Rect ( W, 0, 0, W, 56, BG );
	Rect ( W, 24, 8, 736, 44, WHITE );
	const char *shown = lcdForm.text;
	while ( *shown && LCD_GetWholeStrPxlWidth ( ui.font[1], (char *) shown, 0, NoConstWidth ) > 712 )
	{
		++shown;
		while ( ( (unsigned char) *shown & 0xC0 ) == 0x80 )
			++shown;
	}
	Text ( W, 56, ui.font[1], 36, 20, 712, shown, VALUE_BLACK );
	/* Text helpers render to scratch; keyboard canvas is copied strip by strip. */
	memcpy ( pLcd, pLcd + SCRATCH, W * 56 * sizeof *pLcd );
	for ( unsigned row = 0; row < 4; ++row )
	{
		Rect ( W, 0, 0, W, 52, BG );
		for ( unsigned col = 0; col < 10; ++col )
		{
			char key[8];
			const char *caption = FormKey ( row * 10 + col, key );
			uint32_t key_color = lcdForm.keyPressed == (int) ( row * 10 + col ) ? 0xFF468CCFU : 0xFF666666U;
			TileRound ( W, 24 + col * 74, 0, 68, 48, 5, key_color, key_color );
			Text ( W, 52, ui.font[1], 48 + col * 74, 13, 38, caption, WHITE );
		}
		memcpy ( pLcd + ( 72 + row * 52 ) * W, pLcd + SCRATCH, W * 52 * sizeof *pLcd );
	}
	static const char *special[] = { "ABC", "PL", "Spacja", "<--", "Gotowe", "Anuluj" };
	Rect ( W, 0, 0, W, 48, BG );
	for ( unsigned i = 0; i < 6; ++i )
	{
		TileRound ( W, 24 + i * 124, 0, 116, 44, 4, 0xFF468CCFU, 0xFF468CCFU );
		Text ( W, 48, ui.font[0], 32 + i * 124, 13, 100, special[i], WHITE );
	}
	memcpy ( pLcd + 308 * W, pLcd + SCRATCH, W * 48 * sizeof *pLcd );
	LCD_DisplayBuff ( 0, TOP, W, VIEW, pLcd );
	FormHeader ();
}
static void FormChoicesDraw ( void )
{
	const HTTP_SettingField *field = FormField ( (unsigned) lcdForm.editing );
	unsigned count = FormOptionCount ( field );
	for ( unsigned i = 0; i < W * VIEW; ++i )
		pLcd[i] = BG;
	for ( unsigned i = 0; i < count; ++i )
	{
		int y = (int) i * 52 - lcdForm.selection;
		if ( y + 52 <= 0 || y >= VIEW )
			continue;
		char option[HTTP_LCD_FORM_VALUE], label[HTTP_LCD_FORM_VALUE];
		FormOption ( field, i, option, sizeof option );
		Rect ( W, 0, 0, W, 52, BG );
		TileRound ( W, 36, 3, 690, 46, 4, !strcmp ( option, lcdFormDraft.values[lcdForm.editing] ) ? 0xFF56743AU : CARD, !strcmp ( option, lcdFormDraft.values[lcdForm.editing] ) ? 0xFF56743AU : CARD );
		FormOptionLabel ( field, option, label, sizeof label );
		Text ( W, 52, ui.font[1], 52, 16, 650, label, WHITE );
		int first = y < 0 ? -y : 0, end = y + 52 > VIEW ? VIEW - y : 52;
		for ( int r = first; r < end; ++r )
			memcpy ( pLcd + ( y + r ) * W, pLcd + SCRATCH + r * W, W * sizeof *pLcd );
	}
	int maximum = (int) count * 52 - VIEW;
	if ( maximum > 0 )
	{
		int thumb = VIEW * VIEW / ( (int) count * 52 );
		if ( thumb < 32 )
			thumb = 32;
		int top = lcdForm.selection * ( VIEW - thumb ) / maximum;
		for ( int y = 0; y < VIEW; ++y )
			for ( int x = 738; x < 760; ++x )
				pLcd[y * W + x] = y >= top && y < top + thumb ? GREEN : 0xFF333333U;
	}
	LCD_DisplayBuff ( 0, TOP, W, VIEW, pLcd );
	FormHeader ();
}
static void FormPaint ( void )
{
	if ( lcdForm.editing < 0 )
		FormDraw ();
	else if ( lcdForm.selection >= 0 )
		FormChoicesDraw ();
	else
		FormKeyboardDraw ();
}
static int FormOpen ( unsigned port, int names )
{
	uint32_t serial = HTTP_LCDFormOpen ( port, names );
	if ( !serial )
		return 0;
	memset ( &lcdForm, 0, sizeof lcdForm );
	/* These three copies are also in SDRAM NOLOAD. A retained base serial
	 * must not make the first freshly published snapshot look unchanged. */
	memset ( &lcdFormBase, 0, sizeof lcdFormBase );
	memset ( &lcdFormDraft, 0, sizeof lcdFormDraft );
	memset ( &lcdFormLatest, 0, sizeof lcdFormLatest );
	lcdForm.serial = serial;
	lcdForm.active = 1;
	lcdForm.editing = -1;
	lcdForm.selection = -1;
	lcdForm.armed = -1;
	lcdForm.keyPressed = -1;
	lcdForm.wasDown = LCD_TOUCH_isPress () == press;
	lcdFormDraft.port = port;
	lcdFormDraft.mode = ui.data.ports[port - 1].mode;
	lcdFormDraft.names_only = !!names;
	lcdFormDraft.page = HTTP_LCDFormRoute ( port, lcdFormDraft.mode, names, &lcdFormDraft.instance );
	if ( lcdFormDraft.page )
	{
		if ( !strcmp ( lcdFormDraft.page->route, "/set" ) )
			lcdFormDraft.names_only = 1;
		lcdFormDraft.count = lcdFormDraft.names_only ? 1 : lcdFormDraft.page->field_count;
		if ( lcdFormDraft.count > HTTP_LCD_FORM_FIELDS )
			lcdFormDraft.count = 0;
	}
	/* Show the actual schema immediately, with placeholders until the owner
	 * supplies values. A busy WiFi/TLS task must not leave a black viewport. */
	snprintf ( lcdForm.notice, sizeof lcdForm.notice, "Wczytuje wspolne ustawienia HTTP..." );
	FormDraw ();
	return 1;
}
static void FormFinishEdit ( int commit )
{
	if ( commit && lcdForm.selection < 0 )
	{
		if ( (unsigned) lcdForm.editing == lcdFormDraft.count )
		{
			char *end;
			unsigned long value = strtoul ( lcdForm.text, &end, 10 );
			if ( !lcdForm.text[0] || *end || value > 100 )
			{
				snprintf ( lcdForm.notice, sizeof lcdForm.notice, "PWM: wpisz 0..100." );
				FormPaint ();
				return;
			}
			lcdFormDraft.pwm = (unsigned) value;
		}
		else
			snprintf ( lcdFormDraft.values[lcdForm.editing], HTTP_LCD_FORM_VALUE, "%s", lcdForm.text );
		lcdForm.dirty = 1;
	}
	lcdForm.editing = -1;
	lcdForm.selection = -1;
	snprintf ( lcdForm.notice, sizeof lcdForm.notice, "Zmieniono lokalnie. Nacisnij Zapisz." );
	FormDraw ();
}
static int FormUpdate ( void )
{
	int latest_valid = 0;
	uint32_t now = HAL_GetTick ();
	if ( lcdForm.pending || lcdFormBase.serial != lcdForm.serial || (uint32_t) ( now - lcdForm.sample ) >= 250 )
	{
		lcdForm.sample = now;
		latest_valid = HTTP_LCDFormRead ( &lcdFormLatest );
		if ( !latest_valid )
			lcdFormLatest.serial = 0;
	}

	if ( latest_valid && lcdFormLatest.serial == lcdForm.serial )
	{
		if ( lcdForm.pending && lcdFormLatest.status != LCD_FORM_BUSY )
		{
			lcdForm.pending = 0;
			if ( lcdFormLatest.status == LCD_FORM_SAVED )
				lcdForm.dirty = 0;
			snprintf ( lcdForm.notice, sizeof lcdForm.notice, "%s", lcdFormLatest.status == LCD_FORM_SAVED ? "Zapisano LCD / HTTP / SD." : lcdFormLatest.status == LCD_FORM_CONFLICT ? "Zmiana HTTP. Odrzuc, Wroc i otworz ustawienia ponownie."
																																	   : lcdFormLatest.status == LCD_FORM_SD_ERROR	 ? "Blad SD. Zachowano edycje."
																																													 : "Bledna wartosc. Sprawdz zakres i format jak w HTTP." );
			FormHeader ();
		}
		if ( !lcdForm.dirty && !lcdForm.pending && lcdForm.editing < 0 && ( lcdFormBase.serial != lcdFormLatest.serial || lcdFormBase.hash != lcdFormLatest.hash || lcdFormBase.pwm != lcdFormLatest.pwm ) )
		{
			if ( lcdFormBase.serial != lcdFormLatest.serial )
				snprintf ( lcdForm.notice, sizeof lcdForm.notice, "%s", lcdFormLatest.status == LCD_FORM_INVALID || !lcdFormLatest.page || !lcdFormLatest.count ? "Nie odczytano ustawien. Nacisnij Wroc i ponow." : "Dotknij pola, edytuj, potem Zapisz. Wspolne dane HTTP / SD." );
			lcdFormBase = lcdFormLatest;
			lcdFormDraft = lcdFormLatest;
			FormDraw ();
		}
	}
	XY_Touch_Struct position = LCD_TOUCH_GetPos ();
	(void) LCD_TOUCH_GetTypeAndPosition ( &screenTouchPos );
	int down = LCD_TOUCH_isPress () == press, hit = -1;
	if ( position.y >= 14 && position.y < 50 )
	{
		if ( position.x >= 36 && position.x < 144 )
			hit = -10;
		if ( position.x >= 436 && position.x < 556 )
			hit = -11;
		if ( position.x >= 588 && position.x < 724 )
			hit = -12;
	}
	if ( position.y >= TOP && position.y < TOP + VIEW && position.x >= 24 && position.x < 760 )
	{
		int y = position.y - TOP;
		if ( lcdForm.editing >= 0 && lcdForm.selection < 0 )
		{
			if ( y >= 72 && y < 280 )
			{
				int row = ( y - 72 ) / 52, col = ( position.x - 24 ) / 74;
				if ( col < 10 && ( position.x - 24 ) % 74 < 68 && ( y - 72 ) % 52 < 48 )
					hit = row * 10 + col;
			}
			else if ( y >= 308 && y < 352 )
				hit = 40 + ( position.x - 24 ) / 124;
		}
		else if ( lcdForm.editing >= 0 )
			hit = ( y + lcdForm.selection ) / 52;
		else
			hit = FormControlHit ( position.x, y + lcdForm.scroll );
	}
	if ( down && !lcdForm.wasDown )
	{
		lcdForm.armed = hit;
		lcdForm.startY = position.y;
		lcdForm.startScroll = lcdForm.selection >= 0 ? lcdForm.selection : lcdForm.scroll;
		lcdForm.moved = 0;
		lcdForm.drag = position.y >= TOP && position.y < TOP + VIEW && ( lcdForm.editing < 0 || lcdForm.selection >= 0 );
		if ( hit <= -10 )
			FormHeader ();
	}
	if ( down && lcdForm.drag )
	{
		int delta = (int) position.y - lcdForm.startY;
		if ( delta > 6 || delta < -6 )
			lcdForm.moved = 1;
		if ( lcdForm.moved )
		{
			int maximum = lcdForm.selection >= 0 ? (int) FormOptionCount ( FormField ( (unsigned) lcdForm.editing ) ) * 52 - VIEW : FormMaximum ();
			if ( maximum < 0 )
				maximum = 0;
			int next = lcdForm.startScroll - delta;
			if ( position.x >= 738 && lcdForm.editing < 0 )
				next = lcdForm.startScroll + delta * ( maximum + VIEW ) / VIEW;
			next = Clamp ( next, 0, maximum );
			if ( lcdForm.selection >= 0 )
				lcdForm.selection = next;
			else
				lcdForm.scroll = next;
			FormPaint ();
		}
	}
	if ( lcdForm.editing >= 0 && lcdForm.selection < 0 )
	{
		int pressed = down && hit >= 0 && hit == lcdForm.armed ? hit : -1;
		if ( pressed != lcdForm.keyPressed )
		{
			lcdForm.keyPressed = pressed;
			FormKeyboardDraw ();
		}
	}
	int action = !down && lcdForm.wasDown && !lcdForm.moved && hit == lcdForm.armed ? hit : -1;
	int released = !down && lcdForm.wasDown;
	lcdForm.wasDown = down;
	if ( released )
	{
		lcdForm.armed = -1;
		FormHeader ();
	}
	if ( down || action == -1 )
		return 0;
	lcdForm.armed = -1;
	if ( lcdForm.pending || ( FormLoading () && action != -10 ) )
		return 0;
	if ( action == -10 )
	{
		if ( lcdForm.editing >= 0 )
			FormFinishEdit ( 0 );
		else if ( lcdForm.dirty )
		{
			snprintf ( lcdForm.notice, sizeof lcdForm.notice, "Najpierw Zapisz lub Odrzuc." );
			FormDraw ();
		}
		else
		{
			HTTP_LCDFormClose ();
			lcdForm.active = 0;
			return 1;
		}
	}
	else if ( action == -11 )
	{
		if ( lcdForm.editing >= 0 )
		{
			if ( lcdForm.selection >= 0 )
				lcdForm.selection = 0;
			else
				lcdForm.text[0] = 0;
			FormPaint ();
		}
		else if ( lcdFormLatest.serial == lcdForm.serial )
		{
			lcdFormBase = lcdFormDraft = lcdFormLatest;
			lcdForm.dirty = 0;
			snprintf ( lcdForm.notice, sizeof lcdForm.notice, "Odczytano aktualne ustawienia HTTP." );
			FormDraw ();
		}
	}
	else if ( action == -12 )
	{
		if ( lcdForm.editing >= 0 )
			FormFinishEdit ( 1 );
		else if ( lcdForm.dirty && HTTP_LCDFormSave ( &lcdFormBase, &lcdFormDraft ) )
		{
			lcdForm.pending = 1;
			snprintf ( lcdForm.notice, sizeof lcdForm.notice, "Zapisuje na karcie SD..." );
			FormHeader ();
		}
	}
	else if ( lcdForm.editing >= 0 && lcdForm.selection >= 0 )
	{
		if ( (unsigned) action < FormOptionCount ( FormField ( (unsigned) lcdForm.editing ) ) )
		{
			FormOption ( FormField ( (unsigned) lcdForm.editing ), (unsigned) action, lcdFormDraft.values[lcdForm.editing], HTTP_LCD_FORM_VALUE );
			lcdForm.dirty = 1;
			lcdForm.editing = -1;
			lcdForm.selection = -1;
			FormDraw ();
		}
	}
	else if ( lcdForm.editing >= 0 )
	{
		if ( action == 40 )
			lcdForm.upper = !lcdForm.upper;
		else if ( action == 41 )
			lcdForm.polish = !lcdForm.polish;
		else if ( action == 43 )
		{
			size_t n = strlen ( lcdForm.text );
			if ( n )
			{
				--n;
				while ( n && ( (unsigned char) lcdForm.text[n] & 0xC0 ) == 0x80 )
					--n;
				lcdForm.text[n] = 0;
			}
		}
		else if ( action == 44 || action == 45 )
		{
			FormFinishEdit ( action == 44 );
			return 0;
		}
		else
		{
			char key[8];
			const char *append = action == 42 ? " " : action < 40 ? FormKey ( (unsigned) action, key )
																  : "";
			size_t length = strlen ( lcdForm.text ), extra = strlen ( append );
			unsigned limit = (unsigned) lcdForm.editing == lcdFormDraft.count ? 3 : FormField ( (unsigned) lcdForm.editing )->max_length;
			if ( length + extra <= limit && length + extra < sizeof lcdForm.text )
				memcpy ( lcdForm.text + length, append, extra + 1 );
		}
		FormKeyboardDraw ();
	}
	else if ( action >= 0 && (unsigned) action <= lcdFormDraft.count )
	{
		if ( (unsigned) action == lcdFormDraft.count )
		{
			if ( lcdFormDraft.mode != HTTP_MODE_PWM )
				return 0;
			snprintf ( lcdForm.text, sizeof lcdForm.text, "%u", lcdFormDraft.pwm );
			lcdForm.selection = -1;
		}
		else
		{
			const HTTP_SettingField *field = FormField ( (unsigned) action );
			if ( field->kind == HTTP_SETTING_READONLY )
				return 0;
			if ( field->kind == HTTP_SETTING_CHECKBOX )
			{
				snprintf ( lcdFormDraft.values[action], HTTP_LCD_FORM_VALUE, "%s", !strcmp ( lcdFormDraft.values[action], "0" ) ? "1" : "0" );
				lcdForm.dirty = 1;
				FormDraw ();
				return 0;
			}
			snprintf ( lcdForm.text, sizeof lcdForm.text, "%s", lcdFormDraft.values[action] );
			lcdForm.selection = field->kind == HTTP_SETTING_SELECT ? 0 : -1;
		}
		lcdForm.editing = action;
		snprintf ( lcdForm.notice, sizeof lcdForm.notice, "Edytuj lokalnie, potem Gotowe i Zapisz." );
		FormPaint ();
	}
	return 0;
}
