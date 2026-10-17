#ifndef HTTP_SETTINGS_PAGE_H_
#define HTTP_SETTINGS_PAGE_H_

#include <stddef.h>
#include <stdint.h>
#include "http_settings_catalog.h"

/* Complete Sandwitch settings page, one persistent POST form per settings route.
 * Informational/action-only pages have no save control; /set is rendered by the
 * existing per-port HTTP_TEMPLATE_2_* functions and is deliberately rejected.
 * Values and notices are escaped, passwords stay empty to preserve saved values;
 * the shared style and menu are reused.
 * Returns HTML bytes excluding NUL, or zero with an empty output on failure.
 * The caller owns the output buffer (128 KiB covers the largest matrix).
 */
size_t HTTP_SettingsPageRender(char *out, size_t capacity,
                               const HTTP_SettingPage *page,
                               uint16_t instance, const char *notice);

#endif
