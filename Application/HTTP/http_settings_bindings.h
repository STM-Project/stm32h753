#ifndef HTTP_SETTINGS_BINDINGS_H
#define HTTP_SETTINGS_BINDINGS_H

#include "http_settings_catalog.h"

/* Restore explicitly saved, supported settings after the existing device
 * defaults and the settings model have been initialized. Untouched catalog
 * defaults never replace the device configuration. Registers a resolver so
 * untouched WiFi/SMTP/clock fields show the current nonsecret configuration.
 * The saved date is not a real-time clock and is not reapplied during boot.
 */
void HTTP_SettingsBindingsApply(void);

/* Call only after HTTP_SETTINGS_SAVED. Network settings update the existing
 * configuration for the next connection/reboot; this does not interrupt the
 * HTTP response. /czas sets the software clock only on an explicit save.
 * Other Sandwitch settings remain available through HTTP_SettingsGet* until
 * their hardware/control-loop adapters are implemented.
 */
void HTTP_SettingsBindingsCommitted(const HTTP_SettingPage *page, uint16_t instance);

#endif
