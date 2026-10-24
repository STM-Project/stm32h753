/* LCD adaptation of STM-Project/stm32h753 master 5af0e94:
 * Application/HTTP/http_sensor_page.c (PageCard) and wwwPages.c (HttpStyle).
 * New application code only; uses the user's LCD and Keyboard APIs.
 */
#include "LCD_SensorThresholds.h"
#include "http_sensor_lcd.h"
#include "SCREEN_ReadPanel.h"
#include "LCD_BasicGraphics.h"
#include "LCD_fonts_images.h"
#include "LCD_Hardware.h"
#include "Keyboard.h"
#include "touch.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
extern uint32_t pLcd[];
#define BG 0xFF000000U
#define CARD 0xFF505050U
#define WHITE_TEXT 0xFFEEEEEEU
#define GREEN 0xFF9EDF0FU
#define TOUCH_FIRST 62000
/* Keyboard.c stores startTouchIdx as uint8_t, unlike the LCD touch API.
 * The modal editor owns these IDs after removing the page touch regions.
 */
#define TOUCH_KEYS 180
_Static_assert(TOUCH_KEYS > 0 && TOUCH_KEYS + 35 <= UINT8_MAX,
               "Native Keyboard.c requires 8-bit touch IDs");
#define KEYBOARD_TYPE_ID 1
/* Exact ordering required by the user's Keyboard.c. */
enum { KEY_All_release=0, KEYBOARD_SETTXT_KEYS };
/* Only edit drafts and read snapshots live here; HTTP owns committed values. */
static LCD_SensorThreshold readback[2];
unsigned lcdThresholdPort=1;
volatile int lcdThresholdResult;

static struct {
    int initialized,active,font[3],wasDown,armed,editing,previousKey,dirty,keyPadding,keyWidth,keyHeight,pressed;
    unsigned port;
    int pending;
    LCD_SensorThreshold draft[2],base[2];
    /* Keyboard.c reserves the last TWO bytes for its cursor index. */
    char edit[64],notice[64];
} ui;

int LCD_SensorThresholdSetField(LCD_SensorThreshold *v,unsigned humidity,unsigned field,const char *text)
{
    if(!v||!text||humidity>1||field>7)return 0;
    if(field==0){
        unsigned n=0;
        while(text[n]){unsigned char c=(unsigned char)text[n];if(c<32||c==127||n>=30)return 0;n++;}
        memcpy(v->name,text,n+1);return 1;
    }
    const char *p=text;int negative=0;
    if(*p=='+'||*p=='-'){negative=*p=='-';p++;}
    if(*p<'0'||*p>'9')return 0;
    unsigned integer=0;
    while(*p>='0'&&*p<='9'){
        unsigned digit=(unsigned)(*p++-'0');
        if(integer>(65535U-digit)/10U)return 0;
        integer=integer*10U+digit;
    }
    unsigned fraction=0;
    if(field!=7&&(*p=='.'||*p==',')){
        p++;if(*p<'0'||*p>'9')return 0;fraction=(unsigned)(*p++-'0');
    }
    if(*p)return 0;
    if(field==7){if(negative||integer>5999)return 0;v->delay_seconds=(uint16_t)integer;return 1;}
    if(integer>3276)return 0;
    int number=(int)(integer*10U+fraction);if(negative)number=-number;
    if(number<INT16_MIN||number>INT16_MAX)return 0;
    if(field==5&&number<0)return 0;
    if(humidity){
        if(field<=5&&(number<0||number>1000))return 0;
        if(field==6&&(number < -1000||number>1000))return 0;
    }
    switch(field){case 1:v->max1=number;break;case 2:v->max2=number;break;
    case 3:v->min1=number;break;case 4:v->min2=number;break;
    case 5:v->hysteresis=number;break;case 6:v->correction=number;break;default:return 0;}
    return 1;
}
const LCD_SensorThreshold *LCD_SensorThresholdGet(unsigned port,unsigned humidity)
{
    if(humidity>1 || !HTTP_SensorLCDRead(port,readback))return NULL;
    return &readback[humidity];
}
static uint32_t Mix(uint32_t a,uint32_t b,unsigned t)
{
    unsigned r=(((a>>16)&255)*(255-t)+((b>>16)&255)*t+127)/255;
    unsigned g=(((a>>8)&255)*(255-t)+((b>>8)&255)*t+127)/255;
    unsigned v=((a&255)*(255-t)+(b&255)*t+127)/255;
    return 0xFF000000U|(r<<16)|(g<<8)|v;
}
static void ThresholdRound(int x,int y,int w,int h,int radius,uint32_t top,uint32_t bottom)
{
    for(int j=0;j<h;j++){
      /* The gradient and vertical corner distance are constant in a row. */
      uint32_t rowColor=Mix(top,bottom,h>1?(unsigned)(j*255/(h-1)):0);
      float dy=j<radius?radius-j-0.5f:j>=h-radius?j-(h-radius)+0.5f:0;
      for(int i=0;i<w;i++){
        float dx=i<radius?radius-i-0.5f:i>=w-radius?i-(w-radius)+0.5f:0;
        float coverage=dx>0&&dy>0?radius+0.5f-sqrtf(dx*dx+dy*dy):1;
        if(coverage<=0)continue;
        uint32_t c=rowColor;
        if(coverage<1)c=Mix(pLcd[(y+j)*800+x+i],c,(unsigned)(coverage*255));
        LCD_Buffer(800,x+i,y+j,c);
      }
    }
}
static void Text(int f,int x,int y,char *s,uint32_t color)
{ LCD_StrChangeColor(ui.font[f],x,y,s,fullHight,0,0,color,255,NoConstWidth); }
static void ThresholdCenter(int f,int x,int y,char *s,uint32_t color)
{Text(f,x-LCD_GetWholeStrPxlWidth(ui.font[f],s,0,NoConstWidth)/2,y,s,color);}
static void Fill(uint32_t c)
{for(int y=0;y<480;y++)for(int x=0;x<800;x++)LCD_Buffer(800,x,y,c);}
static void Number(char *out,size_t capacity,int value)
{unsigned n=(unsigned)(value<0?-value:value);snprintf(out,capacity,"%c%02u.%u",value<0?'-':'+',n/10,n%10);}
static void FieldText(int column,int field,char *out,size_t size)
{
    LCD_SensorThreshold *v=&ui.draft[column];
    int values[]={v->max1,v->max2,v->min1,v->min2,v->hysteresis,v->correction};
    if(field==0)snprintf(out,size,"%s",v->name);
    else if(field==7)snprintf(out,size,"%02u:%02u",v->delay_seconds/60,v->delay_seconds%60);
    else Number(out,size,values[field-1]);
}
static void FieldRect(int column,int field,int *x,int *y,int *w,int *h)
{
    int offset=column*388;*h=32;
    if(field==0){*x=44+offset;*y=143;*w=324;return;}
    int rows[]={0,0,1,1,2,3,4};
    *x=176+offset+((field==2||field==4)?84:0);
    *y=190+rows[field-1]*36;*w=76;
}
static void Button(int x,int y,int w,char *s)
{ThresholdRound(x,y,w,40,13,0xFF63B8EEU,0xFF468CCFU);ThresholdCenter(2,x+w/2,y+9,s,WHITE_TEXT);}
/* Repaint only the touched control, including its background. Keyboard.c
 * uses pLcd as scratch, so never present the complete buffer here. */
static void PressPaint(int id,int pressed)
{
    int x,y,w,h=40;char text[64];uint32_t bg=BG;
    if(id<0)return;
    if(id<16){FieldRect(id/8,id%8,&x,&y,&w,&h);bg=CARD;}
    else if(id==16){x=552;y=9;w=44;strcpy(text,"<");}
    else if(id==17){x=732;y=9;w=44;strcpy(text,">");}
    else if(id==18){x=24;y=419;w=176;strcpy(text,"Zapisz");}
    else if(id==19){x=216;y=419;w=176;strcpy(text,"Odrzuc");}
    else {x=id==20?550:668;y=8;w=106;bg=0xFF383838U;strcpy(text,id==20?"Gotowe":"Anuluj");}
    for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)LCD_Buffer(800,i,j,bg);
    if(id<16){
        ThresholdRound(x,y,w,h,4,pressed?0xFF8EBBD9U:0xFFFFFFFFU,pressed?0xFF5994BFU:0xFFBBBBBBU);
        FieldText(id/8,id%8,text,sizeof text);
        while(text[0]&&LCD_GetWholeStrPxlWidth(ui.font[2],text,0,NoConstWidth)>w-10)text[strlen(text)-1]=0;
        Text(2,x+5,y+7,text,0xFF222222U);
    }else{
        ThresholdRound(x,y,w,h,13,pressed?0xFF28577FU:0xFF63B8EEU,pressed?0xFF397BA8U:0xFF468CCFU);
        ThresholdCenter(2,x+w/2,y+9+(pressed?1:0),text,WHITE_TEXT);
    }
    LCD_DisplayPart(0,x,y,w,h);
}
static void PressFeedback(int hit,int down)
{
    int next=down&&hit==ui.armed?hit:-1;
    if(next==ui.pressed)return;
    PressPaint(ui.pressed,0);ui.pressed=next;PressPaint(next,1);
}
static int TouchRect(int x,int y,int w,int h,int id)
{return LCDTOUCH_Set(x,y,w,h,ID_TOUCH_POINT,(uint16_t)id,pressRelease)>=0;}
static int Draw(void)
{
    LCD_TOUCH_DeleteAllSetTouch();Fill(BG);
    Text(1,24,16,"Ustawienia czujnikow",WHITE_TEXT);
    Button(552,9,44,"<");Button(732,9,44,">");
    char text[64];snprintf(text,sizeof(text),"Port %u / 20",ui.port);ThresholdCenter(2,664,21,text,WHITE_TEXT);
    if(!TouchRect(552,9,44,40,TOUCH_FIRST+16)||!TouchRect(732,9,44,40,TOUCH_FIRST+17))return -4;
    for(int c=0;c<2;c++){
        int x=24+c*388;
        ThresholdRound(x,69,364,325,30,0xFF808080U,0xFF808080U);
        ThresholdRound(x+4,73,356,317,26,CARD,CARD);
        snprintf(text,sizeof(text),"%s Port %u",c?"Wilgotnosc":"Temperatura",ui.port);
        ThresholdCenter(1,x+182,87,text,0xFFFF4444U);
        snprintf(text,sizeof(text),"-- -- -- T%u -- -- --",(ui.port-1)*2+(unsigned)c+1);
        ThresholdCenter(0,x+182,119,text,WHITE_TEXT);
        char *labels[]={"Prog max","Prog min","Histereza","Korekta","Czas zwloki"};
        for(int row=0;row<5;row++){
            Text(2,x+20,198+36*row,labels[row],GREEN);
            Text(0,x+322,200+36*row,row==4?"m:s":c?"%":"C",GREEN);
        }
        for(int f=0;f<8;f++){
            int xx,yy,w,h;FieldRect(c,f,&xx,&yy,&w,&h);
            ThresholdRound(xx-1,yy-1,w+2,h+2,5,0xFFC4C4C4U,0xFFC4C4C4U);
            ThresholdRound(xx,yy,w,h,4,0xFFFFFFFFU,0xFFBBBBBBU);
            FieldText(c,f,text,sizeof(text));
            while(text[0]&&LCD_GetWholeStrPxlWidth(ui.font[2],text,0,NoConstWidth)>w-10)
                text[strlen(text)-1]=0;
            Text(2,xx+5,yy+7,text,0xFF222222U);
            if(!TouchRect(xx,yy,w,h,TOUCH_FIRST+c*8+f))return -4;
        }
    }
    Button(24,419,176,"Zapisz");Button(216,419,176,"Odrzuc");
    snprintf(text,sizeof(text),"%s",ui.notice);
    while(text[0]&&LCD_GetWholeStrPxlWidth(ui.font[0],text,0,NoConstWidth)>358)text[strlen(text)-1]=0;
    Text(0,418,423,text,WHITE_TEXT);
    Text(0,418,447,"Wspolne ustawienia LCD / WWW / SD",GREEN);
    if(!TouchRect(24,419,176,40,TOUCH_FIRST+18)||!TouchRect(216,419,176,40,TOUCH_FIRST+19))return -4;
    LCD_Show();return 0;
}
static void Keyboard(int key,int create)
{
    /* Original palette from SCREEN_FontsLCD / SCREEN_VisualParamLCD. */
    KEYBOARD_SetGeneral(ui.font[2],ui.font[0],0xFF999999U,
                        0xFFD0D0D0U,0xFF313131U,0xFFD0D0D0U,0xFF3B3B3BU,
                        0xFFBABABAU,0xFF606060U,0xFF383838U);
    figureShape shape=create?LCD_RoundRectangle:0;
    KEYBOARD_StartUp(KEYBOARD_TYPE_ID,shape,0,10,65,ui.keyWidth,ui.keyHeight,10,0,TOUCH_KEYS,KeysDel);
    KEYBOARD__ServiceSetTxt(KEYBOARD_TYPE_ID-1,key,shape,0,10,65,ui.keyWidth,ui.keyHeight,10,0,
        create?TOUCH_KEYS:0,KeysDel,KEY_All_release,KEY_Q,KEY_big,KEY_back,KEY_alt,
        KEY_enter,KEY_field,KEY_style,KEY_exit,0xFF999999U,ui.edit,sizeof(ui.edit));
}
static int EditorOpen(int field)
{
    /* Twice the original auto-sized keys; compact text field leaves room
     * for four rows of keys within the 800x480 display. */
    int available=480-115-14-7*10-10*LCD_GetFontHeight(ui.font[2]);
    ui.keyPadding=available/8;if(ui.keyPadding>10)ui.keyPadding=10;
    if(ui.keyPadding<4)return -3;
    ui.keyWidth=2*(2*ui.keyPadding+LCD_GetWholeStrPxlWidth(ui.font[2],"q",0,NoConstWidth));
    ui.keyHeight=2*(2*ui.keyPadding+LCD_GetFontHeight(ui.font[2]));
    if(10+11*10+10*ui.keyWidth>800 ||
       65+14+LCD_GetFontHeight(ui.font[2])+7*10+4*ui.keyHeight>480)return -3;
    KEYBOARD_SetTextRows(1);
    ui.pressed=-1;ui.editing=field;ui.previousKey=0;memset(ui.edit,0,sizeof(ui.edit));
    int c=field/8,f=field%8;
    if(f==7)snprintf(ui.edit,sizeof(ui.edit),"%u",ui.draft[c].delay_seconds);
    else FieldText(c,f,ui.edit,sizeof(ui.edit));
    LCD_TOUCH_DeleteAllSetTouch();Fill(0xFF383838U);
    char *names[]={"Nazwa","Prog max 1","Prog max 2","Prog min 1","Prog min 2","Histereza","Korekta","Czas zwloki (sekundy)"};
    Text(1,24,4,c?"Wilgotnosc - edycja":"Temperatura - edycja",WHITE_TEXT);
    Text(2,24,36,names[f],GREEN);
    Button(550,8,106,"Gotowe");Button(668,8,106,"Anuluj");
    LCD_Show();
    Keyboard(KEY_All_release,1);
    if(!TouchRect(550,8,106,40,TOUCH_FIRST+20)||!TouchRect(668,8,106,40,TOUCH_FIRST+21))return -4;
    /* Native Keyboard.c presents its own window using pLcd as scratch.
     * Never present this scratch buffer as a full-screen 800-pixel canvas. */
    return 0;
}
static int EditorClose(int commit)
{
    /* Remove only keyboard-owned touch bookkeeping before rebuilding the page. */
    KEYBOARD_StartUp(0,0,0,0,0,0,0,0,0,0,KeysDel);
    int valid=1;
    if(commit){
        ui.edit[sizeof(ui.edit)-3]=0;
        valid=LCD_SensorThresholdSetField(&ui.draft[ui.editing/8],ui.editing/8,ui.editing%8,ui.edit);
        if(valid)ui.dirty=1;
    }
    KEYBOARD_SetTextRows(6);
    ui.pressed=-1;ui.editing=-1;ui.previousKey=0;ui.armed=-1;ui.wasDown=LCD_TOUCH_isPress()==press;
    snprintf(ui.notice,sizeof(ui.notice),"%s",!commit?"Edycja anulowana":valid?"Zmieniono pole. Nacisnij Zapisz.":"Bledna wartosc - pole bez zmian.");
    return Draw();
}
static int Reload(void)
{
    if(!HTTP_SensorLCDRead(ui.port,ui.base)) return 0;
    memcpy(ui.draft,ui.base,sizeof ui.draft);
    ui.dirty=0;
    return 1;
}
static int Save(void)
{
    if(!HTTP_SensorLCDSubmit(ui.port,ui.base,ui.draft)){
        snprintf(ui.notice,sizeof ui.notice,"Model zajety lub jeszcze niedostepny.");
        return Draw()<0?-4:0;
    }
    ui.pending=1;
    snprintf(ui.notice,sizeof ui.notice,"Zapisywanie na SD...");
    return Draw()<0?-4:0;
}
int LCD_SensorThresholds(uint8_t mode)
{
    if(mode==LCD_THRESH_EXIT){
        KEYBOARD_StartUp(0,0,0,0,0,0,0,0,0,0,KeysDel);
        for(int i=0;i<22;i++)LCD_TOUCH_DeleteSelectTouch(TOUCH_FIRST+i);
        KEYBOARD_SetTextRows(6);ui.pressed=-1;ui.active=0;ui.editing=-1;return 0;
    }
    if(mode>LCD_THRESH_EXIT)return -1;
    if(LCD_GetXSize()!=800||LCD_GetYSize()!=480)return -2;
    if(mode==LCD_THRESH_ENTER||!ui.active){
        SCREEN_ResetAllParameters();LCD_TOUCH_DeleteAllSetTouch();
        const int sizes[]={FONT_12,FONT_18_bold,FONT_14};
        const int ids[]={fontID_14,fontID_15,fontID_16};
        ui.active=0;
        for(int i=0;i<3;i++){ui.font[i]=LCD_LoadFont_ChangeColor(sizes[i],Arial,ids[i]);if(ui.font[i]<0)return -3;}
        ui.port=lcdThresholdPort>=1&&lcdThresholdPort<=20?lcdThresholdPort:1;lcdThresholdPort=ui.port;
        ui.initialized=Reload();
        ui.dirty=0;ui.editing=-1;ui.armed=-1;ui.pressed=-1;ui.wasDown=LCD_TOUCH_isPress()==press;
        snprintf(ui.notice,sizeof(ui.notice),"%s",ui.initialized?"Dotknij pola, aby edytowac.":"Czekam na ustawienia HTTP / SD.");
        int result=Draw();if(result<0)return result;ui.active=1;return 0;
    }
    if(ui.pending){
        int result=HTTP_SensorLCDResult();
        if(result==SENSOR_LCD_PENDING)return 0;
        ui.pending=0;
        if(result==SENSOR_LCD_SAVED){
            Reload();
            snprintf(ui.notice,sizeof ui.notice,"Zapisano LCD / WWW / SD.");
            int drawn=Draw();return drawn<0?drawn:1;
        }
        snprintf(ui.notice,sizeof ui.notice,"%s",result==SENSOR_LCD_CONFLICT?
                 "Zmiana z WWW. Odrzuc i edytuj ponownie.":"Blad zapisu SD. Zachowano edycje.");
        return Draw();
    }
    if(ui.editing<0 && !ui.dirty && LCD_TOUCH_isPress()!=press){
        LCD_SensorThreshold latest[2];
        if(HTTP_SensorLCDRead(ui.port,latest) &&
           (!ui.initialized || memcmp(latest,ui.base,sizeof latest))){
            memcpy(ui.base,latest,sizeof latest);memcpy(ui.draft,latest,sizeof latest);
            ui.initialized=1;
            snprintf(ui.notice,sizeof ui.notice,"Odczytano wspolne ustawienia WWW.");
            return Draw();
        }
    }
    if(!ui.initialized)return 0;
    screenTouchState=LCD_TOUCH_GetTypeAndPosition(&screenTouchPos);
    int down=LCD_TOUCH_isPress()==press;
    if(ui.editing>=0){
        if(down&&screenTouchState>=TOUCH_KEYS&&screenTouchState<TOUCH_KEYS+36){
            int key=KEY_Q+(screenTouchState-TOUCH_KEYS);
            if(key==KEY_exit)return EditorClose(0);
            /* Repeated events of the SAME held key must reach Keyboard.c
             * without release: its keyPressPrev/pressTime select alternatives.
             */
            if(ui.previousKey&&ui.previousKey!=key)Keyboard(KEY_All_release,0);
            Keyboard(key,0);ui.previousKey=key;
        }else if(!down&&ui.previousKey){Keyboard(KEY_All_release,0);ui.previousKey=0;}
        /* No event while still down is the native repeat wait, not release. */
        XY_Touch_Struct p=LCD_TOUCH_GetPos();
        int hit=p.y>=8&&p.y<48?(p.x>=550&&p.x<656?20:p.x>=668&&p.x<774?21:-1):-1;
        if(down&&!ui.wasDown)ui.armed=hit;
        PressFeedback(hit,down);
        if(!down&&ui.wasDown&&hit==ui.armed&&hit>=20){ui.wasDown=down;ui.armed=-1;return EditorClose(hit==20);}
        ui.wasDown=down;return 0;
    }
    XY_Touch_Struct p=LCD_TOUCH_GetPos();int hit=-1;
    for(int c=0;c<2;c++)for(int f=0;f<8;f++){
        int x,y,w,h;FieldRect(c,f,&x,&y,&w,&h);
        if(p.x>=x&&p.x<x+w&&p.y>=y&&p.y<y+h)hit=c*8+f;
    }
    if(p.y>=9&&p.y<49){if(p.x>=552&&p.x<596)hit=16;if(p.x>=732&&p.x<776)hit=17;}
    if(p.y>=419&&p.y<459){if(p.x>=24&&p.x<200)hit=18;if(p.x>=216&&p.x<392)hit=19;}
    if(down&&!ui.wasDown)ui.armed=hit;
    PressFeedback(hit,down);
    int action=!down&&ui.wasDown&&hit>=0&&hit==ui.armed?hit:-1;
    ui.wasDown=down;
    if(!down)ui.armed=-1;
    if(action<0)return 0;
    if(action<16)return EditorOpen(action);
    if(action==18)return Save();
    if(action==19){Reload();snprintf(ui.notice,sizeof(ui.notice),"Odrzucono niezapisane zmiany.");return Draw();}
    if(ui.dirty){snprintf(ui.notice,sizeof(ui.notice),"Najpierw Zapisz lub Odrzuc.");return Draw();}
    if(action==16&&ui.port>1)ui.port--;
    if(action==17&&ui.port<20)ui.port++;
    lcdThresholdPort=ui.port;ui.initialized=Reload();return Draw();
}
