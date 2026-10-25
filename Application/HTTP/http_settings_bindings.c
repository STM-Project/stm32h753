#include "http_settings_bindings.h"
#include "http_settings_model.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "variables.h"

/* AT+CWMODE uses independent STA=1 and AP=2 bits. These local constants mirror
 * the existing esp32wroom.c enum without exposing its private state machine.
 */
#define SETTINGS_WIFI_STA 1u
#define SETTINGS_WIFI_AP 2u

static unsigned sta_profile ( void )
{
	return Const.wifiGeneral.nrSTA >= 0 && Const.wifiGeneral.nrSTA < WIFI_STA_MAX ? (unsigned) Const.wifiGeneral.nrSTA : 0u;
}

static unsigned ap_profile ( void )
{
	return Const.wifiGeneral.nrAP >= 0 && Const.wifiGeneral.nrAP < WIFI_AP_MAX ? (unsigned) Const.wifiGeneral.nrAP : 0u;
}

static const char *format_unsigned ( unsigned value )
{
	static char buffer[16];
	snprintf ( buffer, sizeof buffer, "%u", value );
	return buffer;
}

static const char *format_ipv4 ( uint32_t value )
{
	static char buffer[16];
	snprintf ( buffer, sizeof buffer, "%u.%u.%u.%u", (unsigned) ( value >> 24 ),
			   (unsigned) ( ( value >> 16 ) & 255u ), (unsigned) ( ( value >> 8 ) & 255u ),
			   (unsigned) ( value & 255u ) );
	return buffer;
}

static const char *format_mac ( uint64_t value )
{
	static char buffer[13];
	snprintf ( buffer, sizeof buffer, "%02X%02X%02X%02X%02X%02X",
			   (unsigned) ( ( value >> 40 ) & 255u ), (unsigned) ( ( value >> 32 ) & 255u ),
			   (unsigned) ( ( value >> 24 ) & 255u ), (unsigned) ( ( value >> 16 ) & 255u ),
			   (unsigned) ( ( value >> 8 ) & 255u ), (unsigned) ( value & 255u ) );
	return buffer;
}

/* Initial forms show the existing H7 configuration without making it a saved
 * record. Only nonsecret values are exposed. The model validates each result
 * against the field schema before rendering or using it as a default.
 */
static const char *runtime_default ( const HTTP_SettingPage *page, uint16_t instance,
									 const HTTP_SettingField *field, void *context )
{
	const char *name;
	(void) context;
	if ( !page || !field || instance != 0 || field->kind == HTTP_SETTING_SECRET )
		return NULL;
	name = field->name;
	if ( strcmp ( page->route, "/wifi" ) == 0 )
	{
		const s_wifi_STA *sta = &Const.wifiSTA[sta_profile ()];
		const s_wifi_AP *ap = &Const.wifiAP[ap_profile ()];
		if ( strcmp ( name, "n_s_Wifi_ssid_001" ) == 0 )
			return sta->name;
		if ( strcmp ( name, "n_s_Wifi_ssid_002" ) == 0 )
			return ap->name;
		if ( strcmp ( name, "n_s_Wifi_par1_001" ) == 0 )
			return ( Const.wifiGeneral.mode & SETTINGS_WIFI_STA ) ? "tak" : "nie";
		if ( strcmp ( name, "n_s_Wifi_par1_002" ) == 0 )
			return ( Const.wifiGeneral.mode & SETTINGS_WIFI_AP ) ? "tak" : "nie";
		if ( strcmp ( name, "n_s_Lan_mac_002" ) == 0 )
			return format_mac ( sta->mac );
		if ( strcmp ( name, "n_s_Lan_ip_002" ) == 0 )
			return format_ipv4 ( sta->ip );
		if ( strcmp ( name, "n_s_Lan_mask_002" ) == 0 )
			return format_ipv4 ( sta->mask );
		if ( strcmp ( name, "n_s_Lan_br_002" ) == 0 )
			return format_ipv4 ( sta->gate );
		if ( strcmp ( name, "n_s_Lan_param_002" ) == 0 )
			return sta->dhcp ? "tak" : "nie";
		if ( strcmp ( name, "n_s_Lan_port_002" ) == 0 )
			return format_unsigned ( ( Const.wifiGeneral.mode & SETTINGS_WIFI_STA ) ? sta->port : ap->port );
	}
	else if ( strcmp ( page->route, "/maskimail" ) == 0 )
	{
		unsigned i;
		if ( strcmp ( name, "n_s_email_ip_000" ) == 0 )
			return Const.emailSend[0].server;
		if ( strcmp ( name, "n_s_email_nad_000" ) == 0 )
			return Const.emailSend[0].login;
		if ( strcmp ( name, "n_s_email_port_000" ) == 0 )
			return format_unsigned ( Const.emailSend[0].port );
		for ( i = 0; i < MAX_EMAIL_RECIPIENTS && i < 8u; ++i )
		{
			char recipient[] = "n_s_email_odb1_000";
			recipient[13] = (char) ( '1' + i );
			if ( strcmp ( name, recipient ) == 0 )
				return Const.emailRecv[i].email;
		}
	}
	else if ( strcmp ( page->route, "/czas" ) == 0 && sntpTime &&
			  sntpTime->tm_year >= 100 && sntpTime->tm_year <= 199 &&
			  sntpTime->tm_mon >= 0 && sntpTime->tm_mon <= 11 &&
			  sntpTime->tm_mday >= 1 && sntpTime->tm_mday <= 31 &&
			  sntpTime->tm_hour >= 0 && sntpTime->tm_hour <= 23 &&
			  sntpTime->tm_min >= 0 && sntpTime->tm_min <= 59 &&
			  sntpTime->tm_sec >= 0 && sntpTime->tm_sec <= 59 )
	{
		if ( strcmp ( name, "n_s_Czas_rok_000" ) == 0 )
			return format_unsigned ( (unsigned) sntpTime->tm_year - 100u );
		if ( strcmp ( name, "n_s_Czas_mies_000" ) == 0 )
			return format_unsigned ( (unsigned) sntpTime->tm_mon + 1u );
		if ( strcmp ( name, "n_s_Czas_dzien_000" ) == 0 )
			return format_unsigned ( (unsigned) sntpTime->tm_mday );
		if ( strcmp ( name, "n_s_Czas_godz_000" ) == 0 )
			return format_unsigned ( (unsigned) sntpTime->tm_hour );
		if ( strcmp ( name, "n_s_Czas_min_000" ) == 0 )
			return format_unsigned ( (unsigned) sntpTime->tm_min );
		if ( strcmp ( name, "n_s_Czas_sec_000" ) == 0 )
			return format_unsigned ( (unsigned) sntpTime->tm_sec );
		if ( strcmp ( name, "n_s_Czas_tyg_000" ) == 0 && sntpTime->tm_wday >= 0 && sntpTime->tm_wday <= 6 )
			return format_unsigned ( sntpTime->tm_wday ? (unsigned) sntpTime->tm_wday : 7u );
	}
	return NULL;
}

static const char *saved ( const HTTP_SettingPage *page, const char *name )
{
	return HTTP_SettingsGetSaved ( page, 0, name );
}

static void copy_saved ( const HTTP_SettingPage *page, const char *name,
						 char *destination, size_t capacity )
{
	const char *value = saved ( page, name );
	size_t length;
	if ( !value || !capacity )
		return;
	length = strlen ( value );
	if ( length >= capacity )
		length = capacity - 1u;
	memcpy ( destination, value, length );
	destination[length] = '\0';
}

static int unsigned_value ( const char *value, uint32_t limit, uint32_t *result )
{
	uint32_t number = 0;
	if ( !value || !*value )
		return 0;
	while ( *value )
	{
		unsigned digit = (unsigned char) *value++ - (unsigned) '0';
		if ( digit > 9u || number > limit / 10u ||
			 ( number == limit / 10u && digit > limit % 10u ) )
			return 0;
		number = number * 10u + digit;
	}
	*result = number;
	return 1;
}

/* Match IPDOT/IP2Str: 192.168.1.2 is 0xc0a80102 on either host endian. */
static int ipv4_value ( const char *value, uint32_t *result )
{
	uint32_t address = 0;
	unsigned part;
	if ( !value )
		return 0;
	for ( part = 0; part < 4u; ++part )
	{
		unsigned number = 0, digits = 0;
		while ( *value >= '0' && *value <= '9' )
		{
			if ( ++digits > 3u )
				return 0;
			number = number * 10u + (unsigned) ( *value++ - '0' );
		}
		if ( !digits || number > 255u )
			return 0;
		address = ( address << 8 ) | number;
		if ( part != 3u )
		{
			if ( *value++ != '.' )
				return 0;
		}
		else if ( *value )
			return 0;
	}
	*result = address;
	return 1;
}

static int hex_digit ( unsigned char c )
{
	if ( c >= '0' && c <= '9' )
		return c - '0';
	if ( c >= 'a' && c <= 'f' )
		return c - 'a' + 10;
	if ( c >= 'A' && c <= 'F' )
		return c - 'A' + 10;
	return -1;
}

/* Sandwitch uses 12 hex digits; also accept the catalog's colon/hyphen form.
 * The most significant octet matches MACStr2Int64 and the ESP AT formatter.
 */
static int mac_value ( const char *value, uint64_t *result )
{
	uint64_t mac = 0;
	unsigned byte;
	size_t length;
	char separator;
	if ( !value )
		return 0;
	length = strlen ( value );
	if ( length != 12u && length != 17u )
		return 0;
	separator = length == 17u ? value[2] : '\0';
	if ( separator && separator != ':' && separator != '-' )
		return 0;
	for ( byte = 0; byte < 6u; ++byte )
	{
		int high = hex_digit ( (unsigned char) *value++ );
		int low = hex_digit ( (unsigned char) *value++ );
		if ( high < 0 || low < 0 )
			return 0;
		mac = ( mac << 8 ) | (unsigned) ( ( high << 4 ) | low );
		if ( separator && byte < 5u && *value++ != separator )
			return 0;
	}
	if ( *value )
		return 0;
	*result = mac;
	return 1;
}

static void apply_wifi ( const HTTP_SettingPage *page )
{
	s_wifi_STA *sta = &Const.wifiSTA[sta_profile ()];
	s_wifi_AP *ap = &Const.wifiAP[ap_profile ()];
	const char *value;
	uint32_t number;
	uint64_t mac;
	copy_saved ( page, "n_s_Wifi_ssid_001", sta->name, sizeof sta->name );
	copy_saved ( page, "n_s_Wifi_haslo_001", sta->pass, sizeof sta->pass );
	copy_saved ( page, "n_s_Wifi_ssid_002", ap->name, sizeof ap->name );
	copy_saved ( page, "n_s_Wifi_haslo_002", ap->pass, sizeof ap->pass );
	value = saved ( page, "n_s_Wifi_par1_001" );
	if ( value )
	{
		if ( strcmp ( value, "tak" ) == 0 )
			Const.wifiGeneral.mode |= SETTINGS_WIFI_STA;
		else if ( strcmp ( value, "nie" ) == 0 )
			Const.wifiGeneral.mode &= (uint8_t) ~SETTINGS_WIFI_STA;
	}
	value = saved ( page, "n_s_Wifi_par1_002" );
	if ( value )
	{
		if ( strcmp ( value, "tak" ) == 0 )
			Const.wifiGeneral.mode |= SETTINGS_WIFI_AP;
		else if ( strcmp ( value, "nie" ) == 0 )
			Const.wifiGeneral.mode &= (uint8_t) ~SETTINGS_WIFI_AP;
	}
	if ( ipv4_value ( saved ( page, "n_s_Lan_ip_002" ), &number ) )
		sta->ip = number;
	if ( ipv4_value ( saved ( page, "n_s_Lan_mask_002" ), &number ) )
		sta->mask = number;
	if ( ipv4_value ( saved ( page, "n_s_Lan_br_002" ), &number ) )
		sta->gate = number;
	if ( mac_value ( saved ( page, "n_s_Lan_mac_002" ), &mac ) )
		sta->mac = mac;
	value = saved ( page, "n_s_Lan_param_002" );
	if ( value )
	{
		if ( strcmp ( value, "tak" ) == 0 )
			sta->dhcp = 1;
		else if ( strcmp ( value, "nie" ) == 0 )
			sta->dhcp = 0;
	}
	if ( unsigned_value ( saved ( page, "n_s_Lan_port_002" ), UINT16_MAX, &number ) && number )
	{
		/* Both interfaces share the single ESP HTTP listener. */
		sta->port = (uint16_t) number;
		ap->port = (uint16_t) number;
	}
}

static void apply_email ( const HTTP_SettingPage *page )
{
	unsigned i;
	uint32_t number;
	const char *server = saved ( page, "n_s_email_ip_000" );
	s_smtp_sender *sender = &Const.emailSend[0];
	if ( server )
	{
		copy_saved ( page, "n_s_email_ip_000", sender->server, sizeof sender->server );
		/* A hostname is resolved by the existing startup DNS sequence. Never
         * keep the previous server's resolved address after changing it.
         */
		sender->IP = ipv4_value ( server, &number ) ? number : 0;
	}
	copy_saved ( page, "n_s_email_nad_000", sender->login, sizeof sender->login );
	copy_saved ( page, "n_s_email_has_000", sender->password, sizeof sender->password );
	if ( unsigned_value ( saved ( page, "n_s_email_port_000" ), UINT16_MAX, &number ) && number )
		sender->port = (uint16_t) number;
	for ( i = 0; i < MAX_EMAIL_RECIPIENTS && i < 8u; ++i )
	{
		char name[] = "n_s_email_odb1_000";
		name[13] = (char) ( '1' + i );
		copy_saved ( page, name, Const.emailRecv[i].email, sizeof Const.emailRecv[i].email );
	}
}

static void apply_clock ( const HTTP_SettingPage *page )
{
	static const char *const names[] = {
		"n_s_Czas_rok_000", "n_s_Czas_mies_000", "n_s_Czas_dzien_000",
		"n_s_Czas_godz_000", "n_s_Czas_min_000", "n_s_Czas_sec_000" };
	static const uint32_t maxima[] = { 99, 12, 31, 23, 59, 59 };
	uint32_t values[6];
	struct tm clock = { 0 };
	time_t epoch;
	unsigned i;
	for ( i = 0; i < 6u; ++i )
		if ( !unsigned_value ( saved ( page, names[i] ), maxima[i], &values[i] ) )
			return;
	if ( !values[1] || !values[2] )
		return;
	clock.tm_year = (int) values[0] + 100;
	clock.tm_mon = (int) values[1] - 1;
	clock.tm_mday = (int) values[2];
	clock.tm_hour = (int) values[3];
	clock.tm_min = (int) values[4];
	clock.tm_sec = (int) values[5];
	clock.tm_isdst = -1;
	epoch = mktime ( &clock );
	/* Const.sntp.time is an int in the existing runtime. Refuse a normalized
     * invalid date or an unrepresentable epoch rather than wrapping it.
     */
	if ( epoch == (time_t) -1 || epoch < 0 || (uintmax_t) epoch > (uintmax_t) INT_MAX ||
		 clock.tm_year != (int) values[0] + 100 || clock.tm_mon != (int) values[1] - 1 ||
		 clock.tm_mday != (int) values[2] || clock.tm_hour != (int) values[3] ||
		 clock.tm_min != (int) values[4] || clock.tm_sec != (int) values[5] )
		return;
	Const.sntp.time = (int) epoch;
	if ( sntpTime )
		*sntpTime = clock;
}

void HTTP_SettingsBindingsApply ( void )
{
	uint16_t instance;
	const HTTP_SettingPage *page = HTTP_SettingsCatalogFind ( "/wifi", &instance );
	if ( page )
		apply_wifi ( page );
	page = HTTP_SettingsCatalogFind ( "/maskimail", &instance );
	if ( page )
		apply_email ( page );
	HTTP_SettingsSetDefaultResolver ( runtime_default, NULL );
}

void HTTP_SettingsBindingsCommitted ( const HTTP_SettingPage *page, uint16_t instance )
{
	if ( !page || instance != 0u )
		return;
	if ( strcmp ( page->route, "/wifi" ) == 0 )
		apply_wifi ( page );
	else if ( strcmp ( page->route, "/maskimail" ) == 0 )
		apply_email ( page );
	else if ( strcmp ( page->route, "/czas" ) == 0 )
		apply_clock ( page );
}
