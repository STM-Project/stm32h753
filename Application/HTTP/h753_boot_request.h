#ifndef H753_BOOT_REQUEST_H
#define H753_BOOT_REQUEST_H

#include <stdint.h>

#define H753_BOOT_MAILBOX_ADDRESS UINT32_C(0x38800F00)
#define H753_BOOT_REQUEST_MAGIC UINT32_C(0x48574232)
#define H753_UPDATE_KEY_LENGTH 32u

typedef struct {
    uint32_t magic;
    uint32_t ip, gateway, mask;
    uint16_t port;
    uint8_t dhcp;
    char ssid[64];
    char password[64];
    char update_key[H753_UPDATE_KEY_LENGTH + 1];
    uint32_t check;
} H753_BootRequest;

_Static_assert(sizeof(H753_BootRequest) <= 256, "Backup SRAM mailbox capacity");

static inline uint32_t H753_BootRequestCheck(const H753_BootRequest *request)
{
    const unsigned char *bytes = (const unsigned char *)request;
    uint32_t hash = UINT32_C(2166136261);
    for (unsigned i = 0; i < __builtin_offsetof(H753_BootRequest, check); ++i)
        hash = (hash ^ bytes[i]) * UINT32_C(16777619);
    return hash;
}

static inline int H753_UpdateKeyMatches(const char *key, const char *candidate, unsigned length)
{
    if (length != H753_UPDATE_KEY_LENGTH)
        return 0;
    unsigned difference = 0;
    for (unsigned i = 0; i < H753_UPDATE_KEY_LENGTH; ++i)
        difference |= (unsigned char)key[i] ^ (unsigned char)candidate[i];
    return difference == 0;
}

#endif
