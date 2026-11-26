/* Ethernet HTTP and HTTPS adapters for the shared application pages. */
#include "network_http.h"
#include "http_request_stream.h"
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
	char response[384];
	size_t length = strlen ( body );
	int n = snprintf ( response, sizeof response, "HTTP/1.1 %u %s\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %u\r\nConnection: close\r\n\r\n%s", code, reason, (unsigned) length, body );
	return n > 0 && (size_t) n < sizeof response && send_all ( ctx, write_fn, response, (size_t) n );
}
static int serve_sd_page ( void *ctx, NetworkWrite write_fn )
{
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
struct LanRequest
{
	const char *target;
	size_t length;
	int ready;
};

static HTTP_RequestStream lanRequestStream __attribute__ ( ( section ( ".sdram" ) ) );
/* The LAN lock protects the parser and the buffers for both socket tasks. */
static unsigned char lanResponse[NETWORK_WEB_CHUNK_BYTES] __attribute__ ( ( section ( ".sdram" ) ) );

static void capture_target ( unsigned channel, const char *target, size_t length, void *ctx )
{
	(void) channel;
	struct LanRequest *request = ctx;
	request->target = target;
	request->length = length;
	request->ready = 1;
}

static int serve_lan_request ( void *ctx, NetworkRead read_fn, NetworkWrite write_fn, const char *tls_name, unsigned lockMs )
{
	unsigned started = sys_now ();
	struct LanRequest request = { 0 };
	/* Only channel zero is used by the raw stream. */
	HTTP_RequestStreamResetChannel ( &lanRequestStream, 0 );
	lanRequestStream.callback = capture_target;
	lanRequestStream.context = &request;
	lanRequestStream.discard_payload = 0;

	unsigned char buffer[2048];
	unsigned headerEnd = 0;
	int post = 0;
	size_t received = 0;
	do
	{
		int size = read_fn ( ctx, buffer, sizeof buffer );
		if ( size <= 0 || (size_t) size > sizeof buffer )
		{
			if ( received )
				DbgDma ( 1, "\r\nLAN: request ended before parsing completed" );
			return 0;
		}
		if ( !received )
			post = buffer[0] == 'P';
		received += (size_t) size;
		if ( received > HTTP_REQUEST_STREAM_MAX_TARGET + 8192u + sizeof buffer )
			return message ( ctx, write_fn, 431, "Request Header Fields Too Large", "Request too large" );
		HTTP_RequestStreamFeedRaw ( &lanRequestStream, buffer, (size_t) size );
		if ( !post )
		{
			static const unsigned char end[] = "\r\n\r\n";
			for ( int i = 0; i < size && headerEnd < 4; ++i )
				headerEnd = buffer[i] == end[headerEnd] ? headerEnd + 1u : (buffer[i] == '\r');
		}
	} while ( !request.ready || (!post && request.target && headerEnd < 4) );

	if ( !request.target )
		return message ( ctx, write_fn, 400, "Bad Request", "Invalid request" );
	if ( !post && !strcmp ( request.target, "/test" ) )
	{
		char body[192];
		snprintf ( body, sizeof body, "<html><body>Test: %s</body></html>", tls_name ? tls_name : "HTTP" );
		return message ( ctx, write_fn, 200, "OK", body );
	}
	if ( !post && !strcmp ( request.target, "/aaa.htm" ) )
		return serve_sd_page ( ctx, write_fn );

	unsigned receivedAt = sys_now ();
	if ( !NetworkWebStart ( request.target, request.length ) )
		return message ( ctx, write_fn, 503, "Service Unavailable", "Application pages busy" );

	unsigned renderedAt = sys_now ();
	int count;
	int ok = 1;
	while ( (count = NetworkWebRead ( lanResponse, sizeof lanResponse )) > 0 )
	{
		if ( !send_all ( ctx, write_fn, lanResponse, (size_t) count ) )
		{
			ok = 0;
			break;
		}
	}
	ok = ok && count == 0;
	if ( count < 0 )
		DbgDma ( 1, "\r\nLAN: response producer stopped before the end" );
	else if ( !ok )
		DbgDma ( 1, "\r\nLAN: response write stopped before the end" );
	unsigned sentAt = sys_now ();
	if ( tls_name && lockMs + sentAt - started >= 400u )
	{
		const char *kind = !strncmp ( request.target, "/assets/", 8 ) ? "asset" :
						   !strncmp ( request.target, "/api/", 5 ) ? "api" : "page";
		DbgVarDma ( 1, 144, "\r\nHTTPS ETH slow %s: queue=%u rx=%u render=%u tx=%u ms",
					kind, lockMs, receivedAt - started,
					renderedAt - receivedAt, sentAt - renderedAt );
	}
	NetworkWebFinish ( ok );
	return ok;
}

int NetworkHTTPServe ( void *ctx, NetworkRead read_fn, NetworkWrite write_fn, const char *tls_name )
{
	unsigned queuedAt = sys_now ();
	if ( !NetworkWebLock () )
		return message ( ctx, write_fn, 503, "Service Unavailable", "Application pages busy" );
	int result = serve_lan_request ( ctx, read_fn, write_fn, tls_name, sys_now () - queuedAt );
	NetworkWebUnlock ();
	return result;
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
	size_t written = 0;
	/* lwIP requires the byte count when a send timeout is configured. */
	err_t error = netconn_write_partly ( c->conn, data, size, NETCONN_COPY, &written );
	if ( error != ERR_OK )
	{
		DbgVarDma ( 1, 96, "\r\nHTTP ETH: netconn_write_partly failed (%d), bytes=%u",
					(int) error, (unsigned) size );
		return -1;
	}
	return (int) written;
}
enum { HTTP_PENDING_CONNECTIONS = 4, HTTP_IDLE_TIMEOUT_MS = 5000 };

struct PendingConnection
{
	struct netconn *conn;
	unsigned acceptedAt;
};

/* Browsers can open speculative TCP connections without sending HTTP.
 * Keep those sockets separate from the parser until the first bytes arrive. */
static int http_take_ready ( struct PendingConnection *pending, struct PlainConnection *ready )
{
	if ( !pending->conn )
		return 0;

	struct netbuf *first = NULL;
	err_t error = netconn_recv ( pending->conn, &first );
	if ( error == ERR_OK )
	{
		*ready = (struct PlainConnection) { pending->conn, first, 0 };
		pending->conn = NULL;
		return 1;
	}
	if ( error != ERR_WOULDBLOCK || sys_now () - pending->acceptedAt >= HTTP_IDLE_TIMEOUT_MS )
	{
		netconn_delete ( pending->conn );
		pending->conn = NULL;
	}
	return 0;
}

static void http_serve_client ( struct PlainConnection *client )
{
	netconn_set_nonblocking ( client->conn, 0 );
	netconn_set_recvtimeout ( client->conn, HTTP_IDLE_TIMEOUT_MS );
	netconn_set_sendtimeout ( client->conn, HTTP_IDLE_TIMEOUT_MS );
	NetworkHTTPServe ( client, plain_read, plain_write, NULL );
	if ( client->buf )
		netbuf_delete ( client->buf );
	netconn_close ( client->conn );
	netconn_delete ( client->conn );
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
	struct PendingConnection pending[HTTP_PENDING_CONNECTIONS] = { 0 };
	for ( ;; )
	{
		unsigned occupied = 0;
		for ( unsigned i = 0; i < HTTP_PENDING_CONNECTIONS; ++i )
		{
			struct PlainConnection client;
			if ( http_take_ready ( &pending[i], &client ) )
				http_serve_client ( &client );
			if ( pending[i].conn )
				++occupied;
		}

		/* With no pending sockets, wait normally for a connection. */
		netconn_set_recvtimeout ( listener, occupied ? 20 : 0 );
		struct netconn *conn = NULL;
		err_t error = netconn_accept ( listener, &conn );
		if ( error != ERR_OK )
		{
			if ( error != ERR_TIMEOUT )
				sys_msleep ( 20 );
			continue;
		}
		netconn_set_nonblocking ( conn, 1 );
		struct PendingConnection incoming = { conn, sys_now () };
		struct PlainConnection ready;
		if ( http_take_ready ( &incoming, &ready ) )
			http_serve_client ( &ready );
		if ( incoming.conn )
		{
			unsigned slot = 0;
			for ( unsigned i = 0; i < HTTP_PENDING_CONNECTIONS; ++i )
			{
				if ( !pending[i].conn )
				{
					slot = i;
					break;
				}
				if ( incoming.acceptedAt - pending[i].acceptedAt >
					 incoming.acceptedAt - pending[slot].acceptedAt )
					slot = i;
			}
			if ( pending[slot].conn )
				netconn_delete ( pending[slot].conn );
			pending[slot] = incoming;
		}
	}
}
void http_server_netconn_init ( void )
{
	if ( !sys_thread_new ( "HTTP", http_thread, NULL, 8192, osPriorityBelowNormal ) )
		DbgDma ( 1, "\r\nHTTP ETH: task allocation failed" );
}
