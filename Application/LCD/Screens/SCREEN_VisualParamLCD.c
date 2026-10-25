
#include "SCREEN_VisualParamLCD.h"
#include "SCREEN_ReadPanel.h"
#include "Keyboard.h"
#include "lang.h"
#include "timer.h"
#include "_debug.h"
#include "touch.h"
#include <string.h>
#include "i2c.h"
#include "LCD_PortH7.h"
#include "double_float.h"

/*--------------- Main Macro Settings ------------------*/
#define FILE_NAME(extend) SCREEN_VisualParam_##extend

#define SCREEN_VISUALPARAM_LANG \
	X(LANG_nazwa_0, 			"LANG_nazwa_0", "LANG_name_0") \
	X(LANG_nazwa_1, 			"LANG_nazwa_1", "LANG_name_1") \
	X(LANG_TunningFreq, 		"Dostrajanie", "Tuning") \

#define SCREEN_VISUALPARAM_SET_PARAMETERS \
/* id   name							default value */ \
	X(0, FONT_SIZE_Title, 	 		FONT_24_bold) \
	X(1, FONT_SIZE_Descr, 	 		FONT_8) \
	X(2, FONT_SIZE_Press, 	 		FONT_14) \
	X(3, FONT_SIZE_Param_1, 	 	FONT_14) \
	X(4, FONT_SIZE_Param_2, 	 	FONT_14) \
	X(5, FONT_SIZE_Param_3, 	 	FONT_14) \
	\
	X(6, FONT_STYLE_Title, 			Arial) \
	X(7, FONT_STYLE_Descr, 	 		Arial) \
	X(8, FONT_STYLE_Press, 	 		Arial) \
	X(9, FONT_STYLE_Param_1, 		Arial) \
	X(10, FONT_STYLE_Param_2, 		Arial) \
	X(11, FONT_STYLE_Param_3, 		Arial) \
	\
	X(12, FONT_COLOR_Title,  		WHITE) \
	X(13, FONT_COLOR_Descr, 	 	WHITE) \
	X(14, FONT_COLOR_Press, 	 	DARKRED) \
	X(15, FONT_COLOR_Param_1, 		WHITE) \
	X(16, FONT_COLOR_Param_2, 		WHITE) \
	X(17, FONT_COLOR_Param_3, 		WHITE) \
	\
	X(18, FONT_BKCOLOR_Title,  	MYGRAY2) \
	X(19, FONT_BKCOLOR_Descr, 	 	MYGRAY2) \
	X(20, FONT_BKCOLOR_Press, 	 	WHITE) \
	X(21, FONT_BKCOLOR_Param_1, 	MYGRAY2) \
	X(22, FONT_BKCOLOR_Param_2, 	MYGRAY2) \
	X(23, FONT_BKCOLOR_Param_3, 	MYGRAY2) \
	\
	X(24, FONT_ID_Title,				fontID_1) \
	X(25, FONT_ID_Descr,				fontID_2) \
	X(26, FONT_ID_Press,				fontID_3) \
	X(27, FONT_ID_Param_1,			fontID_4) \
	X(28, FONT_ID_Param_2,			fontID_5) \
	X(29, FONT_ID_Param_3,			fontID_6) \
	\
	X(30, FONT_VAR_Title,			fontVar_1) \
	X(31, FONT_VAR_Descr,			fontVar_2) \
	X(32, FONT_VAR_Press,			fontVar_3) \
	X(33, FONT_VAR_Param_1,			fontVar_4) \
	X(34, FONT_VAR_Param_2,			fontVar_5) \
	X(35, FONT_VAR_Param_3,			fontVar_6) \
	\
	X(36, COLOR_BkScreen,  			COLOR_GRAY(0x38)) \
	X(37, COLOR_MainFrame,  		COLOR_GRAY(0xD0)) \
	X(38, COLOR_FillMainFrame, 	COLOR_GRAY(0x31)) \
	X(39, COLOR_Frame,  				COLOR_GRAY(0xD0)) \
	X(40, COLOR_FillFrame, 			COLOR_GRAY(0x3B)) \
	X(41, COLOR_FramePress, 		COLOR_GRAY(0xBA)) \
	X(42, COLOR_FillFramePress,	COLOR_GRAY(0x60)) \
	X(43, DEBUG_ON,  					1) \
	X(44, BK_FONT_ROUND,  			1) \
	X(45, LANG_SELECT,  				Polish) \

/*------------ Main Functions of the MACROs -----------------*/
#define SL(name)	(char*)FILE_NAME(Lang)[ v.LANG_SELECT==Polish ? 2*(name) : 2*(name)+1 ]

typedef enum{
	#define X(a,b,c) a,
	SCREEN_VISUALPARAM_LANG
	#undef X
}FILE_NAME(Lang_enum);

static const char *FILE_NAME(Lang)[]={
	#define X(a,b,c) b"\x00",c"\x00",
		SCREEN_VISUALPARAM_LANG
	#undef X
};

typedef enum{
	#define X(a,b,c) b,
	SCREEN_VISUALPARAM_SET_PARAMETERS
	#undef X
}FILE_NAME(enum);

typedef struct{
	#define X(a,b,c) int b;
	SCREEN_VISUALPARAM_SET_PARAMETERS
	#undef X
}FILE_NAME(struct);

static FILE_NAME(struct) v ={
	#define X(a,b,c) c,
		SCREEN_VISUALPARAM_SET_PARAMETERS
	#undef X
};

#define SEL_BITS_SIZE	5
static uint32_t FILE_NAME(SelBits)[SEL_BITS_SIZE] = {0};
static uint32_t FILE_NAME(SelTouch)[SEL_BITS_SIZE] = {0};
/*
static int FILE_NAME(SetDefaultParam)(int param){
	int temp;
	#define X(a,b,c) \
		if(b==param){ v.b=c; temp=c; }
		SCREEN_VISUALPARAM_SET_PARAMETERS
	#undef X
		return temp;
}
*/
static int FILE_NAME(GetDefaultParam)(int param){
	int temp;
	#define X(a,b,c) \
		if(b==param) temp=c;
	SCREEN_VISUALPARAM_SET_PARAMETERS
	#undef X
		return temp;
}

void FILE_NAME(printInfo)(void){
	if(v.DEBUG_ON){
		DbgDma ( 1, Clr_ CoG2_ "\r\ntypedef struct{\r\n" _X_ );
		DbgVarDma2 ( 1, 200, CoGr_ "%*s %*s %*s %s\r\n" _X_, -8, "id", -18, "name", -15, "default value", "value" );
#define X( a, b, c ) DbgVarDma2 ( 1, 200, CoGr_ "%*d" _X_ "%*s" CoGr_ "= " _X_ "%*s" \
												"(%s0x%x)\r\n",                      \
								  -4, a, -23, getName ( b ), -15, getName ( c ), CHECK_bit ( FILE_NAME ( SelBits )[a / 32], ( a - 32 * ( a / 32 ) ) ) ? CoR_ "change to: " _X_ : "", v.b );
		SCREEN_VISUALPARAM_SET_PARAMETERS
		#undef X
		DbgVarDma ( 1, 200, CoG2_ "}%s;\r\n" _X_, getName ( FILE_NAME ( struct ) ) );
	}
}

int FILE_NAME(funcGet)(int offs){
	return *( (int*)((int*)(&v) + offs) );
}

void FILE_NAME(funcSet)(int offs, int val){
	*( (int*)((int*)(&v) + offs) ) = val;
	SET_bit( FILE_NAME(SelBits)[offs/32], (offs-32*(offs/32)) );
}

void FILE_NAME(setDefaultAllParam)(int rst){
	#define X(a,b,c) FILE_NAME(funcSet)(b,c);
	SCREEN_VISUALPARAM_SET_PARAMETERS
	#undef X
	if(rst){
		for(int i=0;i<SEL_BITS_SIZE;++i)
			FILE_NAME(SelBits)[i]=0;
	}
}

void FILE_NAME(debugRcvStr)(void);
void FILE_NAME(setTouch)(void);

void 	FILE_NAME(main)(int argNmb, char **argVal);

/*------------ Alternative Defines and Functions -----------------*/
#define NONE_TYPE_REQ	-1
#define MAX_NUMBER_OPENED_KEYBOARD_SIMULTANEOUSLY		20
/* #define TOUCH_MAINFONTS_WITHOUT_DESCR */

#define KEYBUFF_SIZE		500
#define ROLL_1		0

#define MAX_RADIO_CHANNEL	24
#define SIZE_RADIO_NAME		30
#define DIV_ACT	2
#define STEP_FREQ	12.000000
#define DIV_FREQ	75.000000
#define INTER_F1	10.700000
#define TXT_RADIO_STATION		StrAll(5," ",dbl2stri(tempBuff,Test.Radio[Test.selRadio].freq+Test.Radio[Test.selRadio].freqOffs,2)," MHz  -  ",Test.Radio[Test.selRadio].radioName," ")

typedef enum{
	NoTouch = NO_TOUCH,
	Touch_Param_1,
	Touch_Param_2,
	Touch_Param_22,
	Touch_Param_2MoveRight,
	Touch_Param_2MoveLeft,
	Touch_Param_3,
	Touch_TunningFreq_plus,
	Touch_TunningFreq_minus,
	Touch_FieldRoll,
	Touch_NextScreen,
	Touch_PrevScreen,
	AnyPress,
	AnyPressWithWait,
	KEYBOARD_SETTXT_TOUCHS
}TOUCH_POINTS;		/* MAX_OPEN_TOUCH_SIMULTANEOUSLY */

typedef enum{
	KEY_NO_RELEASE,
	KEY_All_release,
	KEY_Select_one,
	KEY_Timer,
	KEY_Timer2,
	KEY_Param_1,
	KEY_Param_2,
	KEY_Param_3,
	KEY_TunningFreq_plus,
	KEY_TunningFreq_minus,
	KEYBOARD_SETTXT_KEYS
}SELECT_PRESS_BLOCK;

typedef enum{
	KEYBOARD_none,
	KEYBOARD_setTxt,
	KEYBOARD_Param_1,
	KEYBOARD_Param_2,
	KEYBOARD_Param_3,
	KEYBOARD_TunningFreq,
}KEYBOARD_TYPES;	/* MAX_NUMBER_OPENED_KEYBOARD_SIMULTANEOUSLY */

typedef enum{
	TIMER_Cpu,
	TIMER_InfoWrite,
	TIMER_Release,
	TIMER_BlockTouch,
	TIMER_Scroll,
}TIMER_FOR_THIS_SCREEN;

typedef struct{
	char radioName[SIZE_RADIO_NAME];
	double freq;
	u16 freqStep;
	u16 freqDiv;
	double freqOffs;
} RADIOPARAM;

typedef struct{
	char *pName[MAX_RADIO_CHANNEL];
	u8 selRadio;
	RADIOPARAM Radio[MAX_RADIO_CHANNEL];
} STRUCT_VISUALPARAM;
static STRUCT_VISUALPARAM Test;

static StructTxtPxlLen lenStr;
static KEYBOARD_TYPES actualKeyboardType = KEYBOARD_none;
static char tempBuff[25];

static void EXPER_FUNC_beforeDispBuffLcd(void);
static void EXPER_FUNC_afterDispBuffLcd(void);

static void FRAMES_GROUP_combined(int argNmb, int startOffsX,int startOffsY, int offsX,int offsY,  int bold);
static void FRAMES_GROUP_separat(int argNmb, int startOffsX,int startOffsY, int offsX,int offsY,  int boldFrame);

static int *ppMain[7] = {(int*)FRAMES_GROUP_combined,(int*)FRAMES_GROUP_separat,(int*)"Rafal", (int*)&Test, NULL, NULL, NULL };
static char keyBuff[KEYBUFF_SIZE]="Test";

static void KEYBOARD_SETTXT_ServiceTxtBuffer(char *buf, int size){
	DbgDma ( 1, "\r\n" );
	DbgDma ( 1, buf );
	DbgDma ( 1, "\r\n" );
}
static void ResetIndexKeyBuff		  (void){ KEYBOARD_SETTXT_ServiceTxtBuffer(keyBuff,KEYBUFF_SIZE); memset(keyBuff,0,KEYBUFF_SIZE); }
static void CopyKeyBuff2ShowTxtBuff(void){ /* STRING_CopyBuff( buffer,keyBuff, SIZE_TXT_SHOW,strlen(keyBuff) ); */ }
static void ServiceKeyCharBuff	  (void){ CopyKeyBuff2ShowTxtBuff(); ResetIndexKeyBuff(); }

static void LoadFonts(int startFontID, int endFontID){
	if(TakeMutex(Semphr_cardSD,1000))
	{
		#define A(x)	 *((int*)((int*)(&v)+x))

		int d = endFontID-startFontID + 1;
		int j=0;

		for(int i=startFontID; i<=endFontID; ++i){
			*((int*)((int*)(&v)+i)) = LCD_LoadFont_DependOnColors( A(j),A(j+d),A(j+3*d),A(j+2*d), FILE_NAME(GetDefaultParam)(i));
			j++;
		}

		GiveMutex(Semphr_cardSD);
		#undef A
	}
/*
	v.FONT_ID_Title 	 	= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(Title),	FILE_NAME(GetDefaultParam)(FONT_ID_Title));
	v.FONT_ID_Descr		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(Descr),	FILE_NAME(GetDefaultParam)(FONT_ID_Descr));
	v.FONT_ID_Press 		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(Press),  	FILE_NAME(GetDefaultParam)(FONT_ID_Press));
*/
}

static void RADIO_CalcFreqDiv(int nr){
	Test.Radio[nr].freqDiv = ((double)Test.Radio[nr].freqStep+1) * (((Test.Radio[nr].freq+Test.Radio[nr].freqOffs)-INTER_F1-DIV_FREQ)/(DIV_ACT*STEP_FREQ)) - 1;
/*	Test.Radio[i].freq 	  = INTER_F1 + DIV_FREQ + DIV_ACT*STEP_FREQ*((Test.Radio[i].freqDiv+1)/(Test.Radio[i].freqStep+1)); */
}

static void VisualParam_LCD_Reset(void)
{
	Test.Radio[0].freq= 101.6;		strcpy(Test.Radio[0].radioName,"Radio Krak"ó"w");
	Test.Radio[1].freq=  87.8;		strcpy(Test.Radio[1].radioName,"RMF Classic");
	Test.Radio[2].freq=  89.4;		strcpy(Test.Radio[2].radioName,"Jedynka");
	Test.Radio[3].freq=  88.8;		strcpy(Test.Radio[3].radioName,"Eska 2");
	Test.Radio[4].freq=  99.4;		strcpy(Test.Radio[4].radioName,"Tr"ó"jka");
	Test.Radio[5].freq=  96.0;		strcpy(Test.Radio[5].radioName,"RMF FM");
	Test.Radio[6].freq= 104.1;		strcpy(Test.Radio[6].radioName,"Radio Zet");
	Test.Radio[7].freq=  96.7;		strcpy(Test.Radio[7].radioName,"RMF MAXXX");
	Test.Radio[8].freq= 106.1;		strcpy(Test.Radio[8].radioName,"Radio Plus");
	Test.Radio[9].freq=  90.6;		strcpy(Test.Radio[9].radioName,"Radio Maryja");
	Test.Radio[10].freq= 92.5;		strcpy(Test.Radio[10].radioName,"Z"ł"ote przeboje");
	Test.Radio[11].freq= 102.4;	strcpy(Test.Radio[11].radioName,"Radio Pogoda");
	Test.Radio[12].freq= 103.0;	strcpy(Test.Radio[12].radioName,"Radio Katowice");
	Test.Radio[13].freq= 102.9;	strcpy(Test.Radio[13].radioName,"TOK FM");
	Test.Radio[14].freq=  97.7;	strcpy(Test.Radio[14].radioName,"Eska");
	Test.Radio[15].freq=  93.7;	strcpy(Test.Radio[15].radioName,"Chillizet");
	Test.Radio[16].freq= 104.9;	strcpy(Test.Radio[16].radioName,"Eska Rock");
	Test.Radio[17].freq= 107.0;	strcpy(Test.Radio[17].radioName,"VOX FM");
	Test.Radio[18].freq= 101.0;	strcpy(Test.Radio[18].radioName,"AntyRadio");
	Test.Radio[19].freq= 102.0;	strcpy(Test.Radio[19].radioName,"Dw"ó"jka");
	Test.Radio[20].freq=  97.2;	strcpy(Test.Radio[20].radioName,"Polskie Radio 24");
	Test.Radio[21].freq= 103.8;	strcpy(Test.Radio[21].radioName,"Rock Radio");
	Test.Radio[22].freq=  95.2;	strcpy(Test.Radio[22].radioName,"Radio Wnet");
	Test.Radio[23].freq= 100.5;	strcpy(Test.Radio[23].radioName,"Radio Famka");

	LOOP_FOR(i,MAX_RADIO_CHANNEL){	Test.pName[i]			  = Test.Radio[i].radioName;
												Test.Radio[i].freqStep = (0x12<<8)|0xBF;
												Test.Radio[i].freqOffs = 0.0;
												RADIO_CalcFreqDiv(i);
	}
	Test.selRadio=12;
}

static StructTxtPxlLen ELEMENT_Param_1(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	StructTxtPxlLen lenStr = {0};

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(Param_1), xPos, yPos, "1.Rafa"ł" "Ó""ś""ź""Ź". Markielowski", fullHight, 0,250, ConstWidth, \
		v.FONT_ID_Descr, GRAY, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	Above_left,  SL(LANG_nazwa_0), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, GRAY, v.FONT_BKCOLOR_Descr, 4, 				Left_mid, 	 "7.",  				 fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, GRAY, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	Under_left,  SL(LANG_nazwa_1), fullHight, 0,250, NoConstWidth, \
		LCD_STR_DESCR_PARAM_NUMBER(3) );

	LCD_SetBkFontShape(v.FONT_VAR_Param_1,BK_LittleRound);

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;

	return lenStr;
}

static StructTxtPxlLen ELEMENT_Param_2(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	StructTxtPxlLen lenStr = {0};
	StructFieldPos fieldTouch = {0};

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(Param_2), xPos, yPos, "2.Rafa"ł"", fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(186,130,50), v.FONT_BKCOLOR_Descr, 	4|(xPos<<16),	Above_left,  SL(LANG_nazwa_0), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, GRAY, 					  v.FONT_BKCOLOR_Descr, 	4, 				Left_mid, 	 "7.",  				 fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(251,29,27),  v.FONT_BKCOLOR_Descr, 	4|(xPos<<16),	Under_left,  SL(LANG_nazwa_1), fullHight, 0,250, NoConstWidth, \
		LCD_STR_DESCR_PARAM_NUMBER(3) );

	LCD_SetBkFontShape(v.FONT_VAR_Param_2,BK_LittleRound);

	fieldTouch 			= *field;
	fieldTouch.width 	= fieldTouch.width/3;
	fieldTouch.x 		= fieldTouch.x + fieldTouch.width;

	if(LoadWholeScreen==argNmb){	SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_RELEASE_WITH_HOLD, Touch_Param_2,  	 		LCD_TOUCH_SetTimeParam_ms(600), v.FONT_VAR_Param_2,0, *field);
											SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_WITH_HOLD, 		    Touch_Param_22, 	 		LCD_TOUCH_SetTimeParam_ms(700), v.FONT_VAR_Param_2,1, *field);
											SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_MOVE_RIGHT, 		    	 Touch_Param_2MoveRight, press, 								  v.FONT_VAR_Param_2,2, fieldTouch);
											SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_MOVE_LEFT, 		    	 	 Touch_Param_2MoveLeft,  press, 								  v.FONT_VAR_Param_2,3, fieldTouch);
	}

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;
	return lenStr;
}

static StructTxtPxlLen ELEMENT_Param_3(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	StructTxtPxlLen lenStr = {0};

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(Param_3), xPos, yPos, "3.Rafa"ł" "Ó""ś""ź""Ź".3", fullHight, 0,250, ConstWidth, \
		v.FONT_ID_Descr, GRAY, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	Above_left,  SL(LANG_nazwa_0), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, GRAY, v.FONT_BKCOLOR_Descr, 4, 				Left_mid, 	 "7.",  				 fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, GRAY, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	Under_left,  SL(LANG_nazwa_1), fullHight, 0,250, NoConstWidth, \
		LCD_STR_DESCR_PARAM_NUMBER(3) );

	LCD_SetBkFontShape(v.FONT_VAR_Param_3,BK_LittleRound);

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;

	return lenStr;
}

static void FRAMES_GROUP_combined(int argNmb, int startOffsX,int startOffsY, int offsX,int offsY, int bold)
{
	#define _LINES_COLOR		COLOR_GRAY(0x77)
	#define _FILL_COLOR		v.COLOR_FillMainFrame

	#define	_Element(name,cmdX,offsX,cmdY,offsY)		lenStr=ELEMENT_##name(&field, LCD_Xpos(lenStr,cmdX,offsX), LCD_Ypos(lenStr,cmdY,offsY), argNmb);
	#define	_LineH(width,cmdX,offsX,cmdY,offsY)		 LCD_LineH(LCD_X,LCD_Xpos(lenStr,cmdX,offsX)-2, LCD_Ypos(lenStr,cmdY,offsY), width+4, _LINES_COLOR, bold );
	#define	_LineV(width,cmdX,offsX,cmdY,offsY)		 LCD_LineV(LCD_X,LCD_Xpos(lenStr,cmdX,offsX), LCD_Ypos(lenStr,cmdY,offsY)-2, width+4, _LINES_COLOR, bold );

	StructFieldPos field={0}, field1={0};
	uint16_t tab[4]={0};
	int X_start=0;

	FILE_NAME(funcSet)(FONT_BKCOLOR_Descr, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_Param_1, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_Param_2, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_Param_3, 	_FILL_COLOR);

	_Element(Param_1,SetPos,X_start=startOffsX,SetPos,startOffsY)		/* _LineV(field.height,GetPos,-startOffsX/2-1,GetPos,0) */	field1=field;
	_Element(Param_2,GetPos,0,IncPos,offsY)									/* _LineV(field.height,GetPos,-startOffsX/2-1,GetPos,0) */
	tab[0]=field1.width;
	tab[1]=field.width;
	MAXVAL(tab,2,0,tab[3])
	_LineH(tab[3],GetPos,0,GetPos,-offsY/2-1)

	_Element(Param_3,SetPos,X_start+=tab[3]+offsX,SetPos,startOffsY)		_LineV(field.height,GetPos,-offsX/2-1,GetPos,0)	field1=field;
//	_Element(Param_1,GetPos,0,IncPos,offsY)										_LineV(field.height,GetPos,-offsX/2-1,GetPos,0)
//	tab[0]=field1.width;
//	tab[1]=field.width;
//	MAXVAL(tab,2,0,tab[3])
//	_LineH(tab[3],GetPos,0,GetPos,-offsY/2-1)
//
//	_Element(Param_2,SetPos,X_start+=tab[3]+offsX,SetPos,startOffsY)	_LineV(field.height,GetPos,-offsX/2-1,GetPos,0)	field1=field;
//	_Element(Param_3,GetPos,0,IncPos,offsY)									_LineV(field.height,GetPos,-offsX/2-1,GetPos,0)
//	tab[0]=field1.width;
//	tab[1]=field.width;
//	MAXVAL(tab,2,0,tab[3])
//	_LineH(tab[3],GetPos,0,GetPos,-offsY/2-1)




	#undef _Element
	#undef _LineH
	#undef _LineV
	#undef _FILL_COLOR
	#undef _LINES_COLOR
}

static void FRAMES_GROUP_separat(int argNmb, int startOffsX,int startOffsY, int offsX,int offsY, int boldFrame)		/* Parameters ..Offs.. is counted from STR (not from FRAME) */
{
		#define _FRAME_COLOR		v.COLOR_Frame
		#define _FILL_COLOR		v.COLOR_FillFrame
																										 /* LCD_BoldRoundRectangle */
		#define _Rectan LCD_Shape(field.x-fontsFrameSpace, field.y-fontsFrameSpace, LCD_RoundRectangle, field.width+2*fontsFrameSpace, field.height+2*fontsFrameSpace, SetBold2Color(_FRAME_COLOR,bold), _FILL_COLOR, v.COLOR_FillMainFrame)

		#define _Element(name,nrX,cmdX,Xoffs,nrY,cmdY,Yoffs)	\
				lenStr=ELEMENT_##name(&field, LCD_posX(nrX,lenStr,cmdX,Xoffs), LCD_posY(nrY,lenStr,cmdY,Yoffs), argNmb); \
				_Rectan; \
				lenStr=ELEMENT_##name(&field, LCD_posX(nrX,lenStr,GetPos,0), 	LCD_posY(nrY,lenStr,GetPos,0), 	argNmb); \
				LCD_posY(nrY,lenStr,IncPos,offsY);

		StructFieldPos field={0};
		uint8_t fontsFrameSpace = boldFrame >>8;
		int bold = boldFrame&0x000000FF;

		FILE_NAME(funcSet)(FONT_BKCOLOR_Descr, 	_FILL_COLOR);
		FILE_NAME(funcSet)(FONT_BKCOLOR_Param_1, 	_FILL_COLOR);
		FILE_NAME(funcSet)(FONT_BKCOLOR_Param_2, 	_FILL_COLOR);
		FILE_NAME(funcSet)(FONT_BKCOLOR_Param_3, 	_FILL_COLOR);


//			_Element(Param_1,0,SetPos,startOffsX,0,SetPos,startOffsY) 	_Element(Param_2,0,IncPos,offsX,1,SetPos,startOffsY)
//			_Element(Param_3,0,SetPos,startOffsX,0,GetPos,0)


		_Element(Param_1,0,SetPos,startOffsX,0,SetPos,startOffsY)
		_Element(Param_2,0,SetPos,startOffsX,0,GetPos,0) 				_Element(Param_3,0,IncPos,offsX,1,SetPos,startOffsY)

		#undef _Element
		#undef _Rectan
		#undef _FRAME_COLOR
		#undef _FILL_COLOR
}

/* ------------ FILE_NAME() functions ------------ */
static void FILE_NAME(timer)(void)  /* alternative RTOS Timer Callback or create new thread vTaskTimer */
{
	if(vTimerService(TIMER_BlockTouch, check_stop_time, 500)){
		BlockTouchForTime(_OFF,TIMER_BlockTouch);
	}
}

static int RADIO_SetFreq(int nr){ 	PCF8575_Init();
	INIT(temp1,PCF8575_Write(0,Test.Radio[nr].freqStep));
	INIT(temp2,PCF8575_Write(1,Test.Radio[nr].freqDiv));
	if(EQUAL2_OR(-1,temp1,temp2)) return 0; else return 1;
}

static int FILE_NAME(keyboard)(KEYBOARD_TYPES type, SELECT_PRESS_BLOCK selBlockPress, INIT_KEYBOARD_PARAM)
{
	KEYBOARD_SetGeneral(v.FONT_ID_Press, v.FONT_ID_Descr, 	v.FONT_COLOR_Descr,
								  	  	  	  	  	 v.COLOR_MainFrame,  v.COLOR_FillMainFrame,
													 v.COLOR_Frame, 		v.COLOR_FillFrame,
													 v.COLOR_FramePress, v.COLOR_FillFramePress, v.COLOR_BkScreen);

	actualKeyboardType = type;
	if(KEYBOARD_StartUp(type, ARG_KEYBOARD_PARAM)) return 1;

	switch((int)type)
	{
		case KEYBOARD_Param_1:
			break;

		case KEYBOARD_Param_2:
			KEYBOARD_KeyAllParamSet3(1,MAX_RADIO_CHANNEL, COLOR_GRAY(0xDD), DARKRED, Test.pName);
			KEYBOARD_ServiceSizeRoll(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_Select_one, ROLL_1,NULL,v.FONT_COLOR_Descr, 8, Test.selRadio);
			break;

		case KEYBOARD_setTxt:
			KEYBOARD__ServiceSetTxt(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY_Q, KEY_big, KEY_back, KEY_alt, KEY_enter,KEY_field,KEY_style,KEY_exit, v.FONT_COLOR_Descr, keyBuff,KEYBUFF_SIZE);
			break;

		case KEYBOARD_TunningFreq:
			KEYBOARD_KeyAllParamSet(2,1, "+", "-", WHITE,WHITE, LIGHTCYAN,LIGHTCYAN);
			KEYBOARD_SetGeneral(Ni,Ni,Ni, Ni,Ni, Ni,BrightIncr(v.COLOR_FillFrame,0xE), Ni,Ni,Ni);
			KEYBOARD_Buttons(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY_TunningFreq_plus, SL(LANG_TunningFreq));
			break;

		default:
			break;
	}
	return 0;
}

void FILE_NAME(debugRcvStr)(void){	 if(v.DEBUG_ON){
		DEBUG_InvalidateDCache (); // Same RX path as esp32wroom.c.
}}

static void RADIO_DispFreq(void){
	RADIO_CalcFreqDiv(Test.selRadio);
	if(RADIO_SetFreq(Test.selRadio));
	SELECT_CURRENT_FONT( Param_2,Press, TXT_RADIO_STATION, unUsed );
}

static void FUNC_TunningFreq(int k){ switch(k){
  case -1: return;
  case  0: INCR( Test.Radio[Test.selRadio].freqOffs, 0.01, 0.50 ); break;
  case  1: DECR( Test.Radio[Test.selRadio].freqOffs, 0.01,-0.50 ); break; }
	RADIO_DispFreq();
}

void FILE_NAME(setTouch)(void)
{

	TOUCH_FUNCTIONS_

	TouchScreenInit();

	void CreateKeyboard(KEYBOARD_TYPES keboard){
		switch((int)keboard){
			case KEYBOARD_Param_1:	break;
			case KEYBOARD_Param_2:  FILE_NAME(keyboard)(KEYBOARD_Param_2,		KEY_Select_one, LCD_Rectangle,	  0,410, 50, KeysAutoSize,10,0, screenTouchState,Touch_FieldRoll, 	 	KeysDel);
											FILE_NAME(keyboard)(KEYBOARD_TunningFreq, KEY_All_release,LCD_RoundRectangle,0,300,205, KeysAutoSize,10,10,screenTouchState,Touch_TunningFreq_plus,KeysNotDel); 	break;
	}}


	screenTouchState = LCD_TOUCH_GetTypeAndPosition(&screenTouchPos);

	/*	----- Service press specific Keys for Keyboard ----- */
	_TouchService(Touch_TunningFreq_plus, Touch_TunningFreq_minus,	KEYBOARD_TunningFreq, KEY_All_release, KEY_TunningFreq_plus, FUNC_TunningFreq);


	switch(screenTouchState)
	{
		/*	----- Initiation new Keyboard ----- */
		CASE_TOUCH_STATE(screenTouchState,Touch_Param_2, Param_2,Press, " Touch_Param_2 ",unUsed,CheckTouchForTime(Touch_Param_2MoveRight,TIMER_BlockTouch),CheckTouchForTime(Touch_Param_2MoveLeft,TIMER_BlockTouch));		/* 'Param_2','Press' are suffix`s for elements of 'SCREEN_FONTS_SET_PARAMETERS' MACRO  */
			if(IsSetTouchFlag())
				DisplayTouchPosXY(screenTouchState,screenTouchPos,"Touch_Param_2");
		break;

		CASE_TOUCH_STATE(screenTouchState,Touch_Param_22, Param_2,Press, " Touch_Param_22 ",unUsed,NoTouch,NoTouch);
			if(IsSetTouchFlag())
				DisplayTouchPosXY(screenTouchState,screenTouchPos,"Touch_Param_22");
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_Param_2MoveRight, Param_2,Press, TXT_RADIO_STATION, unUsed,NoTouch,NoTouch);
			if(IsSetTouchFlag()) CreateKeyboard(KEYBOARD_Param_2);
			else 						_SaveState();
			BlockTouchForTime(_ON,TIMER_BlockTouch);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_Param_2MoveLeft, Param_2,Press, " Touch_Param_2MoveLeft ",unUsed,NoTouch,NoTouch);
			if(IsSetTouchFlag()){
				DisplayTouchPosXY(screenTouchState,screenTouchPos,"Touch_Param_2MoveLeft");
			}
			else _SaveState();
			BlockTouchForTime(_ON,TIMER_BlockTouch);
			break;

		/*	----- Touch parameter text and go to action ----- */
	case Touch_NextScreen: SCREEN_SetNr(0); break;
	case Touch_PrevScreen: SCREEN_SetNr(0); break;

	case Touch_FieldRoll:
		if(LCDTOUCH_IsScrollPress(ROLL_1, screenTouchState, &screenTouchPos, TIMER_Scroll)){	KEYBOARD_TYPE( KEYBOARD_Param_2, KEY_Select_one); 	 }
		_SaveState();
		break;


		default:		 /* ----- Service release specific Keys for Keyboard ----- */

			if(_KEYBOARD_setTxt__SERVICE(screenTouchState,Touch_Q,Touch_keyStyle,KEY_Q)) break;

			_TouchEndService(Touch_TunningFreq_plus, Touch_TunningFreq_minus, KEYBOARD_TunningFreq, 	KEY_All_release, FUNC_TunningFreq);

			if(_WasState(Touch_FieldRoll)){
				int temp;
				if(END_FREEROLL__NOSEL != (temp = LCDTOUCH_IsScrollRelease(ROLL_1, FUNC1_SET( FILE_NAME(keyboard),KEYBOARD_Param_2,KEY_Select_one,0,0,0,0,0,0,0,0,0,0), NULL/*BlockingFunc*/, TIMER_Scroll))){
					DbgVarDma ( 1, 100, "\r\nRoll: %d", temp );
					if(IS_RANGE(temp, 0, MAX_RADIO_CHANNEL-1)){
						Test.selRadio=temp;
						RADIO_DispFreq();
					}
				}
			}

			break;
	}

	FILE_NAME(timer)();
/*	LCDTOUCH_testFunc(); */

}

void FILE_NAME(main)(int argNmb, char **argVal)
{
	if(NULL == argVal)
		argVal = (char**)ppMain;

	LCD_Clear(v.COLOR_BkScreen);

	if(LoadWholeScreen == argNmb)
	{
		SCREEN_ResetAllParameters();
		LCD_TOUCH_DeleteAllSetTouch();
		VisualParam_LCD_Reset();

		DbgVarDma ( v.DEBUG_ON, 100, "" Cya_ "\r\nStart: %s\r\n" _X_, GET_CODE_FUNCTION );

		LoadFonts(FONT_ID_Title, FONT_ID_Param_3);
		DisplayFontsStructState();

		LCDTOUCH_Set( LCD_X-30, LCD_Y/2-30,  30,60,  ID_TOUCH_POINT,Touch_NextScreen,release);
		LCDTOUCH_Set( 0, 			LCD_Y/2-30,  30,60,  ID_TOUCH_POINT,Touch_PrevScreen,release);
	}
	/*FILE_NAME(printInfo)();*/

	INIT(endSetFrame,400);
	LCD_DrawMainFrame(LCD_RoundRectangle,NoIndDisp,0, 0,0, LCD_X,endSetFrame,SHAPE_PARAM(MainFrame,FillMainFrame,BkScreen));

	*ppMain=(int*)FRAMES_GROUP_combined; //tymczasowo !

	if		 (*(argVal+0)==(char*)FRAMES_GROUP_combined)
		FRAMES_GROUP_combined(argNmb,15,15,25,25,1);
	else if(*(argVal+0)==(char*)FRAMES_GROUP_separat)
		FRAMES_GROUP_separat(argNmb,15,15,25,25,FRAME_bold2Space(0,6));





	LCD_TxtShadowInit(fontVar_40, v.FONT_ID_Param_1, v.COLOR_BkScreen, BK_Rectangle);
	LCD_Txt(Display, NULL, 0,0, LCD_X,LCD_Y, v.FONT_ID_Param_1, fontVar_40, 20,420, SL(LANG_nazwa_0), BLACK, 0/*v.COLOR_BkScreen*/, fullHight,0,250, NoConstWidth, TXTSHADECOLOR_DEEP_DIR(0x777777,4,RightDown) /*TXTSHADE_NONE*/);

	LCD_StrDependOnColors(v.FONT_ID_Descr, LCD_X-FV(GetVal,0,NoUse), LCD_Y-FV(GetVal,1,NoUse), SL(LANG_nazwa_1), fullHight,0, v.COLOR_FillFrame, v.FONT_COLOR_Descr, 255, NoConstWidth);

	//if(LoadWholeScreen  == argNmb) TxtTouch(TouchSetNew);
	if(LoadNoDispScreen != argNmb) LCD_Show();

}


