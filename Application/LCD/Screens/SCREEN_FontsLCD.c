
#include "SCREEN_FontsLCD.h"
#include "SCREEN_ReadPanel.h"
#include "LCD_Common.h"
#include "timer.h"
#include "lang.h"
#include "_debug.h"
#include "string_oper.h"
#include "cpu_utils.h"
#include "tim.h"
#include "touch.h"
#include "common.h"
#include "mini_printf.h"
#include "TouchLcdTask.h"
#include "stmpe811.h"

#include "FreeRTOS.h"
#include "task.h"
#include "Keyboard.h"
#include "examples.h"
#include "string.h"

#include "esp32wroom.h"
#include "_smtp.h"


/*----------------- Main Settings ------------------*/
#define FILE_NAME(extend) SCREEN_Fonts_##extend

#define SCREEN_FONTS_LANG \
	X(LANG_nazwa_0, "Czcionki LCD", "Fonts LCD") \
	X(LANG_nazwa_1, "Zmiana kolor"ó"w czcionki", "Press to change color fonts") \
	X(LANG_nazwa_2, "1.", "1.") \
	X(LANG_nazwa_3, "Czer", "Red") \
	X(LANG_nazwa_4, "Ziel", "Green") \
	X(LANG_nazwa_5, "Nieb", "Blue") \
	X(LANG_nazwa_6, "Zmiana kolor"ó"w t"ł"a", "Press to change color fonts background") \
	X(LANG_nazwa_7, "2.", "2.") \
	X(LANG_nazwa_8, "Klawiatura RGB", "Keyboard RGB") \
	X(LANG_FontTypeAbove, "Zmiana typu czcionki", "Press to change type fonts") \
	X(LANG_FontTypeLeft, "3.", "3.") \
	X(LANG_FontTypeUnder, "kolor t"ł"a i czcionki", "background - font") \
	X(LANG_FontSizeAbove,    "Zmiana rozmiaru czcionki", "Press to change size fonts") \
	X(LANG_FontSizeLeft, 	 "4.", "4.") \
	X(LANG_FontSizeUnder, 	 "normalna, t"ł"usta, pochy"ł"a", "normal,bold,italics") \
	X(LANG_FontStyleAbove, "Zmiana stylu czcionki", "Press to change style fonts") \
	X(LANG_FontStyleLeft, 	 "5.", "5.") \
	X(LANG_FontStyleUnder, 	 "Nazwa stylu pod czcionk"ą"", "---2") \
	X(LANG_FontCoeffAbove, "Wsp"ó""ł"czynnik", "Coefficient") \
	X(LANG_FontCoeffLeft, 	 "6.", "6.") \
	X(LANG_FontCoeffUnder, 	 "naci"ś"nij", "press me") \
	X(LANG_CoeffKeyName, "Wsp"ó""ł"czyn", "Coeff") \
	X(LANG_LenOffsWin1, "Okre"ś"lenie odst"ę"p"ó"w pom"ę"dzy literami", "Specifying the spacing between letters") \
	X(LANG_LenOffsWin2, "Przesuwanie tekstu, zmiana pozycji kursora i zapis zmian", "Moving text, changing cursor position, editing and saving changes") \
	X(LANG_LenOffsWin3, "Szeroko"ś""ć" tekstu", "xxxxxxx") \
	X(LANG_LenOffsWin4, "i jego przesuni"ę"cie", "xxxxxxx") \
	X(LANG_TimeSpeed1, "Czas za"ł"adowania czcionek", "xxxxxxx") \
	X(LANG_TimeSpeed2, "i szybko"ś""ć" wy"ś"wietlenia", "xxxxxxx") \
	X(LANG_WinInfo, "Zmiany odst"ę"p"ó"w mi"ę"dzy literami zosta"ł"y zapisane", "xxxxxxx") \
	X(LANG_WinInfo2, "Reset wszystkich ustawie"ń" dla odst"ę"p"ó"w mi"ę"dzy literami", "xxxxxxx") \
	X(LANG_MainFrameType, "Zmie"ń" wygl"ą"d", "Change appearance") \

/* MYGRAY2 in below because function 'LoadFont' must load font type of 'MYGRAY-MYGREEN' to enable to change font colors (not MYGRAY-WHITE and not enable to change font colors) */
#define SCREEN_FONTS_SET_PARAMETERS \
/* id   name							default value */ \
	X(0, FONT_SIZE_Title, 	 		FONT_24_bold) \
	X(1, FONT_SIZE_FontColor, 	 	FONT_14) \
	X(2, FONT_SIZE_BkColor,			FONT_14) \
	X(3, FONT_SIZE_FontType,		FONT_14) \
	X(4, FONT_SIZE_FontSize,		FONT_14) \
	X(5, FONT_SIZE_FontStyle,		FONT_14) \
	X(6, FONT_SIZE_Coeff,			FONT_14) \
	X(7, FONT_SIZE_LenWin,			FONT_14) \
	X(8, FONT_SIZE_OffsWin,			FONT_10) \
	X(9, FONT_SIZE_LoadFontTime,	FONT_10) \
	X(10, FONT_SIZE_PosCursor,		FONT_10) \
	X(11, FONT_SIZE_CPUusage,		FONT_10) \
	X(12, FONT_SIZE_Speed,			FONT_10) \
	X(13, FONT_SIZE_Descr, 	 		FONT_12) \
	X(14, FONT_SIZE_Press, 	 		FONT_14) \
	X(15, FONT_SIZE_Fonts,			FONT_20) \
	\
	X(16, FONT_STYLE_Title, 	 	Arial) \
	X(17, FONT_STYLE_FontColor, 	Arial) \
	X(18, FONT_STYLE_BkColor, 		Arial) \
	X(19, FONT_STYLE_FontType, 	Arial) \
	X(20, FONT_STYLE_FontSize, 	Arial) \
	X(21, FONT_STYLE_FontStyle, 	Arial) \
	X(22, FONT_STYLE_Coeff, 		Arial) \
	X(23, FONT_STYLE_LenWin, 		Arial) \
	X(24, FONT_STYLE_OffsWin, 		Arial) \
	X(25, FONT_STYLE_LoadFontTime,Arial) \
	X(26, FONT_STYLE_PosCursor,	Arial) \
	X(27, FONT_STYLE_CPUusage,		Arial) \
	X(28, FONT_STYLE_Speed,			Arial) \
	X(29, FONT_STYLE_Descr, 		Times_New_Roman) \
	X(30, FONT_STYLE_Press, 		Arial) \
	X(31, FONT_STYLE_Fonts, 		Arial) \
	\
	X(32, FONT_COLOR_Title,  	 	WHITE) \
	X(33, FONT_COLOR_FontColor, 	WHITE) \
	X(34, FONT_COLOR_BkColor, 	 	WHITE) \
	X(35, FONT_COLOR_FontType,  	WHITE) \
	X(36, FONT_COLOR_FontSize,  	WHITE) \
	X(37, FONT_COLOR_FontStyle,  	WHITE) \
	X(38, FONT_COLOR_Coeff,  		WHITE) \
	X(39, FONT_COLOR_LenWin,  		WHITE) \
	X(40, FONT_COLOR_OffsWin,  	WHITE) \
	X(41, FONT_COLOR_LoadFontTime,WHITE) \
	X(42, FONT_COLOR_PosCursor,	WHITE) \
	X(43, FONT_COLOR_CPUusage,		WHITE) \
	X(44, FONT_COLOR_Speed,			WHITE) \
	X(45, FONT_COLOR_Descr, 		COLOR_GRAY(0x99)) \
	X(46, FONT_COLOR_Press, 		DARKRED) \
	X(47, FONT_COLOR_Fonts,  		0xFFE1A000) \
	\
	X(48, FONT_BKCOLOR_Title,  	 	MYGRAY2) \
	X(49, FONT_BKCOLOR_FontColor, 	MYGRAY2) \
	X(50, FONT_BKCOLOR_BkColor, 	 	MYGRAY2) \
	X(51, FONT_BKCOLOR_FontType,  	MYGRAY2) \
	X(52, FONT_BKCOLOR_FontSize,  	MYGRAY2) \
	X(53, FONT_BKCOLOR_FontStyle,  	MYGRAY2) \
	X(54, FONT_BKCOLOR_Coeff,  		MYGRAY2) \
	X(55, FONT_BKCOLOR_LenWin,  		MYGRAY2) \
	X(56, FONT_BKCOLOR_OffsWin,  		MYGRAY2) \
	X(57, FONT_BKCOLOR_LoadFontTime,	MYGRAY2) \
	X(58, FONT_BKCOLOR_PosCursor,		MYGRAY2) \
	X(59, FONT_BKCOLOR_CPUusage,		MYGRAY2) \
	X(60, FONT_BKCOLOR_Speed,			MYGRAY2) \
	X(61, FONT_BKCOLOR_Descr, 			MYGRAY2) \
	X(62, FONT_BKCOLOR_Press, 			WHITE) \
	X(63, FONT_BKCOLOR_Fonts,  		0x090440) \
	\
	X(64, COLOR_BkScreen,  			COLOR_GRAY(0x38)) \
	X(65, COLOR_MainFrame,  		COLOR_GRAY(0xD0)) \
	X(66, COLOR_FillMainFrame, 	COLOR_GRAY(0x31)) \
	X(67, COLOR_Frame,  				COLOR_GRAY(0xD0)) \
	X(68, COLOR_FillFrame, 			COLOR_GRAY(0x3B)) \
	X(69, COLOR_FramePress, 		COLOR_GRAY(0xBA)) \
	X(70, COLOR_FillFramePress,	COLOR_GRAY(0x60)) \
	X(71, DEBUG_ON,  	1) \
	X(72, BK_FONT_ROUND,  	1) \
	X(73, LANG_SELECT,  	Polish) \
	\
	X(74, FONT_ID_Title,			fontID_1) \
	X(75, FONT_ID_FontColor,	fontID_2) \
	X(76, FONT_ID_BkColor, 		fontID_3) \
	X(77, FONT_ID_FontType, 	fontID_4) \
	X(78, FONT_ID_FontSize, 	fontID_5) \
	X(79, FONT_ID_FontStyle,  	fontID_6) \
	X(80, FONT_ID_Coeff,  		fontID_7) \
	X(81, FONT_ID_LenWin,  		fontID_8) \
	X(82, FONT_ID_OffsWin,  	fontID_9) \
	X(83, FONT_ID_LoadFontTime,fontID_10) \
	X(84, FONT_ID_PosCursor,	fontID_11) \
	X(85, FONT_ID_CPUusage,		fontID_12) \
	X(86, FONT_ID_Speed,			fontID_13) \
	X(87, FONT_ID_Descr,  		fontID_14) \
	X(88, FONT_ID_Press,  		fontID_15) \
	X(89, FONT_ID_Fonts,  		fontID_16) \
	\
	X(90, FONT_VAR_Title,			fontVar_1) \
	X(91, FONT_VAR_FontColor,		fontVar_2) \
	X(92, FONT_VAR_BkColor, 		fontVar_3) \
	X(93, FONT_VAR_FontType, 		fontVar_4) \
	X(94, FONT_VAR_FontSize, 		fontVar_5) \
	X(95, FONT_VAR_FontStyle,  	fontVar_6) \
	X(96, FONT_VAR_Coeff,  			fontVar_7) \
	X(97, FONT_VAR_LenWin,  		fontVar_8) \
	X(98, FONT_VAR_OffsWin,  		fontVar_9) \
	X(99, FONT_VAR_LoadFontTime,	fontVar_10) \
	X(100, FONT_VAR_PosCursor,		fontVar_11) \
	X(101, FONT_VAR_CPUusage,		fontVar_12) \
	X(102, FONT_VAR_Speed,			fontVar_13) \
	X(103, FONT_VAR_Fonts,  		fontVar_14) \
	X(104, FONT_VAR_Press,  		fontVar_15) \
/*------------ End Main Settings -----------------*/

/*------------ Main Screen MACROs -----------------*/
#define SL(name)	(char*)FILE_NAME(Lang)[ v.LANG_SELECT==Polish ? 2*(name) : 2*(name)+1 ]

typedef enum{
	#define X(a,b,c) a,
		SCREEN_FONTS_LANG
	#undef X
}FILE_NAME(Lang_enum);

static const char *FILE_NAME(Lang)[]={
	#define X(a,b,c) b"\x00",c"\x00",
		SCREEN_FONTS_LANG
	#undef X
};

typedef enum{
	#define X(a,b,c) b,
		SCREEN_FONTS_SET_PARAMETERS
	#undef X
}FILE_NAME(enum);

typedef struct{
	#define X(a,b,c) int b;
		SCREEN_FONTS_SET_PARAMETERS
	#undef X
}FILE_NAME(struct);

static FILE_NAME(struct) v ={
	#define X(a,b,c) c,
		SCREEN_FONTS_SET_PARAMETERS
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
		SCREEN_FONTS_SET_PARAMETERS
	#undef X
		return temp;
}
*/
static int FILE_NAME(GetDefaultParam)(int param){
	int temp;
	#define X(a,b,c) \
		if(b==param) temp=c;
		SCREEN_FONTS_SET_PARAMETERS
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
		SCREEN_FONTS_SET_PARAMETERS
#undef X
		DbgVarDma ( 1, 200, CoG2_ "}%s;\r\n" _X_, getName ( FILE_NAME ( struct ) ) );
	}
}
//ZROBIC szablon z macro by dla kazdego pliku szybko skopioowac !!!!!!!!!!!!!!!
int FILE_NAME(funcGet)(int offs){
	return *( (int*)((int*)(&v) + offs) );
}

void FILE_NAME(funcSet)(int offs, int val){
	*( (int*)((int*)(&v) + offs) ) = val;
	SET_bit( FILE_NAME(SelBits)[offs/32], (offs-32*(offs/32)) );
}

void FILE_NAME(setDefaultAllParam)(int rst){
	#define X(a,b,c) FILE_NAME(funcSet)(b,c);
		SCREEN_FONTS_SET_PARAMETERS
	#undef X
	if(rst){
		for(int i=0;i<SEL_BITS_SIZE;++i)
			FILE_NAME(SelBits)[i]=0;
	}
}

void FILE_NAME(debugRcvStr)(void);
void FILE_NAME(setTouch)(void);

void 	FILE_NAME(main)(int argNmb, char **argVal);
/*------------ End Main Screen MACRO -----------------*/

#define USE_DBG_CLR	0

#define TEXT_TO_SHOW		"Text to show"
#define SIZE_TXT_SHOW	200

#define ID_MIDDLE_TXT	LCD_XY_MIDDLE_MAX_NUMBER_USE-1
#define POS_X_TXT		LCD_Xmiddle(ID_MIDDLE_TXT,GetPos,v.FONT_ID_Fonts,Test.txt,Test.spaceBetweenFonts,Test.constWidth)
#define POS_Y_TXT		LCD_Ymiddle(ID_MIDDLE_TXT,GetPos,v.FONT_ID_Fonts)

#define TXT_FONT_COLOR 	StrAll(7," ",INT2STR(Test.font[0])," ",INT2STR(Test.font[1])," ",INT2STR(Test.font[2])," ")
#define TXT_BK_COLOR 	StrAll(7," ",INT2STR(Test.bk[0]),  " ",INT2STR(Test.bk[1]),  " ",INT2STR(Test.bk[2])," ")
#define TXT_FONT_TYPE	StrAll(3," ",LCD_FontType2Str(bufTemp,0,Test.type+1)+1," ")
#define TXT_FONT_SIZE	StrAll(3," ",LCD_FontSize2Str(bufTemp+25,Test.size)," ")
#define TXT_FONT_STYLE	StrAll(3," ",LCD_FontStyle2Str(bufTemp,Test.style)," ")
#define TXT_COEFF			StrAll(3," ",Int2Str(Test.coeff,Space,3,Sign_plusMinus)," ")
#define TXT_LEN_WIN		Int2Str(Test.lenWin ,' ',3,Sign_none)
#define TXT_OFFS_WIN		Int2Str(Test.offsWin,' ',3,Sign_none)
#define TXT_LENOFFS_WIN StrAll(5," ",TXT_LEN_WIN," ",TXT_OFFS_WIN," ")
#define TXT_TIMESPEED 			StrAll(4,Int2Str(Test.loadFontTime,' ',5,Sign_none)," ms   ",Int2Str(Test.speed,' ',6,Sign_none)," us")
#define TXT_CPU_USAGE		   StrAll(2,INT2STR(osGetCPUUsage()),"\%")

#define RGB_FONT 	RGB2INT(Test.font[0],Test.font[1],Test.font[2])
#define RGB_BK    RGB2INT(Test.bk[0],  Test.bk[1],  Test.bk[2])




#define NONE_TYPE_REQ	-1
#define MAX_NUMBER_OPENED_KEYBOARD_SIMULTANEOUSLY		20
/* #define TOUCH_MAINFONTS_WITHOUT_DESCR */

#define ROLL_1		0
#define KEYBUFF_SIZE		500

typedef enum{
	NoTouch = NO_TOUCH,
	Touch_FontColor,
	Touch_FontColor2,
	Touch_FontColorMoveRight,
	Touch_FontColorMoveLeft,
	Touch_BkColor,
	Touch_BkColor2,
	Touch_BkColorMove,
	Touch_FontType,
	Touch_FontType2,
	Touch_FontSize,
	Touch_FontSize2,
	Touch_FontSizeMove,
	Touch_FontStyle,
	Touch_FontStyle2,
	Touch_FontCoeff,
	Touch_FontLenOffsWin,

	Touch_fontRp,
	Touch_fontGp,
	Touch_fontBp,
	Touch_fontRm,
	Touch_fontGm,
	Touch_fontBm,

	Touch_bkRp,
	Touch_bkGp,
	Touch_bkBp,
	Touch_bkRm,
	Touch_bkGm,
	Touch_bkBm,

	Touch2_bkSliderR_left,
	Touch2_bkSliderR,
	Touch2_bkSliderR_right,
	Touch2_bkSliderG_left,
	Touch2_bkSliderG,
	Touch2_bkSliderG_right,
	Touch2_bkSliderB_left,
	Touch2_bkSliderB,
	Touch2_bkSliderB_right,
	Touch2_fontSliderR_left,
	Touch2_fontSliderR,
	Touch2_fontSliderR_right,
	Touch2_fontSliderG_left,
	Touch2_fontSliderG,
	Touch2_fontSliderG_right,
	Touch2_fontSliderB_left,
	Touch2_fontSliderB,
	Touch2_fontSliderB_right,

	Touch_fontCircleSliderR,
	Touch_fontCircleSliderG,
	Touch_fontCircleSliderB,
	Touch_CircleSliderStyle,
	Touch_CircleSlider3D,
	Touch_bkCircleSliderR,
	Touch_bkCircleSliderG,
	Touch_bkCircleSliderB,

	Touch_style1,
	Touch_style2,
	Touch_style3,
	Touch_type1,
	Touch_type2,
	Touch_type3,
	Touch_type4,
	Touch_size_plus,
	Touch_size_minus,
	Touch_size_norm,
	Touch_size_bold,
	Touch_size_italic,
	Touch_FontSizeRoll,
	Touch_coeff_plus,
	Touch_coeff_minus,
	Touch_LenWin_plus,
	Touch_LenWin_minus,
	Touch_OffsWin_plus,
	Touch_OffsWin_minus,
	Touch_PosInWin_plus,
	Touch_PosInWin_minus,
	Touch_SpaceFonts_plus,
	Touch_SpaceFonts_minus,
	Touch_DispSpaces,
	Touch_WriteSpaces,
	Touch_ResetSpaces,
	Touch_SpacesInfoUp,
	Touch_SpacesInfoDown,
	Touch_SpacesInfoStyle,
	Touch_SpacesInfoRoll,
	Touch_SpacesInfoSel,
	Touch_SpacesInfoTest,
	Touch_MainFramesType,
	Touch_SetTxt,
	Touch_Chart_1,
	Touch_Chart_2,
	Touch_Chart_3,
	Touch_NextScreen,
	Touch_PrevScreen,
	Move_1,
	Move_2,
	Move_3,
	Point_1,
	AnyPress,
	AnyPressWithWait,
	KEYBOARD_SETTXT_TOUCHS
}TOUCH_POINTS;		/* MAX_OPEN_TOUCH_SIMULTANEOUSLY */

typedef enum{
	KEYBOARD_none,
	KEYBOARD_fontRGB,
	KEYBOARD_bkRGB,
	KEYBOARD_fontSize,
	KEYBOARD_fontSize2,
	KEYBOARD_fontType,
	KEYBOARD_fontStyle,
	KEYBOARD_fontCoeff,
	KEYBOARD_LenOffsWin,
	KEYBOARD_sliderRGB,
	KEYBOARD_sliderBkRGB,
	KEYBOARD_circleSliderRGB,
	KEYBOARD_circleSliderBkRGB,
	KEYBOARD_setTxt,
}KEYBOARD_TYPES;	/* MAX_NUMBER_OPENED_KEYBOARD_SIMULTANEOUSLY */

typedef enum{
	KEY_NO_RELEASE,
	KEY_All_release,
	KEY_Select_one,
	KEY_Timer,
	KEY_Timer2,

	KEY_Red_plus,
	KEY_Green_plus,
	KEY_Blue_plus,
	KEY_Red_minus,
	KEY_Green_minus,
	KEY_Blue_minus,

	KEY2_bkSliderR_left,
	KEY2_bkSliderR,
	KEY2_bkSliderR_right,
	KEY2_bkSliderG_left,
	KEY2_bkSliderG,
	KEY2_bkSliderG_right,
	KEY2_bkSliderB_left,
	KEY2_bkSliderB,
	KEY2_bkSliderB_right,
	KEY2_fontSliderR_left,
	KEY2_fontSliderR,
	KEY2_fontSliderR_right,
	KEY2_fontSliderG_left,
	KEY2_fontSliderG,
	KEY2_fontSliderG_right,
	KEY2_fontSliderB_left,
	KEY2_fontSliderB,
	KEY2_fontSliderB_right,

	KEY_fontCircleSliderR,
	KEY_fontCircleSliderG,
	KEY_fontCircleSliderB,
	KEY_CircleSliderStyle,
	KEY_CircleSlider3D,
	KEY_bkCircleSliderR,
	KEY_bkCircleSliderG,
	KEY_bkCircleSliderB,

	KEY_Style_1,
	KEY_Style_2,
	KEY_Style_3,

	KEY_Size_plus,
	KEY_Size_minus,
	KEY_Size_norm,
	KEY_Size_bold,
	KEY_Size_italic,

	KEY_Coeff_plus,
	KEY_Coeff_minus,

	KEY_LenWin_plus,
	KEY_LenWin_minus,
	KEY_OffsWin_plus,
	KEY_OffsWin_minus,
	KEY_PosInWin_plus,
	KEY_PosInWin_minus,
	KEY_SpaceFonts_plus,
	KEY_SpaceFonts_minus,
	KEY_DispSpaces,
	KEY_WriteSpaces,
	KEY_ResetSpaces,
	KEY_InfoSpacesUp,
	KEY_InfoSpacesDown,
	KEY_InfoSpacesStyle,
	KEY_InfoSpacesRoll,
	KEY_InfoSpacesSel,
	KEYBOARD_SETTXT_KEYS
}SELECT_PRESS_BLOCK;

typedef enum{
	PARAM_TYPE,
	PARAM_SIZE,
	PARAM_COLOR_BK,
	PARAM_COLOR_FONT,
	PARAM_LEN_WINDOW,
	PARAM_OFFS_WINDOW,
	PARAM_STYLE,
	PARAM_COEFF,
	PARAM_SPEED,
	PARAM_LOAD_FONT_TIME,
	PARAM_POS_CURSOR,
	PARAM_CPU_USAGE,
	PARAM_MOV_TXT,
	FONTS
}REFRESH_DATA;

typedef enum{
	TIMER_Cpu,
	TIMER_InfoWrite,
	TIMER_Release,
	TIMER_BlockTouch,
	TIMER_Scroll,
}TIMER_FOR_THIS_SCREEN;

static int temp;
static char bufTemp[50], ShowTxtBuff[SIZE_TXT_SHOW]=TEXT_TO_SHOW;
static int lenTxt_prev;
static StructTxtPxlLen lenStr;
static KEYBOARD_TYPES actualKeyboardType = KEYBOARD_none;

typedef struct{
	int32_t bk[3];
	int32_t font[3];
	uint16_t xFontsField;
	uint16_t yFontsField;
	int8_t step;
	int16_t coeff;
	int16_t coeff_prev[2];
	int8_t size;
	uint8_t style;
	uint32_t time;
	int8_t type;
	char txt[SIZE_TXT_SHOW];
	int16_t lenWin;
	int16_t offsWin;
	int16_t lenWin_prev;
	int16_t offsWin_prev;
	uint8_t normBoldItal;
	uint32_t speed;
	uint32_t loadFontTime;
	uint8_t posCursor;
	uint8_t spaceCoursorY;
	uint8_t heightCursor;
	uint8_t spaceBetweenFonts;
	uint8_t constWidth;
} RGB_BK_FONT;
static RGB_BK_FONT Test;

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

static int ChangeTxt(void){
/*	return CopyCharsTab(Test.txt,Test.lenWin,Test.offsWin,Test.size); */

	char *pChar;
	int i, j, lenChars;

	pChar= ShowTxtBuff;

	lenChars=mini_strlen(pChar);
	for(i=0;i < Test.lenWin;++i)
	{
		j=Test.offsWin + i;
		if(j < lenChars)
			Test.txt[i]=pChar[j];
		else
			break;
	}
	Test.txt[i]=0;

	if(i == Test.lenWin)
		return 0;
	else
		return 1;
}

static void ResetIndexKeyBuff		  (void){ KEYBOARD_SETTXT_ServiceTxtBuffer(keyBuff,KEYBUFF_SIZE); memset(keyBuff,0,KEYBUFF_SIZE); }
static void CopyKeyBuff2ShowTxtBuff(void){ STRING_CopyBuff( ShowTxtBuff,keyBuff, SIZE_TXT_SHOW,strlen(keyBuff) ); }
static void ServiceKeyCharBuff	  (void){ CopyKeyBuff2ShowTxtBuff(); ResetIndexKeyBuff(); ChangeTxt(); }  //tu kopiuj zawartosc textu z Fonts do klawiatury !!!!
/*
static char* TXT_PosCursor(void){
	return Test.posCursor>0 ? Int2Str(Test.posCursor-1,' ',3,Sign_none) : StrAll(1,"off");
}
*/
static void ClearCursorField(void){
	LCD_ShapeIndirect(LCD_GetStrVar_x(v.FONT_VAR_Fonts),LCD_GetStrVar_y(v.FONT_VAR_Fonts)+LCD_GetFontHeight(v.FONT_ID_Fonts)+Test.spaceCoursorY,LCD_Rectangle, lenStr.inPixel,Test.heightCursor, v.COLOR_BkScreen,v.COLOR_BkScreen,v.COLOR_BkScreen);
}
static void TxtTouch(TOUCH_SET_UPDATE type){
	switch((int)type){
		case TouchSetNew:
			LCD_TOUCH_DeleteSelectTouch(Touch_SetTxt);
			LCDTOUCH_Set(POS_X_TXT, POS_Y_TXT, lenStr.inPixel, lenStr.height, ID_TOUCH_POINT,Touch_SetTxt,press);
			break;
		case TouchUpdate:
			LCDTOUCH_Update(POS_X_TXT, POS_Y_TXT, lenStr.inPixel, lenStr.height, ID_TOUCH_POINT,Touch_SetTxt,press);
			break;
	}
}

static void SetCursor(void)  //KURSOR DLA BIG FONT DAC PODWOJNY !!!!!
{
	ClearCursorField();
	if(Test.posCursor)
	{
		uint32_t color;
		switch(Test.type)
		{
			case RGB_RGB:  	color=RGB_FONT; break;
			case Gray_Green:  color=MYGREEN;  break;
			case RGB_White:  	color=WHITE; 	 break;
			case White_Black:
			default: 			color=BLACK; 	 break;
		}
		if(Test.posCursor>Test.lenWin)
			Test.posCursor=Test.lenWin;
		LCD_ShapeIndirect(LCD_GetStrVar_x(v.FONT_VAR_Fonts)+LCD_GetStrPxlWidth(v.FONT_ID_Fonts,Test.txt,Test.posCursor-1,Test.spaceBetweenFonts,Test.constWidth),LCD_GetStrVar_y(v.FONT_VAR_Fonts)+LCD_GetFontHeight(v.FONT_ID_Fonts)+Test.spaceCoursorY,LCD_Rectangle, LCD_GetFontWidth(v.FONT_ID_Fonts,Test.txt[Test.posCursor-1]),Test.heightCursor, color,color,color);
	}
}

static void Data2Refresh(int nr)
{
	switch(nr)
	{
	case PARAM_TYPE:
		lenStr=LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_FontType,TXT_FONT_TYPE);
#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
		SCREEN_SetTouchForNewEndPos(v.FONT_VAR_FontType,0, lenStr);
#endif
		break;
	case PARAM_SIZE:
		lenStr=LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_FontSize,TXT_FONT_SIZE);
#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
		SCREEN_SetTouchForNewEndPos(v.FONT_VAR_FontSize,0, lenStr);
#endif
		break;
	case FONTS:
		switch(Test.type)
		{
		case RGB_RGB:
			LCD_SetStrVar_fontID(v.FONT_VAR_Fonts,v.FONT_ID_Fonts);
			LCD_SetStrVar_fontColor(v.FONT_VAR_Fonts,RGB_FONT);
			LCD_SetStrVar_bkColor(v.FONT_VAR_Fonts,RGB_BK);
			LCD_SetStrVar_coeff(v.FONT_VAR_Fonts,Test.coeff);
			StartMeasureTime_us();
			 lenStr=LCD_StrChangeColorVarIndirect(v.FONT_VAR_Fonts,Test.txt);
			Test.speed=StopMeasureTime_us("");
		   TxtTouch(TouchUpdate);
			break;
		case Gray_Green:
			LCD_SetStrVar_fontID(v.FONT_VAR_Fonts,v.FONT_ID_Fonts);
			LCD_SetStrVar_bkColor(v.FONT_VAR_Fonts,MYGRAY);
			LCD_SetStrVar_coeff(v.FONT_VAR_Fonts,Test.coeff);
			StartMeasureTime_us();
			lenStr=LCD_StrVarIndirect(v.FONT_VAR_Fonts,Test.txt);
			Test.speed=StopMeasureTime_us("");
		   TxtTouch(TouchUpdate);
			break;
		case RGB_White:
			LCD_SetStrVar_fontID(v.FONT_VAR_Fonts,v.FONT_ID_Fonts);
			LCD_SetStrVar_bkColor(v.FONT_VAR_Fonts,RGB_BK);
			LCD_SetStrVar_coeff(v.FONT_VAR_Fonts,Test.coeff);
			StartMeasureTime_us();
			lenStr=LCD_StrVarIndirect(v.FONT_VAR_Fonts,Test.txt);
		   Test.speed=StopMeasureTime_us("");
		   TxtTouch(TouchUpdate);
		   break;
		case White_Black:
			LCD_SetStrVar_fontID(v.FONT_VAR_Fonts,v.FONT_ID_Fonts);
			LCD_SetStrVar_bkColor(v.FONT_VAR_Fonts,WHITE);
			LCD_SetStrVar_coeff(v.FONT_VAR_Fonts,Test.coeff);
			StartMeasureTime_us();
			lenStr=LCD_StrVarIndirect(v.FONT_VAR_Fonts,Test.txt);
		   Test.speed=StopMeasureTime_us("");
		   TxtTouch(TouchUpdate);
			break;
		}
		break;
	case PARAM_COLOR_BK:
		LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_BkColor,TXT_BK_COLOR);
		break;
	case PARAM_COLOR_FONT:
		LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_FontColor,TXT_FONT_COLOR);
		break;
	case PARAM_OFFS_WINDOW:
	case PARAM_LEN_WINDOW:
		LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_LenWin, TXT_LENOFFS_WIN);
		break;
	case PARAM_STYLE:
		lenStr=LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_FontStyle, TXT_FONT_STYLE);
#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
		SCREEN_SetTouchForNewEndPos(v.FONT_VAR_FontStyle,0, lenStr);
#endif
		break;
	case PARAM_COEFF:
		LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_Coeff,TXT_COEFF);
		break;
	case PARAM_SPEED:
	case PARAM_LOAD_FONT_TIME:
		LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_LoadFontTime, TXT_TIMESPEED);
		break;
	case PARAM_CPU_USAGE:
		LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_CPUusage,TXT_CPU_USAGE);
		break;
/*	case PARAM_POS_CURSOR:
		LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_PosCursor,TXT_PosCursor());
		break; */
	}
}
/*
static void RefreshAllParam(void)
{
	Data2Refresh(FONTS);
	Data2Refresh(PARAM_COLOR_FONT);
	Data2Refresh(PARAM_COLOR_BK);
	Data2Refresh(PARAM_TYPE);
	Data2Refresh(PARAM_SIZE);
	Data2Refresh(PARAM_STYLE);
	Data2Refresh(PARAM_COEFF);
	Data2Refresh(PARAM_LEN_WINDOW);
	Data2Refresh(PARAM_OFFS_WINDOW);
	Data2Refresh(PARAM_LOAD_FONT_TIME);
	Data2Refresh(PARAM_SPEED);
	Data2Refresh(PARAM_POS_CURSOR);
}
*/
static void RefreshValRGB(void){
	Data2Refresh(FONTS);
	Data2Refresh(PARAM_COLOR_FONT);
	Data2Refresh(PARAM_COLOR_BK);
	Data2Refresh(PARAM_SPEED);
}
static void ChangeValRGB(char font_bk, char rgb, int32_t sign)
{
	int32_t *color;
	int idx;

	switch(font_bk)
	{
	case 'b': color=&Test.bk[0];	break;
	case 'f': color=&Test.font[0];	break;
	}

	switch (rgb)
	{
	case 'R': idx=0; break;
	case 'G': idx=1; break;
	case 'B': idx=2; break;
	}

	switch (sign)
	{
	case 1:
		if(color[idx] <= 255-Test.step)
			color[idx]+=Test.step;
		else
			color[idx]=255;
		break;
	case -1:
		if(color[idx] >= Test.step)
			color[idx]-=Test.step;
		else
			color[idx]=0;
		break;
	}
	RefreshValRGB();
}
/*
static void IncStepRGB(void){
	Test.step>=255 ? 255 : Test.step++;
}
static void DecStepRGB(void){
	Test.step<=0 ? 0 : Test.step--;
}
*/
static void IncCoeffRGB(void){
	switch(Test.type){
		case RGB_RGB:
			Test.coeff>=255 ? 255 : Test.coeff++;
			break;
		case RGB_White:
			Test.coeff>=127 ? 127 : Test.coeff++;
			break;
		case Gray_Green:
		case White_Black:
			Test.coeff=0;
			break;
	}
	Data2Refresh(FONTS);
	Data2Refresh(PARAM_COEFF);
	Data2Refresh(PARAM_SPEED);
}
static void DecCoeefRGB(void){
	switch(Test.type){
		case RGB_RGB:
			Test.coeff<=0 ? 0 : Test.coeff--;
			break;
		case RGB_White:
			Test.coeff<=-127 ? -127 : Test.coeff--;
			break;
		case Gray_Green:
		case White_Black:
			Test.coeff=0;
			break;
	}
	Data2Refresh(FONTS);
	Data2Refresh(PARAM_COEFF);
	Data2Refresh(PARAM_SPEED);
}

static void FONTS_LCD_ResetParam(void)
{
	Test.xFontsField=0;
	Test.yFontsField=240;

	Test.bk[0]=R_PART(v.FONT_BKCOLOR_Fonts);
	Test.bk[1]=G_PART(v.FONT_BKCOLOR_Fonts);
	Test.bk[2]=B_PART(v.FONT_BKCOLOR_Fonts);

	Test.font[0]=R_PART(v.FONT_COLOR_Fonts);
	Test.font[1]=G_PART(v.FONT_COLOR_Fonts);
	Test.font[2]=B_PART(v.FONT_COLOR_Fonts);

	Test.step=1;
	Test.coeff=255;
	Test.coeff_prev[0]=255;		/* for RGB-RGB */
	Test.coeff_prev[1]=0;		/* for RGB-White */

	Test.type=RGB_RGB;
	Test.speed=0;

	Test.size=v.FONT_SIZE_Fonts;
	Test.style=v.FONT_STYLE_Fonts;

	Test.lenWin=mini_strlen(ShowTxtBuff);
	Test.offsWin=0;

	Test.lenWin_prev=Test.lenWin;
	Test.offsWin_prev=Test.offsWin;

   Test.posCursor=0;
	Test.normBoldItal=0;

	Test.spaceCoursorY=0;
	Test.heightCursor=1;
	Test.spaceBetweenFonts=0;
	Test.constWidth=0;

	ChangeTxt();
}

static void LCD_LoadFontVar(void)
{
	if(TakeMutex(Semphr_cardSD,1000))
	{
		if(v.FONT_ID_Fonts == FILE_NAME(GetDefaultParam)(FONT_ID_Fonts))
			LCD_DeleteFont(FILE_NAME(GetDefaultParam)(FONT_ID_Fonts));

		StartMeasureTime(0);
		switch(Test.type)
		{
		case RGB_RGB:
		case Gray_Green:
			v.FONT_ID_Fonts = LCD_LoadFont_DarkgrayGreen(Test.size,Test.style,FILE_NAME(GetDefaultParam)(FONT_ID_Fonts));
			break;
		case RGB_White:
			v.FONT_ID_Fonts = LCD_LoadFont_DarkgrayWhite(Test.size,Test.style,FILE_NAME(GetDefaultParam)(FONT_ID_Fonts));
			break;
		case White_Black:
			v.FONT_ID_Fonts = LCD_LoadFont_WhiteBlack(Test.size,Test.style,FILE_NAME(GetDefaultParam)(FONT_ID_Fonts));
			break;
		}
		Test.loadFontTime=StopMeasureTime(0,"");

		if(v.FONT_ID_Fonts<0){
			DbgDma ( 1, "\r\nERROR_LoadFontVar " );
			v.FONT_ID_Fonts=0;
		}
		DisplayFontsStructState();

		GiveMutex(Semphr_cardSD);
	}
}

static void AdjustMiddle_X(void){
	LCD_SetStrVar_x(v.FONT_VAR_Fonts,POS_X_TXT);
}
static void AdjustMiddle_Y(void){
	LCD_SetStrVar_y(v.FONT_VAR_Fonts,POS_Y_TXT);
}

static void ChangeFontStyle(int8_t typeReq)
{
	if(typeReq > NONE_TYPE_REQ)
	{
		if(Test.style == typeReq)
			return;
		else
			Test.style = typeReq;
	}
	else
	{
		switch(Test.style)
		{
		case Arial:   			 Test.style=Times_New_Roman; break;
		case Times_New_Roman: Test.style=Comic_Saens_MS;  break;
		case Comic_Saens_MS:  Test.style=Arial; 			  break;
		default:              Test.style=Arial;           break;
		}
	}

	ClearCursorField();
	LCD_LoadFontVar();
	AdjustMiddle_X();
	AdjustMiddle_Y();
	Data2Refresh(FONTS);
	Test.lenWin=lenStr.inChar;
	SetCursor();
	Data2Refresh(PARAM_LOAD_FONT_TIME);
	Data2Refresh(PARAM_LEN_WINDOW);
	Data2Refresh(PARAM_STYLE);
	Data2Refresh(PARAM_SPEED);
}

static void Inc_lenWin(void){
	Test.lenWin++;
	if(ChangeTxt()){
		Test.lenWin--;
		ChangeTxt();
	}
	else{
		lenTxt_prev=lenStr.inChar;
		AdjustMiddle_X();
		Data2Refresh(FONTS);
		if(lenTxt_prev==lenStr.inChar)
			Test.lenWin--;
		else
			Data2Refresh(PARAM_LEN_WINDOW);
	}
	ClearCursorField();
	SetCursor();
	Data2Refresh(PARAM_SPEED);
}
static void Dec_lenWin(void){
	Test.lenWin<=1 ? 1 : Test.lenWin--;
	ChangeTxt();
	ClearCursorField();
	AdjustMiddle_X();
	Data2Refresh(FONTS);
	Test.lenWin=lenStr.inChar;
	SetCursor();
	Data2Refresh(PARAM_LEN_WINDOW);
	Data2Refresh(PARAM_SPEED);
}

static void Inc_offsWin(void){
	Test.offsWin++;
	if(ChangeTxt()){
		Test.offsWin--;
		ChangeTxt();
	}
	ClearCursorField();
	AdjustMiddle_X();
	Data2Refresh(FONTS);
	Test.lenWin=lenStr.inChar;
	SetCursor();
	Data2Refresh(PARAM_LEN_WINDOW);
	Data2Refresh(PARAM_OFFS_WINDOW);
	Data2Refresh(PARAM_SPEED);
}
static void Dec_offsWin(void){
	Test.offsWin<=0 ? 0 : Test.offsWin--;
	ChangeTxt();
	ClearCursorField();
	AdjustMiddle_X();
	Data2Refresh(FONTS);
	Test.lenWin=lenStr.inChar;
	SetCursor();
	Data2Refresh(PARAM_LEN_WINDOW);
	Data2Refresh(PARAM_OFFS_WINDOW);
	Data2Refresh(PARAM_SPEED);
}

static void IncFontSize(int8_t typeReq)
{
	int sizeLimit;

	if(typeReq > NONE_TYPE_REQ){
		Test.size = typeReq;
		if(0==(typeReq%3)) Test.normBoldItal = 0;
		else if(0==((typeReq-1)%3)) Test.normBoldItal = 1;
		else if(0==((typeReq-2)%3)) Test.normBoldItal = 2;
	}
	else
		Test.size+=3;

	switch(Test.normBoldItal)
	{
	default:
	case 0:  sizeLimit=FONT_130; 			break;
	case 1:  sizeLimit=FONT_130_bold; 	break;
	case 2:  sizeLimit=FONT_130_italics; break;
	}
	if(Test.size>sizeLimit){
		Test.size=sizeLimit;
		return;
	}
	ClearCursorField();
	LCD_LoadFontVar();
	if((Test.size==FONT_72)||(Test.size==FONT_72_bold)||(Test.size==FONT_72_italics)||
	   (Test.size==FONT_130)||(Test.size==FONT_130_bold)||(Test.size==FONT_130_italics)){
		Test.lenWin_prev=Test.lenWin;
		Test.offsWin_prev=Test.offsWin;
		Test.lenWin=8;
		Test.offsWin=0;
		ChangeTxt();
	}
	AdjustMiddle_X();
	AdjustMiddle_Y();
	Data2Refresh(FONTS);
	Test.lenWin=lenStr.inChar;
	SetCursor();
	Data2Refresh(PARAM_LOAD_FONT_TIME);
	Data2Refresh(PARAM_LEN_WINDOW);
	Data2Refresh(PARAM_TYPE);
	Data2Refresh(PARAM_SIZE);
	Data2Refresh(PARAM_SPEED);
}

static void DecFontSize(void)
{
	int sizeLimit;
	if((Test.size==FONT_72)||(Test.size==FONT_72_bold)||(Test.size==FONT_72_italics)||
	   (Test.size==FONT_130)||(Test.size==FONT_130_bold)||(Test.size==FONT_130_italics)){
		Test.lenWin=Test.lenWin_prev;
		Test.offsWin=Test.offsWin_prev;
	}
	Test.size-=3;
	switch(Test.normBoldItal)
	{
	default:
	case 0:  sizeLimit=FONT_8; 		  break;
	case 1:  sizeLimit=FONT_8_bold; 	  break;
	case 2:  sizeLimit=FONT_8_italics; break;
	}
	if(Test.size<sizeLimit) Test.size=sizeLimit;

	ClearCursorField();
	LCD_LoadFontVar();
	ChangeTxt();
	AdjustMiddle_X();
	AdjustMiddle_Y();
	Data2Refresh(FONTS);
	Test.lenWin=lenStr.inChar;
	SetCursor();
	Data2Refresh(PARAM_LOAD_FONT_TIME);
	Data2Refresh(PARAM_TYPE);
	Data2Refresh(PARAM_SIZE);
	Data2Refresh(PARAM_LEN_WINDOW);
	Data2Refresh(PARAM_SPEED);
}

static void ChangeFontBoldItalNorm(int8_t typeReq)
{
	if(typeReq > NONE_TYPE_REQ)
	{
		if(Test.normBoldItal == typeReq)
			return;

		if(typeReq > Test.normBoldItal)
			Test.size += (typeReq-Test.normBoldItal);
		else if(typeReq < Test.normBoldItal)
			Test.size -= (Test.normBoldItal-typeReq);

		Test.normBoldItal = typeReq;
	}
	else
	{
		if(Test.normBoldItal>1){
			Test.normBoldItal=0;
			Test.size-=2;
		}
		else{
			Test.normBoldItal++;
			Test.size++;
		}
	}

	ClearCursorField();
	LCD_LoadFontVar();
	AdjustMiddle_X();
	AdjustMiddle_Y();
	Data2Refresh(FONTS);
	Test.lenWin=lenStr.inChar;
	SetCursor();
	Data2Refresh(PARAM_LOAD_FONT_TIME);
	Data2Refresh(PARAM_LEN_WINDOW);
	Data2Refresh(PARAM_TYPE);
	Data2Refresh(PARAM_SIZE);
	Data2Refresh(PARAM_SPEED);
}

static void ReplaceLcdStrType(int8_t typeReq)
{
	int8_t testType=Test.type;
	if(Test.type == typeReq)
		return;

	GOTO_ReplaceLcdStrType:
	INCR_WRAP(Test.type,1, RGB_RGB, White_Black);
	switch(Test.type)
	{
	case RGB_RGB:
		Test.coeff=Test.coeff_prev[0];
		break;
	case RGB_White:
		Test.coeff=Test.coeff_prev[1];
		break;
	case Gray_Green:
	case White_Black:
		if(testType==RGB_RGB){
			Test.coeff_prev[0]=Test.coeff;
			Test.coeff=0;
		}
		else if(testType==RGB_White){
			Test.coeff_prev[1]=Test.coeff;
			Test.coeff=0;
		}
		break;
	}

	if(typeReq > NONE_TYPE_REQ){
		if(typeReq!=Test.type){
			testType=Test.type;
			goto GOTO_ReplaceLcdStrType;
		}
	}

	ClearCursorField();
	LCD_LoadFontVar();
	AdjustMiddle_X();
	AdjustMiddle_Y();
	Data2Refresh(FONTS);
	Test.lenWin=lenStr.inChar;
	SetCursor();
	Data2Refresh(PARAM_LOAD_FONT_TIME);
	Data2Refresh(PARAM_LEN_WINDOW);
	Data2Refresh(PARAM_TYPE);
	Data2Refresh(PARAM_SIZE);
	Data2Refresh(PARAM_SPEED);
	Data2Refresh(PARAM_COEFF);
}

static void Inc_PosCursor(void){
	if(Test.posCursor<Test.lenWin){
		Test.posCursor++;
		Data2Refresh(PARAM_POS_CURSOR);
		SetCursor();
	}
}
static void Dec_PosCursor(void){
	if(Test.posCursor>0){
		Test.posCursor--;
		Data2Refresh(PARAM_POS_CURSOR);
		SetCursor();
	}
}

static void IncDec_SpaceBetweenFont(int incDec){
	if(((LCD_GetStrVar_x(v.FONT_VAR_Fonts)+lenStr.inPixel>=LCD_GetXSize()-1)&&(1==incDec))||
		((0==LCD_GetStrPxlWidth(v.FONT_ID_Fonts,Test.txt,Test.posCursor-1,Test.spaceBetweenFonts,Test.constWidth))&&(0==incDec)))
		return;
	if(Test.posCursor>1){
		if(0xFFFF!=LCD_SelectedSpaceBetweenFontsIncrDecr(incDec, Test.style, Test.size, Test.txt[Test.posCursor-2], Test.txt[Test.posCursor-1])){
			AdjustMiddle_X();
			ClearCursorField();
			Data2Refresh(FONTS);
			Test.lenWin=lenStr.inChar;
			SetCursor();
			Data2Refresh(FONTS);		/* RefreshAllParam(); */
		}
	}
}

/* ------------ FILE_NAME() functions ------------ */
static int RR=0;

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
		case KEYBOARD_fontRGB:
			KEYBOARD_KeyAllParamSet(3,2, "Rafa"ł"", ""ó"lka", "W"ł"ujek", "Misia", "Six", "Markielowski", RED,GREEN,BLUE,RED,GREEN,BLUE, DARKRED,DARKRED, LIGHTGREEN,LIGHTGREEN, DARKBLUE,DARKBLUE);
			KEYBOARD_Buttons(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY_Red_plus, SL(LANG_nazwa_8));
			break;

		case KEYBOARD_bkRGB:
			KEYBOARD_KeyAllParamSet(3,2, "R+","G+","B+","R-","G-","B-", RED,GREEN,BLUE,RED,GREEN,BLUE, WHITE,WHITE,WHITE,WHITE,WHITE,WHITE);
			KEYBOARD_Buttons(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY_Red_plus, SL(LANG_nazwa_8));
			break;

		case KEYBOARD_sliderRGB:
			KEYBOARD_KeyAllParamSet(1,3, "Red","Green","Blue", COLOR_GRAY(0xA0),COLOR_GRAY(0xA0),COLOR_GRAY(0xA0), RED,GREEN,BLUE);
			KEYBOARD_ServiceSliderRGB(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY2_fontSliderR_left, SL(LANG_nazwa_1), (int*)&Test.font[0], RefreshValRGB);
			break;

		case KEYBOARD_sliderBkRGB:
			static uint32_t param= COLOR_GRAY(0xA0);
			if(EQUAL2_OR(forTouchIdx,Touch_FontColorMoveRight,Touch_BkColorMove)) param= COLOR_GRAY(0x60);
			else if(forTouchIdx > 0)												  		 	 param= COLOR_GRAY(0x80);
			KEYBOARD_KeyAllParamSet(3,1, "Red","Green","Blue", param,param,param, RED,GREEN,BLUE);
			KEYBOARD_ServiceSliderRGB(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY2_bkSliderR_left, SL(LANG_nazwa_6), (int*)&Test.bk[0], RefreshValRGB);
			break;

		case KEYBOARD_circleSliderRGB:
			/* CIRCLE_errorDecision(0,_OFF); */
			KEYBOARD_KeyAllParamSet(3,1, "Red","Green","Blue", COLOR_GRAY(0xA0),COLOR_GRAY(0xA0),COLOR_GRAY(0xA0), RED,DARKGREEN,BLUE);
			KEYBOARD_ServiceCircleSliderRGB(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY_fontCircleSliderR, KEY_Timer2, SL(LANG_nazwa_1), (int*)&Test.font[0], RefreshValRGB, (TIMER_ID)TIMER_Release);
			/* CIRCLE_errorDecision(0,_ON); */
			break;

		case KEYBOARD_fontSize2:
			KEYBOARD_KeyAllParamSet3(1,LCD_GetFontSizeMaxNmb(), COLOR_GRAY(0xDD), DARKRED, (char**)LCD_GetFontSizePtr());
			KEYBOARD_ServiceSizeRoll(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_Select_one, ROLL_1, SL(LANG_CoeffKeyName), v.FONT_COLOR_Descr, 8, Test.size);
			break;

		case KEYBOARD_fontCoeff:
			KEYBOARD_KeyAllParamSet(2,1, "+", "-", WHITE,WHITE, LIGHTCYAN,LIGHTCYAN);
			KEYBOARD_SetGeneral(Ni,Ni,Ni, Ni,Ni, Ni,BrightIncr(v.COLOR_FillFrame,0xE), Ni,Ni,Ni);
			KEYBOARD_Buttons(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY_Coeff_plus, SL(LANG_CoeffKeyName));
			break;

		case KEYBOARD_fontStyle:
			KEYBOARD_KeyAllParamSet(3,1, "Arial", "Times_New_Roman", "Comic_Saens_MS", WHITE,WHITE,WHITE, DARKRED,DARKRED,DARKBLUE); // to tez jest w fonts_images ujednolicic !!!!
			KEYBOARD_Select(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_Select_one, NULL, Test.style);
			break;

		case KEYBOARD_fontType:
			KEYBOARD_KeyAllParamSet(1,4, LCD_GetFontTypeStr(0), LCD_GetFontTypeStr(1), LCD_GetFontTypeStr(2), LCD_GetFontTypeStr(3), WHITE,WHITE,WHITE,WHITE, BLACK,BROWN,ORANGE,MYBLUE);  //nazwe te dac w jednym miescu !!!! bo sa i w font_images.c !!!
			KEYBOARD_Select(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_Select_one, NULL, Test.type);
			break;

		case KEYBOARD_fontSize:
			KEYBOARD_ServiceSizeStyle(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_Select_one, KEY_Size_plus, SL(LANG_nazwa_0), Test.normBoldItal);
			break;

		case KEYBOARD_LenOffsWin:
			if(KEYBOARD_ServiceLenOffsWin(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY_LenWin_plus,Touch_SpacesInfoUp, KEY_Timer, SL(LANG_WinInfo), SL(LANG_WinInfo2),SL(LANG_LenOffsWin1),SL(LANG_LenOffsWin2), v.FONT_COLOR_Descr,FILE_NAME(main), LoadNoDispScreen, (char**)ppMain, (TIMER_ID)TIMER_InfoWrite)){
				SELECT_CURRENT_FONT(LenWin,Press, TXT_LENOFFS_WIN,255);
			}
			break;

		case KEYBOARD_setTxt:
			KEYBOARD__ServiceSetTxt(type-1, selBlockPress, ARG_KEYBOARD_PARAM, KEY_All_release, KEY_Q, KEY_big, KEY_back, KEY_alt, KEY_enter,KEY_field,KEY_style,KEY_exit, v.FONT_COLOR_Descr, keyBuff,KEYBUFF_SIZE);
			break;

		default:
			break;
	}
	return 0;
}

//dla ROLL bez select a drugie pole touch to z select,

static void CycleRefreshFunc(void){
	if(vTimerService(TIMER_Cpu, check_restart_time,1000))
		Data2Refresh(PARAM_CPU_USAGE);
}

static void BlockingFunc(void){		/* Call this function in long during while(1) */					/* This thread in this function-BlockingFunc() can be expropriate by another thread with the some or greater priority (like Ethernet Thread having the some priority as LCD thread) */
	CycleRefreshFunc();
}

static void FILE_NAME(timer)(void)  /* alternative RTOS Timer Callback or create new thread vTaskTimer */
{
	if(vTimerService(TIMER_InfoWrite, check_stop_time, 2000)){
		KEYBOARD_TYPE(actualKeyboardType, KEY_Timer);
	}
	if(vTimerService(TIMER_Release, check_stop_time, 100)){
		KEYBOARD_TYPE(actualKeyboardType, KEY_Timer2);
	}
	if(vTimerService(TIMER_BlockTouch, check_stop_time, 500)){
		BlockTouchForTime(_OFF,TIMER_BlockTouch);
	}
	CycleRefreshFunc();
}

void FUNC_fontColorRGB(int k){ switch(k){
  case -1: Test.step=1; return;
	case 0: ChangeValRGB('f','R', 1); break;
	case 1: ChangeValRGB('f','G', 1); break;
	case 2: ChangeValRGB('f','B', 1); break;
	case 3: ChangeValRGB('f','R',-1); break;
	case 4: ChangeValRGB('f','G',-1); break;
	case 5: ChangeValRGB('f','B',-1); break;}
	Test.step=5;
}
void FUNC_bkFontColorRGB(int k){ switch(k){
  case -1: Test.step=1; return;
	case 0: ChangeValRGB('b','R', 1); break;
	case 1: ChangeValRGB('b','G', 1); break;
	case 2: ChangeValRGB('b','B', 1); break;
	case 3: ChangeValRGB('b','R',-1); break;
	case 4: ChangeValRGB('b','G',-1); break;
	case 5: ChangeValRGB('b','B',-1); break;}
	Test.step=5;
}
void FUNC_SliderFontRGB(int k){ switch(k){
  case -1: Test.step=1;   return;
  	case 1:case 4:case 7:  return;
	case 0:case 9:  ChangeValRGB('f','R',-1); break;
	case 2:case 10: ChangeValRGB('f','R', 1); break;
	case 3:case 11: ChangeValRGB('f','G',-1); break;
	case 5:case 12: ChangeValRGB('f','G', 1); break;
	case 6:case 13: ChangeValRGB('f','B',-1); break;
	case 8:case 14: ChangeValRGB('f','B', 1); break;}
	Test.step=5;
}
void FUNC_SliderBkFontRGB(int k){ switch(k){
  case -1: Test.step=1;   return;
  	case 1:case 4:case 7:  return;
	case 0:case 9:  ChangeValRGB('b','R',-1); break;
	case 2:case 10: ChangeValRGB('b','R', 1); break;
	case 3:case 11: ChangeValRGB('b','G',-1); break;
	case 5:case 12: ChangeValRGB('b','G', 1); break;
	case 6:case 13: ChangeValRGB('b','B',-1); break;
	case 8:case 14: ChangeValRGB('b','B', 1); break;}
	Test.step=5;
}
void FUNC_FontStyle(int k){ switch(k){
  case -1: return;
	case 0: ChangeFontStyle(Arial); 				break;
	case 1: ChangeFontStyle(Times_New_Roman); break;
	case 2: ChangeFontStyle(Comic_Saens_MS); 	break;}
}
void FUNC_FontType(int k){ switch(k){
  case -1: return;
	case 0: ReplaceLcdStrType(0); break;
	case 1: ReplaceLcdStrType(1); break;
	case 2: ReplaceLcdStrType(2); break;
	case 3: ReplaceLcdStrType(3); break;}
}
void FUNC_FontSize(int k){ switch(k){
  case -1: return;
	case 0: IncFontSize(NONE_TYPE_REQ); break;
	case 1: DecFontSize(); break;}
}
void FUNC_FontBoldItalNorm(int k){ switch(k){
  case -1: return;
	case 0: ChangeFontBoldItalNorm(0); break;
	case 1: ChangeFontBoldItalNorm(1); break;
	case 2: ChangeFontBoldItalNorm(2); break;}
}
void FUNC_FontCoeff(int k){ switch(k){
  case -1: return;
	case 0: IncCoeffRGB(); break;
	case 1: DecCoeefRGB(); break;}
}

void FUNC_SliderBkFontFontRGB(int k){ switch(k){
  case -1: Test.step=1;   return;
  	case 1:case 4:case 7:case 10:case 13:case 16:  return;
	case 0: ChangeValRGB('b','R',-1); break;
	case 2: ChangeValRGB('b','R', 1); break;
	case 3: ChangeValRGB('b','G',-1); break;
	case 5: ChangeValRGB('b','G', 1); break;
	case 6: ChangeValRGB('b','B',-1); break;
	case 8: ChangeValRGB('b','B', 1); break;
	case 9:  ChangeValRGB('f','R',-1); break;
	case 11: ChangeValRGB('f','R', 1); break;
	case 12: ChangeValRGB('f','G',-1); break;
	case 14: ChangeValRGB('f','G', 1); break;
	case 15: ChangeValRGB('f','B',-1); break;
	case 17: ChangeValRGB('f','B', 1); break;}
	Test.step=5;
}
void FUNC_FontLenOffs(int k){ switch(k){
  case -1: return;
	case 0: Inc_lenWin(); break;
	case 1: Dec_lenWin(); break;
	case 2: Inc_offsWin(); break;
	case 3: Dec_offsWin(); break;
	case 4: Dec_PosCursor(); break;
	case 5: Inc_PosCursor(); break;
	case 6: IncDec_SpaceBetweenFont(1); break;
	case 7: IncDec_SpaceBetweenFont(0); break;
	case 8: break;
	case 9: LCD_WriteSpacesBetweenFontsOnSDcard(); break;
	case 10:
		LCD_ResetSpacesBetweenFonts();
		AdjustMiddle_X();
		ClearCursorField();
		Test.posCursor=0;
		Data2Refresh(FONTS);
		break;
}}


void FILE_NAME(setTouch)(void)
{/*
	#define DESELECT_CURRENT_FONT(src,txt) \
		LCD_SetStrVar_fontID		(v.FONT_VAR_##src, v.FONT_ID_##src);\
		LCD_SetStrVar_fontColor	(v.FONT_VAR_##src, v.FONT_COLOR_##src);\
		LCD_SetStrVar_bkColor	(v.FONT_VAR_##src, v.FONT_BKCOLOR_##src);\
		LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_##src, txt)

	#define DESELECT_ALL_FONTS \
		DESELECT_CURRENT_FONT(FontColor,	TXT_FONT_COLOR);\
		DESELECT_CURRENT_FONT(BkColor,	TXT_BK_COLOR);\
		DESELECT_CURRENT_FONT(FontType,	TXT_FONT_TYPE);\
		DESELECT_CURRENT_FONT(FontSize,	TXT_FONT_SIZE);\
		DESELECT_CURRENT_FONT(FontStyle,	TXT_FONT_STYLE)
*/


	TOUCH_FUNCTIONS_

	TouchScreenInit();

	void CreateKeyboard(KEYBOARD_TYPES keboard){
		switch((int)keboard){
			case KEYBOARD_fontRGB:	break;
			case KEYBOARD_fontSize2:	FILE_NAME(keyboard)(KEYBOARD_fontSize2, KEY_Select_one, LCD_Rectangle,0, 615,100, KeysAutoSize,10, 0, screenTouchState, Touch_FontSizeRoll,KeysDel);  break;
	}}


	screenTouchState = LCD_TOUCH_GetTypeAndPosition(&screenTouchPos);
													/*if prevTouch is this... and actualTouch is not this...*/				/*and yet actualTouch not this...*/							/*then unblock touches this...*/
	_RestoreSusspendedTouchsByAnotherClickItem(Touch_FontSize2,	  Touch_size_plus,	Touch_size_italic,	Touch_FontStyle,Touch_FontType,Touch_FontSize,_ZEROS7,	Touch_FontLenOffsWin,Touch_FontCoeff,										  _ZEROS8);		/* depended on _SaveState2() */
	_RestoreSusspendedTouchsByAnotherClickItem(Touch_FontSizeMove,Touch_FontSizeRoll,Touch_FontSizeRoll,	Touch_FontSize,										 _ZEROS9,	Touch_FontLenOffsWin,Touch_FontCoeff,Touch_FontStyle,Touch_FontType,_ZEROS6);		/* depended on _SaveState2() */

	/*	----- Service press specific Keys for Keyboard ----- */
	_TouchService(Touch_fontRp, Touch_fontBm, KEYBOARD_fontRGB, KEY_All_release, KEY_Red_plus, FUNC_fontColorRGB);
	_TouchService(Touch_bkRp, Touch_bkBm, 	   KEYBOARD_bkRGB,   KEY_All_release, KEY_Red_plus, FUNC_bkFontColorRGB);

	_TouchService(Touch2_bkSliderR_left, Touch2_bkSliderB_right, 		KEYBOARD_sliderBkRGB, 	KEY_All_release, KEY2_bkSliderR_left, 	 FUNC_SliderBkFontRGB);
	_TouchService(Touch2_fontSliderR_left, Touch2_fontSliderB_right,	KEYBOARD_sliderRGB, 		KEY_All_release, KEY2_fontSliderR_left, FUNC_SliderFontRGB);

	_TouchService(Touch_style1, Touch_style3,	 KEYBOARD_fontStyle, 0, KEY_Select_one, FUNC_FontStyle);
	_TouchService(Touch_type1, Touch_type4,	 KEYBOARD_fontType,  0, KEY_Select_one, FUNC_FontType);

	_TouchService(Touch_size_plus, Touch_size_minus,	 KEYBOARD_fontSize,  KEY_Select_one,  KEY_Size_plus,	 FUNC_FontSize);
	_TouchService(Touch_size_norm, Touch_size_italic,	 KEYBOARD_fontSize,  0, 				  KEY_Select_one,  FUNC_FontBoldItalNorm);
	_TouchService(Touch_coeff_plus, Touch_coeff_minus,	 KEYBOARD_fontCoeff, KEY_All_release, KEY_Coeff_plus,  FUNC_FontCoeff);

	_TouchService(Touch_fontCircleSliderR, Touch_CircleSlider3D,	KEYBOARD_circleSliderRGB, 		KEY_All_release, KEY_fontCircleSliderR, NULL);

	_TouchService(Touch_LenWin_plus, Touch_ResetSpaces, 	 	KEYBOARD_LenOffsWin, KEY_All_release, KEY_LenWin_plus,  FUNC_FontLenOffs);
	_TouchService(Touch_SpacesInfoUp, Touch_SpacesInfoTest,	KEYBOARD_LenOffsWin, KEY_NO_RELEASE,  KEY_InfoSpacesUp, NULL);


	switch(screenTouchState)
	{
		/*	----- Initiation new Keyboard ----- */
		CASE_TOUCH_STATE(screenTouchState,Touch_FontColor, FontColor,Press, TXT_FONT_COLOR,252,CheckTouchForTime(Touch_FontColorMoveRight,TIMER_BlockTouch),CheckTouchForTime(Touch_FontColorMoveLeft,TIMER_BlockTouch));		/* 'FontColor','Press' are suffix`s for elements of 'SCREEN_FONTS_SET_PARAMETERS' MACRO  */
			if(IsSetTouchFlag())
				FILE_NAME(keyboard)(KEYBOARD_fontRGB, KEY_All_release, LCD_RoundRectangle,0, 230,160, KeysAutoSize,12, 10, screenTouchState, Touch_fontRp,KeysDel);
			/* DisplayTouchPosXY(screenTouchState,screenTouchPos,"Touch_FontColor"); */
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_FontColor2, FontColor,Press, TXT_FONT_COLOR,252,NoTouch,NoTouch);
			if(IsSetTouchFlag())
				FILE_NAME(keyboard)(KEYBOARD_sliderRGB, KEY_All_release, LCD_RoundRectangle,0, 10,160, 180,39, 16, screenTouchState, Touch2_fontSliderR_left,KeysDel);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_BkColor, BkColor,Press, TXT_BK_COLOR,252,CheckTouchForTime(Touch_BkColorMove,TIMER_BlockTouch),NoTouch);
			if(IsSetTouchFlag())
				FILE_NAME(keyboard)(KEYBOARD_bkRGB, KEY_All_release, LCD_RoundRectangle,0, 400,160, KeysAutoSize,12, 4, screenTouchState, Touch_bkRp,KeysDel);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_BkColor2, BkColor,Press, TXT_BK_COLOR,252,NoTouch,NoTouch);
			if(IsSetTouchFlag())
				FILE_NAME(keyboard)(KEYBOARD_sliderBkRGB, KEY_All_release, LCD_RoundRectangle,0, 10,160, 35,170, 16, screenTouchState, Touch2_bkSliderR_left,KeysDel);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_FontColorMoveRight, FontColor,Press, TXT_FONT_COLOR,252,NoTouch,NoTouch);
			if(IsSetTouchFlag()){
				FILE_NAME(keyboard)(KEYBOARD_sliderRGB, 	KEY_All_release, LCD_RoundRectangle,0,  50,160, 180,30, 16, screenTouchState, Touch2_fontSliderR_left, KeysDel);
				//structSize temp = KEYBOARD_GetSize();
				FILE_NAME(keyboard)(KEYBOARD_sliderBkRGB, KEY_All_release, LCD_RoundRectangle,0, 550,160, 30,180, 16, screenTouchState, Touch2_bkSliderR_left,   KeysNotDel);
			}
			else _SaveState();  //dac np funkcje nazew i wsrodku to ' _SaveState();'
			BlockTouchForTime(_ON,TIMER_BlockTouch);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_FontColorMoveLeft, FontColor,Press, TXT_FONT_COLOR,252,NoTouch,NoTouch);  //pogrupowac po kolei od kolejnosci !!!!!
			if(IsSetTouchFlag()){	FILE_NAME(keyboard)(KEYBOARD_circleSliderRGB, 	KEY_All_release, LCD_RoundRectangle,0,  350,170, 100,100, 16, screenTouchState, Touch_fontCircleSliderR, KeysDel);  }
								//LCDTOUCH_ActiveOnly(screenTouchState,Touch_BkColor,Touch_FontColor,0,0,0,0,0,0,0,Touch_fontCircleSliderR,Touch_CircleSliderStyle); }
			else{  _SaveState(); /*LCD_TOUCH_RestoreAllSusspendedTouchs();*/ }
			BlockTouchForTime(_ON,TIMER_BlockTouch);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_BkColorMove, BkColor,Press, TXT_BK_COLOR,252,NoTouch,NoTouch);
			if(IsSetTouchFlag()){
				FILE_NAME(keyboard)(KEYBOARD_sliderRGB, 	KEY_All_release, LCD_RoundRectangle,0,  50,160, 180,39, 16, screenTouchState, Touch2_fontSliderR_left, KeysDel);
				FILE_NAME(keyboard)(KEYBOARD_sliderBkRGB, KEY_All_release, LCD_RoundRectangle,0, 550,160, 35,170, 16, screenTouchState, Touch2_bkSliderR_left,   KeysNotDel);
			}
			else _SaveState();  //dac np funkcje nazew i wsrodku to ' _SaveState();'
			BlockTouchForTime(_ON,TIMER_BlockTouch);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_FontLenOffsWin, LenWin,Press, TXT_LENOFFS_WIN,252,NoTouch,NoTouch);
			if(IsSetTouchFlag()){	FILE_NAME(keyboard)(KEYBOARD_LenOffsWin, KEY_All_release, LCD_RoundRectangle,0, 0,0, KeysAutoSize,8, 10, screenTouchState, Touch_LenWin_plus,KeysDel);
								LCDTOUCH_ActiveOnly(screenTouchState,0,0,0,0,0,0,0,0,0,Touch_LenWin_plus,Touch_ResetSpaces); }
			else{
				ClearCursorField();
				Test.posCursor=0;
				LCD_TOUCH_RestoreAllSusspendedTouchs();
			}
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_FontCoeff, Coeff,Press, TXT_COEFF,255,NoTouch,NoTouch);
			if(IsSetTouchFlag())
				FILE_NAME(keyboard)(KEYBOARD_fontCoeff, KEY_All_release, LCD_RoundRectangle,0, 400,205, KeysAutoSize,10, 10, screenTouchState, Touch_coeff_plus,KeysDel);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_FontStyle2, FontStyle,Press, TXT_FONT_STYLE,252,NoTouch,NoTouch);
			if(IsSetTouchFlag())
				FILE_NAME(keyboard)(KEYBOARD_fontStyle, KEY_Select_one, LCD_Rectangle,0, 200,160, KeysAutoSize,10, 0, screenTouchState, Touch_style1,KeysDel);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_FontType2, FontType,Press, TXT_FONT_TYPE,252,NoTouch,NoTouch);
			if(IsSetTouchFlag())
				FILE_NAME(keyboard)(KEYBOARD_fontType, KEY_Select_one, LCD_Rectangle,0, 400,160, KeysAutoSize,10, 0, screenTouchState, Touch_type1,KeysDel);
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_FontSize2, FontSize,Press, TXT_FONT_SIZE,252,NoTouch,NoTouch);
			if(IsSetTouchFlag()){	FILE_NAME(keyboard)(KEYBOARD_fontSize, KEY_Select_one, LCD_RoundRectangle,0, 614,200, KeysAutoSize,10/*80,40*/, 10, screenTouchState, Touch_size_plus,KeysDel);
								LCD_TOUCH_SusspendTouchs2(Touch_FontLenOffsWin,Touch_FontCoeff,_ZEROS8); _SaveState2(); }
			else{ LCD_TOUCH_RestoreSusspendedTouchs2(Touch_FontLenOffsWin,Touch_FontCoeff,_ZEROS8); _RstState2();  }
			break;

		CASE_TOUCH_STATE(screenTouchState,Touch_FontSizeMove, FontSize,Press, TXT_FONT_SIZE,252,NoTouch,NoTouch);
			if(IsSetTouchFlag()){ 	CreateKeyboard(KEYBOARD_fontSize2);
								LCD_TOUCH_SusspendTouchs2(Touch_FontLenOffsWin,Touch_FontCoeff,Touch_FontStyle,Touch_FontType,_ZEROS6); _SaveState2(); }
			else{ LCD_TOUCH_RestoreSusspendedTouchs2(Touch_FontLenOffsWin,Touch_FontCoeff,Touch_FontStyle,Touch_FontType,_ZEROS6); _RstState2();  _SaveState(); }
			BlockTouchForTime(_ON,TIMER_BlockTouch);
			break;


		/*	----- Touch parameter text and go to action ----- */
		case Touch_SetTxt:
			FILE_NAME(keyboard)(KEYBOARD_setTxt,KEY_All_release,LCD_RoundRectangle,0,15,15,KeysAutoSize,10,10,screenTouchState,Touch_Q,KeysDel);			/* Here call Keyboard only one at the start */
			LCDTOUCH_ActiveOnly(0,0,0,0,0,0,0,0,0,0,Touch_Q,Touch_keyStyle);
			break;

		case Touch_FontStyle:
			ChangeFontStyle(NONE_TYPE_REQ);
			if(CHECK_TOUCH(Touch_FontStyle2))
				KEYBOARD_TYPE( KEYBOARD_fontStyle, KEY_Select_one );
			break;

		case Touch_FontType:
			ReplaceLcdStrType(NONE_TYPE_REQ);
			if(CHECK_TOUCH(Touch_FontType2))
				KEYBOARD_TYPE( KEYBOARD_fontType, KEY_Select_one );
			break;

		case Touch_FontSize:
			if(NotServiceTouchAboveWhenWasClearedThis(CheckTouchForTime(Touch_FontSizeMove,TIMER_BlockTouch))){		/* When 'Touch_FontSizeMove' was cleared then not service for release 'Touch_FontSize' */
				ChangeFontBoldItalNorm(NONE_TYPE_REQ);
				if(CHECK_TOUCH(Touch_FontSize2)) 	KEYBOARD_TYPE(KEYBOARD_fontSize, KEY_Select_one);
				if(CHECK_TOUCH(Touch_FontSizeMove))	CreateKeyboard(KEYBOARD_fontSize2);
			}
			break;

		case Touch_FontSizeRoll:
			if(LCDTOUCH_IsScrollPress(ROLL_1, screenTouchState, &screenTouchPos, TIMER_Scroll))
				KEYBOARD_TYPE( KEYBOARD_fontSize2, KEY_Select_one);
			_SaveState();
			break;

		case Touch_MainFramesType:
			if(ppMain[0]==(int*)FRAMES_GROUP_separat)	*ppMain=(int*)FRAMES_GROUP_combined;
			else													*ppMain=(int*)FRAMES_GROUP_separat;
			FILE_NAME(main)(LoadPartScreen,(char**)ppMain);
			break;

		case Touch_NextScreen: SCREEN_Change ( 1 ); break;
		case Touch_PrevScreen: SCREEN_Change ( -1 ); break;


		default:		 /* ----- Service release specific Keys for Keyboard ----- */

			if(_KEYBOARD_setTxt__SERVICE(screenTouchState,Touch_Q,Touch_keyStyle,KEY_Q)) break;

			_TouchEndService(Touch_fontRp, Touch_fontBm, KEYBOARD_fontRGB, KEY_All_release, FUNC_fontColorRGB);
			_TouchEndService(Touch_bkRp, Touch_bkBm, 	   KEYBOARD_bkRGB,   KEY_All_release, FUNC_bkFontColorRGB);

			_TouchEndService(Touch2_bkSliderR_left, Touch2_bkSliderB_right,  		KEYBOARD_sliderBkRGB,   KEY_All_release, FUNC_SliderBkFontRGB);
			_TouchEndService(Touch2_fontSliderR_left, Touch2_fontSliderB_right, 	KEYBOARD_sliderRGB, 		KEY_All_release, FUNC_SliderFontRGB);

			_TouchEndService(Touch_size_plus, Touch_size_minus, 	KEYBOARD_fontSize, 	KEY_Select_one,  FUNC_FontSize);
			_TouchEndService(Touch_coeff_plus, Touch_coeff_minus, KEYBOARD_fontCoeff, 	KEY_All_release, FUNC_FontCoeff);

			/* _TouchEndService(Touch_fontCircleSliderR, Touch_CircleSliderStyle, 	KEYBOARD_circleSliderRGB, 		KEY_All_release, NULL); */		/* For circle slider is not needed */

			_TouchEndService(Touch_LenWin_plus, Touch_ResetSpaces, KEYBOARD_LenOffsWin, 	KEY_All_release, FUNC_FontLenOffs);


#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
			if(_WasState(Touch_FontStyle) ||
				_WasState(Touch_style1) ||
				_WasState(Touch_style2) ||
				_WasState(Touch_style3))
				SCREEN_SetTouchForNewEndPos(v.FONT_VAR_FontStyle,1,LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_FontStyle,TXT_FONT_STYLE));
#endif

#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
			if(_WasState(Touch_FontType) ||
				_WasState(Touch_type1) ||
				_WasState(Touch_type2) ||
				_WasState(Touch_type3) ||
				_WasState(Touch_type4))
				SCREEN_SetTouchForNewEndPos(v.FONT_VAR_FontType,1,LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_FontType,TXT_FONT_TYPE));
#endif

#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
			if(_WasState(Touch_FontSize) ||
				_WasState(Touch_size_norm) ||
				_WasState(Touch_size_bold) ||
				_WasState(Touch_size_italic))
				SCREEN_SetTouchForNewEndPos(v.FONT_VAR_FontSize,1,LCD_StrDependOnColorsVarIndirect(v.FONT_VAR_FontSize,TXT_FONT_SIZE));
#endif

			if(_WasState(Touch_FontSizeRoll)){
				if(END_FREEROLL__NOSEL != (temp = LCDTOUCH_IsScrollRelease(ROLL_1, FUNC1_SET( FILE_NAME(keyboard),KEYBOARD_fontSize2,KEY_Select_one,0,0,0,0,0,0,0,0,0,0), BlockingFunc, TIMER_Scroll)))
					IncFontSize(temp);
			}

			/* Not needed */
/*			if(_WasState(Touch_FontSizeMove));
			if(_WasState(Touch_FontColorMoveRight));
			if(_WasState(Touch_FontColorMoveLeft));
			if(_WasState(Touch_BkColorMove));
*/
			break;
	}

	FILE_NAME(timer)();
/*	LCDTOUCH_testFunc(); */

}
/* F7-only, unused TLS include omitted in the H7 LCD port. */
static void* MainFuncRefresh(void *p1,void *p2){
	FILE_NAME(main)(LoadUserScreen,(char**)ppMain);
	return NULL;
}

void FILE_NAME(debugRcvStr)(void){	 if(v.DEBUG_ON){
		DEBUG_InvalidateDCache (); // Same RX path as esp32wroom.c.

		if ( DEBUG_IsTxtReceive ( "abc" ) )
			FILE_NAME ( printInfo ) ();

		/* ----- Debug Test For Touch Resolution ----- */
		else if ( DEBUG_IsTxtReceive ( "resolution" ) )
			TOUCH_SetDefaultResolution ();

		_DBG_PARAM_NOWRAP ( "r1", TOUCH_GetPtr2Resolution (), _uint8, _Incr, _Uint8 ( 1 ), _Uint8 ( 15 ), "Touch Resolution: ", NULL )
		_DBG_PARAM_NOWRAP ( "r2", TOUCH_GetPtr2Resolution (), _uint8, _Decr, _Uint8 ( 1 ), _Uint8 ( 1 ), "Touch Resolution: ", NULL )
		/* ----- END Debug Test For Touch Resolution ----- */

		else if ( DEBUG_IsTxtReceive ( "p" ) )
		{
			DbgVarDma ( 1, 100, Clr_ Mag_ "\r\nStart: %s -> CPU: %d \r\n" _X_, GET_CODE_FUNCTION, osGetCPUUsage () );
			DisplayCoeffCalibration ();
		}
		else if ( DEBUG_IsTxtReceive ( "s\x0D" ) )
		{
			SCREEN_Fonts_funcSet ( FONT_COLOR_LoadFontTime, BLACK );
			SCREEN_Fonts_funcSet ( COLOR_FramePress, BLACK );
		}

		else if ( DEBUG_IsTxtReceive ( "6" ) )
		{
			if ( TOOGLE ( RR ) ) ///DAJ JAKO WYBOR BITOWY \11111111111111 dla toogle !!!!!!!!!!!!!!!!!!!!!!!!
			{
				//			FILE_NAME(keyboard)(KEYBOARD_fontRGB, KEY_All_release, LCD_RoundRectangle,0,  10,160, KeysAutoSize,12, 4, Touch_FontColor, Touch_fontRp, KeysDel);
				//			FILE_NAME(keyboard)(KEYBOARD_bkRGB,   KEY_All_release, LCD_RoundRectangle,0, 600,160, KeysAutoSize,12, 4, Touch_BkColor, 	Touch_bkRp,	  KeysNotDel);
				/*
			FILE_NAME(keyboard)(KEYBOARD_sliderRGB, 	KEY_All_release, LCD_RoundRectangle,0, 50,160, 39,140,  16, Touch_FontColor2, Touch2_fontSliderR_left, KeysDel);
			FILE_NAME(keyboard)(KEYBOARD_sliderBkRGB, KEY_All_release, LCD_RoundRectangle,0, 550,160, 39,140, 16, Touch_BkColor2,   Touch2_bkSliderR_left, 	 KeysNotDel);
*/

				FILE_NAME ( keyboard ) ( KEYBOARD_fontSize2, KEY_Select_one, LCD_Rectangle, 0, 615, 100, KeysAutoSize, 10, 0, Touch_FontSizeMove, Touch_FontSizeRoll, KeysDel );
				FILE_NAME ( keyboard ) ( KEYBOARD_bkRGB, KEY_All_release, LCD_RoundRectangle, 0, 300, 200, KeysAutoSize, 12, 4, Touch_BkColor, Touch_bkRp, KeysNotDel );
			}
			else
			{
				FILE_NAME ( main ) ( LoadPartScreen, (char **) ppMain );
				KEYBOARD_TYPE ( KEYBOARD_none, 0 );
			}

			//Plik_bin_Divide(GETVAL_ptr(0));
		}

		else if ( DEBUG_IsTxtReceive ( "7" ) )
		{
			*ppMain = (int *) FRAMES_GROUP_separat;
			FILE_NAME ( main ) ( LoadUserScreen, (char **) ppMain );
		}

		else if ( DEBUG_IsTxtReceive ( "1" ) )
		{
			DbgDma ( 1, "test" );
			FILE_NAME ( main ) ( LoadPartScreen, (char **) ppMain );
		}

	else if(DEBUG_IsTxtReceive("q"))
	{
		SCREEN_SetNr(7);
	}
	else if(DEBUG_IsTxtReceive("w"))
	{
		SCREEN_SetNr(8);
	}
	else if(DEBUG_IsTxtReceive("e"))
	{
		SCREEN_SetNr(9);
	}
	else if(DEBUG_IsTxtReceive("r"))
	{
		SCREEN_SetNr(10);
	}




#if defined(LCD_ENABLE_F7_NETWORK_DEBUG)
/* F7-only network test commands are not LCD dependencies on H7. */
	else if ( DEBUG_IsTxtReceive ( "8" ) )
	{
		DbgDma ( 1, "8 " );
		ESP_Send("AT+SYSTIMESTAMP?\r\n");
	}
	else if ( DEBUG_IsTxtReceive ( "9" ) )
	{
		DbgDma ( 1, "9 " );
		ESP_Recv();
	}
	else if ( DEBUG_IsTxtReceive ( "0" ) )
	{
		DbgDma ( 1, "0 " );
		RestartWifiTask();
	}
	else if ( DEBUG_IsTxtReceive ( "q" ) )
	{
		DbgDma ( 1, "q " );
		vRestartWifiServer();
	}
	else if ( DEBUG_IsTxtReceive ( "w" ) )
	{
		DbgDma ( 1, "w " );
		vTestATcommand();
	}
	else if ( DEBUG_IsTxtReceive ( "f" ) )
	{
		DbgDma ( 1, "f" );
		vRestoreESP();
	}



// to dla wifi:
	//	else if(DEBUG_IsTxtReceive(","))   //interia
	//	{
	//		DbgDma(1,", ");
	//		SendEmail(1, 1<<5|1<<1, EMAIL_TEST);
	//	}
	//	else if(DEBUG_IsTxtReceive("."))
	//	{
	//		DbgDma(1,". ");
	//		SendEmail(0, 1<<5|1<<1, EMAIL_TEST);
	//
	//		CreateTestEMAILTask("Info_11 ");
	//	}
	//	else if(DEBUG_IsTxtReceive("/"))   //gmail
	//	{
	//		DbgDma(1,"/");
	//		SendEmail(0, 1<<5|1<<1, EMAIL_TEST);
	//	}

	/* !! Wywolywania zadania z zadania generuje hardfault !! */
	else if ( DEBUG_IsTxtReceive ( "." ) )
	{
		DbgDma ( 1, ". " );
		SendEmail(0, 1<<5|1<<1, EMAIL_TEST);
		//CreateTestEMAILTask("asdasd");
	}
	else if ( DEBUG_IsTxtReceive ( "/" ) ) //gmail
	{
		DbgDma ( 1, "/" );
		SendEmail(0, 1<<1, EMAIL_MEASURE);
		CreateTestEMAILTask("0123456789 ABCD abcd");
	}

#endif


}}

static void LoadFonts(int startFontID, int endFontID){
	if(TakeMutex(Semphr_cardSD,1000))
	{
		#define OMITTED_FONTS	1	/*this define delete for another screens*/
		#define A(x)	 *((int*)((int*)(&v)+x))

		int d = endFontID-startFontID + 1 + OMITTED_FONTS;
		int j=0;

		for(int i=startFontID; i<=endFontID; ++i){
			*((int*)((int*)(&v)+i)) = LCD_LoadFont_DependOnColors( A(j),A(j+d),A(j+3*d),A(j+2*d), FILE_NAME(GetDefaultParam)(i));
			j++;
		}

		GiveMutex(Semphr_cardSD);
		#undef OMITTED_FONTS
		#undef A
	}
/*
	v.FONT_ID_Title 	 		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(Title),	  	FILE_NAME(GetDefaultParam)(FONT_ID_Title));
	v.FONT_ID_FontColor		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(FontColor),	FILE_NAME(GetDefaultParam)(FONT_ID_FontColor));
	v.FONT_ID_BkColor 		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(BkColor),  	FILE_NAME(GetDefaultParam)(FONT_ID_BkColor));
	v.FONT_ID_FontType 		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(FontType), 	FILE_NAME(GetDefaultParam)(FONT_ID_FontType));
	v.FONT_ID_FontSize 		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(FontSize), 	FILE_NAME(GetDefaultParam)(FONT_ID_FontSize));
	v.FONT_ID_FontStyle  	= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(FontStyle),	FILE_NAME(GetDefaultParam)(FONT_ID_FontStyle));

	v.FONT_ID_Coeff 			= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(Coeff), 		 FILE_NAME(GetDefaultParam)(FONT_ID_Coeff));
	v.FONT_ID_LenWin 			= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(LenWin), 		 FILE_NAME(GetDefaultParam)(FONT_ID_LenWin));
	v.FONT_ID_OffsWin 		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(OffsWin), 	 FILE_NAME(GetDefaultParam)(FONT_ID_OffsWin));
	v.FONT_ID_LoadFontTime 	= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(LoadFontTime),FILE_NAME(GetDefaultParam)(FONT_ID_LoadFontTime));
	v.FONT_ID_PosCursor 		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(PosCursor), 	 FILE_NAME(GetDefaultParam)(FONT_ID_PosCursor));
	v.FONT_ID_CPUusage 		= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(CPUusage), 	 FILE_NAME(GetDefaultParam)(FONT_ID_CPUusage));
	v.FONT_ID_Speed 			= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(Speed), 		 FILE_NAME(GetDefaultParam)(FONT_ID_Speed));
	v.FONT_ID_Press 			= LCD_LoadFont_DependOnColors( LOAD_FONT_PARAM(Press), 		 FILE_NAME(GetDefaultParam)(FONT_ID_Press));
*/
}

static StructTxtPxlLen ELEMENT_fontRGB(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	#define _TXT_R(x)		 SL(LANG_nazwa_3)
	#define _TXT_G(x)		 SL(LANG_nazwa_4)
	#define _TXT_B(x)		 SL(LANG_nazwa_5)
	#define _TXT_LEFT(x)	 SL(LANG_nazwa_2)

	StructTxtPxlLen lenStr = {0};
	StructFieldPos fieldTouch = {0};

	int spaceMain_width 		= LCD_GetWholeStrPxlWidth(v.FONT_ID_FontColor," ",0,ConstWidth);
	int digit3main_width 	= LCD_GetWholeStrPxlWidth(v.FONT_ID_FontColor,INT2STR(Test.font[0]),0,ConstWidth);
	int xPos_main 				= xPos + LCD_GetWholeStrPxlWidth(v.FONT_ID_Descr,_TXT_LEFT(0),0,NoConstWidth) + 4;

	int _GetWidth(char *txt){ return LCD_GetWholeStrPxlWidth(v.FONT_ID_Descr,txt,0,ConstWidth); }

	int xPos_under_left 		= MIDDLE( xPos_main+spaceMain_width, digit3main_width, _GetWidth(_TXT_R(0)) );
	int xPos_under_right 	= MIDDLE( xPos_main + 3*spaceMain_width + 2*digit3main_width, digit3main_width, _GetWidth(_TXT_B(0)) );

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(FontColor), xPos, yPos, TXT_FONT_COLOR, fullHight, 0,250, ConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	 			 Above_left, 	SL(LANG_nazwa_1), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4, 								 Left_mid, 		_TXT_LEFT(0), fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(251,29,27), v.FONT_BKCOLOR_Descr, 4|(xPos_under_left<<16),  Under_left,	_TXT_R(20), fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(60,247,68), v.FONT_BKCOLOR_Descr, 4, 								 Under_center, _TXT_G(40), fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(51,90,245), v.FONT_BKCOLOR_Descr, 4|(xPos_under_right<<16), Under_right,	_TXT_B(60), fullHight, 0,250, NoConstWidth,\
		LCD_STR_DESCR_PARAM_NUMBER(5) );

	LCD_SetBkFontShape(v.FONT_VAR_FontColor,BK_LittleRound);

	fieldTouch 			= *field;
	fieldTouch.width 	= fieldTouch.width/3;
	fieldTouch.x 		= fieldTouch.x + fieldTouch.width;

#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
	if(LoadWholeScreen==argNmb)	SCREEN_ConfigTouchForStrVar(ID_TOUCH_POINT, Touch_FontColor, press, v.FONT_VAR_FontColor,0, field->len);
#else
	if(LoadWholeScreen==argNmb){	SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_RELEASE_WITH_HOLD, Touch_FontColor,  	 		LCD_TOUCH_SetTimeParam_ms(600), v.FONT_VAR_FontColor,0, *field);
											SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_WITH_HOLD, 		    Touch_FontColor2, 	 		LCD_TOUCH_SetTimeParam_ms(700), v.FONT_VAR_FontColor,1, *field);
											SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_MOVE_RIGHT, 		    	 Touch_FontColorMoveRight, press, 								  v.FONT_VAR_FontColor,2, fieldTouch);
											SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_MOVE_LEFT, 		    	 	 Touch_FontColorMoveLeft,  press, 								  v.FONT_VAR_FontColor,3, fieldTouch);
	}
#endif

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;
	return lenStr;

	#undef _TXT_R
	#undef _TXT_G
	#undef _TXT_B
	#undef _TXT_LEFT
}

static StructTxtPxlLen ELEMENT_fontBkRGB(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	#define _TXT_R(x)		 SL(LANG_nazwa_3)
	#define _TXT_G(x)		 SL(LANG_nazwa_4)
	#define _TXT_B(x)		 SL(LANG_nazwa_5)
	#define _TXT_LEFT(x)	 SL(LANG_nazwa_7)

	StructTxtPxlLen lenStr = {0};
	StructFieldPos fieldTouch = {0};

	int spaceMain_width 		= LCD_GetWholeStrPxlWidth(v.FONT_ID_BkColor," ",0,ConstWidth);
	int digit3main_width 	= LCD_GetWholeStrPxlWidth(v.FONT_ID_BkColor,INT2STR(Test.bk[0]),0,ConstWidth);
	int xPos_main 				= xPos + LCD_GetWholeStrPxlWidth(v.FONT_ID_Descr,_TXT_LEFT(0),0,NoConstWidth) + 4;

	int _GetWidth(char *txt){ return LCD_GetWholeStrPxlWidth(v.FONT_ID_Descr,txt,0,ConstWidth); }

	int xPos_under_left 		= MIDDLE( xPos_main+spaceMain_width, digit3main_width, _GetWidth(_TXT_R(0)) );
	int xPos_under_right 	= MIDDLE( xPos_main + 3*spaceMain_width + 2*digit3main_width, digit3main_width, _GetWidth(_TXT_B(0)) );

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(BkColor), xPos, yPos, TXT_BK_COLOR, fullHight, 0,250, ConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	 			 Above_left,   SL(LANG_nazwa_6), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4, 								 Left_mid, 		_TXT_LEFT(0), fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(251,29,27), v.FONT_BKCOLOR_Descr, 4|(xPos_under_left<<16),  Under_left,	_TXT_R(20), fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(60,247,68), v.FONT_BKCOLOR_Descr, 4, 								 Under_center, _TXT_G(40), fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(51,90,245), v.FONT_BKCOLOR_Descr, 4|(xPos_under_right<<16), Under_right,	_TXT_B(60), fullHight, 0,250, NoConstWidth,\
		LCD_STR_DESCR_PARAM_NUMBER(5) );

	LCD_SetBkFontShape(v.FONT_VAR_BkColor,BK_LittleRound);

	fieldTouch 			= *field;
	fieldTouch.width 	= fieldTouch.width/3;
	fieldTouch.x 		= fieldTouch.x + fieldTouch.width;

#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
	if(LoadWholeScreen==argNmb)	SCREEN_ConfigTouchForStrVar(ID_TOUCH_POINT, Touch_BkColor, press, v.FONT_VAR_BkColor,0, field->len);
#else
	if(LoadWholeScreen==argNmb){	SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_RELEASE_WITH_HOLD, Touch_BkColor,  	  LCD_TOUCH_SetTimeParam_ms(600), v.FONT_VAR_BkColor,0, *field);
											SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_WITH_HOLD, 		    Touch_BkColor2, 	  LCD_TOUCH_SetTimeParam_ms(700), v.FONT_VAR_BkColor,1, *field);
											SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_MOVE_RIGHT, 		    	 Touch_BkColorMove, press, 								 v.FONT_VAR_BkColor,2, fieldTouch);
	}
#endif

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;
	return lenStr;

	#undef _TXT_R
	#undef _TXT_G
	#undef _TXT_B
	#undef _TXT_LEFT
}

static StructTxtPxlLen ELEMENT_fontLenOffsWin(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	StructTxtPxlLen lenStr = {0};

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(LenWin), xPos, yPos, TXT_LENOFFS_WIN, fullHight, 0,250, ConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	Above_left,  SL(LANG_LenOffsWin3), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4, 					Left_mid, 	  "8.",  fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	Under_left,  SL(LANG_LenOffsWin4), fullHight, 0,250, NoConstWidth, \
		LCD_STR_DESCR_PARAM_NUMBER(3) );

	LCD_SetBkFontShape(v.FONT_VAR_LenWin,BK_LittleRound);

	if(LoadWholeScreen==argNmb)	SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT, Touch_FontLenOffsWin, press, v.FONT_VAR_LenWin,0, *field);

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;

	return lenStr;
}

static StructTxtPxlLen ELEMENT_fontCoeff(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	StructTxtPxlLen lenStr = {0};
	int interSp= 4, heightTriang= 10;

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(Coeff), xPos, yPos, TXT_COEFF, fullHight, 0,250, ConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, interSp|(xPos<<16),	Above_left,   SL(LANG_FontCoeffAbove), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, interSp, 					Left_mid, 	  SL(LANG_FontCoeffLeft),  fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, interSp, 					Under_center, SL(LANG_FontCoeffUnder), fullHight, 0,250, NoConstWidth, \
		LCD_STR_DESCR_PARAM_NUMBER(3) );

	LCD_SimpleTriangle(0,LCD_X, xPos+field->width-interSp, yPos+LCD_GetFontHeight(v.FONT_ID_Descr)+interSp+heightTriang-1, heightTriang,heightTriang, v.FONT_COLOR_Descr, v.FONT_COLOR_Descr, v.COLOR_BkScreen, Up);
	LCD_SimpleTriangle(0,LCD_X, xPos+field->width-interSp, yPos+LCD_GetFontHeight(v.FONT_ID_Descr)+interSp+heightTriang+6, heightTriang,heightTriang, v.FONT_COLOR_Descr, v.FONT_COLOR_Descr, v.COLOR_BkScreen, Down);

	LCD_SetBkFontShape(v.FONT_VAR_Coeff,BK_LittleRound);

	if(LoadWholeScreen==argNmb)	SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT, Touch_FontCoeff, press, v.FONT_VAR_Coeff,0, *field);

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;

	return lenStr;
}

static StructTxtPxlLen ELEMENT_fontType(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	StructTxtPxlLen lenStr = {0};

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(FontType), xPos, yPos, TXT_FONT_TYPE, fullHight, 0,255, NoConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4|(xPos<<16), Above_left,   SL(LANG_FontTypeAbove), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4, 				 Left_mid, 		SL(LANG_FontTypeLeft), 	fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(251,29,27), v.FONT_BKCOLOR_Descr, 4|(xPos<<16), Under_left,	SL(LANG_FontTypeUnder), fullHight, 0,250, NoConstWidth, \
		LCD_STR_DESCR_PARAM_NUMBER(3) );

	LCD_SetBkFontShape(v.FONT_VAR_FontType,BK_LittleRound);

#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
	if(LoadWholeScreen==argNmb){ SCREEN_ConfigTouchForStrVar(ID_TOUCH_POINT_RELEASE_WITH_HOLD, Touch_FontType,  LCD_TOUCH_SetTimeParam_ms(600), v.FONT_VAR_FontType,0, field->len);
										  SCREEN_ConfigTouchForStrVar(ID_TOUCH_POINT_WITH_HOLD, 		  	 Touch_FontType2, LCD_TOUCH_SetTimeParam_ms(700), v.FONT_VAR_FontType,1, field->len); }
#else
	if(LoadWholeScreen==argNmb){ SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_RELEASE_WITH_HOLD, Touch_FontType,  LCD_TOUCH_SetTimeParam_ms(600), v.FONT_VAR_FontType,0, *field);
										  SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_WITH_HOLD, 		   Touch_FontType2, LCD_TOUCH_SetTimeParam_ms(700), v.FONT_VAR_FontType,1, *field); }
#endif

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;

	return lenStr;
}

static StructTxtPxlLen ELEMENT_fontSize(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	StructTxtPxlLen lenStr = {0};
	StructFieldPos fieldTouch = {0};
	int interSp= 4;

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(FontSize), xPos, yPos, TXT_FONT_SIZE, fullHight, 0,255, NoConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, interSp|(xPos<<16), Above_left,   SL(LANG_FontSizeAbove), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, interSp, 				 Left_mid, 		SL(LANG_FontSizeLeft),  fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, RGB2INT(186,130,50),v.FONT_BKCOLOR_Descr, interSp|(xPos<<16), Under_left,	SL(LANG_FontSizeUnder), fullHight, 0,250, NoConstWidth, \
		LCD_STR_DESCR_PARAM_NUMBER(3) );

	LCD_SetBkFontShape(v.FONT_VAR_FontSize,BK_LittleRound);

	fieldTouch 			= *field;
	fieldTouch.width 	= fieldTouch.width/3;
	fieldTouch.x 		= fieldTouch.x + fieldTouch.width;

#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
	if(LoadWholeScreen==argNmb){ SCREEN_ConfigTouchForStrVar(ID_TOUCH_POINT_RELEASE_WITH_HOLD, Touch_FontSize, LCD_TOUCH_SetTimeParam_ms(600), v.FONT_VAR_FontSize,0, field->len);
										  SCREEN_ConfigTouchForStrVar(ID_TOUCH_POINT_WITH_HOLD, 		  Touch_FontSize2, LCD_TOUCH_SetTimeParam_ms(700), v.FONT_VAR_FontSize,1, field->len); }
#else
	if(LoadWholeScreen==argNmb){ SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_RELEASE_WITH_HOLD, Touch_FontSize,  	  LCD_TOUCH_SetTimeParam_ms(600), v.FONT_VAR_FontSize,0, *field);
										  SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_WITH_HOLD, 		   Touch_FontSize2, 	  LCD_TOUCH_SetTimeParam_ms(700), v.FONT_VAR_FontSize,1, *field);
										  SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_MOVE_RIGHT, 		    	   Touch_FontSizeMove, press, 								 v.FONT_VAR_FontSize,2, fieldTouch); }
#endif

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;

	return lenStr;
}

static StructTxtPxlLen ELEMENT_fontStyle(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	StructTxtPxlLen lenStr = {0};
	StructFieldPos field_copy = {0};

	StructFieldPos _Function_FontStyleElement(int noDisp, char *txt){
		return LCD_StrDependOnColorsDescrVar_array_xyCorrect(noDisp,STR_FONT_PARAM2(FontStyle), xPos, yPos, txt, fullHight, 0,255, NoConstWidth, \
			v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4|(xPos<<16), Above_left,   SL(LANG_FontStyleAbove), fullHight, 0,250, NoConstWidth,\
			v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4, 				 Left_mid, 		SL(LANG_FontStyleLeft),  fullHight, 0,250, NoConstWidth, \
			v.FONT_ID_Descr, RGB2INT(251,29,27), v.FONT_BKCOLOR_Descr, 4|(xPos<<16), Under_left,	SL(LANG_FontStyleUnder), fullHight, 0,250, NoConstWidth, \
			LCD_STR_DESCR_PARAM_NUMBER(3) );
	}

	*field = _Function_FontStyleElement(1," "LONGEST_TXT_FONTSTYLE" ");		field_copy = *field;		/* To adjust frame width to the longest possible mainTxt */
	*field = _Function_FontStyleElement(0,TXT_FONT_STYLE);

	field->width = field_copy.width;
	field->height = field_copy.height;

	LCD_SetBkFontShape(v.FONT_VAR_FontStyle,BK_LittleRound);

#ifdef TOUCH_MAINFONTS_WITHOUT_DESCR
	if(LoadWholeScreen==argNmb){ SCREEN_ConfigTouchForStrVar(ID_TOUCH_POINT_RELEASE_WITH_HOLD, Touch_FontStyle, LCD_TOUCH_SetTimeParam_ms(600), v.FONT_VAR_FontStyle,0, field->len);
										  SCREEN_ConfigTouchForStrVar(ID_TOUCH_POINT_WITH_HOLD, 		  Touch_FontStyle2, LCD_TOUCH_SetTimeParam_ms(700), v.FONT_VAR_FontStyle,1, field->len); }
#else
	if(LoadWholeScreen==argNmb){ SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_RELEASE_WITH_HOLD, Touch_FontStyle, LCD_TOUCH_SetTimeParam_ms(600), v.FONT_VAR_FontStyle,0, *field);
										  SCREEN_ConfigTouchForStrVar_2(ID_TOUCH_POINT_WITH_HOLD, 		  	 Touch_FontStyle2, LCD_TOUCH_SetTimeParam_ms(700), v.FONT_VAR_FontStyle,1, *field); }
#endif

	lenStr.inPixel = field->width;
	lenStr.height 	= field->height;

	return lenStr;
}

static StructTxtPxlLen ELEMENT_fontTime(StructFieldPos *field, int xPos,int yPos, int argNmb)
{
	StructTxtPxlLen lenStr = {0};

	*field = LCD_StrDependOnColorsDescrVar_array_xyCorrect(0,STR_FONT_PARAM2(LoadFontTime), xPos, yPos, TXT_TIMESPEED, fullHight, 0,250, ConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	Above_left,  SL(LANG_TimeSpeed1), fullHight, 0,250, NoConstWidth,\
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4, 					Left_mid, 	  "7.",  fullHight, 0,250, NoConstWidth, \
		v.FONT_ID_Descr, v.FONT_COLOR_Descr, v.FONT_BKCOLOR_Descr, 4|(xPos<<16),	Under_left,  SL(LANG_TimeSpeed2), fullHight, 0,250, NoConstWidth, \
		LCD_STR_DESCR_PARAM_NUMBER(3) );

	LCD_SetBkFontShape(v.FONT_VAR_LenWin,BK_LittleRound);

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

	FILE_NAME(funcSet)(FONT_BKCOLOR_Descr, 		_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_FontColor, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_BkColor, 		_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_FontType, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_FontSize, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_FontStyle, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_Coeff, 		_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_LenWin, 		_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_LoadFontTime, _FILL_COLOR);

	_Element(fontRGB,SetPos,X_start=startOffsX,SetPos,startOffsY)		/* _LineV(field.height,GetPos,-startOffsX/2-1,GetPos,0) */	field1=field;
	_Element(fontBkRGB,GetPos,0,IncPos,offsY)									/* _LineV(field.height,GetPos,-startOffsX/2-1,GetPos,0) */
	tab[0]=field1.width;
	tab[1]=field.width;
	MAXVAL(tab,2,0,tab[3])
	_LineH(tab[3],GetPos,0,GetPos,-offsY/2-1)

	_Element(fontType,SetPos,X_start+=tab[3]+offsX,SetPos,startOffsY)		_LineV(field.height,GetPos,-offsX/2-1,GetPos,0)	field1=field;
	_Element(fontSize,GetPos,0,IncPos,offsY)										_LineV(field.height,GetPos,-offsX/2-1,GetPos,0)
	tab[0]=field1.width;
	tab[1]=field.width;
	MAXVAL(tab,2,0,tab[3])
	_LineH(tab[3],GetPos,0,GetPos,-offsY/2-1)

	_Element(fontStyle,SetPos,X_start+=tab[3]+offsX,SetPos,startOffsY)	_LineV(field.height,GetPos,-offsX/2-1,GetPos,0)	field1=field;
	_Element(fontTime,GetPos,0,IncPos,offsY)										_LineV(field.height,GetPos,-offsX/2-1,GetPos,0)
	tab[0]=field1.width;
	tab[1]=field.width;
	MAXVAL(tab,2,0,tab[3])
	_LineH(tab[3],GetPos,0,GetPos,-offsY/2-1)

	int offsX_temp = 0;
	_Element(fontLenOffsWin,SetPos,X_start+=tab[3]+offsX,SetPos,startOffsY)	_LineV(field.height,GetPos,-offsX/2-1,				 GetPos,0)	field1=field;
	_Element(fontCoeff,GetPos,0-offsX_temp,IncPos,offsY)							_LineV(field.height,GetPos,-offsX/2-1-offsX_temp,GetPos,0)
	tab[0]=field1.width;
	tab[1]=field.width;
	MAXVAL(tab,2,0,tab[3])
	_LineH(tab[3],GetPos,0,GetPos,-offsY/2-1)

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

	FILE_NAME(funcSet)(FONT_BKCOLOR_Descr, 		_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_FontColor, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_BkColor, 		_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_FontType, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_FontSize, 	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_FontStyle,	_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_Coeff, 		_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_LenWin, 		_FILL_COLOR);
	FILE_NAME(funcSet)(FONT_BKCOLOR_LoadFontTime, _FILL_COLOR);

	_Element(fontRGB,0,SetPos,startOffsX,0,SetPos,startOffsY) 	_Element(fontType,0,IncPos,offsX,1,SetPos,startOffsY)  _Element(fontStyle,0,IncPos,offsX,2,SetPos,startOffsY)  _Element(fontLenOffsWin,0,IncPos,offsX,3,SetPos,startOffsY)
	_Element(fontBkRGB,0,SetPos,startOffsX,0,GetPos,0) 		 	_Element(fontSize,0,IncPos,offsX,1,GetPos,0)				 _Element(fontTime,0,IncPos,offsX,2,GetPos,0)				_Element(fontCoeff,0,IncPos,offsX,3,GetPos,0)

	#undef _Element
	#undef _Rectan
	#undef _FRAME_COLOR
	#undef _FILL_COLOR
}

void FILE_NAME(main)(int argNmb, char **argVal)   //Dla Zmiana typu czcionki Touch left dac mozliwosc wspolczynnik zmiany
{
	if(NULL == argVal)
		argVal = (char**)ppMain;

	LCD_Clear(v.COLOR_BkScreen);

	if(LoadWholeScreen == argNmb)
	{
		SCREEN_ResetAllParameters();
		LCD_TOUCH_DeleteAllSetTouch();
		FONTS_LCD_ResetParam();

		DbgVarDma ( v.DEBUG_ON, 100, "%s" Cya_ "\r\nStart: %s\r\n" _X_, CONDITION ( USE_DBG_CLR, Clr_, "" ), GET_CODE_FUNCTION );

		LoadFonts(FONT_ID_Title, FONT_ID_Press);
		LCD_LoadFontVar();

		LCDTOUCH_Set(LCD_X-FV(SetVal,0,LCD_GetWholeStrPxlWidth(v.FONT_ID_Descr,SL(LANG_MainFrameType),0,NoConstWidth)+5), \
						 LCD_Y-FV(SetVal,1,LCD_GetFontHeight(v.FONT_ID_Descr)+5), \
						 	 	 FV(GetVal,0,NoUse),\
								 FV(GetVal,1,NoUse), ID_TOUCH_POINT,Touch_MainFramesType,press);


		LCD_Ymiddle(ID_MIDDLE_TXT,SetPos, SetPosAndWidth(Test.yFontsField,240) );
		LCD_Xmiddle(ID_MIDDLE_TXT,SetPos, SetPosAndWidth(Test.xFontsField,LCD_GetXSize()),NULL,0,NoConstWidth);
		LCD_SetBkFontShape(v.FONT_VAR_Fonts,BK_Rectangle);

		vTimerService(TIMER_Cpu,start_time,noUse);
	}
	/*FILE_NAME(printInfo)();*/

	INIT(endSetFrame,195);
	LCD_DrawMainFrame(LCD_RoundRectangle,NoIndDisp,0, 0,0, LCD_X,endSetFrame,SHAPE_PARAM(MainFrame,FillMainFrame,BkScreen));

	if		 (*(argVal+0)==(char*)FRAMES_GROUP_combined)
		FRAMES_GROUP_combined(argNmb,15,15,25,25,1);
	else if(*(argVal+0)==(char*)FRAMES_GROUP_separat)
		FRAMES_GROUP_separat(argNmb,15,15,25,25,FRAME_bold2Space(0,6));


	LCD_StrDependOnColorsVar(STR_FONT_PARAM(CPUusage,FillMainFrame),0,420,TXT_CPU_USAGE,halfHight,0,255,ConstWidth);
	LCD_StrDependOnColors(v.FONT_ID_Descr, LCD_X-FV(GetVal,0,NoUse), LCD_Y-FV(GetVal,1,NoUse), SL(LANG_MainFrameType), fullHight,0, v.COLOR_FillFrame, v.FONT_COLOR_Descr, 255, NoConstWidth);

	if(LoadUserScreen == argNmb){

	}

	StartMeasureTime_us();
	 if(Test.type==RGB_RGB) lenStr= LCD_StrChangeColorVar(v.FONT_VAR_Fonts,v.FONT_ID_Fonts, POS_X_TXT, POS_Y_TXT, Test.txt, fullHight, Test.spaceBetweenFonts, RGB_BK, RGB_FONT,																		  Test.coeff, Test.constWidth, v.COLOR_BkScreen);
	 else  						lenStr= LCD_StrVar			  (v.FONT_VAR_Fonts,v.FONT_ID_Fonts, POS_X_TXT, POS_Y_TXT, Test.txt, fullHight, Test.spaceBetweenFonts, argNmb==0 ? v.COLOR_BkScreen : LCD_GetStrVar_bkColor(v.FONT_VAR_Fonts), Test.coeff, Test.constWidth, v.COLOR_BkScreen);
	Test.speed=StopMeasureTime_us("");

	EXPER_FUNC_beforeDispBuffLcd();

	if(LoadWholeScreen  == argNmb) TxtTouch(TouchSetNew);
	if(LoadNoDispScreen != argNmb) LCD_Show();

	EXPER_FUNC_afterDispBuffLcd();
}

static void EXPER_FUNC_beforeDispBuffLcd(void)
{
	#define ONLY_ONE_AT_START		0

	static int only_one = 0;
	if(only_one==0)
	{
		StartMeasureTime_us();


		StopMeasureTime_us("");
	}

	if(1)	/* This display ever time */
	{
		LCD_TxtShadowInit(fontVar_40, v.FONT_ID_FontColor, v.COLOR_BkScreen, BK_Rectangle);
		LCD_Txt(Display, NULL, 0,0, LCD_X,LCD_Y, v.FONT_ID_FontColor, fontVar_40, 20,200, "12345", BLACK, 0/*v.COLOR_BkScreen*/, fullHight,0,250, NoConstWidth, TXTSHADECOLOR_DEEP_DIR(0x777777,4,RightDown) /*TXTSHADE_NONE*/);
	}

	only_one = ONLY_ONE_AT_START;
	#undef ONLY_ONE_AT_START
}

static void EXPER_FUNC_afterDispBuffLcd(void)
{


}

#undef POS_X_TXT
#undef POS_Y_TXT
#undef TEXT_TO_SHOW
#undef ID_MIDDLE_TXT
#undef POS_X_TXT
#undef POS_Y_TXT
#undef TXT_FONT_COLOR
#undef TXT_BK_COLOR
#undef TXT_FONT_TYPE
#undef TXT_FONT_SIZE
#undef TXT_FONT_STYLE
#undef TXT_COEFF
#undef TXT_LEN_WIN
#undef TXT_OFFS_WIN
#undef TXT_LENOFFS_WIN
#undef TXT_TIMESPEED
#undef TXT_CPU_USAGE
#undef RGB_FONT
#undef RGB_BK
#undef CHECK_TOUCH
#undef SET_TOUCH
#undef CLR_TOUCH
#undef CLR_ALL_TOUCH
#undef GET_TOUCH
#undef NONE_TYPE_REQ
#undef MAX_NUMBER_OPENED_KEYBOARD_SIMULTANEOUSLY
#undef SELECT_CURRENT_FONT



//po KLAWIATURZE rob ustawienie calegho textu aby dopasowac spacje pomiedzy roznuymoi literami

//ZROBIC cieniowanie pol i text 3d na nim jak w biletcie automatu na muzeum naradowe


//ROBIMY :  1. LISTVIEW  tabelko z lista   2. klawiature i koniec !!!

	//ATTENTION IN FUTURE   tylko w jednej funkcji umieszczamy zmienne odswiezane reularnie i ta funkcja idzie do jakiegos watku !!!!!


//w harfoult interrup ddacv mozliwosc odczyti linijki kodu poprzedniego !!!!
//Zrobic szablon nowego okna -pliku LCD !!!! aby latwo wystartowac !!!
//ZROBIC animacje ze samo sie klioka i chmurka z info ze przytrzymac na 2 sekundy ....
//ZROBIC AUTOMATYCZNE testy wszystkich mozliwosci !!!!!!! taki interfejs testowy
//tu W **arcv PRZEKAZ TEXT !!!!!! dla fonts !!!
//tester po uart czy eth zeby samo ekran klikalo i ustawialo !!!! taka setup animacja
