#ifndef HTTP_SENSOR_PAGE_H_
#define HTTP_SENSOR_PAGE_H_

#include <stddef.h>

/* Render the complete Sandwitch-style settings page for physical input 1..20.
 * humidity_enabled adds the humidity card next to the temperature card.
 * Returns the HTML length, excluding NUL. Invalid arguments or insufficient
 * capacity return zero and leave an empty string when an output is available.
 * notice is optional plain text and is HTML-escaped before inclusion.
 * Numeric fields retain the original 50px width, but maxlength is 7 rather
 * than Sandwitch's 5 to accommodate 100.0% and all signed int16_t tenths.
 */
size_t HTTP_SensorPageRender(char *out, size_t capacity, unsigned port,
                           int humidity_enabled, const char *notice);

#endif /* HTTP_SENSOR_PAGE_H_ */
