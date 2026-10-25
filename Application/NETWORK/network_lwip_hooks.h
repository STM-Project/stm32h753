#ifndef NETWORK_LWIP_HOOKS_H
#define NETWORK_LWIP_HOOKS_H
extern int networkEthernetReady, networkDriverReady;
void NetworkPrepareDmaMemory ( void );
void NetworkTCPIPInit ( void ( *callback ) ( void * ), void *argument );
void NetworkLogEthernetStatus ( int available );
#endif
