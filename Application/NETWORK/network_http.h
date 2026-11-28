#ifndef NETWORK_HTTP_H
#define NETWORK_HTTP_H
#include <stddef.h>
typedef int ( *NetworkRead ) ( void *, unsigned char *, size_t );
typedef int ( *NetworkWrite ) ( void *, const unsigned char *, size_t );
/* One request per connection; callbacks may return short reads/writes. */
int NetworkHTTPServe ( void *context, NetworkRead read_fn, NetworkWrite write_fn, const char *tls_name );
#define NETWORK_WEB_CHUNK_BYTES 8192u

/* The web owner task runs the same routes and renderers for Wi-Fi and LAN. */
int NetworkWebInit ( void );
int NetworkWebLock ( void );
void NetworkWebUnlock ( void );
int NetworkWebStart ( const char *target, size_t length );
int NetworkWebRead ( unsigned char *out, size_t capacity );
void NetworkWebFinish ( int delivered );
void http_server_netconn_init ( void );
#endif
