#ifndef LPC_PAGES_H
#define LPC_PAGES_H
#include <stddef.h>
#include <stdint.h>
#include "lpc_manifest.h"
#define LPC_VALUE_MAX 256u
#define LPC_BLOB_SIZE ( 16u + LPC_FIELD_COUNT * ( LPC_VALUE_MAX + 2u ) )
/* HTTP uses 2 KiB chunks; the legacy full-page API keeps its size limit. */
#define LPC_STREAM_BUFFER_SIZE ( 2u * 1024u )
#define LPC_PAGE_BUFFER_SIZE ( 768u * 1024u )
#if defined( __arm__ ) || defined( __thumb__ )
#define LPC_SDRAM __attribute__ ( ( section ( ".sdram" ), aligned ( 32 ) ) )
#else
#define LPC_SDRAM __attribute__ ( ( aligned ( 32 ) ) )
#endif
typedef struct
{
	const char *name, *default_value, *choices;
	uint16_t maximum;
	uint8_t kind;
} LpcField;
typedef struct
{
	uint32_t start;
	uint16_t count;
} LpcForm;
typedef struct
{
	const char *title, *source;
	uint16_t first_form, form_count;
} LpcPage;
extern const LpcField LPC_Fields[LPC_FIELD_COUNT];
extern const LpcForm LPC_Forms[];
extern const LpcPage LPC_Pages[LPC_PAGE_COUNT];
extern const uint16_t LPC_Members[];
typedef int ( *LpcLoad ) ( void *, size_t, void * );
typedef int ( *LpcSave ) ( const void *, size_t, void * );
/* Initialize after SDRAM is ready; HTTP task owns reads/writes. */
int LPC_SettingsInit ( LpcLoad load, LpcSave save, void *context );
int LPC_ValidateSettings ( const void *bytes, size_t length );
const char *LPC_GetSetting ( const char *name );	  /* saved or original form default */
const char *LPC_GetSavedSetting ( const char *name ); /* NULL if never set */
size_t LPC_SettingsJson ( unsigned page, char *out, size_t capacity );
size_t LPC_SettingsJsonWindow ( unsigned page, char *out, size_t capacity, size_t offset, size_t *total, uint32_t *hash );
/* 0 unrelated; 200 saved; 400 malformed; 501 unsupported hardware action;
 * 503 storage failure. No RAM commit before successful durable save. */
int LPC_ApplyTarget ( const char *target, size_t length, unsigned *page );
unsigned LPC_ResolvePage ( const char *target, size_t length );
unsigned LPC_ResolveSettingsApi ( const char *target, size_t length );
size_t LPC_CatalogRender ( char *out, size_t capacity );
/* FatFs adapters; do not hold the SD mutex while sending HTTP data. */
int LPC_StorageInit ( void );
const char *LPC_StoragePageError ( void );
size_t LPC_StorageReadPage ( unsigned page, char *out, size_t capacity );
size_t LPC_StoragePageSize ( unsigned page );
size_t LPC_StorageReadPageChunk ( unsigned page, size_t expected, size_t offset, char *out, size_t length );
#endif
