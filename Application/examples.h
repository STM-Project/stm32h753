/*
 * examples.h
 *
 *  Created on: Feb 7, 2025
 *      Author: maraf
 */

#ifndef LCD_SCREENS_EXAMPLES_H_
#define LCD_SCREENS_EXAMPLES_H_

void 				  LCDEXAMPLE_RectangleGradient					(u32 COLOR_FillFrame, u32 COLOR_Frame, u32 COLOR_BkScreen, int FONT_ID_Descr);
void 				  LCDEXAMPLE_GradientCircleButtonAndSlider	(int FONT_ID_Title, int FONT_VAR_Title, u32 COLOR_FillMainFrame, u32 COLOR_Frame, u32 COLOR_BkScreen);
void 				  LCDEXAMPLE_LcdTxt									(int FONT_ID_Fonts, int FONT_VAR_Fonts, u32 COLOR_FillMainFrame, u32 COLOR_Frame, u32 COLOR_BkScreen);
void 				  LCDEXAMPLE_DrawLine								(u32 COLOR_Frame, u32 COLOR_BkScreen);
USER_GRAPH_PARAM LCDEXAMPLE_Graph									(int displayOption, USER_GRAPH_PARAM graph, int touchStart, int FONT_ID_Descr, int COLOR_BkScreen);
void 				  LCDEXAMPLE_CreateLoadDisplayFonts				(u32 COLOR_BkScreen);
void 				  LCDEXAMPLE_SpecificCircle						(uint32_t posBuff,uint32_t BkpSizeX, uint32_t x,uint32_t y, uint32_t lineLen, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor, int angleInclination, int iterix, float aa);
void 				  LCDEXAMPLE_BezierCurves							(u32 xMidd,u32 yMidd, u32 xRight,u32 yRight, u32 BkpColor);
void 				  LCDEXAMPLE_RotMovText								(int FONT_ID1, int FONT_ID2, int FONT_VAR_Start, u32 COLOR_FillMainFrame, u32 COLOR_Frame, u32 COLOR_BkScreen);
void 				  LCDEXAMPLE_RotVarText								(int FONT_ID1, int FONT_ID2, int FONT_VAR_Start, u32 COLOR_FillMainFrame, u32 COLOR_Frame, u32 COLOR_BkScreen);

void MEMEXAMPLE_MemoryFunc	(void);
void DBGEXAMPLE_Graph		(void);

#endif /* LCD_SCREENS_EXAMPLES_H_ */
