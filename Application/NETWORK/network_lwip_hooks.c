#include "network_lwip_hooks.h"
#include "network_log.h"
#include "lwip/tcpip.h"
#include "main.h"
#include <string.h>
int networkEthernetReady, networkDriverReady;
unsigned char network_lwip_heap[16384] __attribute__ ( ( section ( ".eth_heap" ), aligned ( 32 ) ) );
extern unsigned char __eth_dma_start__, __eth_dma_end__, __eth_rx_start__, __eth_rx_end__;
void NetworkPrepareDmaMemory ( void )
{
	memset ( &__eth_dma_start__, 0, (size_t) ( &__eth_dma_end__ - &__eth_dma_start__ ) );
	memset ( &__eth_rx_start__, 0, (size_t) ( &__eth_rx_end__ - &__eth_rx_start__ ) );
	memset ( network_lwip_heap, 0, sizeof network_lwip_heap );
	SCB_CleanInvalidateDCache_by_Addr ( (uint32_t *) &__eth_rx_start__, (int32_t) ( &__eth_rx_end__ - &__eth_rx_start__ ) );
}
void NetworkTCPIPInit ( void ( *callback ) ( void * ), void *argument )
{
	tcpip_init ( callback, argument );
	LOCK_TCPIP_CORE ();
}
void NetworkLogEthernetStatus ( int available )
{
	DbgDma ( 1, available ? "\r\nETH: LAN8742 interface initialized" : "\r\nETH: interface unavailable; Wi-Fi/LCD continue" );
}
