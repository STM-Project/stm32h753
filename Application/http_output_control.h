#ifndef HTTP_OUTPUT_CONTROL_H
#define HTTP_OUTPUT_CONTROL_H
#include <stddef.h>
typedef enum { HTTP_OUTPUT_TOGGLE, HTTP_OUTPUT_PWM } HTTP_OutputCommandKind;
typedef struct {
    HTTP_OutputCommandKind kind;
    unsigned port;
    unsigned value;
} HTTP_OutputCommand;
/* Parse exactly the GET targets emitted by the existing button/PWM scripts.
 * 1=valid command, -1=malformed command, 0=unrelated route. Does not modify state.
 * The caller must check the configured port mode and reserve a response slot. */
int HTTP_OutputControlParse(const char *target, size_t length, HTTP_OutputCommand *command);
#endif
