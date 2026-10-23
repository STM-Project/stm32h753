
/*
 * SCREEN_CalibrationLCD.h
 *
 *  Created on: Apr 23, 2024
 *      Author: maraf
 */

#ifndef LCD_SCREENS_SCREEN_CALIBRATIONLCD_H_
#define LCD_SCREENS_SCREEN_CALIBRATIONLCD_H_

#include "stm32h7xx_hal.h"

typedef enum{
	CALIBRATION_FONT_SIZE_Title,
	CALIBRATION_FONT_SIZE_CircleName,
	CALIBRATION_FONT_SIZE_PosLog,
	CALIBRATION_FONT_SIZE_PosPhys,

	CALIBRATION_FONT_STYLE_Title,
	CALIBRATION_FONT_STYLE_CircleName,
	CALIBRATION_FONT_STYLE_PosLog,
	CALIBRATION_FONT_STYLE_PosPhys,

	CALIBRATION_FONT_COLOR_Title,
	CALIBRATION_FONT_COLOR_CircleName,
	CALIBRATION_FONT_COLOR_PosLog,
	CALIBRATION_FONT_COLOR_PosPhys,

	CALIBRATION_COLOR_BkScreen,
	CALIBRATION_COLOR_CircleFrame,
	CALIBRATION_COLOR_CircleFill,
	CALIBRATION_COLOR_CircleFillPress,

	CALIBRATION_COEFF_COLOR_CircleName,
	CALIBRATION_COEFF_COLOR_PosLog,
	CALIBRATION_COEFF_COLOR_PosPhys,

	CALIBRATION_DEBUG_ON

}_SCREEN_Calibration_enum;

void 	SCREEN_Calibration_main(int argNmb, char **argVal);

int 	SCREEN_Calibration_funcGet(int offs);
void 	SCREEN_Calibration_funcSet(int offs, int val);
void 	SCREEN_Calibration_setDefaultAllParam(int rst);
void 	SCREEN_Calibration_printInfo(void);

#endif /* LCD_SCREENS_SCREEN_CALIBRATIONLCD_H_ */
