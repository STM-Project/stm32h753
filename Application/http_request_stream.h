/* Incremental ESP-AT +IPD framing and HTTP request-line decoder.
 * This module has no STM32, RTOS, allocation, or socket dependencies.
 */
#ifndef HTTP_REQUEST_STREAM_H_
#define HTTP_REQUEST_STREAM_H_

#include <stddef.h>
#include <stdint.h>

#ifndef HTTP_REQUEST_STREAM_CHANNELS
#define HTTP_REQUEST_STREAM_CHANNELS 5u
#endif

/* Encoded origin-form target, including the path and query string. */
#ifndef HTTP_REQUEST_STREAM_MAX_TARGET
#define HTTP_REQUEST_STREAM_MAX_TARGET 3072u
#endif

/* ESP-AT reports at most 65535 bytes in a normal +IPD frame. */
#ifndef HTTP_REQUEST_STREAM_MAX_FRAME
#define HTTP_REQUEST_STREAM_MAX_FRAME 65535u
#endif

#define HTTP_REQUEST_STREAM_LINE_CAPACITY (HTTP_REQUEST_STREAM_MAX_TARGET + 20u)

#ifdef __cplusplus
extern "C" {
#endif

/* Exactly one event is delivered per channel until ResetChannel is called.
 * A NULL target and zero length report a malformed or oversized request.
 * Otherwise target is NUL terminated, and length excludes the terminator.
 * The target belongs to stream and must be copied during the callback if it
 * will be needed later. Feed must not be called recursively from a callback.
 */
typedef void (*HTTP_RequestStreamCallback)(unsigned channel,
                                           const char *target,
                                           size_t length,
                                           void *context);

typedef struct {
    char line[HTTP_REQUEST_STREAM_LINE_CAPACITY];
    size_t length;
    uint8_t finished;
    uint8_t saw_cr;
} HTTP_RequestStreamChannel;

/* Allocate this object statically (or in SDRAM); it is about 15.5 KiB with
 * the default limits. All state is per object; no global state is used.
 * Members are implementation details and should not be modified directly.
 */
typedef struct {
    HTTP_RequestStreamChannel channels[HTTP_REQUEST_STREAM_CHANNELS];
    HTTP_RequestStreamCallback callback;
    void *context;
    uint32_t frame_channel;
    uint32_t number;
    uint32_t remaining;
    char at_line[24];
    uint8_t state;
    uint8_t prefix_length;
    uint8_t digits;
    uint8_t discard_payload;
    uint8_t at_line_length;
    uint8_t at_line_overflow;
} HTTP_RequestStream;

void HTTP_RequestStreamInit(HTTP_RequestStream *stream,
                            HTTP_RequestStreamCallback callback,
                            void *context);

/* Feed only newly received UART bytes, once, in arrival order. Arbitrary
 * chunk boundaries (including inside +IPD headers and CRLF) are supported.
 * This accepts +IPD,<channel>,<length>: in ESP-AT multiplexed mode, without
 * CIPDINFO. Responses outside frames are ignored, except complete
 * '<channel>,CONNECT\r\n' and '<channel>,CLOSED\r\n' lines reset that channel
 * in stream order. Bytes inside a frame are never scanned for nested +IPD
 * markers or connection notifications, even for unhandled channels.
 */
void HTTP_RequestStreamFeed(HTTP_RequestStream *stream,
                            const uint8_t *data,
                            size_t length);

/* Call when the corresponding connection is CLOSED, before its next use.
 * This clears that channel's HTTP state, not other channels or AT framing.
 * Reinitialize the entire object when restarting/resetting the ESP module.
 */
void HTTP_RequestStreamResetChannel(HTTP_RequestStream *stream,
                                    unsigned channel);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_REQUEST_STREAM_H_ */
