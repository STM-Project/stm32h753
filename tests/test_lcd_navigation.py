"""Run the real navigation controller and overlay against a modeled RGB888 LCD."""
from pathlib import Path
import subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
HEADERS={
'LCD_Hardware.h': '''#include <stdint.h>
uint32_t LCD_GetXSize(void); uint32_t LCD_GetYSize(void);
void LCD_DisplayBuff(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*);
''',
'LCD_PortH7.h': '''#include <stdint.h>
#define LCD_H7_WIDTH 800U
#define LCD_H7_HEIGHT 480U
#define LCD_H7_SDRAM_BASE 0x60000000UL
extern struct Status {int initialized;} lcdH7Status;
''',
'timer.h': '''#include <stdint.h>
#define osWaitForever 0xffffffffU
#define osOK 0
#define osDeviceMutex 1
#define Semphr_sdram 2
int osMutexWait(int,unsigned); void osMutexRelease(int);
int TakeMutex(int,unsigned); void GiveMutex(int);
void SCB_InvalidateDCache_by_Addr(uint32_t*,int32_t);
#define __DSB() ((void)0)
'''}
C=r'''
#define _GNU_SOURCE
#include "SCREEN_Navigation.h"
#include "LCD_PortH7.h"
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
static unsigned displays;
static unsigned char original[800*480*3];
static unsigned char *frame;
struct Status lcdH7Status={1};
static int mutex, memory;
int osMutexWait(int id,unsigned t){(void)id;(void)t;assert(!mutex);mutex=1;return 0;}
void osMutexRelease(int id){(void)id;assert(mutex&&!memory);mutex=0;}
int TakeMutex(int id,unsigned t){(void)id;(void)t;assert(mutex&&!memory);memory=1;return 1;}
void GiveMutex(int id){(void)id;assert(memory);memory=0;}
void SCB_InvalidateDCache_by_Addr(uint32_t *p,int32_t n){assert(memory&&mutex);assert(!((uintptr_t)p%32)&&!(n%32));}
uint32_t LCD_GetXSize(void){return 800;} uint32_t LCD_GetYSize(void){return 480;}
void LCD_DisplayBuff(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t *pixels){
 assert(!mutex&&!memory&&x+w<=800&&y+h<=480);displays++;
 int masked=SCREEN_NavigationUpdate(x,y,w,h,pixels,w);
 for(unsigned j=0;j<h;j++)for(unsigned i=0;i<w;i++){
  if(masked&&y+j>=210&&y+j<270&&(x+i<30||x+i>=770))continue;
  uint32_t c=pixels[j*w+i];unsigned o=3*((y+j)*800+x+i);
  frame[o]=(unsigned char)c;frame[o+1]=(unsigned char)(c>>8);frame[o+2]=(unsigned char)(c>>16);
 }
}
int main(void){
 frame=mmap((void*)LCD_H7_SDRAM_BASE,sizeof original,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
 assert(frame==(void*)LCD_H7_SDRAM_BASE);
 for(unsigned i=0;i<sizeof original;i++)original[i]=(unsigned char)(i*17);
 memcpy(frame,original,sizeof original);
 for(int i=0;i<=12;i++){
  assert(SCREEN_NavigationNext(i,1)==(i==12?0:i+1));
  assert(SCREEN_NavigationNext(i,-1)==(i==0?12:i-1));
 }
 SCREEN_NavigationEnter(100,0);int dir;
 assert(SCREEN_NavigationInput(1,790,240,&dir)&&!dir);
 for(int i=0;i<50;i++)assert(SCREEN_NavigationInput(1,790,240,&dir)&&!dir);
 assert(SCREEN_NavigationInput(0,790,240,&dir)&&dir==1);
 assert(!SCREEN_NavigationInput(0,790,240,&dir)&&!dir);
 assert(SCREEN_NavigationInput(1,0,210,&dir)&&!dir);
 assert(SCREEN_NavigationInput(0,0,210,&dir)&&dir==-1);
 assert(SCREEN_NavigationInput(1,10,240,&dir));
 assert(SCREEN_NavigationInput(1,100,240,&dir));
 assert(SCREEN_NavigationInput(0,100,240,&dir)&&!dir); /* canceled drag */
 assert(!SCREEN_NavigationInput(1,400,240,&dir));
 assert(!SCREEN_NavigationInput(1,790,240,&dir));
 assert(!SCREEN_NavigationInput(0,790,240,&dir)&&!dir); /* drag began in panel */
 SCREEN_NavigationEnter(100,1);
 assert(SCREEN_NavigationInput(1,790,240,&dir)&&!dir);
 assert(SCREEN_NavigationInput(0,790,240,&dir)&&!dir); /* no click-through after ENTER */
 SCREEN_NavigationDraw(100);assert(displays==2&&memcmp(frame,original,sizeof original));
 for(unsigned y=0;y<480;y++)for(unsigned x=0;x<800;x++){
  unsigned o=3*(y*800+x);
  if(y<210||y>=270||(x>=30&&x<770))assert(!memcmp(frame+o,original+o,3));
 }
 unsigned char arrows[2][30*60*3];
 for(unsigned side=0;side<2;side++)for(unsigned row=0;row<60;row++)
  memcpy(arrows[side]+row*90,frame+3*((210+row)*800+(side?770:0)),90);
 for(unsigned t=101;t<5100;t+=20)SCREEN_NavigationDraw(t);
 assert(displays==2); /* no erase/repaint every LCD cycle */
 static uint32_t update[800*480];
 for(unsigned i=0;i<800*480;i++)update[i]=0xff123456;
 LCD_DisplayBuff(0,0,800,480,update);assert(displays==3);
 for(unsigned side=0;side<2;side++)for(unsigned row=0;row<60;row++)
  assert(!memcmp(arrows[side]+row*90,frame+3*((210+row)*800+(side?770:0)),90));
 assert(update[240*800+5]==0xff123456); /* source never overwritten */
 for(unsigned i=0;i<800*480;i++){original[3*i]=0x56;original[3*i+1]=0x34;original[3*i+2]=0x12;}
 SCREEN_NavigationDraw(5100);assert(displays==5&&!memcmp(frame,original,sizeof original));
 SCREEN_NavigationDraw(7000);assert(displays==5);
 assert(SCREEN_NavigationInput(1,790,240,&dir));
 assert(SCREEN_NavigationInput(0,790,240,&dir)&&dir==1);
 SCREEN_NavigationEnter(0xfffffff0U,0);SCREEN_NavigationDraw(0x10U);assert(displays==7);
 SCREEN_NavigationDraw(0xfffffff0U+5000U);assert(displays==9&&!memcmp(frame,original,sizeof original));
 puts("PASS: prev/next wrap, held touch, release, canceled drags, normal panel input, hidden hotspots, ENTER guard, steady arrows over 250 idle cycles/full redraw, unmodified source, 5s expiry/tick wrap and latest RGB888 background restoration.");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 for name,text in HEADERS.items():(p/name).write_text(text)
 (p/'test.c').write_text(C)
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+d,'-I'+str(ROOT/'Application/LCD/Screens'),str(p/'test.c'),str(ROOT/'Application/LCD/Screens/SCREEN_Navigation.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
