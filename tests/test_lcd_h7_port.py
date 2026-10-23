"""H7 LCD adapter checks: actual C with modeled registers/RTOS, plus linker map.
This cannot validate the physical panel, I2C wiring or cache/bus behaviour.
"""
from pathlib import Path
import subprocess, tempfile, shutil
root=Path(__file__).resolve().parents[1]
hal=r'''
#ifndef MOCK_H
#define MOCK_H
#include <stdint.h>
#include <stddef.h>
typedef struct { volatile uint32_t CR,ISR,IFCR,FGMAR,FGOR,FGPFCCR,OPFCCR,OMAR,OOR,NLR;} DMA_Type;
extern DMA_Type dma;
#define DMA2D (&dma)
#define DMA2D_ISR_TEIF 1U
#define DMA2D_ISR_TCIF 2U
#define DMA2D_ISR_CEIF 32U
#define DMA2D_CR_START 1U
#define DMA2D_CR_ABORT 4U
#define DMA2D_CR_TEIE (1U<<8)
#define DMA2D_CR_TCIE (1U<<9)
#define DMA2D_CR_CEIE (1U<<13)
#define DMA2D_IRQn 90
#define osWaitForever UINT32_MAX
#define osOK 0
#define osErrorOS 129
extern void *osDma2dSemph,*osDeviceMutex,*LcdUpdateSemaphoreId;
enum {Semphr_sdram};
typedef struct {uint32_t ErrorCode;struct {uint32_t ImageWidth,ImageHeight;} LayerCfg[1];struct {uint32_t AccumulatedActiveH;} Init;} LTDC_HandleTypeDef;
extern LTDC_HandleTypeDef hltdc;
int32_t osSemaphoreWait(void*,uint32_t);int osSemaphoreRelease(void*);
int osMutexWait(void*,uint32_t);void osMutexRelease(void*);
int TakeMutex(int,uint32_t);void GiveMutex(int);
void SCB_CleanDCache_by_Addr(uint32_t*,int32_t);
void NVIC_ClearPendingIRQ(int);void HAL_LTDC_IRQHandler(LTDC_HandleTypeDef*);
void HAL_LTDC_ProgramLineEvent(LTDC_HandleTypeDef*,uint32_t);
#define __DSB() ((void)0)
#endif
'''
test=r'''
#include "LCD_Hardware.c"
#include <assert.h>
#include <string.h>
#include <stdio.h>
DMA_Type dma;
LTDC_HandleTypeDef hltdc={.LayerCfg={{800,480}},.Init={484}};
volatile struct LCD_H7_Status lcdH7Status={.initialized=1};
void *osDma2dSemph=(void*)1,*osDeviceMutex=(void*)2,*LcdUpdateSemaphoreId=(void*)3;
static int token,mode,deviceHeld,memoryHeld,waits,locksAllowed=1;
static uint32_t programmedLine;
static uintptr_t cleaned;static int32_t cleanedLength;
static uint32_t source[800*480] __attribute__((aligned(32)));
int32_t osSemaphoreWait(void *s,uint32_t ms){
 assert(s==osDma2dSemph);assert(++waits<8);
 if(ms){assert(ms==1000&&deviceHeld&&memoryHeld);
  assert(dma.CR&DMA2D_CR_START);
  if(mode==1)return osErrorOS;
  dma.ISR=mode==2?DMA2D_ISR_TEIF:DMA2D_ISR_TCIF;DMA2D_IRQHandler();
 }
 if(token){token--;return osOK;}return osErrorOS;
}
int osSemaphoreRelease(void *s){if(s==osDma2dSemph)token++;else assert(s==LcdUpdateSemaphoreId);return 0;}
int osMutexWait(void*s,uint32_t timeout){assert(s==osDeviceMutex&&timeout==osWaitForever);deviceHeld=1;return osOK;}
void osMutexRelease(void*s){assert(s==osDeviceMutex&&deviceHeld);deviceHeld=0;}
int TakeMutex(int id,uint32_t timeout){assert(id==Semphr_sdram&&timeout==osWaitForever);memoryHeld=locksAllowed;return locksAllowed;}
void GiveMutex(int id){assert(id==Semphr_sdram&&memoryHeld);memoryHeld=0;}
void SCB_CleanDCache_by_Addr(uint32_t*p,int32_t n){cleaned=(uintptr_t)p;cleanedLength=n;}
void NVIC_ClearPendingIRQ(int id){assert(id==DMA2D_IRQn);}
void HAL_LTDC_IRQHandler(LTDC_HandleTypeDef*h){assert(h==&hltdc);}
void HAL_LTDC_ProgramLineEvent(LTDC_HandleTypeDef*h,uint32_t line){assert(h==&hltdc);programmedLine=line;}
static void transfer(uint32_t x,uint32_t y,uint32_t w,uint32_t h){waits=0;LCD_DisplayBuff(x,y,w,h,source);assert(!deviceHeld&&!memoryHeld);}
int main(void){
 transfer(0,0,800,480);assert(lcdH7Status.transfers==1&&lcdH7Status.last_error==0);
 assert(dma.OMAR==0x60000000&&dma.OPFCCR==1&&dma.FGPFCCR==0&&dma.NLR==((800U<<16)|480));
 assert(cleaned==(uintptr_t)source&&cleanedLength==sizeof(source));
 token=1;transfer(11,17,19,23);assert(lcdH7Status.transfers==2&&dma.OMAR==0x60000000+3*(17*800+11)&&dma.OOR==781);
 assert((cleanedLength&31)==0&&cleanedLength>=19*23*4);
 /* One strided partial update must configure one DMA for the whole region. */
 waits=0;uint32_t prior=lcdH7Status.transfers;
 uint32_t *part=source+183*800+48;
 LCD_DisplayPart(0,48,183,374,155);
 assert(lcdH7Status.transfers==prior+1&&dma.FGOR==426&&dma.OOR==426);
 assert(dma.NLR==((374U<<16)|155)&&dma.OMAR==0x60000000+3*(183*800+48));
 assert(cleaned==(uintptr_t)part&&cleanedLength==((154*800+374)*4+31)/32*32);
 waits=0;LCD_DisplayBuffStride(0,0,100,2,source,99);assert(lcdH7Status.last_error==-10);
 hltdc.ErrorCode=2;HAL_LTDC_ErrorCallback(&hltdc);
 assert(lcdH7Status.ltdc_error_flags==2&&lcdH7Status.ltdc_error_events==1);
 hltdc.ErrorCode=1;HAL_LTDC_ErrorCallback(&hltdc);assert(lcdH7Status.ltdc_error_flags==3);
 uint32_t before=lcdH7Status.transfers;transfer(799,479,2,2);assert(lcdH7Status.last_error==-10&&lcdH7Status.transfers==before);
 mode=1;transfer(0,0,2,2);assert(lcdH7Status.timeouts==1&&lcdH7Status.last_error==-13&&(dma.CR&DMA2D_CR_ABORT));
 mode=2;transfer(0,0,2,2);assert(lcdH7Status.dma_errors==1&&lcdH7Status.last_error==-14);
 mode=0;transfer(0,0,2,2);assert(lcdH7Status.last_error==0&&lcdH7Status.transfers==before+1);
 locksAllowed=0;transfer(0,0,2,2);assert(lcdH7Status.last_error==-12);
 HAL_LTDC_LineEventCallback(&hltdc);assert(programmedLine==485);
 puts("H7 LCD DMA adapter: RGB888 addresses, source cache alignment, CMSIS osOK semantics, IRQ, timeout/error and bounds: PASS");
}
'''
# Include the production partial-copy function, not a test reimplementation.
basic=(root/'Application/LCD/LCD_BasicGraphics.c').read_text()
a=basic.index('void LCD_DisplayPart(')
b=basic.index('\nvoid LCD_Show(void)',a)
partial='\n#define LCD_X 800U\n#define LCD_Y 480U\nstatic uint32_t *pLcd=source;\n'+basic[a:b]+'\n'
test=test.replace('int main(void){',partial+'int main(void){',1)

with tempfile.TemporaryDirectory(prefix='h7-lcd-port-') as folder:
    work=Path(folder)
    for name in ('stm32h7xx_hal.h','ltdc.h','timer.h'): (work/name).write_text(hal if name=='stm32h7xx_hal.h' else '#include "stm32h7xx_hal.h"\n')
    for name in ('LCD_Hardware.c','LCD_Hardware.h','LCD_PortH7.h'):shutil.copy(root/'Application/LCD'/name,work/name)
    (work/'test.c').write_text(test)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wno-pointer-to-int-cast','-fsanitize=address,undefined',str(work/'test.c'),'-o',str(work/'test')],check=True)
    subprocess.run([str(work/'test')],check=True)

# A missing STMPE811 must not let calibration delete the calling GUI task.
with tempfile.TemporaryDirectory(prefix='h7-touch-task-') as folder:
    work=Path(folder)
    (work/'FreeRTOS.h').write_text('''#include <stdint.h>
#include <stddef.h>
typedef void *xTaskHandle;
typedef uint32_t portTickType;
#define portBASE_TYPE long
''')
    (work/'task.h').write_text('''void vTaskDelayUntil(portTickType*,portTickType);
portTickType xTaskGetTickCount(void);
int xTaskCreate(void(*)(void*),char*,unsigned,void*,unsigned long,xTaskHandle*);
void vTaskDelete(xTaskHandle);
''')
    (work/'touch.h').write_text('#define SERVICE_TOUCH_PROB_TIME_MS 20\nvoid LCD_TOUCH_Service(void);\n')
    for name in ('TouchLcdTask.c','TouchLcdTask.h'):shutil.copy(root/'Application/Tasks'/name,work/name)
    (work/'test.c').write_text(r'''
#include "TouchLcdTask.c"
#include <assert.h>
static int creates,deletes;
void vTaskDelayUntil(portTickType*t,portTickType n){(void)t;(void)n;}
portTickType xTaskGetTickCount(void){return 0;}
void LCD_TOUCH_Service(void){}
int xTaskCreate(void(*fn)(void*),char*name,unsigned stack,void*arg,unsigned long priority,xTaskHandle*out){
 (void)fn;(void)name;(void)arg;assert(stack==2048&&priority==1);*out=(void*)1;creates++;return 1;
}
void vTaskDelete(xTaskHandle task){assert(task);deletes++;}
int main(void){
 Delete_TouchLcd_Task();assert(deletes==0);
 Create_TouchLcd_Task();Create_TouchLcd_Task();assert(creates==1);
 Delete_TouchLcd_Task();Delete_TouchLcd_Task();assert(deletes==1&&!vtask_TouchLcd_Handle);
 Create_TouchLcd_Task();assert(creates==2);
}
''')
    subprocess.run(['cc','-std=c11','-Wall',str(work/'test.c'),'-o',str(work/'test')],check=True)
    subprocess.run([str(work/'test')],check=True)
print('Touch task lifecycle: missing/duplicate task guards: PASS')

# When an ARM build exists, check actual linker output as well as the C model.
mapfile=root/'Debug/cloud/firmware.map'
if mapfile.exists():
    import re
    contents=mapfile.read_text()
    def symbol(name):
        match=re.search(r'0x([0-9a-fA-F]+)\s+'+re.escape(name)+r'\s*=',contents)
        assert match,name
        return int(match.group(1),16)
    assert symbol('__lcd_framebuffer_start__')==0x60000000
    assert symbol('__lcd_framebuffer_end__')-symbol('__lcd_framebuffer_start__')==800*480*3
    assert symbol('__lcd_work_start__')>=symbol('__lcd_framebuffer_end__')
    match=re.search(r'^\.sdram\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)',contents,re.M)
    assert match
    address,size=(int(x,16) for x in match.groups())
    assert address>=symbol('__lcd_work_end__') and address+size<=0x61000000
    print('ARM linker map: 0x60000000 framebuffer and disjoint SDRAM buffers within 16 MiB: PASS')
