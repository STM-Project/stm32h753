#ifndef LCD_SCREENS_SCREEN_VISUALPARAMLCD_H_
#define LCD_SCREENS_SCREEN_VISUALPARAMLCD_H_


#include "stm32h7xx_hal.h"

typedef enum{
	VISUALPARAM_FONT_SIZE_Title,
	VISUALPARAM_FONT_SIZE_FontColor,

	VISUALPARAM_FONT_STYLE_Title,
	VISUALPARAM_FONT_STYLE_FontColor,

	VISUALPARAM_FONT_COLOR_Title,
	VISUALPARAM_FONT_COLOR_FontColor,

	VISUALPARAM_FONT_BKCOLOR_Title,
	VISUALPARAM_FONT_BKCOLOR_FontColor,

	VISUALPARAM_COLOR_BkScreen,
	VISUALPARAM_COLOR_MainFrame,
	VISUALPARAM_COLOR_FillMainFrame,
	VISUALPARAM_COLOR_Frame,
	VISUALPARAM_COLOR_FillFrame,
	VISUALPARAM_COLOR_FramePress,
	VISUALPARAM_COLOR_FillFramePress,
	VISUALPARAM_DEBUG_ON,
	VISUALPARAM_BK_FONT_ROUND,
	VISUALPARAM_LANG_SELECT,

}_SCREEN_VisualParam_enum;

void SCREEN_VisualParam_main(int argNmb, char **argVal);

int SCREEN_VisualParam_funcGet(int offs);
void SCREEN_VisualParam_funcSet(int offs, int val);
void SCREEN_VisualParam_setDefaultAllParam(int rst);
void SCREEN_VisualParam_printInfo(void);

void SCREEN_VisualParam_debugRcvStr(void);
void SCREEN_VisualParam_setTouch(void);

#endif /* LCD_SCREENS_SCREEN_VISUALPARAMLCD_H_ */
