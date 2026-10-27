"""Execute both real LCD screens: clipping, scroll, touch feedback and live PWM.
Also save 800x480 previews with host Arial-compatible glyphs (not hardware timing).
"""
from pathlib import Path
import re,subprocess,tempfile,shutil,os
from PIL import Image,ImageFont
ROOT=Path(__file__).resolve().parents[1]
mode=re.search(r'typedef enum\s*\{\s*HTTP_MODE_RELAY.*?\} HTTP_PORT_MODE;',(ROOT/'Application/wwwPages.h').read_text(),re.S)[0]
SUPPORT=r'''
#ifndef SUPPORT_H
#define SUPPORT_H
#include <stdint.h>
#include <stddef.h>
typedef struct {uint16_t x,y;} XY_Touch_Struct;
extern XY_Touch_Struct screenTouchPos;
enum {press=1,release=0,FONT_12=12,FONT_16=16,FONT_28_bold=28,Arial=0,fontID_14=14,fullHight=-3,NoConstWidth=0};
void SCREEN_ResetAllParameters(void);void LCD_TOUCH_DeleteAllSetTouch(void);
int LCD_LoadFont_ChangeColor(int,int,uint32_t);int LCD_GetFontHeight(int);
int LCD_GetWholeStrPxlWidth(int,char*,int,int);
void LCD_StrChangeColorWindow(uint32_t,uint32_t,uint32_t,int,int,int,char*,int,int,uint32_t,uint32_t,uint8_t,int);
uint32_t LCD_GetXSize(void);uint32_t LCD_GetYSize(void);uint32_t HAL_GetTick(void);
void LCD_DisplayBuff(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*);
void LCD_DisplayBuffStride(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*,uint32_t);
int LCD_TOUCH_isPress(void);XY_Touch_Struct LCD_TOUCH_GetPos(void);uint16_t LCD_TOUCH_GetTypeAndPosition(XY_Touch_Struct*);
#endif
'''
C=r'''
#include "SCREEN_HttpPanels.c"
#include <assert.h>
#include <stdlib.h>
uint32_t pLcd[800*480];static uint32_t shown[800*480];
static HTTP_LCDPanelData model;
static int down,active,pwm_calls,toggle_calls,draws,viewport_draws;
static unsigned pwm_port,pwm_percent;
static uint32_t tick;
static XY_Touch_Struct pos;
XY_Touch_Struct screenTouchPos;
void SCREEN_ResetAllParameters(void){}void LCD_TOUCH_DeleteAllSetTouch(void){}
void HTTP_LCDPanelActive(int a){active=a;}
int HTTP_LCDPanelRead(HTTP_LCDPanelData*out){*out=model;return 1;}
int HTTP_LCDPanelPwm(unsigned p,unsigned v){assert(p>=21&&p<=23&&v<=100);pwm_calls++;pwm_port=p;pwm_percent=v;model.ports[p-1].pwm=v;return 1;}
int HTTP_LCDPanelToggle(unsigned p){assert(p>=21&&p<=33);toggle_calls++;return 1;}
int LCD_LoadFont_ChangeColor(int size,int style,uint32_t id){(void)style;(void)id;return size;}
int LCD_GetFontHeight(int font){return font==28?35:font==16?21:16;}
int LCD_GetWholeStrPxlWidth(int f,char*t,int s,int c){(void)s;(void)c;int n=0;for(;*t;t++)n+=glyph_width[f==28?2:f==16?1:0][(unsigned char)*t];return n;}
void LCD_StrChangeColorWindow(uint32_t offset,uint32_t w,uint32_t h,int font,int x,int y,char*t,int only,int space,uint32_t bg,uint32_t color,uint8_t a,int fixed){
 (void)only;(void)space;(void)bg;(void)a;(void)fixed;assert(offset==SCRATCH&&offset+w*h<=800*480&&x>=0&&y>=0);
 int f=font==28?2:font==16?1:0;
 for(;*t;t++){
  unsigned c=(unsigned char)*t;assert(x+glyph_width[f][c]<=(int)w);
  for(int row=0;row<glyph_height[f];row++)for(int col=0;col<glyph_width[f][c];col++){
   assert(y+row<(int)h);unsigned alpha=glyphs[glyph_offset[f][c]+row*glyph_width[f][c]+col];
   uint32_t *p=pLcd+offset+(y+row)*w+x+col;uint32_t mixed=0xff000000;
   for(int shift=0;shift<=16;shift+=8)mixed|=((((color>>shift)&255)*alpha+((*p>>shift)&255)*(255-alpha))/255)<<shift;
   *p=mixed;
  }
  x+=glyph_width[f][c];
 }
}
uint32_t LCD_GetXSize(void){return 800;}uint32_t LCD_GetYSize(void){return 480;}uint32_t HAL_GetTick(void){return tick;}
int LCD_TOUCH_isPress(void){return down;}XY_Touch_Struct LCD_TOUCH_GetPos(void){return pos;}
uint16_t LCD_TOUCH_GetTypeAndPosition(XY_Touch_Struct*p){*p=pos;return 0;}
void LCD_DisplayBuff(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t *s){
 assert(x+w<=800&&y+h<=480&&s>=pLcd&&s+w*h<=pLcd+800*480);draws++;if(w==800&&h==368)viewport_draws++;
 for(unsigned row=0;row<h;row++)memcpy(shown+(y+row)*800+x,s+row*w,w*4);
}
void LCD_DisplayBuffStride(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t *s,uint32_t stride){
 assert(x+w<=800&&y+h<=480&&s>=pLcd&&s+(h-1)*stride+w<=pLcd+800*480&&stride==800);draws++;
 for(unsigned row=0;row<h;row++)memcpy(shown+(y+row)*800+x,s+row*stride,w*4);
}
static void touch(int d,int x,int y){down=d;pos=(XY_Touch_Struct){(uint16_t)x,(uint16_t)y};tick+=20;assert(LCD_HttpPanel(0,ui.settings)==0);}
static void preview(const char*name){char path[256];snprintf(path,sizeof path,"%s/%s.ppm",getenv("PREVIEW_DIR"),name);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n800 480\n255\n");for(unsigned i=0;i<800*480;i++){fputc(shown[i]>>16,f);fputc(shown[i]>>8,f);fputc(shown[i],f);}fclose(f);}
int main(void){
 for(unsigned i=0;i<33;i++){
  HTTP_LCDPort*p=&model.ports[i];p->port=i+1;p->mode=i<4?HTTP_MODE_TEMP:i<8?HTTP_MODE_TEMP_RHU:i<14?HTTP_MODE_INPUT:i<16?HTTP_MODE_ADC:i<20?HTTP_MODE_COUNTER:i<23?HTTP_MODE_PWM:i<29?HTTP_MODE_RELAY:HTTP_MODE_TIMER;
  snprintf(p->name,31,"Nazwa portu nr %u",i+1);snprintf(p->value[0],32,"%s",i<8?"25.6 C":i<14?"Stan OFF":i<16?"5.125 V":i<20?"1234.50 Wh":"Stan ON");
  snprintf(p->value[1],32,"65.0 %%");for(unsigned r=2;r<5;r++)snprintf(p->value[r],32,"%u.125",r*123);for(unsigned r=0;r<5;r++)p->color[r]=0xffeeeeee;p->pwm=42;p->pwm_valid=1;
 }
 strcpy(model.time,"12:34:56");model.revision=1;
 assert(LCD_HttpPanel(1,0)==0&&active&&ui.count==33&&Maximum()>0);preview("lcd-http-read-top");
 int before=draws;tick+=500;LCD_HttpPanel(0,0);assert(draws==before); /* no redraw without changes */
 touch(1,746,100);touch(1,746,435);touch(0,746,435);assert(ui.scroll==Maximum());preview("lcd-http-read-bottom");
 /* Every scroll offset clips native text safely, including partial first/last cards. */
 for(int s=0;s<=Maximum();s+=37){ui.scroll=s;ui.full=1;ui.dirty=1;LCD_HttpPanel(0,0);}
 int index=-1;for(int i=0;i<ui.count;i++)if(ui.tiles[i].port==21)index=i;assert(index>=0);
 ui.scroll=Clamp(ui.tiles[index].y,0,Maximum());ui.full=1;ui.dirty=1;LCD_HttpPanel(0,0);preview("lcd-http-pwm");
 int x=ui.tiles[index].x,y=TOP+ui.tiles[index].y-ui.scroll+94;
 int whole=viewport_draws;
 touch(1,x+18,y);assert(pwm_port==21&&pwm_percent==0);touch(1,x+198,y);assert(pwm_percent==100);touch(0,x+198,y);assert(pwm_calls>=2&&viewport_draws==whole);
 index=-1;for(int i=0;i<ui.count;i++)if(ui.tiles[i].port==28)index=i;assert(index>=0);
 ui.scroll=Clamp(ui.tiles[index].y,0,Maximum());ui.full=1;ui.dirty=1;LCD_HttpPanel(0,0);
 x=ui.tiles[index].x;y=TOP+ui.tiles[index].y-ui.scroll+60;
 touch(1,x+90,y);assert(ui.pressed==28);touch(0,x+90,y);assert(toggle_calls==1);
 touch(1,x+90,y);touch(1,x+90,y-20);touch(0,x+90,y-20);assert(toggle_calls==1);
 assert(LCD_HttpPanel(2,0)==0&&!active);
 assert(LCD_HttpPanel(1,1)==0&&active&&ui.count==33);preview("lcd-http-settings-top");
 int old_pwm=pwm_calls,old_toggle=toggle_calls;
 uint8_t options[6];assert(ModeOptions(1,options)==6&&options[0]==HTTP_MODE_TEMP&&options[4]==HTTP_MODE_EOL);
 assert(ModeOptions(15,options)==2&&options[0]==HTTP_MODE_ADC);
 assert(ModeOptions(17,options)==5&&options[3]==HTTP_MODE_COUNTER);
 assert(ModeOptions(21,options)==6&&options[3]==HTTP_MODE_PWM);
 assert(ModeOptions(24,options)==5&&options[3]==HTTP_MODE_IR);
 assert(ModeOptions(33,options)==4&&options[3]==HTTP_MODE_DISABLED);
 assert(!ModeOptions(0,options)&&!ModeOptions(34,options));
 touch(1,CONTENT_X+300,TOP+30);touch(0,CONTENT_X+300,TOP+30);
 assert(ui.menuPort==1&&!ui.menuUp&&ui.menuY>=TOP&&ui.menuY+ui.menuCount*MENU_ITEM+4<=TOP+VIEW);
 preview("lcd-http-settings-dropdown");
 int mx=ui.menuX+50,my=ui.menuY+2+MENU_ITEM+16;
 touch(1,mx,my);touch(0,mx,my);assert(!ui.menuPort&&ShownMode(1)==HTTP_MODE_PT1000&&model.ports[0].mode==HTTP_MODE_TEMP);
 assert(pwm_calls==old_pwm&&toggle_calls==old_toggle);
 touch(1,CONTENT_X+300,TOP+30);touch(0,CONTENT_X+300,TOP+30);
 touch(1,50,TOP+30);touch(0,50,TOP+30);assert(!ui.menuPort&&ShownMode(1)==HTTP_MODE_PT1000);

 whole=viewport_draws;
 touch(1,CONTENT_X+410,TOP+30);assert(ui.pressed==1);preview("lcd-http-settings-pressed");
 touch(0,CONTENT_X+410,TOP+30);assert(ui.pressed==-1&&ui.settings==1&&viewport_draws==whole);
 touch(1,CONTENT_X+410,TOP+30);touch(1,CONTENT_X+410,TOP+90);touch(0,CONTENT_X+410,TOP+90);assert(ui.scroll==0); /* clamp at top */
 touch(1,100,400);touch(1,100,120);touch(0,100,120);assert(ui.scroll>0);
 touch(1,746,75);touch(1,746,439);touch(0,746,439);assert(ui.scroll==Maximum());preview("lcd-http-settings-bottom");

 int row=32,field_y=TOP+ui.tiles[row].y-ui.scroll+30;
 touch(1,CONTENT_X+300,field_y);touch(0,CONTENT_X+300,field_y);
 assert(ui.menuPort==33&&ui.menuUp&&ui.menuY>=TOP&&ui.menuY+ui.menuCount*MENU_ITEM+4<=TOP+VIEW);
 preview("lcd-http-settings-dropup");
 mx=ui.menuX+50;my=ui.menuY+2+3*MENU_ITEM+16;
 touch(1,mx,my);touch(0,mx,my);
 assert(ShownMode(33)==HTTP_MODE_DISABLED&&model.ports[32].mode==HTTP_MODE_TIMER);
 assert(pwm_calls==old_pwm&&toggle_calls==old_toggle); /* settings never writes or navigates */
 assert(ui.tiles[32].y-ui.scroll+ui.tiles[32].h<=VIEW);
 index=20;ui.scroll=Clamp(ui.tiles[index].y,0,Maximum());ui.full=1;ui.dirty=1;LCD_HttpPanel(0,1);
 y=TOP+ui.tiles[index].y-ui.scroll+34;x=CONTENT_X;
 touch(1,x+522,y);assert(pwm_port==21&&pwm_percent==0);
 touch(1,x+666,y);assert(pwm_percent==100);touch(0,x+666,y);
 preview("lcd-http-settings-pwm");
 index=27;ui.scroll=Clamp(ui.tiles[index].y,0,Maximum());ui.full=1;ui.dirty=1;LCD_HttpPanel(0,1);
 y=TOP+ui.tiles[index].y-ui.scroll+34;
 old_toggle=toggle_calls;touch(1,x+570,y);assert(ui.outputPressed==28);touch(0,x+570,y);assert(toggle_calls==old_toggle+1);
 preview("lcd-http-settings-relays");
 /* A preview mode is never allowed to operate hardware of a different actual mode. */
 ui.draftMode[27]=HTTP_MODE_PWM;ui.full=1;ui.dirty=1;LCD_HttpPanel(0,1);
 old_pwm=pwm_calls;touch(1,x+600,y);touch(0,x+600,y);assert(pwm_calls==old_pwm);

 assert(LCD_HttpPanel(2,1)==0&&!active);
 puts("PASS: two 33-port screens, native clipped text, scrollbar/content drag, endpoints, no idle redraw, PWM 0/100 to port 21, settings press/release/cancel without writes/navigation; preview frames.");
}
'''
def glyph_header():
 widths=[];offsets=[];heights=[];pixels=bytearray()
 for size in [16,21,35]:
  font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',size-3)
  ws=[];offs=[];heights.append(size)
  for c in range(256):
   w=max(1,int(font.getlength(chr(c))+.5));ws.append(w);offs.append(len(pixels));im=Image.new('L',(w,size));from PIL import ImageDraw
   ImageDraw.Draw(im).text((0,-2),chr(c),font=font,fill=255);pixels.extend(im.tobytes())
  widths.append(ws);offsets.append(offs)
 def arr(a):return '{'+','.join(map(str,a))+'}'
 return 'static const unsigned char glyphs[]='+arr(pixels)+';\nstatic const int glyph_height[]='+arr(heights)+';\nstatic const int glyph_width[3][256]={' + ','.join(arr(a) for a in widths)+'};\nstatic const unsigned glyph_offset[3][256]={' + ','.join(arr(a) for a in offsets)+'};\n'
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 for n in ['SCREEN_HttpPanels.c','SCREEN_HttpPanels.h']:shutil.copy(ROOT/'Application/LCD/Screens'/n,p/n)
 shutil.copy(ROOT/'Application/HTTP/http_lcd_panels.h',p/'http_lcd_panels.h')
 (p/'support.h').write_text(SUPPORT)
 for n in ['SCREEN_ReadPanel.h','LCD_BasicGraphics.h','LCD_fonts_images.h','LCD_Hardware.h','touch.h']:(p/n).write_text('#include "support.h"\n')
 (p/'SCREEN_TemperaturePk.h').write_text('enum{LCD_TEMP_UPDATE=0,LCD_TEMP_ENTER=1,LCD_TEMP_EXIT=2};\n')
 (p/'wwwPages.h').write_text(mode)
 (p/'test.c').write_text(glyph_header()+'\n'+C)
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+d,str(p/'test.c'),'-o',str(p/'test')],check=True)
 target=ROOT/'Debug/cloud/http-lcd-previews';target.mkdir(parents=True,exist_ok=True)
 env=dict(os.environ,PREVIEW_DIR=str(target));subprocess.run([str(p/'test')],env=env,check=True)
 for ppm in target.glob('*.ppm'):
  Image.open(ppm).save(ppm.with_suffix('.png'))
 print('Previews:',target)
