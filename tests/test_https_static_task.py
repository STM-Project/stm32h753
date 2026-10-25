"""Exercise actual HTTPS task creation with an exhausted dynamic RTOS heap."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Application/NETWORK/network_tls.c').read_text()
a=s.index('enum\n{\n\tHTTPS_STACK_BYTES');b=s.index('osMutexId networkRngMutex;',a)
decls=s[a:b].replace('__attribute__ ( ( section ( ".sdram" ), aligned ( 32 ) ) )','')
a=s.index('void https_server_netconn_init ( void )');fn=s[a:]
code=r'''
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef uint32_t StackType_t;typedef unsigned UBaseType_t;
typedef struct {unsigned data[32];} StaticTask_t;typedef void *TaskHandle_t;
enum {osPriorityBelowNormal=-1,osPriorityIdle=-3};
static int creates,fail;
static void https_thread(void *arg){(void)arg;}
static void DbgDma(int on,const char *text){assert(on&&text);}
''' +decls+r'''
static TaskHandle_t xTaskCreateStatic(void (*entry)(void*),const char *name,unsigned words,void *arg,UBaseType_t priority,StackType_t *stack,StaticTask_t *control){
 assert(entry==https_thread&&!strcmp(name,"HTTPS")&&!arg&&priority==2);
 assert(words*sizeof(StackType_t)==65536&&stack==httpsStack&&control==&httpsTaskControl);
 creates++;return fail?NULL:control;
}
''' +fn+r'''
int main(void){
 https_server_netconn_init();assert(!creates);
 tlsReady=1;fail=1;https_server_netconn_init();assert(creates==1&&!httpsTaskHandle);
 fail=0;https_server_netconn_init();assert(creates==2&&httpsTaskHandle==&httpsTaskControl);
 https_server_netconn_init();assert(creates==2);
 puts("HTTPS: 64 KiB static stack/TCB, word units, preserved priority, readiness/idempotence and retry: PASS");
}
'''
with tempfile.TemporaryDirectory(prefix='https-static-') as d:
 p=Path(d);(p/'test.c').write_text(code)
 subprocess.run(['gcc','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
