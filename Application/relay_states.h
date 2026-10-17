#ifndef RELAY_STATES_H
#define RELAY_STATES_H

#include <stdint.h>

#define RELAY_OUTPUT_COUNT 13u

typedef struct {
    uint8_t port;  /* Logical Sandwitch port, not an STM32 GPIO pin number. */
    uint8_t on;    /* 0=OFF, 1=ON; meaningful only when valid==1. */
    uint8_t valid; /* 0=unknown/unconnected, 1=reported by the output driver. */
} RelayState;

typedef struct {
    uint32_t revision;
    /* Same order as HTTP_AllSettings.outputs/timers/thermostats:
     * 28..33 (PK1..6), 24..27 (OC), 21..23 (PWM-capable outputs). */
    RelayState outputs[RELAY_OUTPUT_COUNT];
} RelayStates;

/* Runtime data only: never restored from SD as if it were physical feedback.
 * Treat the global object/pointer as read-only and publish through the functions
 * below. Use a single owner task, or synchronize readers/writers externally.
 * The current project has no relay GPIO mapping: all states start unknown. */
extern RelayStates RelayStatesData;
const RelayStates *RELAY_GetStates(void);

/* Reset reported states to unknown; retains the fixed port mapping. */
void RELAY_ResetStates(void);
/* Report a state from the output driver after applying/reading the output.
 * This function records state; it does not switch GPIO or confirm relay contacts.
 * Returns 1 for an accepted report, 0 for invalid port/state (no modification). */
int RELAY_UpdateState(unsigned port, unsigned on);
/* Mark a channel unknown when its driver/feedback is unavailable. */
int RELAY_InvalidateState(unsigned port);

#endif
