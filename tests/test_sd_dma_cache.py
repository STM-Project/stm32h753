"""Run actual FatFs disk DMA paths with modeled write-back scratch memory."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'FATFS/Target/sd_diskio.c').read_text()
def fn(signature):
 a=s.index(signature);return s[a:s.index('\n}',a)+2]
c=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t BYTE;typedef uint32_t DWORD;typedef unsigned UINT;typedef int DRESULT;
#define osCMSIS 0x10000U
#define ENABLE_SD_DMA_CACHE_MAINTENANCE 1
#define ENABLE_SCRATCH_BUFFER
#define BLOCKSIZE 512
#define SD_TIMEOUT 1000
#define RES_OK 0
#define RES_ERROR 1
#define MSD_OK 0
#define MSD_ERROR 1
#define SD_TRANSFER_OK 0
#define osEventMessage 1
#define READ_CPLT_MSG 1
#define WRITE_CPLT_MSG 2
static int SDQueueID,msg,lastReadScratch,cleanCount;
static uint32_t ticks;
static uint8_t scratch[512] __attribute__((aligned(32)));
static uint8_t physical[512],written[1024];static unsigned writtenBytes;
static uint8_t storage[1120] __attribute__((aligned(32)));
typedef struct {int status;struct{int v;} value;} osEvent;
static int SD_CheckStatusWithTimeout(unsigned t){(void)t;return 0;}
static uint32_t osKernelSysTick(void){return ticks++;}
static int BSP_SD_GetCardState(void){return MSD_OK;}
static osEvent osMessageGet(int q,unsigned timeout){(void)q;(void)timeout;return (osEvent){osEventMessage,{msg}};}
static void SCB_InvalidateDCache_by_Addr(uint32_t *p,uint32_t n){
 assert(((uintptr_t)p&31)==0&&(n&31)==0);
 if(p==(uint32_t*)scratch){assert(n==512);memcpy(scratch,physical,512);}
 else assert((uint8_t*)p==storage+32&&n<=1024); /* No adjacent dirty owner. */
}
static void SCB_CleanDCache_by_Addr(uint32_t *p,uint32_t n){
 assert(((uintptr_t)p&31)==0&&(n&31)==0);
 if(p==(uint32_t*)scratch){assert(n==512);memcpy(physical,scratch,512);cleanCount++;}
}
static int BSP_SD_ReadBlocks_DMA(uint32_t *p,uint32_t sector,unsigned count){
 (void)sector;msg=READ_CPLT_MSG;lastReadScratch=p==(uint32_t*)scratch;
 if(lastReadScratch){assert(count==1);memset(physical,0x3c,512);}
 else memset(p,0x3c,count*512);
 return MSD_OK;
}
static int BSP_SD_WriteBlocks_DMA(uint32_t *p,uint32_t sector,unsigned count){
 (void)sector;msg=WRITE_CPLT_MSG;
 if(p==(uint32_t*)scratch){assert(count==1);assert(!memcmp(physical,scratch,512));}
 assert(writtenBytes+count*512<=sizeof written);
 memcpy(written+writtenBytes,p==(uint32_t*)scratch?physical:(uint8_t*)p,count*512);
 writtenBytes+=count*512;return MSD_OK;
}
'''
c+=fn('DRESULT SD_read(')+'\n'+fn('DRESULT SD_write(')
c+=r'''
int main(void){
 for(unsigned offset=0;offset<32;offset++){
  memset(storage,0xa5,sizeof storage);assert(SD_read(0,storage+32+offset,0,2)==RES_OK);
  assert(lastReadScratch==(offset!=0));
  for(unsigned i=0;i<32+offset;i++)assert(storage[i]==0xa5);
  for(unsigned i=32+offset;i<32+offset+1024;i++)assert(storage[i]==0x3c);
  for(unsigned i=32+offset+1024;i<sizeof storage;i++)assert(storage[i]==0xa5);
 }
 for(unsigned offset=0;offset<4;offset++){
  for(unsigned i=0;i<1024;i++)storage[32+offset+i]=(uint8_t)i;
  writtenBytes=0;cleanCount=0;
  int result=SD_write(0,storage+32+offset,0,2);
  if(result!=RES_OK)fprintf(stderr,"offset=%u result=%d writes=%u msg=%d ticks=%u\n",offset,result,writtenBytes,msg,ticks);
  assert(result==RES_OK);
  assert(writtenBytes==1024&&!memcmp(written,storage+32+offset,1024));
  if(offset)assert(cleanCount==2);
 }
 puts("PASS: SD DMA reads at all 32 alignments preserve neighboring bytes; scratch writes clean each sector and accept WRITE completion.");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(c)
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-Wno-sign-compare','-Wno-unused-parameter','-Wno-pointer-to-int-cast','-Wno-int-to-pointer-cast','-no-pie','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
