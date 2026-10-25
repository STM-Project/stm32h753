/* Typed view of committed settings. Included by http_settings_model.c. */
#include <stddef.h>

typedef enum
{
	ALL_TEXT,
	ALL_UINT,
	ALL_BYTE,
	ALL_BOOL,
	ALL_DECIMAL,
	ALL_DURATION,
	ALL_TIME,
	ALL_IPV4,
	ALL_MAC
} SettingsAllFormat;

typedef struct
{
	uint16_t page, field, instances;
	size_t stride, offset, capacity;
	SettingsAllFormat format;
} SettingsAllBinding;

HTTP_AllSettings HTTP_AllSettingsData SETTINGS_SDRAM;
static int settingsAllReady;

#include "http_all_settings_bindings.inc"

static void SettingsAllEnsureInitialized ( void )
{
	if ( settingsAllReady )
		return;
	memset ( &HTTP_AllSettingsData, 0, sizeof HTTP_AllSettingsData );
	for ( unsigned port = 0; port < HTTP_SENSOR_PORT_COUNT; ++port )
	{
		strcpy ( HTTP_AllSettingsData.sensors[port][0].name, "Temperatura" );
		strcpy ( HTTP_AllSettingsData.sensors[port][1].name, "Wilgotnosc" );
	}
	settingsAllReady = 1;
}

static void SettingsAllAssign ( void *destination, size_t capacity,
								SettingsAllFormat format, const char *value )
{
	uint32_t number = 0;
	if ( !value )
		value = "";
	if ( format == ALL_TEXT )
	{
		size_t length = strlen ( value );
		if ( length >= capacity )
			length = capacity - 1u;
		memset ( destination, 0, capacity );
		memcpy ( destination, value, length );
	}
	else if ( format == ALL_TIME )
	{
		HTTP_SettingsTime time = { 0, 0, 0 };
		if ( valid_time ( value, 0 ) )
		{
			time.hour = (uint8_t) strtoul ( value, NULL, 10 );
			time.minute = (uint8_t) strtoul ( value + 3, NULL, 10 );
			if ( strlen ( value ) == 8u )
				time.second = (uint8_t) strtoul ( value + 6, NULL, 10 );
		}
		memcpy ( destination, &time, sizeof time );
	}
	else if ( format == ALL_DECIMAL )
	{
		int64_t decimal = 0;
		(void) decimal_value ( value, 1, &decimal );
		memcpy ( destination, &decimal, sizeof decimal );
	}
	else if ( format == ALL_MAC )
	{
		uint64_t mac = 0;
		for ( const char *p = value; *p; ++p )
		{
			int digit = hex_digit ( (unsigned char) *p );
			if ( digit >= 0 )
				mac = ( mac << 4 ) | (unsigned) digit;
		}
		memcpy ( destination, &mac, sizeof mac );
	}
	else if ( format == ALL_BOOL || format == ALL_BYTE )
	{
		uint8_t byte = format == ALL_BOOL ? (uint8_t) ( strcmp ( value, "1" ) == 0 || strcmp ( value, "tak" ) == 0 ) : (uint8_t) strtoul ( value, NULL, 10 );
		memcpy ( destination, &byte, sizeof byte );
	}
	else
	{
		if ( format == ALL_DURATION )
		{
			const char *colon = strchr ( value, ':' );
			if ( colon )
				number = (uint32_t) strtoul ( value, NULL, 10 ) * 60u +
						 (uint32_t) strtoul ( colon + 1, NULL, 10 );
		}
		else if ( format == ALL_IPV4 )
		{
			const char *p = value;
			for ( unsigned part = 0; part < 4 && *p; ++part )
			{
				char *end;
				number = ( number << 8 ) | (uint32_t) strtoul ( p, &end, 10 );
				if ( *end != '.' )
					break;
				p = end + 1;
			}
		}
		else
			number = (uint32_t) strtoul ( value, NULL, 10 );
		memcpy ( destination, &number, sizeof number );
	}
}

static void SettingsRefreshAll ( void )
{
	SettingsAllEnsureInitialized ();
	for ( size_t i = 0; i < sizeof settingsAllBindings / sizeof settingsAllBindings[0]; ++i )
	{
		const SettingsAllBinding *binding = &settingsAllBindings[i];
		const HTTP_SettingPage *page = HTTP_SettingsCatalogAt ( binding->page );
		for ( unsigned index = 0; index < ( binding->instances ? binding->instances : 1u ); ++index )
		{
			uint16_t instance = binding->instances ? (uint16_t) ( index + 1u ) : 0u;
			void *destination = (unsigned char *) &HTTP_AllSettingsData + binding->offset + index * binding->stride;
			SettingsAllAssign ( destination, binding->capacity, binding->format,
								HTTP_SettingsGet ( page, instance, &page->fields[binding->field] ) );
		}
	}
	++HTTP_AllSettingsData.revision;
}

const HTTP_AllSettings *HTTP_SettingsGetAll ( void )
{
	if ( !settingsAllReady )
		SettingsRefreshAll ();
	return &HTTP_AllSettingsData;
}

void HTTP_SettingsSensorsUpdate ( const HttpSensorSettings *values, size_t count )
{
	if ( !values || count != HTTP_SENSOR_COUNT )
		return;
	if ( !settingsAllReady )
		SettingsRefreshAll ();
	memcpy ( HTTP_AllSettingsData.sensors, values, sizeof HTTP_AllSettingsData.sensors );
	++HTTP_AllSettingsData.revision;
}
