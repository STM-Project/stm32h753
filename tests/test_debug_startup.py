"""Queued LCD diagnostics must work before the logger task is first scheduled."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'Application/General_Utilities/_debug.c').read_text()
startup = (ROOT / 'Core/Src/freertos.c').read_text().split('void StartDefaultTask(void const * argument)', 1)[1]
assert startup.index('DEBUG_Init ();') < startup.index('CreateLogTask ();') < startup.index('LCD_H7_Start ()')

def function(signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]

harness = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef void *QueueHandle_t;
typedef void *TaskHandle_t;
#define portBASE_TYPE int
#define pdFALSE 0
#define pdPASS 1
#define LOG_QUEUE_SIZE 20
#define HEAP_MAX_ALLOC_BYTE 2048
#define CONDITION(c,a,b) ((c)?(a):(b))
#define CACHE_ALLIGN_LEN(n) (((n)+31)&~31)
#define mini_strlen(s) ((int)strlen(s))
#define pvPortMalloc malloc
#define vPortFree free
static QueueHandle_t xLogQueue;
static TaskHandle_t xLogTaskHandle;
static int queueFails,taskFails,queueCreates,taskCreates,warnings;
static char *message;
static void vLogTask(void *p){(void)p;}
static void Dbg(int on,char *text){assert(on&&text);warnings++;}
static QueueHandle_t xQueueCreate(int count,size_t size){
 assert(count==20&&size==sizeof(char*));queueCreates++;
 return queueFails?NULL:(void*)1;
}
static int xQueueSend(QueueHandle_t q,const void *p,unsigned ticks){
 assert(q==(void*)1&&ticks==0&&!message);message=*(char *const*)p;return pdPASS;
}
static int xTaskCreate(void(*fn)(void*),char *name,int stack,void *arg,unsigned priority,TaskHandle_t *handle){
 assert(xLogQueue==(void*)1&&fn==vLogTask&&!strcmp(name,"vLogTask"));
 assert(stack==1024&&!arg&&priority==4);taskCreates++;
 if(taskFails)return pdFALSE;
 *handle=(void*)2;return pdPASS; /* Task deliberately has NOT run yet. */
}
'''
harness += function('void DbgDmaQue(int on, char *txt, int lenTxt)\t\t/*')
harness += '\n' + function('void CreateLogTask(void)')
harness += r'''
int main(void){
 queueFails=1;CreateLogTask();assert(!xLogQueue&&!xLogTaskHandle&&warnings==1);
 queueFails=0;taskFails=1;CreateLogTask();assert(xLogQueue&&!xLogTaskHandle&&warnings==2);
 taskFails=0;CreateLogTask();assert(xLogTaskHandle&&queueCreates==2&&taskCreates==2);
 char fontInfo[]="Arial 18: loaded";DbgDmaQue(1,fontInfo,0);
 memset(fontInfo,'x',sizeof(fontInfo)-1);
 assert(message&&!strcmp(message,"Arial 18: loaded"));free(message);message=NULL;
 CreateLogTask();assert(queueCreates==2&&taskCreates==2);
 puts("PASS: logger queue before LCD/task scheduling, copied font diagnostics, allocation failures and retry/idempotence.");
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'test.c').write_text(harness)
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(path/'test.c'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
