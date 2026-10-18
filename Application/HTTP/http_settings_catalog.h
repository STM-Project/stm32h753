#ifndef HTTP_SETTINGS_CATALOG_H
#define HTTP_SETTINGS_CATALOG_H

#include <stddef.h>
#include <stdint.h>

/* Generated schemas contain only source-template defaults, never device credentials. */
typedef enum {
    HTTP_SETTING_TEXT = 0,
    HTTP_SETTING_UINT,
    HTTP_SETTING_DECIMAL,
    HTTP_SETTING_DURATION, /* mm:ss, minutes may exceed 59 */
    HTTP_SETTING_TIME,     /* hh:mm[:ss], hours 0..23 */
    HTTP_SETTING_IPV4,
    HTTP_SETTING_MAC,
    HTTP_SETTING_SELECT,
    HTTP_SETTING_SECRET,
    HTTP_SETTING_READONLY,
    HTTP_SETTING_CHECKBOX
} HTTP_SettingFieldKind;

enum {
    HTTP_SETTING_OPTIONAL = 1u
};

typedef struct {
    const char *name;          /* {{N}} is replaced by a three-digit instance. */
    const char *default_value;
    const char *choices;       /* Allowed select values separated with '|'. */
    uint16_t max_length;
    uint8_t kind;
    uint8_t flags;
    uint8_t form_id;            /* Scope of checkbox reset for multi-form pages. */
    int32_t minimum;
    int32_t maximum;
} HTTP_SettingField;

typedef struct {
    const char *route;         /* Prefix for numbered pages, exact path otherwise. */
    const char *title;
    const char *html;          /* UTF-8 Sandwitch layout; expanded by page renderer. */
    const HTTP_SettingField *fields;
    uint16_t field_count;
    uint16_t max_instance;     /* Zero for a single, unnumbered page. */
} HTTP_SettingPage;

size_t HTTP_SettingsCatalogCount(void);
const HTTP_SettingPage *HTTP_SettingsCatalogAt(size_t index);
const HTTP_SettingPage *HTTP_SettingsCatalogFind(const char *path, uint16_t *instance);
int HTTP_SettingsCatalogFieldName(const HTTP_SettingField *field, uint16_t instance,
                                 char *output, size_t capacity);
int HTTP_SettingsCatalogPath(const HTTP_SettingPage *page, uint16_t instance,
                            char *output, size_t capacity);

#endif
