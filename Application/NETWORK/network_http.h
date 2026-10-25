#ifndef NETWORK_HTTP_H
#define NETWORK_HTTP_H
#include <stddef.h>
typedef int (*NetworkRead)(void *, unsigned char *, size_t);
typedef int (*NetworkWrite)(void *, const unsigned char *, size_t);
/* One request per connection; callbacks may return short reads/writes. */
int NetworkHTTPServe(void *context, NetworkRead read_fn, NetworkWrite write_fn, const char *tls_name);
void http_server_netconn_init(void);
#endif
