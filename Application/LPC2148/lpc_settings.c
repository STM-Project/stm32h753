#include "lpc_pages.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* Shared keys mean classic/mobile/SSL variants edit the same parameter. */
static unsigned char current[LPC_BLOB_SIZE] LPC_SDRAM;
static unsigned char work[LPC_BLOB_SIZE] LPC_SDRAM;
static unsigned char seen[LPC_FIELD_COUNT] LPC_SDRAM;
static LpcSave save_callback;
static void *save_context;
static int initialized;
static unsigned char *record ( unsigned char *blob, unsigned i )
{
	return blob + 16u + i * ( LPC_VALUE_MAX + 2u );
}
static void put32 ( unsigned char *p, uint32_t v )
{
	for ( unsigned i = 0; i < 4; i++ )
		p[i] = (unsigned char) ( v >> ( 8u * i ) );
}
static uint32_t get32 ( const unsigned char *p )
{
	return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}
static uint32_t crc ( const unsigned char *data, size_t size )
{
	uint32_t c = ~0u;
	for ( size_t i = 0; i < size; i++ )
	{
		c ^= data[i];
		for ( unsigned b = 0; b < 8; b++ )
			c = ( c >> 1 ) ^ ( 0xedb88320u & ( 0u - ( c & 1u ) ) );
	}
	return ~c;
}
static void finish ( unsigned char *blob )
{
	memcpy ( blob, "LPC1", 4 );
	put32 ( blob + 4, LPC_SCHEMA );
	put32 ( blob + 8, LPC_FIELD_COUNT );
	put32 ( blob + 12, crc ( blob + 16, LPC_BLOB_SIZE - 16 ) );
}
static int field_index ( const char *name )
{
	unsigned a = 0, b = LPC_FIELD_COUNT;
	while ( a < b )
	{
		unsigned m = a + ( b - a ) / 2;
		int c = strcmp ( name, LPC_Fields[m].name );
		if ( !c )
			return (int) m;
		if ( c < 0 )
			b = m;
		else
			a = m + 1;
	}
	return -1;
}
static int utf8 ( const unsigned char *s )
{
	while ( *s )
	{
		unsigned c = *s++, n, cp, min;
		if ( c < 128 )
		{
			if ( c < 32 && c != '\r' && c != '\n' && c != '\t' )
				return 0;
			continue;
		}
		if ( c >= 0xc2 && c <= 0xdf )
		{
			n = 1;
			cp = c & 31;
			min = 128;
		}
		else if ( c >= 0xe0 && c <= 0xef )
		{
			n = 2;
			cp = c & 15;
			min = 2048;
		}
		else if ( c >= 0xf0 && c <= 0xf4 )
		{
			n = 3;
			cp = c & 7;
			min = 65536;
		}
		else
			return 0;
		while ( n-- )
		{
			if ( ( *s & 0xc0 ) != 0x80 )
				return 0;
			cp = ( cp << 6 ) | ( *s++ & 63 );
		}
		if ( cp < min || cp > 0x10ffff || ( cp >= 0xd800 && cp <= 0xdfff ) )
			return 0;
	}
	return 1;
}
static int valid ( unsigned id, const char *value )
{
	const LpcField *f = &LPC_Fields[id];
	size_t n = strlen ( value );
	if ( n > f->maximum || n > LPC_VALUE_MAX || !utf8 ( (const unsigned char *) value ) )
		return 0;
	if ( f->kind == 1 )
		return !strcmp ( value, "0" ) || !strcmp ( value, "1" );
	if ( f->kind == 3 && *f->choices )
	{
		const char *p = f->choices;
		while ( *p )
		{
			const char *e = strchr ( p, '|' );
			size_t k = e ? (size_t) ( e - p ) : strlen ( p );
			if ( k == n && !memcmp ( p, value, n ) )
				return 1;
			if ( !e )
				break;
			p = e + 1;
		}
		return 0;
	}
	return 1;
}
int LPC_ValidateSettings ( const void *bytes, size_t length )
{
	const unsigned char *p = bytes;
	if ( !p || length != LPC_BLOB_SIZE || memcmp ( p, "LPC1", 4 ) || get32 ( p + 4 ) != LPC_SCHEMA || get32 ( p + 8 ) != LPC_FIELD_COUNT || get32 ( p + 12 ) != crc ( p + 16, length - 16 ) )
		return 0;
	for ( unsigned i = 0; i < LPC_FIELD_COUNT; i++ )
	{
		const unsigned char *r = p + 16u + i * ( LPC_VALUE_MAX + 2u );
		if ( r[0] > 1 || !memchr ( r + 1, 0, LPC_VALUE_MAX + 1u ) || ( r[0] && !valid ( i, (const char *) r + 1 ) ) )
			return 0;
	}
	return 1;
}
int LPC_SettingsInit ( LpcLoad load, LpcSave save, void *context )
{
	save_callback = save;
	save_context = context;
	memset ( current, 0, sizeof current );
	finish ( current );
	initialized = 1;
	if ( !load )
		return 0;
	int result = load ( work, sizeof work, context );
	if ( result == 1 && LPC_ValidateSettings ( work, sizeof work ) )
	{
		memcpy ( current, work, sizeof current );
		return 1;
	}
	return result == 0 ? 0 : -1;
}
const char *LPC_GetSavedSetting ( const char *name )
{
	if ( !initialized )
		return NULL;
	int f = field_index ( name );
	if ( f < 0 )
		return NULL;
	const unsigned char *r = record ( current, (unsigned) f );
	return r[0] ? (const char *) r + 1 : NULL;
}
extern const char *LPC_DefaultFromH7 ( const char *name );
const char *LPC_GetSetting ( const char *name )
{
	const char *value = LPC_GetSavedSetting ( name );
	if ( value )
		return value;
	value = LPC_DefaultFromH7 ( name );
	if ( value )
		return value;
	int f = field_index ( name );
	return f < 0 ? NULL : LPC_Fields[f].default_value;
}
unsigned LPC_ResolvePage ( const char *target, size_t length )
{
	if ( !target || length <= 5 || memcmp ( target, "/nowa", 5 ) )
		return 0;
	unsigned n = 0;
	for ( size_t i = 5; i < length; i++ )
	{
		if ( target[i] < '0' || target[i] > '9' || n > LPC_PAGE_COUNT )
			return 0;
		n = n * 10u + (unsigned) ( target[i] - '0' );
	}
	if ( target[5] == '0' )
		return 0;
	return n && n <= LPC_PAGE_COUNT ? n : 0;
}
unsigned LPC_ResolveSettingsApi ( const char *target, size_t length )
{
	if ( !target || length <= 8 || memcmp ( target, "/api/lpc", 8 ) )
		return 0;
	char path[24];
	if ( length - 8 + 5 >= sizeof path )
		return 0;
	memcpy ( path, "/nowa", 5 );
	memcpy ( path + 5, target + 8, length - 8 );
	return LPC_ResolvePage ( path, length - 8 + 5 );
}
typedef struct
{
	char *out;
	size_t cap, n;
	int failed;
	int window;
	size_t offset, position;
	uint32_t hash;
} Writer;
static void append ( Writer *w, const char *fmt, ... )
{
	if ( w->failed )
		return;
	va_list v;
	va_start ( v, fmt );
	if ( w->window )
	{
		/* JSON emits only short scalar fragments; catalogue rendering continues
		 * to use the original direct writer below. */
		char fragment[64];
		int count = vsnprintf ( fragment, sizeof fragment, fmt, v );
		va_end ( v );
		if ( count < 0 || (size_t) count >= sizeof fragment )
		{
			w->failed = 1;
			return;
		}
		for ( int i = 0; i < count; ++i )
		{
			w->hash = ( w->hash ^ (unsigned char) fragment[i] ) * 16777619U;
			if ( w->position >= w->offset && w->n < w->cap )
				w->out[w->n++] = fragment[i];
			++w->position;
		}
		return;
	}
	int n = vsnprintf ( w->out + w->n, w->cap - w->n, fmt, v );
	va_end ( v );
	if ( n < 0 || (size_t) n >= w->cap - w->n )
		w->failed = 1;
	else
		w->n += (size_t) n;
}
static void string ( Writer *w, const char *s )
{
	append ( w, "\"" );
	for ( ; s && *s; s++ )
	{
		unsigned char c = (unsigned char) *s;
		if ( c == '"' || c == '\\' )
			append ( w, "\\%c", c );
		else if ( c < 32 )
			append ( w, "\\u%04x", c );
		else
			append ( w, "%c", c );
	}
	append ( w, "\"" );
}
static void JsonFields ( unsigned page, Writer *w )
{
	append ( w, "{\"page\":%u,\"fields\":{", page );
	memset ( seen, 0, sizeof seen );
	unsigned count = 0;
	const LpcPage *p = &LPC_Pages[page - 1];
	for ( unsigned f = 0; f < p->form_count; f++ )
	{
		const LpcForm *form = &LPC_Forms[p->first_form + f];
		for ( unsigned i = 0; i < form->count; i++ )
		{
			unsigned id = LPC_Members[form->start + i];
			if ( seen[id] )
				continue;
			seen[id] = 1;
			if ( count++ )
				append ( w, "," );
			string ( w, LPC_Fields[id].name );
			append ( w, ":" );
			string ( w, LPC_Fields[id].kind == 2 ? "" : LPC_GetSetting ( LPC_Fields[id].name ) );
		}
	}
	append ( w, "}}" );
}
size_t LPC_SettingsJson ( unsigned page, char *out, size_t capacity )
{
	if ( !out || !capacity || !page || page > LPC_PAGE_COUNT )
		return 0;
	out[0] = 0;
	Writer w = { .out = out, .cap = capacity };
	JsonFields ( page, &w );
	if ( w.failed )
	{
		out[0] = 0;
		return 0;
	}
	return w.n;
}
/* Fallback for JSON larger than the normal per-connection page buffer.
 * A full-response fingerprint prevents mixing settings changed mid-transfer. */
size_t LPC_SettingsJsonWindow ( unsigned page, char *out, size_t capacity, size_t offset, size_t *total, uint32_t *hash )
{
	if ( !total || !hash || ( capacity && !out ) || !page || page > LPC_PAGE_COUNT )
		return 0;
	Writer w = { .out = out, .cap = capacity, .window = 1, .offset = offset, .hash = 2166136261U };
	JsonFields ( page, &w );
	*total = w.failed ? 0 : w.position;
	*hash = w.hash;
	return w.failed ? 0 : w.n;
}

static int unhex ( char c )
{
	return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10
										: c >= 'A' && c <= 'F'	 ? c - 'A' + 10
																 : -1;
}
static int decode ( char *out, size_t capacity, const char *in, size_t length )
{
	size_t used = 0;
	for ( size_t i = 0; i < length; i++ )
	{
		unsigned char c = (unsigned char) in[i];
		if ( c == '%' )
		{
			if ( i + 2 >= length )
				return 0;
			int a = unhex ( in[++i] ), b = unhex ( in[++i] );
			if ( a < 0 || b < 0 )
				return 0;
			c = (unsigned char) ( ( a << 4 ) | b );
		}
		else if ( c == '+' )
			c = ' ';
		if ( !c || used + 1 >= capacity )
			return 0;
		out[used++] = (char) c;
	}
	out[used] = 0;
	return 1;
}
static int pair ( const char *query, size_t length, size_t *offset, char *name, char *value )
{
	if ( *offset >= length )
		return 0;
	size_t a = *offset, b = a, equal;
	while ( b < length && query[b] != '&' )
		b++;
	equal = a;
	while ( equal < b && query[equal] != '=' )
		equal++;
	*offset = b < length ? b + 1 : b;
	if ( equal == a || equal == b || !decode ( name, 96, query + a, equal - a ) || !decode ( value, LPC_VALUE_MAX + 1u, query + equal + 1, b - equal - 1 ) )
		return -1;
	return 1;
}
static int number ( const char *s, unsigned *n )
{
	if ( !*s )
		return 0;
	unsigned value = 0;
	for ( ; *s; s++ )
	{
		if ( *s < '0' || *s > '9' || value > 65535 )
			return 0;
		value = value * 10u + (unsigned) ( *s - '0' );
	}
	*n = value;
	return 1;
}
int LPC_ApplyTarget ( const char *target, size_t length, unsigned *result_page )
{
	if ( result_page )
		*result_page = 0;
	if ( !target || length < 14 || memcmp ( target, "/lpc.cgi/A117?", 14 ) )
		return 0;
	const char *q = target + 14;
	size_t n = length - 14, pos = 0;
	char name[96], value[LPC_VALUE_MAX + 1u], action[LPC_VALUE_MAX + 1u] = "";
	unsigned page = 0, form_id = 0, pm = 0, fm = 0, sm = 0;
	int status, recognized = 0;
	while ( ( status = pair ( q, n, &pos, name, value ) ) > 0 )
	{
		if ( !strcmp ( name, "lpc_page" ) )
		{
			recognized = 1;
			if ( pm++ || !number ( value, &page ) || !page || page > LPC_PAGE_COUNT )
				return 400;
		}
		else if ( !strcmp ( name, "lpc_form" ) )
		{
			if ( fm++ || !number ( value, &form_id ) )
				return 400;
		}
		else if ( !strcmp ( name, "lpc_submit" ) )
		{
			if ( sm++ )
				return 400;
			strcpy ( action, value );
		}
	}
	if ( !recognized )
		return 0;
	if ( result_page )
		*result_page = page;
	if ( status < 0 || pm != 1 || fm != 1 || !initialized || form_id >= LPC_Pages[page - 1].form_count )
		return 400;
	/* Read/reset/transmit hardware commands must not masquerade as a save. */
	if ( *action && !strstr ( action, "zapisz" ) && ( strstr ( action, "odczyt" ) || strstr ( action, "reset" ) || strstr ( action, "wysl" ) || strstr ( action, "ping" ) || strstr ( action, "fab" ) ) )
		return 501;
	const LpcForm *form = &LPC_Forms[LPC_Pages[page - 1].first_form + form_id];
	if ( !form->count )
		return 501;
	memset ( seen, 0, sizeof seen );
	for ( unsigned i = 0; i < form->count; i++ )
		seen[LPC_Members[form->start + i]] = 1;
	memcpy ( work, current, sizeof work );
	pos = 0;
	while ( ( status = pair ( q, n, &pos, name, value ) ) > 0 )
	{
		if ( !strcmp ( name, "lpc_page" ) || !strcmp ( name, "lpc_form" ) || !strcmp ( name, "lpc_submit" ) )
			continue;
		int id = field_index ( name );
		if ( id < 0 || seen[id] != 1 )
			return 400;
		seen[id] = 2;
		const LpcField *f = &LPC_Fields[id];
		if ( f->kind == 1 )
		{
			if ( !strcmp ( value, "on" ) || !strcmp ( value, "tak" ) || !strcmp ( value, "true" ) )
				strcpy ( value, "1" );
		}
		if ( !valid ( (unsigned) id, value ) )
			return 400;
		if ( f->kind == 2 && !*value )
			continue;
		unsigned char *r = record ( work, (unsigned) id );
		r[0] = 1;
		memset ( r + 1, 0, LPC_VALUE_MAX + 1u );
		memcpy ( r + 1, value, strlen ( value ) );
	}
	if ( status < 0 )
		return 400;
	for ( unsigned i = 0; i < form->count; i++ )
	{
		unsigned id = LPC_Members[form->start + i];
		if ( seen[id] == 1 )
		{
			if ( LPC_Fields[id].kind != 1 )
				return 400;
			unsigned char *r = record ( work, id );
			r[0] = 1;
			memset ( r + 1, 0, LPC_VALUE_MAX + 1u );
			r[1] = '0';
		}
	}
	finish ( work );
	if ( !save_callback || save_callback ( work, sizeof work, save_context ) != 1 )
		return 503;
	memcpy ( current, work, sizeof current );
	return 200;
}
size_t LPC_CatalogRender ( char *out, size_t capacity )
{
	if ( !out || !capacity )
		return 0;
	Writer w = { .out = out, .cap = capacity };
	append ( &w, "<!doctype html><html lang='pl'><meta charset='UTF-8'><title>Strony LPC2148</title><style>body{font:16px Arial;margin:24px}td,th{padding:8px;text-align:left}tr:nth-child(even){background:#eee}</style><h1>Strony LPC2148</h1><p><a href='/nowa'>Panel we Flash</a> | <a href='/set'>Ustawienia H7</a></p><p>Strony numerowane wymagają folderu LPC z pakietu sdcard.zip na karcie SD.</p><table><tr><th>Adres</th><th>Plik źródłowy</th></tr>" );
	for ( unsigned i = 0; i < LPC_PAGE_COUNT; i++ )
		append ( &w, "<tr><td><a href='/nowa%u'>/nowa%u</a></td><td>%s</td></tr>", i + 1, i + 1, LPC_Pages[i].source );
	append ( &w, "</table></html>" );
	if ( w.failed )
	{
		out[0] = 0;
		return 0;
	}
	return w.n;
}
