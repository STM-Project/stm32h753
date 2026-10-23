"""Exercise actual debug polling with modeled H7 DMA/cache and independent TX."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
debug=(ROOT/'Application/General_Utilities/_debug.c').read_text()
uart=(ROOT/'Core/Src/usart.c').read_text()
def function(source,signature):
    a=source.index(signature)
    return source[a:source.index('\n}',a)+2]
c=r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#define RECV_BUFF_SIZE 128
static char dbgRecvBuffer[RECV_BUFF_SIZE],dmaMemory[RECV_BUFF_SIZE];
static int huart7,txBusy=1,rxRunning=1,invalidates,aborts;
static void SCB_InvalidateDCache_by_Addr(uint32_t *address,int size){
 assert(address==(uint32_t*)dbgRecvBuffer&&size==RECV_BUFF_SIZE);
 memcpy(dbgRecvBuffer,dmaMemory,sizeof dmaMemory);invalidates++;
}
static int HAL_UART_AbortReceive(int *uart){assert(uart==&huart7);rxRunning=0;aborts++;return 0;}
#define CACHE_ALLIGN_LEN(n) (((n)+31)&~31)
#define UART_CLEAR_FEF 1
#define UART_CLEAR_PEF 2
#define UART_CLEAR_OREF 4
#define UART_CLEAR_NEF 8
#define UART_CLEAR_IDLEF 16
#define UART_CLEAR_RTOF 32
#define UART_RXDATA_FLUSH_REQUEST 1
static void SCB_CleanDCache_by_Addr(uint32_t *buffer,int size){
 assert(buffer==(uint32_t*)dbgRecvBuffer&&size==RECV_BUFF_SIZE);
 memcpy(dmaMemory,dbgRecvBuffer,size);
}
static void __HAL_UART_CLEAR_FLAG(int *uart,int flags){assert(uart==&huart7&&flags==63&&txBusy);}
static void __HAL_UART_SEND_REQ(int *uart,int request){assert(uart==&huart7&&request==1&&txBusy);}
static void UART_ClearFlags2(int *uart){assert(uart==&huart7);}
static int HAL_UART_Receive_DMA(int *uart,uint8_t *buffer,uint16_t size){
 assert(uart==&huart7&&buffer==(uint8_t*)dbgRecvBuffer&&size==RECV_BUFF_SIZE&&!rxRunning);
 rxRunning=1;return 0;
}
'''
c+=function(uart,'void DEBUG_ReceiveStart(uint8_t* buffer, uint16_t len)')+'\n'
c+=function(uart,'void DEBUG_ReceiveStop(void)')+'\n'
for signature in ('void DEBUG_InvalidateDCache(void)','int DEBUG_IsTxtReceive(char *txt)','int DEBUG_RcvStr(char *txt)'):
    c+=function(debug,signature)+'\n'
c+=r'''
int main(void){
 /* LCD polls a cached zero before WiFi has handled the UART interrupt. */
 memcpy(dmaMemory,"6\r\n",3);assert(dbgRecvBuffer[0]==0);
 assert(!DEBUG_RcvStr("abc")&&aborts==0&&txBusy);
 assert(DEBUG_RcvStr("6")&&aborts==1&&rxRunning&&txBusy);
 assert(!DEBUG_RcvStr("6")&&aborts==1); /* consume only once */
 memcpy(dmaMemory,"cloud\r\n",7);assert(DEBUG_IsTxtReceive("cloud")&&txBusy);
 memset(dmaMemory,'x',sizeof dmaMemory);assert(!DEBUG_RcvStr("6"));
 dmaMemory[127]='6';assert(DEBUG_RcvStr("6")&&txBusy);
 memset(dmaMemory,'x',sizeof dmaMemory);assert(!DEBUG_RcvStr("xx6"));
 assert(!DEBUG_RcvStr(NULL)&&!DEBUG_RcvStr(""));
 assert(invalidates>=9&&aborts==3);
 puts("PASS: LCD detects DMA command 6 without WiFi cache refresh; single consumption, full-buffer bounds and uninterrupted queued TX.");
}
'''
with tempfile.TemporaryDirectory() as d:
    p=Path(d);(p/'test.c').write_text(c)
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
