#ifndef GOOGLE_CLOUD_H
#define GOOGLE_CLOUD_H
#include <stdint.h>
typedef struct {
    uint32_t successes, failures;
    int last_error; /* -1 config, -2 clock, -3 TLS INIT, -4 timeout, -5 AT,
                    * -6 HTTP/redirect, -7 nonce/reply, -8 random, -18 sensor. */
    int temperature10;
    int demo_relay; /* Google checkbox; RAM only, no implicit GPIO writes. */
    int configured, ready;
} GoogleCloudStatus;
extern volatile GoogleCloudStatus googleCloudStatus;
/* Override if measurements are published elsewhere; default reads HttpRefr_sdram.
 * Return -32768 if no valid measurement is available. */
int GoogleCloud_ReadTemperature10(void);
#endif
