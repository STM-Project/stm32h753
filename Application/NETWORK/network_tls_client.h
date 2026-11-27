#ifndef NETWORK_TLS_CLIENT_H
#define NETWORK_TLS_CLIENT_H

#include <stddef.h>
#include <stdint.h>

int NetworkTLSReady ( void );
int NetworkTLSRandom ( void *context, unsigned char *output, size_t length );
int NetworkTLSHasDate ( void );
int NetworkTLSUseEsp32Time ( uint32_t seconds );

#endif
