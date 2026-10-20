#ifndef GOOGLE_CLOUD_AT_H
#define GOOGLE_CLOUD_AT_H
#include <stddef.h>
#include <stdint.h>
#define CLOUD_AT_LINK_ID 3u
#define CLOUD_AT_OK 1u
#define CLOUD_AT_ERROR 2u
#define CLOUD_AT_PROMPT 4u
#define CLOUD_AT_SENT 8u
#define CLOUD_AT_CLOSED 16u
#define CLOUD_AT_WIRE_SIZE 8192u
typedef struct {
    char wire[CLOUD_AT_WIRE_SIZE], line[96];
    size_t used, line_used;
    uint32_t remaining, frame_channel, events;
    int payload, overflow, line_overflow, eof;
} CloudAt;
void CloudAt_Reset(CloudAt *at);
void CloudAt_Feed(CloudAt *at,const uint8_t *data,size_t size);
int CloudAt_Has(const CloudAt *at,const char *answer);
int CloudConfig_Valid(const char *token,const char *device,const char *path);
size_t CloudRequest_Build(char *out,size_t capacity,const char *host,const char *path,
                        const char *token,const char *device,int temperature10,int relay,
                        const char *nonce);
#endif
