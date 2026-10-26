"""Execute real debug report and trace hooks with modeled FreeRTOS snapshots."""
from pathlib import Path
import re, shutil, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
config=(ROOT/'Core/Inc/FreeRTOSConfig.h').read_text()
hooks='\n'.join(re.findall(r'^#define traceTASK_(?:CREATE|DELETE).*$',config,re.M))
assert 'configRECORD_STACK_HIGH_ADDRESS 1' in config
assert config.index('#define traceTASK_CREATE')>config.index('/* USER CODE BEGIN Defines */')
esp=(ROOT/'Application/esp32wroom.c').read_text()
assert esp.index('DEBUG_IsTxtReceive ( "|" )')<esp.index('DEBUG_IsTxtReceive ( "cloud" )')
headers={
'FreeRTOS.h':'''#include <stdint.h>
#include <stddef.h>
#define configMAX_TASK_NAME_LEN 16
#define configTOTAL_HEAP_SIZE 2097152U
typedef uint32_t StackType_t; typedef unsigned UBaseType_t;
''',
'task.h':'''#include "FreeRTOS.h"
typedef struct {void *xHandle; const char *pcTaskName; StackType_t *pxStackBase;unsigned usStackHighWaterMark;} TaskStatus_t;
void vTaskSuspendAll(void);int xTaskResumeAll(void);
unsigned uxTaskGetSystemState(TaskStatus_t*,unsigned,void*);
''',
'portable.h':'''#include <stddef.h>
typedef struct {size_t xAvailableHeapSpaceInBytes,xMinimumEverFreeBytesRemaining,xSizeOfLargestFreeBlockInBytes,xNumberOfFreeBlocks;} HeapStats_t;
void vPortGetHeapStats(HeapStats_t*);
''',
'_debug.h':'void DbgDmaQue(int,char*,int);\n'}
C=r'''
#include "memory_debug.c"
#include <assert.h>
static unsigned count,suspended,chunks;
static int mutate;
static char names[64][16];
static TaskStatus_t tasks[64];
static char text[20000];
void vTaskSuspendAll(void){assert(!suspended);suspended=1;}
int xTaskResumeAll(void){assert(suspended);suspended=0;if(mutate)memset(names,'X',sizeof names);return 0;}
unsigned uxTaskGetSystemState(TaskStatus_t*out,unsigned cap,void*t){(void)t;assert(suspended);if(count>cap)return 0;memcpy(out,tasks,count*sizeof *out);return count;}
void vPortGetHeapStats(HeapStats_t*h){assert(!suspended);*h=(HeapStats_t){1000000,900000,800000,3};}
void DbgDmaQue(int on,char *s,int length){assert(on&&!suspended&&length>0&&length<960);assert((size_t)length==strlen(s));assert(strlen(text)+length<sizeof text);strcat(text,s);chunks++;}
static void setup(unsigned i,const char*name,unsigned total,unsigned free_words,uintptr_t base){
 snprintf(names[i],16,"%s",name);tasks[i]=(TaskStatus_t){(void*)(uintptr_t)(i+1),names[i],(StackType_t*)base,free_words};MemoryDebugTaskCreate(tasks[i].xHandle,total);
}
int main(void){
 StackType_t stack[32];struct {StackType_t *pxStack,*pxEndOfStack;} tcb={stack,stack+30};
 traceTASK_CREATE(&tcb);assert(stacks[0].stack_bytes==124);traceTASK_DELETE(&tcb);assert(!stacks[0].handle);
 setup(0,"LCD",16096,1000,0x60010000);setup(1,"IDLE",512,64,0x24000000);setup(2,"HTTPS",65536,4000,0x60b00000);
 count=3;mutate=1;MemoryDebugPrint();
 assert(strstr(text,"rezerwa=2097152, wolne=1000000, zajete/rezerwa=1097152"));
 assert(strstr(text,"min. wolne=900000, najwiekszy wolny blok=800000, wolnych blokow=3"));
 assert(strstr(text,"| LCD             |    16096 |      12096 |       4000 |  75% | SDRAM"));
 assert(strstr(text,"| IDLE            |      512 |        256 |        256 |  50% | RAM_D1"));
 assert(strstr(text,"| HTTPS           |    65536 |      49536 |      16000 |  75% | SDRAM"));
 assert(!strstr(text,"XXXXX")); /* no dangling name pointer after resume */
 MemoryDebugTaskDelete(tasks[0].xHandle);MemoryDebugTaskCreate(tasks[0].xHandle,8192);
 assert(stacks[0].stack_bytes==8192);MemoryDebugTaskDelete(tasks[0].xHandle);
 mutate=0;setup(0,"recreated",4096,0,0x30010000);text[0]=0;MemoryDebugPrint();assert(strstr(text,"100% | RAM_D2"));
 for(unsigned i=0;i<64;i++)setup(i,"worker",1024,128,0x60020000);
 count=64;text[0]=0;chunks=0;MemoryDebugPrint();assert(chunks>1);unsigned n=0;
 for(char*p=text;(p=strstr(p,"| worker"));p++)n++;
 assert(n==64); /* all rows survive chunking within logger limits */
 puts("PASS: real trace stack size, bytes/peak/minimum/percent, static stacks, copied names, task reuse, heap stats and 64-row bounded queued report.");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 for n,s in headers.items():(p/n).write_text(s)
 for n in ['memory_debug.c','memory_debug.h']:shutil.copy(ROOT/'Application/General_Utilities'/n,p/n)
 (p/'test.c').write_text(hooks+'\n'+C)
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+d,str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
