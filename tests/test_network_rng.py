"""Run the H7 mbedTLS entropy adapter: real bytes, tails and HAL failures."""
from pathlib import Path
import tempfile,subprocess
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='network-rng-') as d:
 p=Path(d);(p/'mbedtls').mkdir()
 (p/'mbedtls_config.h').write_text('#define MBEDTLS_ENTROPY_HARDWARE_ALT\n')
 (p/'mbedtls/entropy.h').write_text('#define MBEDTLS_ERR_ENTROPY_SOURCE_FAILED -60\n')
 (p/'rng.h').write_text('#include <stdint.h>\n#define HAL_OK 0\nextern int hrng;\nint HAL_RNG_GenerateRandomNumber(int*,uint32_t*);\n')
 (p/'cmsis_os.h').write_text('#include <stddef.h>\ntypedef void *osMutexId;\n#define osOK 0\nint osMutexWait(osMutexId,unsigned);\nint osMutexRelease(osMutexId);\n')
 (p/'test.c').write_text(r'''
#include "hardware_rng.c"
#include <assert.h>
#include <stdio.h>
int hrng;osMutexId networkRngMutex=(void*)1;
static int calls,held,failAt;
int osMutexWait(osMutexId mutex,unsigned timeout){assert(mutex==(void*)1&&timeout==1000&&!held);held=1;return 0;}
int osMutexRelease(osMutexId mutex){assert(mutex==(void*)1&&held);held=0;return 0;}
int HAL_RNG_GenerateRandomNumber(int *h,uint32_t *out){assert(h==&hrng&&held);calls++;*out=calls==1?0x12345678:0x90ABCDEF;return calls==failAt?1:0;}
int main(void){unsigned char data[8]={0};size_t n=99;
 assert(mbedtls_hardware_poll(NULL,data,7,&n)==0&&n==7&&!held);
 unsigned char expected[]={0x78,0x56,0x34,0x12,0xef,0xcd,0xab};assert(!memcmp(data,expected,7)&&data[7]==0);
 calls=0;failAt=2;assert(mbedtls_hardware_poll(NULL,data,8,&n)==-60&&n==4&&!held);
 networkRngMutex=NULL;assert(mbedtls_hardware_poll(NULL,data,8,&n)==-60&&n==0);
 puts("H7 TLS entropy: distinct random bytes, partial word, initialized output length and failure cleanup: PASS");}
''')
 subprocess.run(['gcc','-Wall','-Wextra','-Werror','-I'+str(p),'-I'+str(root/'MBEDTLS/Target'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
