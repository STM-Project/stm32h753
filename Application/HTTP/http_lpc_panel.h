#ifndef HTTP_LPC_PANEL_H
#define HTTP_LPC_PANEL_H
#include <stddef.h>
#include <stdint.h>

#define HTTP_LPC_TEMPERATURE_COUNT 3u
#define HTTP_LPC_INPUT_COUNT 7u
#define HTTP_LPC_OUTPUT_COUNT 4u
/* Logical ports, not STM32 GPIO numbers. Default: temperatures 1..3,
 * inputs 4..10, outputs 28..31 (PK1..4). */
extern const uint8_t HTTP_LpcTemperaturePorts[HTTP_LPC_TEMPERATURE_COUNT];
extern const uint8_t HTTP_LpcInputPorts[HTTP_LPC_INPUT_COUNT];
extern const uint8_t HTTP_LpcOutputPorts[HTTP_LPC_OUTPUT_COUNT];

typedef struct {
    int16_t temperature_tenths[HTTP_LPC_TEMPERATURE_COUNT];
    uint8_t temperature_valid[HTTP_LPC_TEMPERATURE_COUNT];
    uint8_t input_on[HTTP_LPC_INPUT_COUNT];
    uint8_t input_valid[HTTP_LPC_INPUT_COUNT];
} HTTP_LpcMeasurements;
extern HTTP_LpcMeasurements HTTP_LpcMeasurementsData;
/* Publish from the owner task, or synchronize externally with the HTTP task.
 * valid=0 explicitly clears a disconnected/failed measurement. No SD writes.
 * Temperatures are raw tenths of deg C; the saved correction is applied once
 * while rendering. Accept only physical temperatures -55..150 deg C. */
int HTTP_LpcSetTemperature(unsigned port, int tenths, int valid);
int HTTP_LpcSetInput(unsigned port, int on, int valid);
void HTTP_LpcResetMeasurements(void);
size_t HTTP_LpcPanelJson(char *out, size_t capacity);
/* Live /nowa source: the same fixed-width snapshot as /TME.txt.
 * Temperatures are already display values; do not apply correction twice.
 * Inputs use the legacy active-low encoding (0/2=ON, 1/3=OFF).
 * Missing/malformed fields become null, never the previous measurement. */
int HTTP_LpcOutputStateFromRefresh(const char *refresh, size_t length, unsigned port);
size_t HTTP_LpcPanelJsonFromRefresh(char *out, size_t capacity,
                                  const char *refresh, size_t length);

typedef struct { const char *path, *data, *content_type; size_t length; } HTTP_LpcAsset;
const HTTP_LpcAsset *HTTP_LpcFindAsset(const char *path, size_t length);
#endif
