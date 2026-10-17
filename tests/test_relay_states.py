#!/usr/bin/env python3
"""Validate unknown/known output states, mapping, isolation and invalid reports."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include "relay_states.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    const RelayStates *states=RELAY_GetStates();
    const unsigned ports[]={28,29,30,31,32,33,24,25,26,27,21,22,23};
    assert(states==&RelayStatesData);
    for(unsigned i=0;i<RELAY_OUTPUT_COUNT;i++) {
        assert(states->outputs[i].port==ports[i] && !states->outputs[i].valid);
        unsigned revision=states->revision;
        assert(RELAY_UpdateState(ports[i],1));
        assert(states->outputs[i].on==1 && states->outputs[i].valid);
        assert(states->revision==revision+1);
        assert(RELAY_UpdateState(ports[i],1));assert(states->revision==revision+1);
        for(unsigned j=i+1;j<RELAY_OUTPUT_COUNT;j++)assert(!states->outputs[j].valid);
    }
    RelayStates saved=*states;
    assert(!RELAY_UpdateState(0,1));assert(!RELAY_UpdateState(20,1));
    assert(!RELAY_UpdateState(34,1));assert(!RELAY_UpdateState(UINT_MAX,1));
    assert(!RELAY_UpdateState(28,2));assert(!RELAY_UpdateState(28,UINT_MAX));
    assert(!RELAY_InvalidateState(0));assert(!memcmp(&saved,states,sizeof saved));
    assert(RELAY_UpdateState(28,0));assert(states->outputs[0].valid && !states->outputs[0].on);
    assert(RELAY_InvalidateState(29));assert(!states->outputs[1].valid && !states->outputs[1].on);
    assert(states->outputs[2].valid && states->outputs[2].on);
    RELAY_ResetStates();assert(RELAY_GetStates()==states);
    for(unsigned i=0;i<RELAY_OUTPUT_COUNT;i++)
        assert(states->outputs[i].port==ports[i] && !states->outputs[i].on && !states->outputs[i].valid);
    assert(RELAY_RequestToggle(28));assert(states->outputs[0].requested_on && states->outputs[0].request_valid);
    assert(!states->outputs[0].valid);assert(RELAY_RequestToggle(28));assert(!states->outputs[0].requested_on);
    assert(RELAY_UpdateState(29,1));assert(RELAY_RequestToggle(29));
    assert(states->outputs[1].valid && states->outputs[1].on && !states->outputs[1].requested_on);
    assert(RELAY_RequestPwm(21,37));const RelayState *pwm=RELAY_GetState(21);
    assert(pwm->pwm_percent==37 && pwm->pwm_valid && pwm->requested_on && !pwm->valid);
    memcpy(&saved,states,sizeof saved);
    assert(!RELAY_RequestPwm(21,101));assert(!RELAY_RequestPwm(24,50));assert(!RELAY_RequestToggle(0));
    assert(!memcmp(&saved,states,sizeof saved));
    RELAY_ResetStates();assert(!pwm->pwm_valid && !pwm->request_valid && !pwm->valid);
    puts("PASS: stable pointer, mapped outputs, feedback/commands, PWM limits, reset and rejected invalid input.");
    return 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="relay-states-") as directory:
        source = Path(directory) / "test.c"
        source.write_text(HARNESS)
        binary = Path(directory) / "test"
        subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie",
                        "-I" + str(ROOT / "Application"), str(source),
                        str(ROOT / "Application/relay_states.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
