#ifndef HTTP_SETTINGS_MODEL_H
#define HTTP_SETTINGS_MODEL_H

#include "http_settings_catalog.h"

/* Sparse settings: untouched defaults remain in the read-only catalog. The buffers live
 * in external SDRAM on ARM and must be initialized after SDRAM startup.
 * Only the single HTTP task may call this API. No credential is ever logged.
 */
#define HTTP_SETTINGS_VALUE_MAX 128u
#define HTTP_SETTINGS_FIELD_NAME_MAX 95u
#define HTTP_SETTINGS_ENTRY_MAX 4096u
#define HTTP_SETTINGS_FIELDS_MAX 4096u
#define HTTP_SETTINGS_TARGET_MAX 65535u
#define HTTP_SETTINGS_RECORD_SIZE ( 8u + HTTP_SETTINGS_VALUE_MAX + 1u )
#define HTTP_SETTINGS_BLOB_SIZE ( 24u + HTTP_SETTINGS_ENTRY_MAX * HTTP_SETTINGS_RECORD_SIZE )

typedef int ( *HTTP_SettingsLoad ) ( void *buffer, size_t length, void *context );
typedef int ( *HTTP_SettingsSave ) ( const void *buffer, size_t length, void *context );
/* Optional runtime defaults (for example existing WiFi configuration). Return
 * NULL to use the catalog; invalid values also fall back to catalog defaults.
 * The returned string must survive at least until the next resolver call.
 * No runtime default is written to storage until explicitly submitted.
 */
typedef const char *( *HTTP_SettingsDefaultResolver ) ( const HTTP_SettingPage *page,
														uint16_t instance, const HTTP_SettingField *field, void *context );

enum
{
	HTTP_SETTINGS_INVALID = -1,
	HTTP_SETTINGS_STORAGE_ERROR = -2,
	HTTP_SETTINGS_CAPACITY_ERROR = -3,
	HTTP_SETTINGS_NOT_HANDLED = 0,
	HTTP_SETTINGS_SAVED = 1
};

/* Load/save callbacks use exactly HTTP_SETTINGS_BLOB_SIZE bytes. Load returns
 * 1=loaded, 0=missing, negative=I/O error. Save returns 1 only after a durable
 * complete commit. Failed validation/storage leaves all RAM values unchanged.
 */
int HTTP_SettingsModelInit ( HTTP_SettingsLoad load, HTTP_SettingsSave save, void *context );
int HTTP_SettingsModelValidateBlob ( const void *buffer, size_t length );
/* The resolver survives Init/reload. Pass NULL to remove it. */
void HTTP_SettingsSetDefaultResolver ( HTTP_SettingsDefaultResolver resolver, void *context );

/* Field pointer must belong to page->fields. Unknown arguments return NULL.
 * Stored/catalog values remain valid until the next successful save/init.
 * Runtime default pointers may expire at the next call to the resolver.
 */
const char *HTTP_SettingsGet ( const HTTP_SettingPage *page, uint16_t instance,
							   const HTTP_SettingField *field );
const char *HTTP_SettingsGetByName ( const HTTP_SettingPage *page, uint16_t instance,
									 const char *name );
/* Explicitly submitted values only; NULL for an untouched field. An intentional
 * empty string/default is retained so runtime bindings can distinguish it.
 * Secret fields are the exception: an empty submission retains the prior secret.
 */
const char *HTTP_SettingsGetSaved ( const HTTP_SettingPage *page, uint16_t instance,
									const char *name );
/* Common n_s_PK_* output labels, n_s_WE_* input labels/delays and the SMS/mail
 * daily report time share the same stored key across their source pages.
 * Other fields and distinct instances, including logic banks, stay separate.
 */

/* Parse a normalized settings target (POST body joined to the CGI path by
 * http_request_stream, or a legacy GET target) containing h7_page=<expanded route>, h7_form=<id>
 * and a recognized submit button. Decode %xx/+, reject unknown/duplicate fields,
 * validate the full form, and clear absent checkboxes only in that form.
 * An empty secret retains the prior value; nonempty secrets replace it.
 * page/instance receive the recognized source page even on a validation error.
 */
int HTTP_SettingsApplyTarget ( const char *target, size_t length,
							   const HTTP_SettingPage **page, uint16_t *instance );

#endif
