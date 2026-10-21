#include "google_cloud_protocol.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

static int equal(const char *a, size_t n, const char *b)
{
    size_t i;
    if (strlen(b) != n) return 0;
    for (i = 0; i < n; ++i)
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return 0;
    return 1;
}

static const char *line_end(const char *p, const char *end)
{
    for (; end-p >= 2; ++p) if (p[0] == '\r' && p[1] == '\n') return p;
    return NULL;
}

int CloudHttp_Parse(const char *data, size_t size, int eof, CloudHttpResponse *out)
{
    const char *end = data + size, *p, *q;
    size_t length = 0, used = 0;
    int has_length = 0, chunked = 0;
    memset(out, 0, sizeof(*out));
    q = line_end(data, end);
    if (!q) return eof ? -1 : 1;
    if (q - data < 12 ||
        (memcmp(data, "HTTP/1.1 ", 9) && memcmp(data, "HTTP/1.0 ", 9)) ||
        !isdigit((unsigned char)data[9]) || !isdigit((unsigned char)data[10]) ||
        !isdigit((unsigned char)data[11]) || (q-data > 12 && data[12] != ' ')) return -1;
    out->status = (data[9]-'0')*100 + (data[10]-'0')*10 + data[11]-'0';
    p = q + 2;
    for (;;) {
        const char *colon, *value, *finish;
        q = line_end(p, end);
        if (!q) return eof ? -1 : 1;
        if (q == p) { p += 2; break; }
        colon = memchr(p, ':', (size_t)(q-p));
        if (!colon) return -1;
        value = colon + 1;
        while (value < q && (*value == ' ' || *value == '\t')) ++value;
        finish = q;
        while (finish > value && (finish[-1] == ' ' || finish[-1] == '\t')) --finish;
        if (equal(p, (size_t)(colon-p), "location")) {
            if (out->location[0] || finish-value >= (int)sizeof(out->location)) return -1;
            memcpy(out->location, value, (size_t)(finish-value));
        } else if (equal(p, (size_t)(colon-p), "content-length")) {
            const char *v;
            if (has_length || value == finish) return -1;
            has_length = 1;
            for (v = value; v < finish; ++v) {
                if (!isdigit((unsigned char)*v) || length > (SIZE_MAX-9)/10) return -1;
                length = length*10 + (size_t)(*v-'0');
            }
        } else if (equal(p, (size_t)(colon-p), "transfer-encoding")) {
            if (chunked || !equal(value, (size_t)(finish-value), "chunked")) return -1;
            chunked = 1;
        }
        p = q + 2;
    }
    if (has_length && chunked) return -1;
    /* Redirect bodies are irrelevant; never forward the POST or its token. */
    if (out->status == 302 || out->status == 303) return out->location[0] ? 0 : -1;
    if (out->status != 200) return -1;
    if (chunked) {
        for (;;) {
            size_t n = 0;
            const char *v;
            q = line_end(p, end);
            if (!q) return eof ? -1 : 1;
            if (q == p) return -1;
            for (v = p; v < q && *v != ';'; ++v) {
                unsigned digit;
                if (!isxdigit((unsigned char)*v) || n > (SIZE_MAX-15)/16) return -1;
                digit = isdigit((unsigned char)*v) ? (unsigned)(*v-'0') :
                    (unsigned)(tolower((unsigned char)*v)-'a'+10);
                n = n*16 + digit;
            }
            if (v == p) return -1;
            p = q + 2;
            if (!n) {
                if (memchr(out->body, 0, used)) return -1;
                /* Accept a complete trailer block, including the empty block. */
                do {
                    q = line_end(p, end);
                    if (!q) return eof ? -1 : 1;
                    if (q == p) { out->body[used] = 0; return 0; }
                    p = q + 2;
                } while (1);
            }
            if (n >= sizeof(out->body)-used) return -1;
            if ((size_t)(end-p) < n+2) return eof ? -1 : 1;
            if (p[n] != '\r' || p[n+1] != '\n') return -1;
            memcpy(out->body+used, p, n); used += n; p += n+2;
        }
    }
    if (has_length) {
        if (length >= sizeof(out->body)) return -1;
        if ((size_t)(end-p) < length) return eof ? -1 : 1;
    } else {
        if (!eof) return 1;
        length = (size_t)(end-p);
        if (length >= sizeof(out->body)) return -1;
    }
    memcpy(out->body, p, length);
    if (memchr(p, 0, length)) return -1;
    return 0;
}

int CloudReply_Parse(const char *body, const char *nonce, int *relay)
{
    char expected[80];
    size_t n;
    if (strlen(nonce) != 32) return -1;
    snprintf(expected, sizeof(expected), "STM32CLOUD1:%s:", nonce);
    n = strlen(expected);
    if (strlen(body) != n+1 || memcmp(body, expected, n) ||
        (body[n] != '0' && body[n] != '1')) return -1;
    *relay = body[n]-'0';
    return 0;
}

int CloudRedirect_Path(const char *url, char *path, size_t capacity)
{
    static const char prefix[] = "https://script.googleusercontent.com/";
    const char *p;
    size_t n = sizeof(prefix)-1;
    if (strncmp(url, prefix, n)) return -1;
    p = url+n-1;
    if (strlen(p)+1 > capacity) return -1;
    for (const char *v=p; *v; ++v)
        if ((unsigned char)*v <= 32 || (unsigned char)*v >= 127 || *v == '#' || *v == '\\') return -1;
    strcpy(path, p);
    return 0;
}
