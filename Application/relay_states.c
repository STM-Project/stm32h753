#include "relay_states.h"

RelayStates RelayStatesData = {
    .revision = 0,
    .outputs = {
        {.port=28}, {.port=29}, {.port=30}, {.port=31}, {.port=32}, {.port=33},
        {.port=24}, {.port=25}, {.port=26}, {.port=27},
        {.port=21}, {.port=22}, {.port=23}
    }
};

static RelayState *find_output(unsigned port)
{
    for (unsigned i = 0; i < RELAY_OUTPUT_COUNT; ++i)
        if (RelayStatesData.outputs[i].port == port)
            return &RelayStatesData.outputs[i];
    return 0;
}

const RelayStates *RELAY_GetStates(void)
{
    return &RelayStatesData;
}

const RelayState *RELAY_GetState(unsigned port)
{
    return find_output(port);
}

int RELAY_GetDisplayState(unsigned port)
{
    const RelayState *state = find_output(port);
    if (!state) return -1;
    if (state->request_valid) return state->requested_on;
    return state->valid ? state->on : -1;
}

int RELAY_RequestToggle(unsigned port)
{
    RelayState *state = find_output(port);
    if (!state) return 0;
    state->requested_on = RELAY_GetDisplayState(port) > 0 ? 0 : 1;
    state->request_valid = 1;
    ++RelayStatesData.revision;
    return 1;
}

int RELAY_RequestPwm(unsigned port, unsigned percent)
{
    RelayState *state = find_output(port);
    if (!state || port < 21u || port > 23u || percent > 100u) return 0;
    if (!state->pwm_valid || state->pwm_percent != percent || !state->request_valid ||
        state->requested_on != (percent > 0u)) {
        state->pwm_percent = (uint8_t)percent;
        state->pwm_valid = 1;
        state->requested_on = percent > 0u;
        state->request_valid = 1;
        ++RelayStatesData.revision;
    }
    return 1;
}

void RELAY_ResetStates(void)
{
    for (unsigned i = 0; i < RELAY_OUTPUT_COUNT; ++i) {
        unsigned port = RelayStatesData.outputs[i].port;
        RelayStatesData.outputs[i] = (RelayState){.port=(uint8_t)port};
    }
    ++RelayStatesData.revision;
}

int RELAY_UpdateState(unsigned port, unsigned on)
{
    RelayState *state = find_output(port);
    if (!state || on > 1u) return 0;
    if (!state->valid || state->on != on) {
        state->on = (uint8_t)on;
        state->valid = 1;
        ++RelayStatesData.revision;
    }
    return 1;
}

int RELAY_InvalidateState(unsigned port)
{
    RelayState *state = find_output(port);
    if (!state) return 0;
    if (state->valid) {
        state->on = 0;
        state->valid = 0;
        ++RelayStatesData.revision;
    }
    return 1;
}
