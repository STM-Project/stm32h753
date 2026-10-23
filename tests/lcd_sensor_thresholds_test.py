"""Real LCD editing + HTTP model/mailbox tests with mocked graphics and SD bytes."""
from pathlib import Path
import tempfile, subprocess, shutil
root=Path(__file__).resolve().parents[1]
resources = {
    'test_support.h': '#ifndef TEST_SUPPORT_H\n#define TEST_SUPPORT_H\n#include <stdint.h>\n#include <stddef.h>\ntypedef struct {uint16_t x,y;} XY_Touch_Struct;\nextern uint16_t screenTouchState;\nextern XY_Touch_Struct screenTouchPos;\nenum {press=1,release=0,pressRelease=2,ID_TOUCH_POINT=0};\nenum {FONT_12=12,FONT_14=14,FONT_18_bold=18,FONT_9=9,FONT_14_bold=14,FONT_36=36,FONT_16=16,FONT_24=24,FONT_28_bold=28,FONT_72=72,Arial=0};\nenum {fontID_14=13,fontID_15=14,fontID_16=15,fontID_17=16};\nenum {fullHight=-3,NoConstWidth=0,ConstWidth=1};\n#define LCD_Rectangle ((void*)1)\n#define LCD_RoundRectangle ((void*)2)\ntypedef void* figureShape;\nuint32_t HAL_GetTick(void);\nvoid SCREEN_ResetAllParameters(void);\nvoid LCD_Buffer(uint16_t,uint16_t,uint16_t,uint32_t);\nvoid LCD_Shape(uint32_t,uint32_t,void*,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);\nvoid LCD_StrChangeColor(int,int,int,char*,int,int,uint32_t,uint32_t,uint8_t,int);\nint LCD_GetFontHeight(int);\nint LCD_GetWholeStrPxlWidth(int,char*,int,int);\nint LCD_LoadFont_ChangeColor(int,int,uint32_t);\nvoid LCD_Show(void);\nvoid LCD_DisplayPart(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);\nuint32_t LCD_GetXSize(void);\nuint32_t LCD_GetYSize(void);\nvoid LCD_TOUCH_DeleteSelectTouch(uint16_t);\nvoid LCD_TOUCH_DeleteAllSetTouch(void);\nint LCDTOUCH_Set(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint8_t);\nint LCD_TOUCH_isPress(void);\nXY_Touch_Struct LCD_TOUCH_GetPos(void);\nuint16_t LCD_TOUCH_GetTypeAndPosition(XY_Touch_Struct*);\n#endif\n',
    'Keyboard.h': '#include "test_support.h"\n#define KEYBOARD_SETTXT_KEYS KEY_Q,KEY_W,KEY_E,KEY_R,KEY_T,KEY_Y,KEY_U,KEY_I,KEY_O,KEY_P,KEY_A,KEY_S,KEY_D,KEY_F,KEY_G,KEY_H,KEY_J,KEY_K,KEY_L,KEY_big,KEY_Z,KEY_X,KEY_C,KEY_V,KEY_B,KEY_N,KEY_M,KEY_back,KEY_alt,KEY_exit,KEY_space,KEY_comma,KEY_dot,KEY_enter,KEY_field,KEY_style\nenum {KeysAutoSize=0,KeysNotDel=1,KeysDel=2};\nvoid KEYBOARD_SetGeneral(int,int,int,int,int,int,int,int,int,int);\nint KEYBOARD_StartUp(int,figureShape,uint8_t,uint16_t,uint16_t,uint16_t,uint16_t,uint8_t,uint16_t,uint16_t,uint8_t);\nvoid KEYBOARD__ServiceSetTxt(int,int,figureShape,uint8_t,uint16_t,uint16_t,uint16_t,uint16_t,uint8_t,uint16_t,uint16_t,uint8_t,int,int,int,int,int,int,int,int,int,uint32_t,char*,int);\n',
    'font_metrics.h': 'static const int mockSizes[3]={16,24,19};\nstatic const unsigned short mockWidths[3][128]={{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,6,9,9,14,11,3,5,5,6,9,4,5,4,4,9,9,9,9,9,9,9,9,9,9,4,4,9,9,9,9,16,11,11,12,12,11,10,12,12,4,8,11,9,13,12,12,11,12,12,11,10,12,11,15,11,11,10,4,4,4,8,9,5,9,9,8,9,9,4,9,9,4,4,8,4,13,9,9,9,9,5,8,4,9,8,12,8,8,8,5,4,5,9,12},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,8,11,13,13,21,17,6,8,8,9,14,7,8,7,7,13,13,13,13,13,13,13,13,13,13,8,8,14,14,14,15,23,17,17,17,17,16,15,19,17,7,13,17,15,20,17,19,16,19,17,16,15,17,16,23,16,16,15,8,7,8,14,13,8,13,15,13,15,13,8,15,15,7,7,13,7,21,15,15,15,15,9,13,8,15,13,19,13,13,12,9,7,9,14,18},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,5,7,11,11,17,13,4,6,6,7,11,5,6,5,5,11,11,11,11,11,11,11,11,11,11,5,5,11,11,11,11,19,13,13,14,14,13,12,15,14,5,10,13,11,16,14,15,13,15,14,13,12,14,13,18,13,13,12,5,5,5,9,11,6,11,11,10,11,11,5,11,11,4,4,10,4,16,11,11,11,11,6,10,5,11,10,14,10,10,10,6,5,6,11,14}};',
    'test.c': r"""#include "LCD_SensorThresholds.c"
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
static int decimalChecks,fractionChecks;
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
 fontIndex[id]=size==12?0:size==18?1:2;return (int)id;
}
int LCD_GetFontHeight(int f){return mockSizes[fontIndex[f]];}
int LCD_GetWholeStrPxlWidth(int font,char *text,int space,int fixed){
 (void)space;(void)fixed;int n=0;for(;*text;text++)n+=mockWidths[fontIndex[font]][(unsigned char)*text];return n;
}
void LCD_StrChangeColor(int f,int x,int y,char*t,int h,int s,uint32_t bg,uint32_t color,uint8_t max,int fixed){
 (void)h;(void)s;(void)bg;(void)max;(void)fixed;

 assert(x>=0&&y>=0&&x+LCD_GetWholeStrPxlWidth(f,t,0,0)<=800&&y+mockSizes[fontIndex[f]]<=480);
 if(textLog)fprintf(textLog,"%d\t%d\t%d\t%08x\t%d\t%s\n",x,y,mockSizes[fontIndex[f]],color,(fontIndex[f]==2 || fontIndex[f]==5),t);
}
void LCD_Show(void){shows++;}
void LCD_DisplayPart(uint32_t offset,uint32_t x,uint32_t y,uint32_t w,uint32_t h){assert(offset==0&&x+w<=800&&y+h<=480);parts++;}
void LCD_TOUCH_DeleteSelectTouch(uint16_t id){assert(id>=62000&&id<=62135);registered=0;}
void LCD_TOUCH_DeleteAllSetTouch(void){registered=0;}
int LCDTOUCH_Set(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t id,uint16_t index,uint8_t param){
 assert(x+w<=800&&y+h<=480&&id==ID_TOUCH_POINT&&index>=62000&&index<=62135&&param==pressRelease);registered=1;return 0;
}
int LCD_TOUCH_isPress(void){return down?press:release;}
XY_Touch_Struct LCD_TOUCH_GetPos(void){return position;}
uint16_t LCD_TOUCH_GetTypeAndPosition(XY_Touch_Struct*p){*p=position;return registered&&down?(uint16_t)tick:0;}
static int sensor(void){samples++;return currentTemperature;}
static void touch(int isDown,int x,int y){down=isDown;position.x=x;position.y=y;}

static int keyboardCalls,keyboardCreates,saveResult,saveCalls;
static void* lastShape;
static uint8_t nativeStartTouch;
static unsigned lastKeyEvent;
static int nativePreviousKey,keyReleases;
static char *keyboardBuffer;
void KEYBOARD_SetGeneral(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;}
int KEYBOARD_StartUp(int type,figureShape shape,uint8_t bold,uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t space,uint16_t forTouch,uint16_t start,uint8_t erase){
 (void)bold;(void)forTouch;(void)erase;
 if(type&&shape){nativeStartTouch=(uint8_t)start;assert(nativeStartTouch==start&&start+35<=UINT8_MAX);assert(x==24&&y==115&&w==KeysAutoSize&&h==10&&space==10&&start==TOUCH_KEYS);keyboardCreates++;}
 return 0;
}
void KEYBOARD__ServiceSetTxt(int k,int key,figureShape shape,uint8_t bold,uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t space,uint16_t forTouch,uint16_t start,uint8_t erase,int releaseKey,int action,int big,int back,int alt,int enter,int field,int style,int exitKey,uint32_t color,char *buffer,int size){
 (void)bold;(void)x;(void)y;(void)w;(void)h;(void)space;(void)forTouch;(void)erase;(void)big;(void)back;(void)alt;(void)enter;(void)field;(void)style;(void)exitKey;(void)color;
 assert(k==0&&releaseKey==0&&action==KEY_Q&&key>=0&&key<=KEY_style&&size==64);
 if(shape)assert(start==TOUCH_KEYS&&start==nativeStartTouch);else assert(start==0);
 if(key==KEY_All_release){nativePreviousKey=0;keyReleases++;}
 if(!shape&&key==KEY_Q){
  size_t length=strlen(buffer);assert(length+3<(size_t)size);
  /* Emulate the native contract: repeat replaces the held key's character;
   * release resets the native previous-key state. */
  if(nativePreviousKey==KEY_Q&&length)buffer[length-1]='0';
  else {buffer[length]='q';buffer[length+1]=0;}
  lastKeyEvent=nativeStartTouch;
 }
 if(key!=KEY_All_release)nativePreviousKey=key;


 keyboardCalls++;keyboardBuffer=buffer;lastShape=shape;
}
static unsigned char disk[HTTP_SENSOR_SETTINGS_BLOB_SIZE];
static int save(const void *bytes,size_t size,void *context){
 (void)context;assert(size==sizeof disk);saveCalls++;
 if(saveResult)memcpy(disk,bytes,size);
 return saveResult;
}
static int load(void *bytes,size_t size,void *context){
 (void)context;assert(size==sizeof disk);memcpy(bytes,disk,size);return 1;
}
static int step(int isDown,int x,int y){touch(isDown,x,y);return LCD_SensorThresholds(0);}
static int click(int x,int y){assert(step(1,x,y)>=0);return step(0,x,y);}
static void edit(int x,int y,const char *value){assert(click(x,y)==0&&ui.editing>=0&&keyboardBuffer==ui.edit);strcpy(keyboardBuffer,value);assert(click(700,170)==0&&ui.editing==-1);}
static void preview(void){
 textLog=fopen("thresholds.text","w");assert(textLog);assert(Draw()==0);fclose(textLog);textLog=NULL;
 FILE*out=fopen("thresholds.ppm","wb");assert(out);fprintf(out,"P6\n800 480\n255\n");
 for(int i=0;i<800*480;i++){unsigned char b[3]={pLcd[i]>>16,pLcd[i]>>8,pLcd[i]};fwrite(b,1,3,out);}fclose(out);
}
int main(void){
 (void)sensor;(void)decimalChecks;(void)fractionChecks;
 LCD_SensorThreshold value={0},before;
 assert(!LCD_SensorThresholdSetField(NULL,0,0,"a"));
 assert(LCD_SensorThresholdSetField(&value,0,1,"-3276.8")&&value.max1==INT16_MIN);
 before=value;assert(!LCD_SensorThresholdSetField(&value,0,1,"3276.8")&&!memcmp(&before,&value,sizeof(value)));
 assert(!LCD_SensorThresholdSetField(&value,0,1,"999999999999999999"));
 assert(!LCD_SensorThresholdSetField(&value,0,1,"12.34"));
 assert(LCD_SensorThresholdSetField(&value,0,6,"-1,2")&&value.correction==-12);
 assert(!LCD_SensorThresholdSetField(&value,1,1,"100.1"));assert(!LCD_SensorThresholdSetField(&value,1,1,"-0.1"));
 assert(!LCD_SensorThresholdSetField(&value,0,5,"-0.1"));
 assert(LCD_SensorThresholdSetField(&value,1,5,"100.0"));
 assert(!LCD_SensorThresholdSetField(&value,1,6,"-100.1"));
 assert(LCD_SensorThresholdSetField(&value,0,7,"5999"));assert(!LCD_SensorThresholdSetField(&value,0,7,"6000"));
 assert(!LCD_SensorThresholdSetField(&value,0,7,"01:20"));
 assert(!LCD_SensorThresholdSetField(&value,0,0,"1234567890123456789012345678901"));
 assert(!LCD_SensorThresholdSetField(&value,0,0,"abc\ndef"));
 assert(LCD_SensorThresholdSetField(&value,0,0,"Nazwa czujnika"));
 saveResult=1;HTTP_SensorSettingsInit(NULL,save,NULL);HTTP_SensorLCDService();
 assert(LCD_SensorThresholds(3)==-1);resolution=640;assert(LCD_SensorThresholds(1)==-2);resolution=800;
 fontFailure=1;assert(LCD_SensorThresholds(1)==-3);fontFailure=0;
 assert(LCD_SensorThresholds(1)==0&&ui.port==1);
 assert(!strcmp(ui.draft[0].name,"Temperatura")&&!strcmp(ui.draft[1].name,"Wilgotnosc"));
 edit(200,205,"25.5");assert(ui.draft[0].max1==255&&HTTP_SensorSettingsGet(1,0)->max1==0&&ui.dirty);
 assert(click(750,25)==0&&ui.port==1); /* dirty draft blocks changing port */
 assert(click(100,440)==0&&ui.pending);
 assert(HTTP_SensorSettingsGet(1,0)->max1==0);
 HTTP_SensorLCDService();assert(LCD_SensorThresholds(0)==1);
 assert(HTTP_SensorSettingsGet(1,0)->max1==255&&!ui.dirty);
 edit(580,205,"100.1");assert(ui.draft[1].max1==0&&!ui.dirty);
 edit(580,205,"80.0");assert(ui.draft[1].max1==800&&ui.dirty);
 assert(click(250,440)==0&&ui.draft[1].max1==0&&!ui.dirty);
 assert(click(80,155)==0&&ui.editing==0);strcpy(keyboardBuffer,"Nowa nazwa");
 assert(click(700,222)==0&&!strcmp(ui.draft[0].name,"Temperatura"));
 edit(80,155,"Pomiar temperatury");
 saveResult=0;
 assert(click(100,440)==0&&ui.pending);HTTP_SensorLCDService();
 assert(LCD_SensorThresholds(0)==0&&saveCalls==2&&ui.dirty&&!strcmp(HTTP_SensorSettingsGet(1,0)->name,"Temperatura"));
 saveResult=1;assert(click(100,440)==0&&ui.pending);HTTP_SensorLCDService();
 assert(LCD_SensorThresholds(0)==1&&saveCalls==3&&!strcmp(HTTP_SensorSettingsGet(1,0)->name,"Pomiar temperatury"));
 assert(click(750,25)==0&&ui.port==2);assert(click(570,25)==0&&ui.port==1);
 assert(click(200,205)==0);tick=nativeStartTouch;touch(1,150,350);assert(LCD_SensorThresholds(0)==0&&keyboardCalls>keyboardCreates&&lastShape==0);
 assert(lastKeyEvent==TOUCH_KEYS&&strchr(ui.edit,'q')!=NULL);
 int releases=keyReleases;size_t heldLength=strlen(ui.edit);
 tick=0;touch(1,150,350);LCD_SensorThresholds(0);
 assert(keyReleases==releases); /* repeat-wait gap is still held */
 tick=nativeStartTouch;LCD_SensorThresholds(0);
 assert(keyReleases==releases&&strlen(ui.edit)==heldLength&&ui.edit[heldLength-1]=='0');
 tick=0;touch(0,150,350);LCD_SensorThresholds(0);assert(keyReleases==releases+1);
 tick=nativeStartTouch;touch(1,150,350);LCD_SensorThresholds(0);
 assert(strlen(ui.edit)==heldLength+1&&ui.edit[heldLength]=='q');

 tick=TOUCH_KEYS+KEY_exit-KEY_Q;assert(LCD_SensorThresholds(0)==0&&ui.editing==-1);touch(0,0,0);tick=0;LCD_SensorThresholds(0);
 for(int field=0;field<8;field++)for(int c=0;c<2;c++){
  int x,y,w,h;FieldRect(c,field,&x,&y,&w,&h);assert(click(x+4,y+4)==0&&ui.editing==c*8+field);assert(click(700,222)==0);
 }
 ui.port=20;strcpy(ui.draft[0].name,"WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");preview();
 assert(LCD_SensorThresholds(2)==0&&!ui.active);
 assert(LCD_SensorThresholds(1)==0&&ui.port==1&&HTTP_SensorSettingsGet(1,0)->max1==255);

 /* Browser commit appears automatically on an already open, clean LCD page. */
 const char *target="/lpc.cgi/A117?n_s_Czujki_nazwa_001=WWW&n_s_Czujki_prog_max1_001=30.0&n_s_Czujki_prog_max2_001=40.0&n_s_Czujki_prog_min1_001=1.0&n_s_Czujki_prog_min2_001=2.0&n_s_Czujki_hist_001=0.5&n_s_Czujki_korekcja_001=-1.2&n_s_Czujki_czas_reak_001=02:03&n_109_zapisz=";
 unsigned port;assert(HTTP_SensorSettingsApplyTarget(target,strlen(target),&port)==1);
 HTTP_SensorLCDService();assert(LCD_SensorThresholds(0)==0);
 assert(ui.draft[0].max1==300&&!strcmp(ui.draft[0].name,"WWW")&&ui.draft[0].delay_seconds==123);
 /* Concurrent HTTP edit must not be silently overwritten by an LCD draft. */
 edit(200,205,"42.5");HttpSensorSettings remote[2]={*HTTP_SensorSettingsGet(1,0),*HTTP_SensorSettingsGet(1,1)};
 remote[0].max1=500;assert(HTTP_SensorSettingsSavePort(1,remote)==1);HTTP_SensorLCDService();
 assert(click(100,440)==0);HTTP_SensorLCDService();assert(LCD_SensorThresholds(0)==0&&ui.dirty);
 assert(HTTP_SensorSettingsGet(1,0)->max1==500&&ui.draft[0].max1==425);
 assert(click(250,440)==0&&ui.draft[0].max1==500&&!ui.dirty);
 /* LCD humidity save uses the same durable representation, reload and HTTP getter. */
 edit(565,205,"75.5");assert(click(100,440)==0);HTTP_SensorLCDService();assert(LCD_SensorThresholds(0)==1);
 assert(HTTP_SensorSettingsGet(1,1)->max1==755);
 assert(HTTP_SensorSettingsInit(load,save,NULL)==1);HTTP_SensorLCDService();
 assert(HTTP_SensorSettingsGet(1,1)->max1==755&&HTTP_SensorSettingsGet(1,0)->max1==500);
 puts("Threshold page: field validation, keyboard routing, save/cancel, port selection, shared HTTP/LCD commits, SD errors/reload, conflict protection and display bounds: PASS");
}
""",
    'FreeRTOS.h': '#define taskENTER_CRITICAL() ((void)0)\n#define taskEXIT_CRITICAL() ((void)0)\n',
    'task.h': '#include "FreeRTOS.h"\n',
}
with tempfile.TemporaryDirectory(prefix="lcd-thresholds-") as directory:
    work=Path(directory)
    for name,content in resources.items(): (work/name).write_text(content)
    for name in ("SCREEN_ReadPanel.h","LCD_BasicGraphics.h","LCD_fonts_images.h","LCD_Hardware.h","touch.h"):
        (work/name).write_text('#include "test_support.h"\n')
    for name in ("LCD_SensorThresholds.c","LCD_SensorThresholds.h"):
        shutil.copyfile(root/"Application/LCD/SensorThresholds"/name,work/name)
    subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror","-fsanitize=address,undefined","-I"+str(root/"Application/HTTP"),"-I"+str(work),str(work/"test.c"),str(root/"Application/HTTP/http_sensor_settings.c"),str(root/"Application/HTTP/http_sensor_lcd.c"),"-lm","-o",str(work/"test")],check=True)
    subprocess.run([str(work/"test")],cwd=work,check=True)
