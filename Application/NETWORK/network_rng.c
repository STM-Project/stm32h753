/* H7 entropy adapter: return actual random bytes, including a trailing partial
 * word. Never repeat one byte four times as the F7 adapter did. */
#include "mbedtls_config.h"
#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT
#include "rng.h"
#include "cmsis_os.h"
#include "mbedtls/entropy.h"
#include <string.h>
extern osMutexId networkRngMutex;
int mbedtls_hardware_poll(void *context,unsigned char *output,size_t length,size_t *written)
{
    (void)context;*written=0;
    if(!networkRngMutex||osMutexWait(networkRngMutex,1000)!=osOK)return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    while(*written<length){uint32_t word;
        if(HAL_RNG_GenerateRandomNumber(&hrng,&word)!=HAL_OK){osMutexRelease(networkRngMutex);return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;}
        size_t n=length-*written;if(n>sizeof word)n=sizeof word;
        memcpy(output+*written,&word,n);*written+=n;
    }
    osMutexRelease(networkRngMutex);return 0;
}
#endif
