#include "google_cloud.h"
#include "google_cloud_at.h"
#include "google_cloud_config.h"
#include "google_cloud_protocol.h"
#include "google_cloud_roots.h"
#include "network_log.h"
#include "network_tls_client.h"
#include "lwip.h"
#include "lwip/apps/sntp.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

enum { CLOUD_STACK_WORDS = 8192 };
static StackType_t cloudStack[CLOUD_STACK_WORDS] __attribute__ ( ( section ( ".sdram" ), aligned ( 32 ) ) );
static StaticTask_t cloudTaskStorage;
static TaskHandle_t cloudTask;
static char request[4096];
static char wire[CLOUD_AT_WIRE_SIZE];
static char redirect[2048];
static CloudHttpResponse response;

static void start_sntp ( void *arg )
{
	(void) arg;
	if ( sntp_enabled () )
		return;
	sntp_setoperatingmode ( SNTP_OPMODE_POLL );
	sntp_setservername ( 0, "time.google.com" );
	sntp_init ();
}

static int create_nonce ( char nonce[33] )
{
	unsigned char random[16];
	static const char hex[] = "0123456789abcdef";
	if ( NetworkTLSRandom ( NULL, random, sizeof random ) )
		return 0;
	for ( unsigned i = 0; i < sizeof random; ++i )
	{
		nonce[2 * i] = hex[random[i] >> 4];
		nonce[2 * i + 1] = hex[random[i] & 0x0F];
	}
	nonce[32] = 0;
	return 1;
}

static int exchange ( const mbedtls_ssl_config *config, const char *host, size_t length )
{
	mbedtls_net_context socket;
	mbedtls_ssl_context ssl;
	mbedtls_net_init ( &socket );
	mbedtls_ssl_init ( &ssl );

	int result = -3;
	int error = mbedtls_ssl_setup ( &ssl, config );
	if ( !error )
		error = mbedtls_ssl_set_hostname ( &ssl, host );
	if ( error )
		goto done;

	error = mbedtls_net_connect ( &socket, host, "443", MBEDTLS_NET_PROTO_TCP );
	if ( error )
		goto done;
	int timeout = (int) GOOGLE_CLOUD_TIMEOUT_MS;
	if ( lwip_setsockopt ( socket.fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout ) != 0 )
		goto done;
	mbedtls_ssl_set_bio ( &ssl, &socket, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout );
	error = mbedtls_ssl_handshake ( &ssl );
	if ( error )
		goto done;

	result = -6;
	for ( size_t sent = 0; sent < length; )
	{
		error = mbedtls_ssl_write ( &ssl, (const unsigned char *) request + sent, length - sent );
		if ( error <= 0 )
			goto done;
		sent += (size_t) error;
	}

	size_t used = 0;
	while ( used < sizeof wire )
	{
		error = mbedtls_ssl_read ( &ssl, (unsigned char *) wire + used, sizeof wire - used );
		int eof = error == 0 || error == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;
		if ( error < 0 && !eof )
			goto done;
		if ( error > 0 )
			used += (size_t) error;
		int parsed = CloudHttp_Parse ( wire, used, eof, &response );
		if ( parsed <= 0 || eof )
		{
			result = parsed == 0 ? 0 : -6;
			break;
		}
	}
done:
	if ( error == MBEDTLS_ERR_SSL_TIMEOUT )
		result = -4;
	if ( result && error < 0 && error != MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
		DbgVarDma ( 1, 100, "\r\nGoogle ETH: HTTPS %s failed -0x%04X", host, (unsigned) -error );
	mbedtls_ssl_free ( &ssl );
	mbedtls_net_free ( &socket );
	/* The buffer can contain the device token or a one-use redirect. */
	memset ( request, 0, length );
	return result;
}

static int publish ( const mbedtls_ssl_config *config )
{
	int temp = GoogleCloud_ReadTemperature10 ();
	if ( temp < -800 || temp > 1500 )
		return -18;
	char nonce[33];
	if ( !create_nonce ( nonce ) )
		return -8;

	googleCloudStatus.temperature10 = temp;
	const char *host = "script.google.com";
	size_t length = CloudRequest_Build ( request, sizeof request, host, GOOGLE_CLOUD_PATH,
		GOOGLE_CLOUD_TOKEN, GOOGLE_CLOUD_DEVICE, temp, googleCloudStatus.demo_relay, nonce );
	if ( !length )
		return -1;
	int result = exchange ( config, host, length );
	if ( result )
		return result;

	if ( response.status == 302 || response.status == 303 )
	{
		if ( CloudRedirect_Path ( response.location, redirect, sizeof redirect ) )
			return -6;
		host = "script.googleusercontent.com";
		length = CloudRequest_Build ( request, sizeof request, host, redirect, NULL, NULL, 0, 0, NULL );
		if ( !length )
			return -6;
		result = exchange ( config, host, length );
		if ( result )
			return result;
	}
	int relay;
	if ( response.status != 200 || CloudReply_Parse ( response.body, nonce, &relay ) )
		return -7;
	googleCloudStatus.demo_relay = relay;
	return 0;
}

static void cloud_ethernet ( void *arg )
{
	(void) arg;
	mbedtls_ssl_config config;
	mbedtls_x509_crt roots;
	mbedtls_ssl_config_init ( &config );
	mbedtls_x509_crt_init ( &roots );

	int error = mbedtls_x509_crt_parse ( &roots, (const unsigned char *) google_cloud_roots, sizeof google_cloud_roots );
	if ( !error )
		error = mbedtls_ssl_config_defaults ( &config, MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT );
	if ( !error )
		error = tcpip_callback ( start_sntp, NULL );
	if ( error )
	{
		googleCloudStatus.last_error = -3;
		DbgVarDma ( 1, 100, "\r\nGoogle ETH: client init failed (%d)", error );
		mbedtls_ssl_config_free ( &config );
		mbedtls_x509_crt_free ( &roots );
		cloudTask = NULL;
		vTaskDelete ( NULL );
		return;
	}
	mbedtls_ssl_conf_min_version ( &config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3 );
	mbedtls_ssl_conf_authmode ( &config, MBEDTLS_SSL_VERIFY_REQUIRED );
	mbedtls_ssl_conf_ca_chain ( &config, &roots, NULL );
	mbedtls_ssl_conf_rng ( &config, NetworkTLSRandom, NULL );
	mbedtls_ssl_conf_read_timeout ( &config, GOOGLE_CLOUD_TIMEOUT_MS );

	DbgDma ( 1, "\r\nGoogle ETH: waiting for Ethernet link and SNTP" );
	for ( ;; )
	{
		LOCK_TCPIP_CORE ();
		int linked = netif_default && netif_is_up ( netif_default ) && netif_is_link_up ( netif_default );
		UNLOCK_TCPIP_CORE ();
		googleCloudStatus.ready = linked && NetworkTLSHasDate ();
		if ( !googleCloudStatus.ready )
		{
			googleCloudStatus.last_error = linked ? -2 : -10;
			vTaskDelay ( pdMS_TO_TICKS ( 1000 ) );
			continue;
		}

		int result = publish ( &config );
		googleCloudStatus.last_error = result;
		if ( result )
			++googleCloudStatus.failures;
		else
			++googleCloudStatus.successes;
		DbgVarDma ( 1, 128, "\r\nGoogle ETH: result=%d OK=%u failures=%u demo=%d",
			result, (unsigned) googleCloudStatus.successes, (unsigned) googleCloudStatus.failures,
			googleCloudStatus.demo_relay );
		vTaskDelay ( pdMS_TO_TICKS ( GOOGLE_CLOUD_PERIOD_MS ) );
	}
}

void GoogleCloud_StartEthernet ( void )
{
	if ( googleCloudTransport != GOOGLE_CLOUD_ETHERNET || cloudTask )
		return;
	googleCloudStatus.configured = CloudConfig_Valid ( GOOGLE_CLOUD_TOKEN, GOOGLE_CLOUD_DEVICE, GOOGLE_CLOUD_PATH );
	if ( !googleCloudStatus.configured || !NetworkTLSReady () )
	{
		googleCloudStatus.last_error = googleCloudStatus.configured ? -3 : -1;
		DbgVarDma ( 1, 96, "\r\nGoogle ETH: startup unavailable (%d)", googleCloudStatus.last_error );
		return;
	}
	cloudTask = xTaskCreateStatic ( cloud_ethernet, "GoogleETH", CLOUD_STACK_WORDS, NULL,
		tskIDLE_PRIORITY, cloudStack, &cloudTaskStorage );
	if ( !cloudTask )
	{
		googleCloudStatus.last_error = -3;
		DbgDma ( 1, "\r\nGoogle ETH: task creation failed" );
	}
}
