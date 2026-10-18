#include "http_output_control.h"
#include "relay_states.h"
#include <string.h>

int HTTP_OutputControlParse(const char *target, size_t length, HTTP_OutputCommand *command)
{
    static const char pwm[] = "/lpc.cgi/n_pwm";
    static const char save[] = "&n_115_zapisz=";
    static const char relay[] = "/mobile/pk";
    if (!target || !command) return 0;
    *command = (HTTP_OutputCommand){0};
    if (length >= sizeof pwm - 1u && !memcmp(target,pwm,sizeof pwm - 1u)) {
        size_t position = sizeof pwm - 1u, digits = 0;
        unsigned value = 0;
        if (length < position + 3u || target[position] < '1' || target[position] > '3' ||
            target[position + 1u] != '=') return -1;
        unsigned port = 20u + (unsigned)(target[position] - '0');
        position += 2u;
        while (position < length && target[position] >= '0' && target[position] <= '9') {
            if (++digits > 3u) return -1;
            value = value * 10u + (unsigned)(target[position++] - '0');
        }
        if (!digits || value > 100u || length - position != sizeof save - 1u ||
            memcmp(target + position,save,sizeof save - 1u)) return -1;
        *command = (HTTP_OutputCommand){HTTP_OUTPUT_PWM,port,value};
        return 1;
    }
    if (length >= sizeof relay - 1u && !memcmp(target,relay,sizeof relay - 1u)) {
        size_t offset = sizeof relay - 1u;
        unsigned instance = 0;
        if (length != offset + 3u) return -1;
        for (unsigned i = 0; i < 3u; ++i) {
            if (target[offset + i] < '0' || target[offset + i] > '9') return -1;
            instance = instance * 10u + (unsigned)(target[offset + i] - '0');
        }
        if (!instance || instance > RELAY_OUTPUT_COUNT) return -1;
        *command = (HTTP_OutputCommand){HTTP_OUTPUT_TOGGLE,
            RELAY_GetStates()->outputs[instance - 1u].port,0};
        return 1;
    }
    return 0;
}
