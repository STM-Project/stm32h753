#include "network_start.h"
#include "network_log.h"
#include "network_http.h"
#include "lwip.h"
#include "mbedtls.h"
#include "FreeRTOS.h"
#include "task.h"
/* TLS certificate parsing must not run on the small default-task stack. */
static void start_network ( void *argument )
{
	(void) argument;
	MX_LWIP_Init ();
	if ( networkEthernetReady )
	{
		MX_MBEDTLS_Init ();
		http_server_netconn_init ();
		https_server_netconn_init ();
	}
	vTaskDelete ( NULL );
}
void NetworkStartEthernet ( void )
{
	if ( xTaskCreate ( start_network, "NetStart", 2048, NULL, 1, NULL ) != pdPASS )
		DbgDma ( 1, "\r\nETH: startup task allocation failed" );
}
