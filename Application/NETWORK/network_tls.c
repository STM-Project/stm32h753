/* F7 TLS configuration and certificate, adapted to H7 with dedicated memory
 * and the shared SD-streaming HTTP handler. */
#include "mbedtls.h"
#include "threading_alt.h"
#include "mbedtls/certs.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "network_http.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "network_log.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
static unsigned char tlsMemory[327680] __attribute__((section(".sdram"),aligned(32)));
static mbedtls_ssl_config config;
static mbedtls_x509_crt certificate;
static mbedtls_pk_context key;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context drbg;
/* Static HTTPS task storage: no allocation from the 128 KiB RTOS heap.
 * Like F7's xTaskCreateStatic SMTPS example, but a dedicated owned buffer. */
enum { HTTPS_STACK_BYTES=65536, HTTPS_STACK_WORDS=HTTPS_STACK_BYTES/sizeof(StackType_t) };
static StackType_t httpsStack[HTTPS_STACK_WORDS] __attribute__((section(".sdram"),aligned(32)));
static StaticTask_t httpsTaskControl __attribute__((section(".sdram"),aligned(32)));
static TaskHandle_t httpsTaskHandle;
static int tlsReady;
osMutexId networkRngMutex;
void MX_MBEDTLS_Init(void)
{
    /* CubeMX calls MX_MBEDTLS_Init before starting the scheduler too. */
    if(!osKernelRunning() || tlsReady)return;
    osMutexDef(NetworkRng);networkRngMutex=osMutexCreate(osMutex(NetworkRng));
    if(!networkRngMutex)return;
    mbedtls_threading_set_alt(cmsis_os_mutex_init,cmsis_os_mutex_free,cmsis_os_mutex_lock,cmsis_os_mutex_unlock);
    mbedtls_memory_buffer_alloc_init(tlsMemory,sizeof tlsMemory);
    mbedtls_ssl_config_init(&config);mbedtls_x509_crt_init(&certificate);mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);mbedtls_ctr_drbg_init(&drbg);
    int ret=mbedtls_ctr_drbg_seed(&drbg,mbedtls_entropy_func,&entropy,(const unsigned char *)"stm32h753_https",15);
    if(!ret)ret=mbedtls_x509_crt_parse(&certificate,(const unsigned char *)mbedtls_test_srv_crt_ec,mbedtls_test_srv_crt_ec_len);
    if(!ret)ret=mbedtls_pk_parse_key(&key,(const unsigned char *)mbedtls_test_srv_key_ec,mbedtls_test_srv_key_ec_len,NULL,0);
    if(!ret)ret=mbedtls_ssl_config_defaults(&config,MBEDTLS_SSL_IS_SERVER,MBEDTLS_SSL_TRANSPORT_STREAM,MBEDTLS_SSL_PRESET_DEFAULT);
    if(!ret)ret=mbedtls_ssl_conf_own_cert(&config,&certificate,&key);
    if(ret){DbgVarDma(1,96,"\r\nHTTPS: TLS init failed -0x%04X",(unsigned)-ret);return;}
    mbedtls_ssl_conf_rng(&config,mbedtls_ctr_drbg_random,&drbg);
    mbedtls_ssl_conf_read_timeout(&config,5000);
    tlsReady=1;
}
static int tls_read(void *ctx,unsigned char *data,size_t length)
{return mbedtls_ssl_read(ctx,data,length);}
static int tls_write(void *ctx,const unsigned char *data,size_t length)
{return mbedtls_ssl_write(ctx,data,length);}
static void https_thread(void *arg)
{
    (void)arg;mbedtls_net_context listener;mbedtls_net_init(&listener);
    int ret=mbedtls_net_bind(&listener,NULL,"443",MBEDTLS_NET_PROTO_TCP);
    if(ret){DbgVarDma(1,96,"\r\nHTTPS ETH: bind failed -0x%04X",(unsigned)-ret);mbedtls_net_free(&listener);vTaskDelete(NULL);return;}
    DbgDma(1,"\r\nHTTPS ETH: listening on port 443 (F7 test certificate)");
    for(;;){
        mbedtls_net_context client;mbedtls_ssl_context ssl;
        mbedtls_net_init(&client);mbedtls_ssl_init(&ssl);
        ret=mbedtls_net_accept(&listener,&client,NULL,0,NULL);
        if(!ret){
            int timeout=5000;
            lwip_setsockopt(client.fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);
            lwip_setsockopt(client.fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof timeout);
            ret=mbedtls_ssl_setup(&ssl,&config);
        }
        if(!ret){
            mbedtls_ssl_set_bio(&ssl,&client,mbedtls_net_send,mbedtls_net_recv,mbedtls_net_recv_timeout);
            do{ret=mbedtls_ssl_handshake(&ssl);}while(ret==MBEDTLS_ERR_SSL_WANT_READ||ret==MBEDTLS_ERR_SSL_WANT_WRITE);
            if(!ret){NetworkHTTPServe(&ssl,tls_read,tls_write,mbedtls_ssl_get_ciphersuite(&ssl));mbedtls_ssl_close_notify(&ssl);}
        }
        if(ret)DbgVarDma(1,96,"\r\nHTTPS ETH: connection failed -0x%04X",(unsigned)-ret);
        mbedtls_ssl_free(&ssl);mbedtls_net_free(&client);sys_msleep(10);
    }
}
void https_server_netconn_init(void)
{
    if(!tlsReady || httpsTaskHandle)return;
    /* Match sys_thread_new's CMSIS osPriorityBelowNormal -> FreeRTOS mapping.
     * xTaskCreateStatic expects stack depth in words, not bytes. */
    UBaseType_t priority=(UBaseType_t)(osPriorityBelowNormal-osPriorityIdle);
    httpsTaskHandle=xTaskCreateStatic(https_thread,"HTTPS",HTTPS_STACK_WORDS,NULL,
                                    priority,httpsStack,&httpsTaskControl);
    if(!httpsTaskHandle)DbgDma(1,"\r\nHTTPS: static task creation failed");
}
