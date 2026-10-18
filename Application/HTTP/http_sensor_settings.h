#ifndef HTTP_SENSOR_SETTINGS_H
#define HTTP_SENSOR_SETTINGS_H

#include <stddef.h>
#include <stdint.h>

#define HTTP_SENSOR_PORT_COUNT 20u
#define HTTP_SENSOR_COUNT (HTTP_SENSOR_PORT_COUNT * 2u)
#define HTTP_SENSOR_NAME_LENGTH 30u
#define HTTP_SENSOR_TARGET_MAX 2048u
/* Versioned, padding-independent representation; storage adapters use these bytes. */
#define HTTP_SENSOR_SETTINGS_BLOB_SIZE (16u + HTTP_SENSOR_COUNT * 45u)

typedef struct {
    char name[HTTP_SENSOR_NAME_LENGTH + 1u];
    /* Temperature is in tenths of a degree, humidity in tenths of a percent. */
    int16_t max1;
    int16_t max2;
    int16_t min1;
    int16_t min2;
    int16_t hysteresis;
    int16_t correction;
    uint16_t delay_seconds;
} HttpSensorSettings;

/* Return 1 on success, 0 for an absent file, negative for an I/O failure.
 * Save must return 1 only once its complete byte buffer is durably committed.
 * Call from the single HTTP task, not concurrently with settings access.
 */
typedef int (*HttpSensorSettingsLoad)(void *buffer, size_t length, void *context);
typedef int (*HttpSensorSettingsSave)(const void *buffer, size_t length, void *context);

enum {
    HTTP_SENSOR_SETTINGS_INVALID = -1,
    HTTP_SENSOR_SETTINGS_STORAGE_ERROR = -2,
    HTTP_SENSOR_SETTINGS_NOT_HANDLED = 0,
    HTTP_SENSOR_SETTINGS_SAVED = 1
};

/* Initializes defaults and optionally loads saved settings. 1=loaded, 0=defaults,
 * -1=invalid saved data/I/O (defaults retained). No persistence callback means
 * subsequent save attempts fail explicitly rather than silently losing data.
 */
int HTTP_SensorSettingsInit(HttpSensorSettingsLoad load,
                           HttpSensorSettingsSave save, void *context);
const HttpSensorSettings *HTTP_SensorSettingsGet(unsigned port, int humidity);
int HTTP_SensorSettingsValidateBlob(const void *buffer, size_t length);

/* target is the request target only (without GET or HTTP/version). Its length
 * is explicit: it need not be NUL terminated. /lpc.cgi/A117 sensor forms are
 * parsed strictly. All 8 fields for each included sensor and n_109_zapisz are
 * required. Either temperature alone or both sensors of one port may be saved.
 * Unknown, duplicate, truncated and invalid fields reject the whole update.
 * Other CGI forms return NOT_HANDLED. port receives the recognized port for
 * either a valid submission or an error encountered after its first sensor ID.
 */
int HTTP_SensorSettingsApplyTarget(const char *target, size_t length, unsigned *port);

#endif
