#include "relay_states.h"

RelayStates RelayStatesData = {
    .revision = 0,
    .outputs = {
        {28, 0, 0}, {29, 0, 0}, {30, 0, 0}, {31, 0, 0}, {32, 0, 0}, {33, 0, 0},
        {24, 0, 0}, {25, 0, 0}, {26, 0, 0}, {27, 0, 0},
        {21, 0, 0}, {22, 0, 0}, {23, 0, 0}
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

void RELAY_ResetStates(void)
{
    for (unsigned i = 0; i < RELAY_OUTPUT_COUNT; ++i) {
        RelayStatesData.outputs[i].on = 0;
        RelayStatesData.outputs[i].valid = 0;
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
