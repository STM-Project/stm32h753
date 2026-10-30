/* /main and /set only: cacheable setting-independent templates and a small
 * fresh snapshot. Included by esp32wroom.c; all model reads stay in its owner
 * task. Existing per-response buffers retain independent queued snapshots. */
#include "http_panel_assets.inc"

static int HTTP_PanelIsAsset ( int page )
{
	return page == WWW_PANEL_SCRIPT || page == WWW_PANEL_READ_TEMPLATE || page == WWW_PANEL_SET_TEMPLATE;
}

static size_t HTTP_PanelConfigJson ( char *out, size_t capacity )
{
	size_t used = 0;
	if ( !out || capacity < 16 )
		return 0;
	memcpy ( out, "{\"ports\":[", 10 );
	used = 10;
	for ( unsigned port = 1; port <= 33; port++ )
	{
		char number[32];
		int n = snprintf ( number, sizeof number, "%s[%u,\"", port == 1 ? "" : ",", HTTP_SettingsPortMode ( port ) );
		if ( n < 0 || (size_t) n >= sizeof number || used + (size_t) n >= capacity )
			return 0;
		memcpy ( out + used, number, (size_t) n );
		used += (size_t) n;
		const unsigned char *name = (const unsigned char *) HTTP_SettingsPortName ( port, NULL );
		for ( unsigned i = 0; name && name[i] && i < 30; i++ )
		{
			unsigned char c = name[i];
			char escaped[7];
			size_t length = 1;
			escaped[0] = (char) c;
			if ( c == '"' || c == '\\' )
			{
				escaped[0] = '\\';
				escaped[1] = (char) c;
				length = 2;
			}
			else if ( c < 32 )
			{
				snprintf ( escaped, sizeof escaped, "\\u%04x", c );
				length = 6;
			}
			if ( used + length >= capacity )
				return 0;
			memcpy ( out + used, escaped, length );
			used += length;
		}
		const RelayState *state = RELAY_GetState ( port );
		if ( port >= 21 && port <= 23 && state && state->pwm_valid )
			n = snprintf ( number, sizeof number, "\",%u]", state->pwm_percent );
		else
			n = snprintf ( number, sizeof number, "\",null]" );
		if ( n < 0 || used + (size_t) n >= capacity )
			return 0;
		memcpy ( out + used, number, (size_t) n );
		used += (size_t) n;
	}
	if ( used + 3 > capacity )
		return 0;
	memcpy ( out + used, "]}", 3 );
	return used + 2;
}
