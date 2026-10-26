"""LCD dashboard interaction and partial repaint regression checks."""
from pathlib import Path
import tempfile, subprocess, shutil
root = Path(__file__).resolve().parents[1]
resources = {
    'test.c': r"""#include "SCREEN_TemperaturePk.c"
#include "font_metrics.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
uint32_t pLcd[800*480];
uint16_t screenTouchState;
XY_Touch_Struct screenTouchPos;
static XY_Touch_Struct position;
static uint32_t tick;
static int down,samples,fontFailure,shows,parts,registered,resolution=800;
static int fontIndex[17];
static int currentTemperature=255;
static FILE *textLog;
static int decimalChecks,fractionChecks,textPixels,subtitleY;
uint32_t HAL_GetTick(void){return tick;}
uint32_t LCD_GetXSize(void){return resolution;}
uint32_t LCD_GetYSize(void){return 480;}
void SCREEN_ResetAllParameters(void){memset(pLcd,0,sizeof(pLcd));}
void LCD_Buffer(uint16_t w,uint16_t x,uint16_t y,uint32_t color){assert(w==800&&x<800&&y<480);pLcd[y*w+x]=color;}
void LCD_Shape(uint32_t x,uint32_t y,void *shape,uint32_t w,uint32_t h,uint32_t fc,uint32_t fill,uint32_t bk){
 (void)shape;(void)fc;(void)bk;assert(x+w<=800&&y+h<=480);
 for(unsigned j=y;j<y+h;j++)for(unsigned i=x;i<x+w;i++)pLcd[j*800+i]=fill;
}
int LCD_LoadFont_ChangeColor(int size,int style,uint32_t id){
 (void)style;if(fontFailure)return -3;assert(id<17);
 fontIndex[id]=size==16?0:size==24?1:size==28?2:size==72?3:size==9?4:size==14?5:6;return (int)id;
}
int LCD_GetFontHeight(int f){return mockSizes[fontIndex[f]];}
int LCD_GetWholeStrPxlWidth(int font,char *text,int space,int fixed){
 (void)space;(void)fixed;int n=0;for(;*text;text++)n+=mockWidths[fontIndex[font]][(unsigned char)*text];return n;
}
void LCD_StrChangeColor(int f,int x,int y,char*t,int h,int s,uint32_t bg,uint32_t color,uint8_t max,int fixed){
 (void)h;(void)s;(void)bg;(void)max;(void)fixed;
 if(f==dashboard.fonts[2] && fixed==ConstWidth){
   assert(y==140);
   if(strcmp(t,".")==0){assert(x==270);decimalChecks++;}
   else if(x==290){assert(strlen(t)==1);fractionChecks++;}
   else assert(x+LCD_GetWholeStrPxlWidth(f,t,0,ConstWidth)==270);
 }

 assert(x>=0&&y>=0&&x+LCD_GetWholeStrPxlWidth(f,t,0,0)<=800&&y+mockSizes[fontIndex[f]]<=480);
 if(!strcmp(t,"Ostatni pomiar ze STM32"))subtitleY=y;
 /* Model translucent glyph pixels so uncleared lower rows visibly accumulate. */
 if(textPixels){
  int width=LCD_GetWholeStrPxlWidth(f,t,0,0),height=LCD_GetFontHeight(f);
  for(int j=0;j<height;j++)for(int i=0;i<width;i++){
   unsigned index=(unsigned)((y+j)*800+x+i);
   pLcd[index]=PanelMix(pLcd[index],color,128);
  }
 }
 if(textLog)fprintf(textLog,"%d\t%d\t%d\t%08x\t%d\t%s\n",x,y,mockSizes[fontIndex[f]],color,(fontIndex[f]==2 || fontIndex[f]==5),t);
}
void LCD_Show(void){shows++;}
void LCD_DisplayPart(uint32_t offset,uint32_t x,uint32_t y,uint32_t w,uint32_t h){assert(offset==0&&x+w<=800&&y+h<=480);parts++;}
void LCD_TOUCH_DeleteSelectTouch(uint16_t id){assert(id>=61000&&id<=61003);registered=0;}
void LCD_TOUCH_DeleteAllSetTouch(void){registered=0;}
int LCDTOUCH_Set(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t id,uint16_t index,uint8_t param){
 assert(x+w<=800&&y+h<=480&&id==ID_TOUCH_POINT&&index>=61000&&index<=61003&&param==pressRelease);registered=1;return 0;
}
int LCD_TOUCH_isPress(void){return down?press:release;}
XY_Touch_Struct LCD_TOUCH_GetPos(void){return position;}
uint16_t LCD_TOUCH_GetTypeAndPosition(XY_Touch_Struct*p){*p=position;return registered&&down?61000:0;}
static int sensor(void){samples++;return currentTemperature;}
static void touch(int isDown,int x,int y){down=isDown;position.x=x;position.y=y;}
static void preview(const char *name,int on){
 char path[150];uint8_t pk=(uint8_t)on;
 snprintf(path,sizeof(path),"%s.text",name);textLog=fopen(path,"w");assert(textLog);
 LCD_TemperaturePkPanel2(LCD_TEMP_EXIT,NULL,NULL);touch(0,0,0);
 assert(LCD_TemperaturePkPanel2(LCD_TEMP_ENTER,sensor,&pk)==0);fclose(textLog);textLog=NULL;
 snprintf(path,sizeof(path),"%s.ppm",name);FILE *out=fopen(path,"wb");assert(out);
 fprintf(out,"P6\n800 480\n255\n");for(int i=0;i<800*480;i++){unsigned char b[3]={pLcd[i]>>16,pLcd[i]>>8,pLcd[i]};fwrite(b,1,3,out);}fclose(out);
}

int main(void){
 uint8_t pk=0;currentTemperature=991;
 assert(LCD_TemperaturePkPanel2(LCD_TEMP_ENTER,sensor,&pk)==0&&samples==1);
 tick=499;LCD_TemperaturePkPanel2(LCD_TEMP_UPDATE,sensor,&pk);assert(samples==1);
 tick=500;LCD_TemperaturePkPanel2(LCD_TEMP_UPDATE,sensor,&pk);assert(samples==2);
 touch(1,570,205);LCD_TemperaturePkPanel2(0,sensor,&pk);assert(pk==0);
 touch(0,570,205);assert(LCD_TemperaturePkPanel2(0,sensor,&pk)==1&&pk==1);
 touch(1,650,205);LCD_TemperaturePkPanel2(0,sensor,&pk);
 touch(0,650,205);assert(LCD_TemperaturePkPanel2(0,sensor,&pk)==1&&pk==0);
 touch(1,570,205);LCD_TemperaturePkPanel2(0,sensor,&pk);
 touch(0,700,240);assert(LCD_TemperaturePkPanel2(0,sensor,&pk)==0&&pk==0);
 for(int i=0;i<140;i++){tick+=500;currentTemperature=i%3==0?INT_MIN:(i%2?-800:1500);LCD_TemperaturePkPanel2(0,sensor,&pk);}
 assert(dashboard.count==120&&dashboard.head<120);
 LCD_TemperaturePkPanel2(2,NULL,NULL);tick=UINT32_MAX-250;touch(0,0,0);
 LCD_TemperaturePkPanel2(1,sensor,&pk);int before=samples;tick=249;
 LCD_TemperaturePkPanel2(0,sensor,&pk);assert(samples==before+1);
 fontFailure=1;assert(LCD_TemperaturePkPanel2(1,sensor,&pk)==-3);fontFailure=0;
 currentTemperature=991;preview("dashboard-on",1);
 for(int i=0;i<120;i++){tick+=500;currentTemperature=250+(i<60?i*10:600-i*5);LCD_TemperaturePkPanel2(0,sensor,&pk);}
 LCD_TemperaturePkPanel2(2,NULL,NULL);touch(0,0,0);
 assert(LCD_TemperaturePkPanel2(1,NULL,&pk)==0&&lcdPanel2Temperature10==255);
 int last=lcdPanel2Temperature10,ups=0,downs=0;
 for(int i=0;i<4000;i++){
  tick+=499;LCD_TemperaturePkPanel2(0,NULL,&pk);assert(lcdPanel2Temperature10==last);
  tick++;LCD_TemperaturePkPanel2(0,NULL,&pk);
  int value=lcdPanel2Temperature10,delta=value-last;
  assert(value>=-500&&value<=900&&abs(delta)<=74);
  ups+=delta>0;downs+=delta<0;last=value;
 }
 assert(ups>0&&downs>0&&dashboard.count==120);
 int checks=decimalChecks;
 const int transitions[]={99,100,999,1000,-1,-100,1500,INT_MIN};
 for(unsigned i=0;i<sizeof(transitions)/sizeof(transitions[0]);i++){
  tick+=500;currentTemperature=transitions[i];LCD_TemperaturePkPanel2(0,sensor,&pk);
 }
 assert(decimalChecks==checks+8&&fractionChecks==decimalChecks);
 LCD_TemperaturePkPanel2(2,NULL,NULL);tick=1000;touch(0,0,0);
 LCD_TemperaturePkPanel2(1,NULL,&pk);assert(lcdPanel2Temperature10==255);
 tick=16000;LCD_TemperaturePkPanel2(0,NULL,&pk);assert(lcdPanel2Temperature10==145);
 tick=31000;LCD_TemperaturePkPanel2(0,NULL,&pk);assert(lcdPanel2Temperature10==255);
 const int temps[]={-500,200,900};const uint32_t colors[]={0xFF168BFFU,0xFFFFDF20U,0xFFFF1C16U};
 for(int i=0;i<3;i++){tick+=500;currentTemperature=temps[i];LCD_TemperaturePkPanel2(0,sensor,&pk);assert(pLcd[192*800+117]==colors[i]);}
 DashSurface(115,340,30,30);uint32_t beforeEdge=pLcd[349*800+121];
 DashCurveAA(120,350,130,353,DASH_CYAN);
 assert(pLcd[349*800+121]!=beforeEdge&&pLcd[349*800+121]!=DASH_CYAN);
 LCD_TemperaturePkPanel2(2,NULL,NULL);tick=50000;touch(0,0,0);currentTemperature=-123;
 LCD_TemperaturePkPanel2(1,sensor,&pk);uint8_t savedPk=pk;
 int readsBefore=samples;
 touch(1,115,350);LCD_TemperaturePkPanel2(0,sensor,&pk);
 assert(dashboard.tooltip==0&&pk==savedPk&&samples==readsBefore);
 assert(dashboard.historyTick[0]==50000);
 touch(1,735,400);LCD_TemperaturePkPanel2(0,sensor,&pk);assert(dashboard.tooltip==0);
 for(int i=0;i<140;i++){tick+=500;currentTemperature=i%2?-500:900;LCD_TemperaturePkPanel2(0,sensor,&pk);}
 assert(dashboard.tooltip==119&&dashboard.count==120&&pk==savedPk);
 touch(1,115,331);LCD_TemperaturePkPanel2(0,sensor,&pk);assert(dashboard.tooltip==0);
 touch(1,400,380);LCD_TemperaturePkPanel2(0,sensor,&pk);assert(dashboard.tooltip>0&&dashboard.tooltip<119);
 touch(0,400,380);LCD_TemperaturePkPanel2(0,sensor,&pk);assert(dashboard.tooltip==-1);
 touch(1,100,320);LCD_TemperaturePkPanel2(0,sensor,&pk);
 touch(1,400,380);LCD_TemperaturePkPanel2(0,sensor,&pk);assert(dashboard.tooltip==-1);
 touch(0,400,380);LCD_TemperaturePkPanel2(0,sensor,&pk);
 touch(1,735,331);currentTemperature=INT_MIN;tick+=500;LCD_TemperaturePkPanel2(0,sensor,&pk);assert(dashboard.tooltip==119);
 touch(0,570,205);LCD_TemperaturePkPanel2(0,sensor,&pk);assert(pk==savedPk&&dashboard.tooltip==-1);
 dashboard.count=0;touch(1,115,331);LCD_TemperaturePkPanel2(0,sensor,&pk);assert(dashboard.tooltip==-1);
 touch(0,115,331);LCD_TemperaturePkPanel2(0,sensor,&pk);
 LCD_TemperaturePkPanel2(2,NULL,NULL);assert(!dashboard.active);
 /* Fixed text must survive repeated partial repaints without darkening. */
 textPixels=1;LCD_TemperaturePkPanel2(LCD_TEMP_EXIT,NULL,NULL);
 touch(0,0,0);currentTemperature=500;
 assert(LCD_TemperaturePkPanel2(LCD_TEMP_ENTER,sensor,&pk)==0);
 assert(subtitleY>=210);
 uint32_t left[80*12],right[60*12],caption[120*12];
 for(int j=0;j<12;j++){
  memcpy(left+j*80,pLcd+(410+j)*800+115,80*4);
  memcpy(right+j*60,pLcd+(410+j)*800+695,60*4);
  memcpy(caption+j*120,pLcd+(subtitleY+j)*800+180,120*4);
 }
 for(int repeat=0;repeat<5;repeat++){
  tick+=500;LCD_TemperaturePkPanel2(LCD_TEMP_UPDATE,sensor,&pk);
  for(int j=0;j<12;j++){
   assert(!memcmp(left+j*80,pLcd+(410+j)*800+115,80*4));
   assert(!memcmp(right+j*60,pLcd+(410+j)*800+695,60*4));
   assert(!memcmp(caption+j*120,pLcd+(subtitleY+j)*800+180,120*4));
  }
 }
 puts("Dark dashboard: 500 ms, ON/OFF, drag cancellation, 120-sample ring, invalid values, tick wrap, bounds: PASS");
}
""",
    'test_support.h': '#ifndef TEST_SUPPORT_H\n#define TEST_SUPPORT_H\n#include <stdint.h>\n#include <stddef.h>\ntypedef struct {uint16_t x,y;} XY_Touch_Struct;\nextern uint16_t screenTouchState;\nextern XY_Touch_Struct screenTouchPos;\nenum {press=1,release=0,pressRelease=2,ID_TOUCH_POINT=0};\nenum {FONT_9=9,FONT_14_bold=14,FONT_36=36,FONT_16=16,FONT_24=24,FONT_28_bold=28,FONT_72=72,Arial=0};\nenum {fontID_14=13,fontID_15=14,fontID_16=15,fontID_17=16};\nenum {fullHight=-3,NoConstWidth=0,ConstWidth=1};\n#define LCD_Rectangle ((void*)1)\nuint32_t HAL_GetTick(void);\nvoid SCREEN_ResetAllParameters(void);\nvoid LCD_Buffer(uint16_t,uint16_t,uint16_t,uint32_t);\nvoid LCD_Shape(uint32_t,uint32_t,void*,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);\nvoid LCD_StrChangeColor(int,int,int,char*,int,int,uint32_t,uint32_t,uint8_t,int);\nint LCD_GetFontHeight(int);\nint LCD_GetWholeStrPxlWidth(int,char*,int,int);\nint LCD_LoadFont_ChangeColor(int,int,uint32_t);\nvoid LCD_Show(void);\nvoid LCD_DisplayPart(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);\nuint32_t LCD_GetXSize(void);\nuint32_t LCD_GetYSize(void);\nvoid LCD_TOUCH_DeleteSelectTouch(uint16_t);\nvoid LCD_TOUCH_DeleteAllSetTouch(void);\nint LCDTOUCH_Set(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint8_t);\nint LCD_TOUCH_isPress(void);\nXY_Touch_Struct LCD_TOUCH_GetPos(void);\nuint16_t LCD_TOUCH_GetTypeAndPosition(XY_Touch_Struct*);\n#endif\n',
    'font_metrics.h': 'static const int mockSizes[7]={21,32,37,96,12,19,48};\nstatic const unsigned short mockWidths[7][128]={{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6,7,12,12,19,14,4,7,7,8,12,6,7,6,6,12,12,12,12,12,12,12,12,12,12,6,6,12,12,12,12,21,14,14,15,15,14,13,16,15,6,10,14,12,18,15,16,14,16,15,14,13,15,14,20,14,14,13,6,6,6,10,12,7,12,12,10,12,12,6,12,12,5,5,10,5,18,12,12,12,12,7,10,6,12,10,15,10,10,10,7,5,7,12,16},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,11,18,18,28,21,6,11,11,12,19,9,11,9,9,18,18,18,18,18,18,18,18,18,18,9,9,19,19,19,18,32,21,21,23,23,21,20,25,23,9,16,21,18,27,23,25,21,25,23,21,20,23,21,30,21,21,20,9,9,9,15,18,11,18,18,16,18,18,9,18,18,7,7,16,7,27,18,18,18,18,11,16,9,18,16,23,16,16,16,11,8,11,19,24},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,12,18,21,21,33,27,9,12,12,14,22,10,12,10,10,21,21,21,21,21,21,21,21,21,21,12,12,22,22,22,23,36,27,27,27,27,25,23,29,27,10,21,27,23,31,27,29,25,29,27,25,23,27,25,35,25,25,23,12,10,12,22,21,12,21,23,21,23,21,12,23,23,10,10,21,10,33,23,23,23,23,14,21,12,23,21,29,21,21,18,14,10,14,22,28},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,27,27,34,53,53,85,64,18,32,32,37,56,27,32,27,27,53,53,53,53,53,53,53,53,53,53,27,27,56,56,56,53,97,64,64,69,69,64,59,75,69,27,48,64,53,80,69,75,64,75,69,64,59,69,64,91,64,64,59,27,27,27,45,53,32,53,53,48,53,53,27,53,53,21,21,48,21,80,53,53,53,53,32,48,27,53,48,69,48,48,48,32,25,32,56,72},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,4,7,7,11,8,2,4,4,5,7,3,4,3,3,7,7,7,7,7,7,7,7,7,7,3,3,7,7,7,7,12,8,8,9,9,8,7,9,9,3,6,8,7,10,9,9,8,9,9,8,7,9,8,11,8,8,7,3,3,3,6,7,4,7,7,6,7,7,3,7,7,3,3,6,3,10,7,7,7,7,4,6,3,7,6,9,6,6,6,4,3,4,7,9},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,6,9,11,11,17,14,5,6,6,7,11,5,6,5,5,11,11,11,11,11,11,11,11,11,11,6,6,11,11,11,12,19,14,14,14,14,13,12,15,14,5,11,14,12,16,14,15,13,15,14,13,12,14,13,18,13,13,12,6,5,6,11,11,6,11,12,11,12,11,6,12,12,5,5,11,5,17,12,12,12,12,7,11,6,12,11,15,11,11,10,7,5,7,11,14},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,13,17,27,27,43,32,9,16,16,19,28,13,16,13,13,27,27,27,27,27,27,27,27,27,27,13,13,28,28,28,27,49,32,32,35,35,32,29,37,35,13,24,32,27,40,35,37,32,37,35,32,29,35,32,45,32,32,29,13,13,13,23,27,16,27,27,24,27,27,13,27,27,11,11,24,11,40,27,27,27,27,16,24,13,27,24,35,24,24,24,16,12,16,28,36}};',
}
resources['test_support.h'] += '\nvoid LCD_DisplayBuffStride(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*,uint32_t);\n'
resources['test.c'] += '\nvoid LCD_DisplayBuffStride(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t *source,uint32_t stride){assert(source==pLcd+y*800+x&&stride==800);LCD_DisplayPart(0,x,y,w,h);}\n'
with tempfile.TemporaryDirectory(prefix="stm32-lcd-panel-") as directory:
    work = Path(directory)
    for name, content in resources.items():
        (work / name).write_text(content)
    for name in ("SCREEN_ReadPanel.h", "LCD_BasicGraphics.h", "LCD_fonts_images.h", "LCD_Hardware.h", "touch.h"):
        (work / name).write_text('#include "test_support.h"\n')
    for name in ("SCREEN_TemperaturePk.c", "SCREEN_TemperaturePk.h"):
        shutil.copyfile(root / "Application/LCD/Screens" / name, work / name)
    subprocess.run(["cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-I" + str(work),
                    str(work / "test.c"), "-lm", "-o", str(work / "test")], check=True)
    subprocess.run([str(work / "test")], cwd=work, check=True)
