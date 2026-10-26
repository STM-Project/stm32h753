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
 for(unsigned j=0;j<h;j++)for(unsigned i=0;i<w;i++){
  uint32_t c=pixels[j*w+i];unsigned o=3*((y+j)*800+x+i);
  frame[o]=(unsigned char)c;frame[o+1]=(unsigned char)(c>>8);frame[o+2]=(unsigned char)(c>>16);
 }
}
int main(void){
 frame=mmap((void*)LCD_H7_SDRAM_BASE,sizeof original,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
 assert(frame==(void*)LCD_H7_SDRAM_BASE);
 for(unsigned i=0;i<sizeof original;i++)original[i]=(unsigned char)(i*17);
 memcpy(frame,original,sizeof original);
 for(int i=0;i<=10;i++){
  assert(SCREEN_NavigationNext(i,1)==(i==10?0:i+1));
  assert(SCREEN_NavigationNext(i,-1)==(i==0?10:i-1));
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
 SCREEN_NavigationRestore();assert(displays==4&&!memcmp(frame,original,sizeof original));
 /* A panel redraw below the arrows must survive expiry, not restore an old snapshot. */
 frame[3*(240*800+5)]=23;memcpy(original,frame,sizeof original);
 SCREEN_NavigationDraw(5099);assert(displays==6);
 SCREEN_NavigationRestore();assert(!memcmp(frame,original,sizeof original));
 SCREEN_NavigationDraw(5100);assert(displays==8);
 SCREEN_NavigationDraw(7000);assert(displays==8);
 assert(SCREEN_NavigationInput(1,790,240,&dir));
 assert(SCREEN_NavigationInput(0,790,240,&dir)&&dir==1); /* hidden hotspot */
 SCREEN_NavigationEnter(0xfffffff0U,0);SCREEN_NavigationDraw(0x10U);assert(displays==10);
 SCREEN_NavigationRestore();assert(!memcmp(frame,original,sizeof original));
 SCREEN_NavigationDraw(0xfffffff0U+5000U);assert(displays==12);
 puts("PASS: prev/next wrap, held touch, release, canceled drags, normal panel input, hidden hotspots, ENTER guard, 5s expiry/tick wrap and exact RGB888 background restoration.");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 for name,text in HEADERS.items():(p/name).write_text(text)
 (p/'test.c').write_text(C)
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+d,'-I'+str(ROOT/'Application/LCD/Screens'),str(p/'test.c'),str(ROOT/'Application/LCD/Screens/SCREEN_Navigation.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
