/* F7 Ethernet HTTP/HTTPS endpoints, shared by plain TCP and TLS on H7.
 * Streams aaa.htm from SD; no LCD scratch-buffer or SDRAM mutex ownership.
 */
#include "network_http.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "ff.h"
#include "timer.h"
#include "network_log.h"
#include <stdio.h>
#include <string.h>

static int send_all ( void *ctx, NetworkWrite write_fn, const void *data, size_t size )
{
	const unsigned char *p = data;
	while ( size )
	{
		int n = write_fn ( ctx, p, size );
		if ( n <= 0 || (size_t) n > size )
			return 0;
		p += n;
		size -= (size_t) n;
	}
	return 1;
}
static int message ( void *ctx, NetworkWrite write_fn, unsigned code, const char *reason, const char *body )
{
	char header[192];
	size_t length = strlen ( body );
	int n = snprintf ( header, sizeof header, "HTTP/1.1 %u %s\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %u\r\nConnection: close\r\n\r\n", code, reason, (unsigned) length );
	return n > 0 && (size_t) n < sizeof header && send_all ( ctx, write_fn, header, (size_t) n ) && send_all ( ctx, write_fn, body, length );
}
int NetworkHTTPServe ( void *ctx, NetworkRead read_fn, NetworkWrite write_fn, const char *tls_name )
{
	char request[1024];
	size_t used = 0;
	while ( used < sizeof request - 1 )
	{
		int n = read_fn ( ctx, (unsigned char *) request + used, sizeof request - 1 - used );
		if ( n <= 0 || (size_t) n > sizeof request - 1 - used )
			return 0;
		used += (size_t) n;
		request[used] = 0;
		if ( strstr ( request, "\r\n\r\n" ) )
			break;
	}
	if ( !strstr ( request, "\r\n\r\n" ) )
		return message ( ctx, write_fn, 431, "Request Header Fields Too Large", "Request too large" );
	if ( strncmp ( request, "GET ", 4 ) )
		return message ( ctx, write_fn, 405, "Method Not Allowed", "GET required" );
	char *path = request + 4, *end = strchr ( path, ' ' );
	if ( !end )
		return message ( ctx, write_fn, 400, "Bad Request", "Invalid request" );
	*end = 0;
	if ( !strcmp ( path, "/test" ) )
	{
		char body[192];
		snprintf ( body, sizeof body, "<html><body>Test: %s</body></html>", tls_name ? tls_name : "HTTP" );
		return message ( ctx, write_fn, 200, "OK", body );
	}
	if ( strcmp ( path, "/" ) && strcmp ( path, "/aaa.htm" ) )
		return message ( ctx, write_fn, 404, "Not Found", "Not found" );
	FIL file;
	FRESULT result;
	unsigned long length;
	if ( !TakeMutex ( Semphr_cardSD, 1000 ) )
		return message ( ctx, write_fn, 503, "Service Unavailable", "SD busy" );
	result = f_open ( &file, "0:/aaa.htm", FA_READ );
	length = result == FR_OK ? f_size ( &file ) : 0;
	GiveMutex ( Semphr_cardSD );
	if ( result != FR_OK )
		return message ( ctx, write_fn, 503, "Service Unavailable", "Cannot open SD aaa.htm" );
	char header[192];
	int n = snprintf ( header, sizeof header, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n", length );
	int ok = n > 0 && (size_t) n < sizeof header && send_all ( ctx, write_fn, header, (size_t) n );
	unsigned char buffer[2048];
	while ( ok )
	{
		UINT count = 0;
		if ( !TakeMutex ( Semphr_cardSD, 1000 ) )
		{
			ok = 0;
			break;
		}
		result = f_read ( &file, buffer, sizeof buffer, &count );
		GiveMutex ( Semphr_cardSD );
		if ( result != FR_OK )
		{
			ok = 0;
			break;
		}
		if ( !count )
			break;
		ok = send_all ( ctx, write_fn, buffer, count );
	}
	/* Always close, without retaining the SD mutex during network operations. */
	if ( TakeMutex ( Semphr_cardSD, osWaitForever ) )
	{
		f_close ( &file );
		GiveMutex ( Semphr_cardSD );
	}
	return ok;
}
struct PlainConnection
{
	struct netconn *conn;
	struct netbuf *buf;
	unsigned short offset;
};
static int plain_read ( void *context, unsigned char *out, size_t capacity )
{
	struct PlainConnection *c = context;
	for ( ;; )
	{
		if ( !c->buf )
		{
			if ( netconn_recv ( c->conn, &c->buf ) != ERR_OK )
				return -1;
			c->offset = 0;
		}
		unsigned length = netbuf_len ( c->buf );
		if ( c->offset < length )
		{
			unsigned n = length - c->offset;
			if ( n > capacity )
				n = (unsigned) capacity;
			if ( netbuf_copy_partial ( c->buf, out, n, c->offset ) != n )
				return -1;
			c->offset += (unsigned short) n;
			return (int) n;
		}
		netbuf_delete ( c->buf );
		c->buf = NULL;
	}
}
static int plain_write ( void *context, const unsigned char *data, size_t size )
{
	struct PlainConnection *c = context;
	return netconn_write ( c->conn, data, size, NETCONN_COPY ) == ERR_OK ? (int) size : -1;
}
static void http_thread ( void *arg )
{
	(void) arg;
	struct netconn *listener = netconn_new ( NETCONN_TCP );
	if ( !listener )
	{
		DbgDma ( 1, "\r\nHTTP ETH: allocation failed" );
		vTaskDelete ( NULL );
		return;
	}
	if ( netconn_bind ( listener, IP_ADDR_ANY, 80 ) != ERR_OK || netconn_listen ( listener ) != ERR_OK )
	{
		netconn_delete ( listener );
		DbgDma ( 1, "\r\nHTTP ETH: bind/listen failed" );
		vTaskDelete ( NULL );
		return;
	}
	DbgDma ( 1, "\r\nHTTP ETH: listening on port 80" );
	for ( ;; )
	{
		struct netconn *client = NULL;
		if ( netconn_accept ( listener, &client ) != ERR_OK )
		{
			sys_msleep ( 20 );
			continue;
		}
		netconn_set_recvtimeout ( client, 5000 );
		netconn_set_sendtimeout ( client, 5000 );
		struct PlainConnection c = { client, NULL, 0 };
		NetworkHTTPServe ( &c, plain_read, plain_write, NULL );
		if ( c.buf )
			netbuf_delete ( c.buf );
		netconn_close ( client );
		netconn_delete ( client );
	}
}
void http_server_netconn_init ( void )
{
	if ( !sys_thread_new ( "HTTP", http_thread, NULL, 8192, osPriorityBelowNormal ) )
		DbgDma ( 1, "\r\nHTTP ETH: task allocation failed" );
}
