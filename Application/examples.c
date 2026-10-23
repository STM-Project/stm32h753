
#include "LCD_BasicGraphics.h"
#include "LCD_fonts_images.h"
#include "string_oper.h"
#include "LCD_Common.h"
#include "tim.h"
#include "timer.h"
#include "mini_printf.h"
#include <string.h>
#include "common.h"
#include "_debug.h"
#include "touch.h"


void LCDEXAMPLE_RectangleGradient(u32 COLOR_FillFrame, u32 COLOR_Frame, u32 COLOR_BkScreen, int FONT_ID_Descr)
{
	/* Call as:  LCDEXAMPLE_RectangleGradient(v.COLOR_FillFrame, v.COLOR_Frame, v.COLOR_BkScreen, v.FONT_ID_Descr); */
	SHAPE_PARAMS par1,par2,par3,par4,par5,par6;		/* for LCD_RoundRectangle2() for 'AllEdge' argument 'DIRECTIONS' has no effect */

	void _Str(int x,int y, char* txt){ LCD_StrDependOnColors(FONT_ID_VAR(FONT_ID_Descr,fontVar_40), x,y, txt, fullHight,0, unUsed, WHITE, 255, NoConstWidth); }

	LCD_BkFontTransparent(fontVar_40, FONT_ID_Descr);

	/* Direction in PARAM32() is color gradient direction for frame of shape		and		Direction at the end of arguments is color gradient direction for fill of shape */

	LCD_Rectangle2(0, LCD_X,LCD_Y, 220,190, 100, 40,  COLOR_FillFrame,BrightIncr(COLOR_FillFrame,0x55),   		  COLOR_FillFrame,ORANGE,  COLOR_BkScreen, 0.0, Down);  										_Str(260,200,"1");
	LCD_Rectangle2(0, LCD_X,LCD_Y, 160,240, 100, 40, BrightDecr(COLOR_Frame,0xA5),BrightDecr(COLOR_Frame,0xAE),   COLOR_FillFrame,YELLOW,  COLOR_BkScreen, 0.0, Up);  										_Str(200,250,"2");
	LCD_Rectangle2(0, LCD_X,LCD_Y, 270,240, 100, 40, BrightDecr(COLOR_Frame,0xA0),BrightIncr(COLOR_Frame,0x01),   COLOR_FillFrame,BrightDecr(COLOR_FillFrame,0x09),  COLOR_BkScreen, 0.0, Midd_Y);  _Str(310,250,"3");
	LCD_Rectangle2(0, LCD_X,LCD_Y, 380,240, 100, 40, BrightDecr(COLOR_Frame,0xA0),BrightIncr(COLOR_Frame,0x01),   COLOR_FillFrame,BrightDecr(COLOR_FillFrame,0x15),  COLOR_BkScreen, 0.0, Midd_X);  _Str(420,250,"4");

	LCD_Rectangle2(0, LCD_X,LCD_Y, 20,300, 100, 40,  0xFF202020,0xFF282828,   0xFF909090,BrightDecr(0xFF909090,0x20),  COLOR_BkScreen, 0.0, Midd_Y);  _Str(60,310,"5");
	//LCD_Rectangle2(0, LCD_X,LCD_Y, 160,300, 100, 40, 0xFF202020,0xFF282828,   0xFF909090,BrightDecr(0xFF909090,0x20),  v.COLOR_BkScreen, 0.0, Midd_Y2);
	LCD_Rectangle2(0, LCD_X,LCD_Y, 160,300, 100, 40, WHITE,(u32)bold8,   0xFF909090,BrightIncr(0xFF909090,0x20),  COLOR_BkScreen, 0.0, AllEdge); 	  _Str(200,310,"6");

	par3=LCD_Rectangle2(ToStructAndReturn, LCD_X,LCD_Y, 500,350, 100, 25, BrightDecr(COLOR_Frame,0xA0),BrightIncr(COLOR_Frame,0x01),  COLOR_FillFrame,BrightDecr(COLOR_FillFrame,0x35),  COLOR_BkScreen, 0.0, RightDown);
	par4=LCD_Rectangle2(ToStructAndReturn, LCD_X,LCD_Y, 610,360, 29, 100, BrightDecr(COLOR_Frame,0xA0),BrightIncr(COLOR_Frame,0x01),  COLOR_FillFrame,BrightDecr(COLOR_FillFrame,0x35),  COLOR_BkScreen, 0.0, RightDown);

	par1=LCD_RoundRectangle2(ToStructAndReturn,PARAM32(AllEdge, bold10,0x20,Rectangle),LCD_X,LCD_Y,35, 350,70,80 ,WHITE,BrightDecr(WHITE,0x20), 0xFFAAAAAA,BrightDecr(0xFFAAAAAA,0x20), COLOR_BkScreen, 0.0, Down);  /* 0x20 give effect 3D*/
		  LCD_RoundRectangle2(0,					 PARAM32(AllEdge2,bold10,0x20,Rectangle),LCD_X,LCD_Y,120,350,70,80 ,WHITE,BrightDecr(WHITE,0x40), 0xFFAAAAAA,BrightDecr(0xFFAAAAAA,0x40), COLOR_BkScreen, 0.0, Down);  /* 0x20 give effect 3D*/ 	  _Str(150,370,"7");

	par2=LCD_RoundRectangle2(ToStructAndReturn,PARAM32(Up,bold2,unUsed,Rectangle), 		LCD_X,LCD_Y,200,350,70,80, BrightDecr(COLOR_Frame,0x20),BrightIncr(COLOR_Frame,0x65), 0xFF808080,BrightDecr(0xFF808080,0x44), COLOR_BkScreen, 0.0, Left);
		  LCD_RoundRectangle2(0,					 PARAM32(Down,bold3,unUsed,Rectangle), 	LCD_X,LCD_Y,280,390,70,80, BrightDecr(COLOR_Frame,0x20),BrightIncr(COLOR_Frame,0x65), 0xFF808080,BrightDecr(0xFF808080,0x44), COLOR_BkScreen, 0.0, Down); 	  _Str(310,410,"8");

	LCDSHAPE_RoundRectangle(0,par1); 	  _Str(65,370,"9");
	LCDSHAPE_RoundRectangle(0,par2); 	  _Str(220,370,"10");

	par5=LCD_Rectangle2(ToStructAndReturn, LCD_X,LCD_Y, 570,240, 100, 40, COLOR_FillFrame,				   BrightIncr(COLOR_FillFrame,0x55),   COLOR_FillFrame,ORANGE,  COLOR_BkScreen, 0.25, Down);
	par6=LCD_Rectangle2(ToStructAndReturn, LCD_X,LCD_Y, 570,300, 100, 40, BrightDecr(COLOR_Frame,0xA5),BrightDecr(COLOR_Frame,0xAE), 		COLOR_FillFrame,YELLOW,  COLOR_BkScreen, 0.3, Up);

	LCDSHAPE_Rectangle(0,par3); 	  _Str(530,355,"11");
	LCDSHAPE_Rectangle(0,par4); 	  _Str(615,380,"12");
	LCDSHAPE_Rectangle(0,par5); 	  _Str(590,250,"13");
	LCDSHAPE_Rectangle(0,par6); 	  _Str(590,310,"14");

	LCD_RoundRectangle2(0, PARAM32(Down2,bold5,3,Rectangle), LCD_X,LCD_Y,710,290,70,80, BrightDecr(COLOR_Frame,0x40),0x202020, 0xFF808080,BrightDecr(0xFF808080,0x40), COLOR_BkScreen, 0.0, Down); 	 	  _Str(730,310,"15");
	LCD_RoundRectangle2(0, PARAM32(Down2,bold5,12,Rectangle), LCD_X,LCD_Y,710,390,70,80, BrightDecr(COLOR_Frame,0x40),0x202020, 0xFF808080,BrightDecr(0xFF808080,0x40), COLOR_BkScreen, 0.0, Down); 	  _Str(730,410,"16");

	LCD_RoundRectangle2(0,PARAM32(AllEdge,bold10,unUsed,Rectangle),LCD_X,LCD_Y,370,390,70,80 ,BrightDecr(WHITE,0x50), BrightDecr(WHITE,0x50), 0xFFA0A0A0,BrightDecr(0xFFA0A0A0,0x50), COLOR_BkScreen, 0.0, Down);
	LCD_RoundRectangle2(0,PARAM32(AllEdge,bold10,unUsed,Rectangle),LCD_X,LCD_Y,370,390,70,80 ,BrightDecr(WHITE,0x20),BrightDecr(WHITE,0x50),BrightDecr(0xFFAAAAAA,0x20) ,BrightDecr(0xFFAAAAAA,0x50), COLOR_BkScreen, 0.0, Down); 	 _Str(390,410,"17");

	LCD_RoundRectangle2(0,PARAM32(Shade,  bold7, unUsed,Frame),		LCD_X,LCD_Y,480,383,70,80, BrightDecr(0xFFA0A0A0,0x77),BrightDecr(0xFFA0A0A0,0x77),  unUsed,								unUsed, 							 COLOR_BkScreen, 0.0, Down);
	LCD_RoundRectangle2(0,PARAM32(AllEdge,bold10,unUsed,Rectangle),LCD_X,LCD_Y,480,383,70,80, 0xFFA0A0A0,						 0xFFA0A0A0,  	 					 BrightDecr(0xFFA0A0A0,0x30), BrightDecr(0xFFA0A0A0,0x10),COLOR_BkScreen, 0.0, Down); 	  _Str(500,400,"18");

/*	LCDSHAPE_Rectangle_Indirect(par3); 	  _Str(530,355,"11");
	LCDSHAPE_Rectangle_Indirect(par4); 	  _Str(615,380,"12");
	LCDSHAPE_Rectangle_Indirect(par5); 	  _Str(590,250,"13");
	LCDSHAPE_Rectangle_Indirect(par6);	  _Str(590,310,"14");
*/
	LCD_RoundRectangleTransp	 (0, LCD_X,LCD_Y,   5,210, 120,60, WHITE, 					  DARKYELLOW, READ_BGCOLOR, 0.85);		/* version of background transparent (mixed with 2 colors) */
	LCD_RoundFrameTransp			 (0, LCD_X,LCD_Y, 670,210, 120,60, WHITE, 					  unUsed, 	  READ_BGCOLOR, 0.5);
	LCD_BoldRoundRectangleTransp(0, LCD_X,LCD_Y,   5,210, 120,60, SetBold2Color(WHITE,6), DARKYELLOW, READ_BGCOLOR, 0.5);
	LCD_BoldRoundFrameTransp	 (0, LCD_X,LCD_Y, 670,210, 120,60, SetBold2Color(WHITE,6), unUsed, 	  READ_BGCOLOR, 0.5);
}

void LCDEXAMPLE_GradientCircleButtonAndSlider(int FONT_ID_Title, int FONT_VAR_Title, u32 COLOR_FillMainFrame, u32 COLOR_Frame, u32 COLOR_BkScreen)
{
	/* Call as:  LCDEXAMPLE_GradientCircleButtonAndSlider(v.FONT_ID_Title,v.FONT_VAR_Title,v.COLOR_FillFrame, v.COLOR_Frame, v.COLOR_BkScreen); */
	/* Common for noIndirect an Indirect */
	uint32_t CircleWidth= 150;
	SHAPE_PARAMS par={0}, par2={0};
	uint16_t calcWidth = LCD_CalculateCircleWidth(CircleWidth);

	LCD_SetCircleAA(0.0, 0.0);
	CorrectLineAA_on();
	LCD_BkFontTransparent(FONT_VAR_Title, FONT_ID_Title);

	par=LCD_GradientCircleSlider(ToStructAndReturn,LCD_X,LCD_Y, LCD_X-calcWidth-10-625, LCD_Y-calcWidth-30,CircleWidth,CircleWidth,   SetBold2Color(COLOR_BkScreen,23),COLOR_FillMainFrame,0x666666,0xBBBBBB,0, SetBold2Color(COLOR_FillMainFrame,15),0xC0C0C0,0x333333, COLOR_BkScreen,270,Round,0);
	par2=par;
	/* -------  END -------- */

	/* Methods for noIndirect */
	LCDSHAPE_GradientCircleSlider(0,par);
	LCD_StrDependOnColorsMidd(FONT_ID_VAR(FONT_ID_Title,FONT_VAR_Title), POS_SIZE_CIRCLEBUTTONSLIDER(par,0,0), "23", fullHight,0, BK_COLOR_CIRCLESLIDER(par), WHITE, 250, ConstWidth);

	LCDSHAPE_Create(0,LCD_X,LCD_Y,LCD_X-calcWidth-10-450, LCD_Y-calcWidth-30, SetParamWidthCircle(0,CircleWidth),CircleWidth, 					SetBold2Color(WHITE,30), 			COLOR_BkScreen, 		COLOR_BkScreen, RED,BLUE,0,360,RightDown,1);
	LCDSHAPE_Create(0,LCD_X,LCD_Y,LCD_X-calcWidth-10-150, LCD_Y-calcWidth-30, SetParamWidthCircle(Percent_Circle,CircleWidth),CircleWidth, SetBold2Color(COLOR_BkScreen,0), COLOR_FillMainFrame, COLOR_BkScreen, RED,BLUE,0,350,Left,1);
	LCDSHAPE_Create(0,LCD_X,LCD_Y,LCD_X-calcWidth-10, 	   LCD_Y-calcWidth-50, SetParamWidthCircle(Percent_Circle,CircleWidth),CircleWidth, SetBold2Color(COLOR_BkScreen,0), COLOR_FillMainFrame, COLOR_BkScreen, RED,BLUE,0,355,Right,0);
	/* -------  END -------- */

	StartMeasureTime_us(); StopMeasureTime_us("Time:");
/*
	LOOP_FOR(i,10){
		LCD_GradientCircleButton(0,LCD_X,LCD_Y,LCD_X-calcWidth-100-i, LCD_Y-calcWidth-50-i, CircleWidth,CircleWidth, SetBold2Color(BrightDecr(0xFFA0A0A0,0x80),18),BrightDecr(0xFFA0A0A0,0x80), BrightDecr(0xFFA0A0A0,0x80),COLOR_BkScreen,1);
	}
	LCD_GradientCircleButton(0,LCD_X,LCD_Y,LCD_X-calcWidth-100-10, LCD_Y-calcWidth-50-10, CircleWidth,CircleWidth, SetBold2Color(BrightDecr(WHITE,0x90),18),BrightDecr(0xFFA0A0A0,0x30), BrightDecr(0xFFA0A0A0,0x70),COLOR_BkScreen,1);
*/

	/* 2 Methods for Indirect */
	par=LCD_GradientCircleButton(ToStructAndReturn,CircleWidth,CircleWidth,LCD_X-calcWidth-10, LCD_Y-calcWidth-30, CircleWidth,CircleWidth, SetBold2Color(COLOR_BkScreen,18),LIGHTBLUE,DARKBLUE,COLOR_BkScreen,0);
	LCDSHAPE_GradientCircleButton_Indirect(par);
	LCD_StrDependOnColorsWindowMiddIndirect(0, POS_SIZE_CIRCLEBUTTONSLIDER_indirect(par,0,0), FONT_ID_VAR(FONT_ID_Title,FONT_VAR_Title), "987", fullHight,0, unUsed /*BK_COLOR_CIRCLESLIDER(par2)*/, WHITE, 248, ConstWidth); 	/* bkColor unUsed because called function: LCD_BkFontTransparent() */

	LCDSHAPE_GradientCircleSlider_Indirect(par2);
	LCD_StrDependOnColorsWindowMiddIndirect(0, POS_SIZE_CIRCLEBUTTONSLIDER_indirect(par2,0,0), FONT_ID_VAR(FONT_ID_Title,FONT_VAR_Title), "23", fullHight,0, unUsed /*BK_COLOR_CIRCLESLIDER(par2)*/, WHITE, 248, ConstWidth);
	/* -------  END -------- */
}

void LCDEXAMPLE_LcdTxt(int FONT_ID_Fonts, int FONT_VAR_Fonts, u32 COLOR_FillMainFrame, u32 COLOR_Frame, u32 COLOR_BkScreen)
{
	/* Call as:  LCDEXAMPLE_LcdTxt(v.FONT_ID_Fonts,v.FONT_VAR_Fonts,v.COLOR_FillFrame, v.COLOR_Frame, v.COLOR_BkScreen); */
	u32 txtColor;
	/* For shadow option is important to define parameters for index FONT_VAR_Fonts */
	LCD_TxtShadowInit(FONT_VAR_Fonts, FONT_ID_Fonts, COLOR_BkScreen, BK_Rectangle);

	/* For ..Indirect if bkColor=0 then DONT read bkColor! It is necessary define value of bkColor,  otherwise bkColor=0 means readout bkColor */

	/* 5 Methods for noIndirect */
	txtColor = BLACK;																																						/* TXTSHADE_NONE */
	LCD_STR_PARAM temp, new;		/* For all methods below 'new' =='temp' */																			/* TXTSHADECOLOR_DEEP_DIR( 0x777777, 4, RightDown) */
	new =  LCD_Txt(noDisplay, NULL, 0,0, LCD_X,LCD_Y, FONT_ID_Fonts, FONT_VAR_Fonts, 320,200, "12345", txtColor, COLOR_BkScreen, fullHight,0,250, NoConstWidth, 0x777777, 2, RightDown);	/* load parameters to 'new' */
	temp = LCD_Txt(noDisplay, &new, 0,0, LCD_X,LCD_Y, FONT_ID_Fonts, FONT_VAR_Fonts, 320,200, "12345", txtColor, COLOR_BkScreen, fullHight,0,250, NoConstWidth, 0x777777, 2, RightDown);	/* load parameters to 'new' and 'temp' */
	temp = LCD_Txt(Display,   NULL, 0,0, LCD_X,LCD_Y, FONT_ID_Fonts, FONT_VAR_Fonts, 320,200, "12345", txtColor, COLOR_BkScreen, fullHight,0,250, NoConstWidth, 0x777777, 2, RightDown);	/* 1 method */
	temp = LCD_Txt(Display,   &new, 0,0, LCD_X,LCD_Y, FONT_ID_Fonts, FONT_VAR_Fonts, 320,200, "12345", txtColor, COLOR_BkScreen, fullHight,0,250, NoConstWidth, 0x777777, 2, RightDown);	/* 2 method */

	LCD_Shape(new.txt.pos.x, new.txt.pos.y, LCD_Frame,  new.txt.size.w+new.shadow.deep, new.txt.size.h+new.shadow.deep,  RED,COLOR_BkScreen,COLOR_BkScreen);	/* test frame around the text */

	temp = LCD_Txt(DisplayViaStruct,&new,NO_TXT_ARGS);		/* 3 method */
	temp = LCD_TxtVar(&new,NULL);									/* 4 method */
	temp = LCD_TxtVar(&new,"new text");							/* 5 method */
	temp = temp;
	/* -------  END -------- */

	/* 5 Methods for Indirect */
	txtColor = WHITE;
	LCD_STR_PARAM temp2,new2;		/* For all methods below 'new2' =='temp2' */
	temp2 = LCD_Txt(noDisplay, &new2, 50,200, BK_SIZE_IS_TXT_SIZE, FONT_ID_Fonts, FONT_VAR_Fonts, 0,0, "12345", txtColor, COLOR_BkScreen, fullHight,0,250, NoConstWidth, 0x777777, 3, RightDown);	/* load parameters to 'new2' and 'temp2' */
	new2 =  LCD_Txt(noDisplay, NULL,  50,200, BK_SIZE_IS_TXT_SIZE, FONT_ID_Fonts, FONT_VAR_Fonts, 0,0, "12345", txtColor, COLOR_BkScreen, fullHight,0,250, NoConstWidth, 0x777777, 3, RightDown);	/* load parameters to 'new2' */
												/* {BkpSizeX,BkpSizeXY}=0 because is calculated as text length  */
	temp2 = LCD_Txt(DisplayIndirectViaStruct,&new2,NO_TXT_ARGS);	/* 1 method */
	temp2 = LCD_TxtVarInd(&new2, NULL);										/* 2 method */
	temp2 = LCD_TxtVarInd(&new2, "new text");								/* 3 method */
	temp2 = LCD_Txt(DisplayIndirect, &new2, 50,250, BK_SIZE_IS_TXT_SIZE, FONT_ID_Fonts, FONT_VAR_Fonts, 0,0, "abcde", txtColor, COLOR_BkScreen, fullHight,0,250, NoConstWidth, 0x777777, 6, RightDown);		/* 4 method */
	temp2 = LCD_Txt(DisplayIndirect, NULL,  50,250, BK_SIZE_IS_TXT_SIZE, FONT_ID_Fonts, FONT_VAR_Fonts, 0,0, "abcde", txtColor, COLOR_BkScreen, fullHight,0,250, NoConstWidth, 0x777777, 6, RightDown);		/* 5 method */
	temp2 = temp2;
	/* -------  END -------- */

	/* Various shapes of text field  (here it is necessary to define additional bkColor for field of font,  in this example RED is bkColor for font but COLOR_BkScreen for field of font) */
	LCD_TxtShadowInit(fontVar_40, FONT_ID_Fonts, COLOR_BkScreen, BK_Rectangle);
	LCD_Txt(DisplayIndirect, NULL, 50,200, BK_SIZE_IS_TXT_SIZE, FONT_ID_Fonts, fontVar_40, 0,0, "12345", BLACK, RED, fullHight,0,250, NoConstWidth, TXTSHADECOLOR_DEEP_DIR(0x777777,4,RightDown) /*TXTSHADE_NONE*/);

	LCD_TxtShadowInit(fontVar_40, FONT_ID_Fonts, COLOR_BkScreen, BK_Round);
	LCD_Txt(DisplayIndirect, NULL, 50,250, BK_SIZE_IS_TXT_SIZE, FONT_ID_Fonts, fontVar_40, 0,0, "12345", BLACK, RED, fullHight,0,250, NoConstWidth, TXTSHADECOLOR_DEEP_DIR(0x777777,4,RightDown) /*TXTSHADE_NONE*/);

	LCD_TxtShadowInit(fontVar_40, FONT_ID_Fonts, COLOR_BkScreen, BK_LittleRound);
	LCD_Txt(DisplayIndirect, NULL, 50,300, BK_SIZE_IS_TXT_SIZE, FONT_ID_Fonts, fontVar_40, 0,0, "12345", BLACK, RED, fullHight,0,250, NoConstWidth, TXTSHADECOLOR_DEEP_DIR(0x777777,4,RightDown) /*TXTSHADE_NONE*/);

	LCD_TxtShadowInit(fontVar_40, FONT_ID_Fonts, COLOR_BkScreen, BK_None);
	LCD_Txt(DisplayIndirect, NULL, 50,350, BK_SIZE_IS_TXT_SIZE, FONT_ID_Fonts, fontVar_40, 0,0, "12345", BLACK, RED, fullHight,0,250, NoConstWidth, TXTSHADECOLOR_DEEP_DIR(0x777777,4,RightDown) /*TXTSHADE_NONE*/);
	/* -------  END -------- */

	/* In future:
	 * - check if get we space sign as space shadow color in shadow option ?
	 */
}

void LCDEXAMPLE_DrawLine(u32 COLOR_Frame, u32 COLOR_BkScreen)
{
	/* 1 Show 'DrawLine()' for degree (45-11, 45) */
	CorrectLineAA_off();
	for(int i=0; i<15; i++)
		DrawLine(0,130,220+i*17, 150, 45-i, COLOR_Frame,LCD_X, 1.0, 1.0 ,COLOR_BkScreen,COLOR_BkScreen);

	CorrectLineAA_on();
	for(int i=0; i<15; i++)
		DrawLine(0,300,220+i*17, 150, 45-i, COLOR_Frame,LCD_X, 0.0, 0.0 ,COLOR_BkScreen,COLOR_BkScreen);		/* for i=12,13,14 we don`t have yet special correct (thicker line) */

	/* 2 Show 'DrawLine()' for degree (45, 45+11) */
	CorrectLineAA_off();
	for(int i=0; i<15; i++)
		DrawLine(0,430+i*10,350, 150, 45+i, COLOR_Frame,LCD_X, 1.0, 1.0 ,COLOR_BkScreen,COLOR_BkScreen);

	CorrectLineAA_on();
	for(int i=0; i<15; i++)
		DrawLine(0,620+i*10,350, 150, 45+i, COLOR_Frame,LCD_X, 0.0, 0.0 ,COLOR_BkScreen,COLOR_BkScreen);		/* for i=12,13,14 we don`t have yet special correct (thicker line) */

	/* In future:
	 * - if we haven`t 45 degree correct range we should give AA coefficient as  0.4 - 0.6	 and not 0.0  ->	This gives a better visual effect.
	 */
}

void MEMEXAMPLE_MemoryFunc(void)
{
	/* Common variables for all number of functions */
	int valueToMemory = 0x55;
	int retValue = 0;
	int retFlag = 0;
	int *pRetValue = NULL;
	char *pChar = NULL;
	char vChar = 0;
	char strBuff[] = "example";
	uint16_t buff16[] = {1,2,3};
	uint32_t buff32[] = {1,2,3};
	uint32_t lenBuff = 3;
	uint32_t freeMemorySize = 0;

	/* 1. Functions of volatile memory */
	retValue = FV(SetVal,0,valueToMemory);		/* retValue == valeToMemory */
	retValue = FV(SetVal,1,valueToMemory);		/* memory number 0,1,2,..  not exceed value _FV_MEMORY_SIZE */
	retValue = FV(SetVal,2,valueToMemory);

	retValue = FV(GetVal,0,NoUse);
	retValue = FV(GetVal,1,NoUse);
	retValue = FV(GetVal,2,NoUse);
	retValue = retValue;

	/* 2. Functions of volatile memory */
	pRetValue = _Int(valueToMemory);				/* address memory is auto-scroll when exceed value _BUFF_VAL_SIZE */
	pRetValue = (int*)_Uint8((uint8_t)valueToMemory);
	pRetValue = (int*)_Uint16((uint16_t)valueToMemory);
	pRetValue = (int*)_Uint32((uint32_t)valueToMemory);
	pRetValue = (int*)_Int16((int16_t)valueToMemory);
	pRetValue = (int*)_Int32((int32_t)valueToMemory);
	pRetValue = (int*)_Float((float)valueToMemory);
	pRetValue = pRetValue;

	/* 3. Functions of volatile memory */
	freeMemorySize = GETVAL_freeMemSize(0);
	pChar = GETVAL_ptr(0);		/* for this memory type use Semphr_sdram */
	pChar = GETVAL_ptr(1);
	pChar = GETVAL_ptr(2);
	vChar = GETVAL_char(0);
	retFlag = GETVAL_str(0, strBuff, strlen(strBuff));		/* When address memory 0,1,2,... exceeds return 1 and no-write otherwise return 0 and ok-write */
	valueToMemory = (int)GETVAL_int16(0);
	valueToMemory = (int)GETVAL_int32(0);
	retFlag = GETVAL_array16(0, buff16, lenBuff);
	retFlag = GETVAL_array32(0, buff32, lenBuff);

	retFlag = SETVAL_char(0, (char)valueToMemory);
	retFlag = SETVAL_char(1, (char)valueToMemory);
	retFlag = SETVAL_char(2, (char)valueToMemory);
	retFlag = SETVAL_str(0, strBuff, strlen(strBuff));
	retFlag = SETVAL_int16(0, (uint16_t)valueToMemory);
	retFlag = SETVAL_int32(0, (uint32_t)valueToMemory);
	retFlag = SETVAL_array16(0, buff16, lenBuff);
	retFlag = SETVAL_array32(0, buff32, lenBuff);

	_NOT_USED(freeMemorySize);
	_NOT_USED(retFlag);
	_NOT_USED(pChar);
	_NOT_USED(vChar);
}

void DBGEXAMPLE_KeyFunctions(void)
{
	double testVal = 0.0;
	void* FuncExample(void *arg1,void *arg2){ return NULL; }

	if(DEBUG_RcvStr("example")){ ; }

	/* 1. Debug option */
	_DBG_PARAM_NOWRAP("a",&testVal,_double,_Incr,_Double(0.1),_Double(20.0),"Test debug 1: ",FuncExample)		/* for 'a' increment value by step 0.1 */
	_DBG_PARAM_NOWRAP("b",&testVal,_double,_Decr,_Double(0.1),_Double( 1.0),"Test debug 1: ",FuncExample)		/* for 'b' decrement value by step 0.1 */

	/* 2. Debug option */
	_DBG2_PARAM_NOWRAP("a","A",&testVal,_double,_Incr,_Double(0.1),_Double(1.3),_Double(20.0),"Test debug 2: ",FuncExample,NULL)		/* for 'a' increment value by step 0.1 but for 'A' increment value by step 1.3 */
	_DBG2_PARAM_NOWRAP("b","B",&testVal,_double,_Decr,_Double(0.1),_Double(1.3),_Double( 1.0),"Test debug 2: ",FuncExample,NULL)		/* for 'b' decrement value by step 0.1 but for 'B' decrement value by step 1.3 */

	/* 3. Debug option */
	_DBG3_PARAM_NOWRAP("a","A","b","B",&testVal,_double,_Double(0.1),_Double(1.5),_Double(20.0),_Double(1.0),"Test debug 3: ",FuncExample,NULL)		/* for 'a' increment value by step 0.1 but for 'A' increment value by step 1.5 */
																																																	/* for 'b' decrement value by step 0.1 but for 'B' decrement value by step 1.5 */
}

USER_GRAPH_PARAM LCDEXAMPLE_Graph(int displayOption, USER_GRAPH_PARAM graph, int touchStart, int FONT_ID_Descr, int COLOR_BkScreen)
{
	/* Call as:  LCDEXAMPLE_Graph(1, testGraph, Touch_Chart_1, v.FONT_ID_Descr, v.COLOR_BkScreen); */
	void _ResetChartsTouchs(void){	LOOP_FOR(i,3){	LCD_TOUCH_DeleteSelectTouch(touchStart+i); }	 }

	switch(displayOption)
	{
	case 1:		/* 1 - GRAPH option */

		int offsMem = 0;
		int nrChart = 0;										/* must be always negative  	  must be always positive */
		int xPosChart = 50, 	yMiddPosChart = 250;			int yOffsToTopChart = -100,  yOffsToBottomChart = 100;				int widthChart = 700;
		int chartPtrPos = 70;

		u32 colorPosXY 	= RED;			int offsK_PosXY 	 = 20*LCD_X+0;
		u32 colorPosXYrep = DARKGREEN;	int offsK_PosXYrep = 40*LCD_X+0;

		u32 colorLine45degCorrectOFF = graph.colorLinePosXY;			 int offsK_45degCorrectOFF = graph.KoffsPosXY;
		u32 colorLineAAoff 			  = graph.colorLinePosXYrep;	 	 int offsK_AAoff 			   = graph.KoffsPosXYrep;

		if(touchStart) _ResetChartsTouchs();
		switch(graph.grad.bkType)		/* Alternating display of charts */
		{																																																																								  			/* Draw line (colorLine) but OutColor and InColor (needed for AA) is read from bkColor */
																																																																																	/* Alternative options:  0,0  ->  means READ_BK							 						Disp_no																															 							GRAD_None 																			 CHART_PTR_NONE 																													 GRID_NONE */
		case 0:	GRAPH_GetSamplesAndDraw(0, NR_MEM(offsMem,nrChart), LCD_X, XYPOS_YMIN_YMAX(xPosChart,yMiddPosChart, yOffsToTopChart,yOffsToBottomChart), POINTS_STEP_XYSCALE(widthChart,1.0, graph.par.scaleX,graph.par.scaleY), FUNC_TYPE(graph.funcType), LINE_COLOR(WHITE,0,0), AA_VAL(graph.AAoutCoeff,graph.AAinCoeff), 	DRAW_OPT(Disp_AA, 								  colorLine45degCorrectOFF, colorLineAAoff,   offsK_45degCorrectOFF, offsK_AAoff	  ), 	GRAD_YmaxYmin(ORANGE), 		GRAD_COEFF(1.0,0.0), graph.corr45degAA, CHART_PTR_DEFAULT(							  chartPtrPos,												FONT_ID_Descr), GRID_DOTS(15,15,0.35) );		LCDTOUCH_Set(xPosChart, yMiddPosChart-ABS(yOffsToTopChart), widthChart, ABS(yOffsToTopChart)+yOffsToBottomChart, ID_TOUCH_GET_ANY_POINT, touchStart+0, pressRelease);	break;
		case 1:	GRAPH_GetSamplesAndDraw(0, NR_MEM(offsMem,nrChart), LCD_X, XYPOS_YMIN_YMAX(xPosChart,yMiddPosChart, yOffsToTopChart,yOffsToBottomChart), POINTS_STEP_XYSCALE(widthChart,1.0, graph.par.scaleX,graph.par.scaleY), FUNC_TYPE(graph.funcType), LINE_COLOR(WHITE,0,0), AA_VAL(graph.AAoutCoeff,graph.AAinCoeff), 	DRAW_OPT(Disp_AA,  							     colorLine45degCorrectOFF, colorLineAAoff,   offsK_45degCorrectOFF, offsK_AAoff	  ), 	GRAD_Ystrip(DARKGREEN,51),	GRAD_COEFF(1.0,0.0), graph.corr45degAA, CHART_PTR_DEFAULT(							  chartPtrPos,												FONT_ID_Descr), GRID_LINE(20,15,0.15) );		LCDTOUCH_Set(xPosChart, yMiddPosChart-ABS(yOffsToTopChart), widthChart, ABS(yOffsToTopChart)+yOffsToBottomChart, ID_TOUCH_GET_ANY_POINT, touchStart+1, pressRelease);	break;
		case 2:  GRAPH_GetSamplesAndDraw(0, NR_MEM(offsMem,nrChart), LCD_X, XYPOS_YMIN_YMAX(xPosChart,yMiddPosChart, yOffsToTopChart,yOffsToBottomChart), POINTS_STEP_XYSCALE(widthChart,1.0, graph.par.scaleX,graph.par.scaleY), FUNC_TYPE(graph.funcType), LINE_COLOR(WHITE,0,0), AA_VAL(graph.AAoutCoeff,graph.AAinCoeff), 	DRAW_OPT(Disp_AA|Disp_posXY|Disp_posXYrep,  colorPosXY, 				  	 colorPosXYrep,    offsK_PosXY, 			 	offsK_PosXYrep), 	GRAD_Ycolor(RED,BLUE), 		GRAD_COEFF(1.0,0.0), graph.corr45degAA, CHART_PTR_PARAM  (LIGHTBLUE,DARKBLUE,18,chartPtrPos,1, WHITE,DARKYELLOW,80,30,10,10,1,FONT_ID_Descr), GRID_DEFAULT );					LCDTOUCH_Set(xPosChart, yMiddPosChart-ABS(yOffsToTopChart), widthChart, ABS(yOffsToTopChart)+yOffsToBottomChart, ID_TOUCH_GET_ANY_POINT, touchStart+2, pressRelease);	break;
																																																																																										 /* AA_VAL(0.0,0.0) and AA_ON  is the same 		DRAW_AA				   Disp_all 																																 	GRAD_PARAM(...) 																																																			 GRID_PARAM(...) */
																																																																																										 /* AA_VAL(1.0,1.0) and AA_OFF is the same 		DRAW_NO  only get samples without draw graph */
		}
		/* Touch Test GRAPH	-	use only if touch is necessary in function FILE_NAME(setTouch)() */
		/*
			case Touch_Chart_1:	if(GRAPH_IsMemReloaded(nrChart)) FILE_NAME(main)(LoadPartScreen,(char**)ppMain); 	GRAPH_ptrTouchService(pos.x,pos.y,nrChart);	break;		// Attention:  Charts use memory for samples pointed by CounterBusyBytesForFontsImages what is changed by load fonts function...
			case Touch_Chart_2:	if(GRAPH_IsMemReloaded(nrChart)) FILE_NAME(main)(LoadPartScreen,(char**)ppMain); 	GRAPH_ptrTouchService(pos.x,pos.y,nrChart);	break;		// 				... and you must reloaded charts if you have changed CounterBusyBytesForFontsImages before.
			case Touch_Chart_3:	if(GRAPH_IsMemReloaded(nrChart)) FILE_NAME(main)(LoadPartScreen,(char**)ppMain); 	GRAPH_ptrTouchService(pos.x,pos.y,nrChart);	break;
		 */
		return USER_GRAPH_PARAM_Zero;


	case 2:		/* 2 - GRAPH option */

		USER_GRAPH_PARAM par1={0}, par2={0}, par3={0};
		int offsMem2[3] = { 1000000, 2000000, 3000000 };
		int nrChart2[3] = { 0, 1, 2 };															/* must be always negative 	  						 must be always positive */
		int xPosChart2[3] = {50, 300, 0}, 	yMiddPosChart2[3] = {250, 250, 100};		int yOffsToTopChart2[3] = {-100, -80, -100},  yOffsToBottomChart2[3] = {100, 80, 100};			int widthChart2[3] = {200, 200, 240};
		int chartPtrPos2[3] = {70,70,70};

		int xOffsForInd = 550,  yOffsForInd = 250;

		GRAPH_GetSamples(NR_MEM(offsMem2[0],nrChart2[0]), XYPOS_YMIN_YMAX(xPosChart2[0],yMiddPosChart2[0], yOffsToTopChart2[0],yOffsToBottomChart2[0]), POINTS_STEP_XYSCALE(widthChart2[0],1.0, graph.par.scaleX,graph.par.scaleY), FUNC_TYPE(Func_sin));
		GRAPH_GetSamples(NR_MEM(offsMem2[1],nrChart2[1]), XYPOS_YMIN_YMAX(xPosChart2[1],yMiddPosChart2[1], yOffsToTopChart2[1],yOffsToBottomChart2[1]), POINTS_STEP_XYSCALE(widthChart2[1],1.0, graph.par.scaleX,graph.par.scaleY), FUNC_TYPE(Func_sin));
		GRAPH_GetSamples(NR_MEM(offsMem2[2],nrChart2[2]), XYPOS_YMIN_YMAX(xPosChart2[2],yMiddPosChart2[2], yOffsToTopChart2[2],yOffsToBottomChart2[2]), POINTS_STEP_XYSCALE(widthChart2[2],1.0, graph.par.scaleX,graph.par.scaleY), FUNC_TYPE(Func_sin));

		/* 1 option:  Via structure displaying of charts */
																		  /* alternative: LINE_AACOLOR(WHITE,0,0) */
		par1 = LCD_Chart(ToStructAndReturn, nrChart2[0], WIDTH_BK(LCD_X), LINE_AA_BKCOLOR(WHITE,0,0,COLOR_BkScreen), AA_VAL(graph.AAoutCoeff,graph.AAinCoeff), DRAW_AA, GRAD_YmaxYmin(ORANGE), GRAD_COEFF(1.0,0.0),graph.corr45degAA, CHART_PTR_DEFAULT(chartPtrPos2[0],FONT_ID_Descr), GRID_NONE);
		par2 = LCD_Chart(ToStructAndReturn, nrChart2[1], WIDTH_BK(LCD_X), LINE_AA_BKCOLOR(WHITE,0,0,COLOR_BkScreen), AA_VAL(graph.AAoutCoeff,graph.AAinCoeff), DRAW_AA, GRAD_YmaxYmin(ORANGE), GRAD_COEFF(1.0,0.0),graph.corr45degAA, CHART_PTR_DEFAULT(chartPtrPos2[1],FONT_ID_Descr), GRID_DEFAULT);
		/* int widthBk = GRAPH_GetNmbrPoints(2);																																					 					  DRAW_OPT(Disp_all, RED,DARKGREEN, 20*widthBk+0, 40*widthBk+0) */
		par3 = LCD_Chart(ToStructAndReturn, nrChart2[2], XY_WIN(xOffsForInd,yOffsForInd), LINE_AA_BKCOLOR(WHITE,0,0,COLOR_BkScreen), AA_VAL(graph.AAoutCoeff,graph.AAinCoeff), DRAW_AA, GRAD_YmaxYmin(ORANGE), GRAD_COEFF(1.0,0.0),graph.corr45degAA, CHART_PTR_DEFAULT(chartPtrPos2[2],FONT_ID_Descr), GRID_DEFAULT);

		LCDSHAPE_Chart(0,par1);
		LCDSHAPE_Chart(0,par2);
		/*	LCDSHAPE_Chart_Indirect(par3);	// ATTENTION !   This put after LCD_Display() as indirect displaying or use LCDEXAMPLE_Graph(3,par3,unUsed,unUsed,unUsed) */

		/* 2 option:  Simultaneous displaying of charts */
		/*	LCD_Chart(0, nrChart2[0],LCD_X, LINE_AA_BKCOLOR(WHITE,0,0,COLOR_BkScreen), AA_VAL(graph.AAoutCoeff,graph.AAinCoeff), DRAW_AA, GRAD_YmaxYmin(ORANGE), GRAD_COEFF(1.0,0.0),graph.corr45degAA, CHART_PTR_DEFAULT(chartPtrPos2[0],FONT_ID_Descr), GRID_NONE);
			LCD_Chart(0, nrChart2[1],LCD_X, LINE_AA_BKCOLOR(WHITE,0,0,COLOR_BkScreen), AA_VAL(graph.AAoutCoeff,graph.AAinCoeff), DRAW_AA, GRAD_YmaxYmin(ORANGE), GRAD_COEFF(1.0,0.0),graph.corr45degAA, CHART_PTR_DEFAULT(chartPtrPos2[1],FONT_ID_Descr), GRID_DEFAULT);
			xOffsForInd=0; yOffsForInd=0;  // for this option we omitted Indirect function
		*/
		if(touchStart) _ResetChartsTouchs();
		if(touchStart){  LCDTOUCH_Set(xPosChart2[0], yMiddPosChart2[0]-ABS(yOffsToTopChart2[0]), widthChart2[0], ABS(yOffsToTopChart2[0])+yOffsToBottomChart2[0], ID_TOUCH_GET_ANY_POINT, touchStart+0, pressRelease);
		  	  	  	  	  	  LCDTOUCH_Set(xPosChart2[1], yMiddPosChart2[1]-ABS(yOffsToTopChart2[1]), widthChart2[1], ABS(yOffsToTopChart2[1])+yOffsToBottomChart2[1], ID_TOUCH_GET_ANY_POINT, touchStart+1, pressRelease);
		  	  	  	  	  	  LCDTOUCH_Set(xOffsForInd, 	yOffsForInd		  -ABS(yOffsToTopChart2[2]), widthChart2[2], ABS(yOffsToTopChart2[2])+yOffsToBottomChart2[2], ID_TOUCH_GET_ANY_POINT, touchStart+2, pressRelease); }

		/* Touch Test GRAPH	-	use only if touch is necessary in function FILE_NAME(setTouch)() */
		/*
			case Touch_Chart_1:	if(GRAPH_IsMemReloaded(0)) FILE_NAME(main)(LoadPartScreen,(char**)ppMain); 	GRAPH_ptrTouchService(pos.x,pos.y,0);	break;		// Attention:  Charts use memory for samples pointed by CounterBusyBytesForFontsImages what is changed by load fonts function...
			case Touch_Chart_2:	if(GRAPH_IsMemReloaded(1)) FILE_NAME(main)(LoadPartScreen,(char**)ppMain); 	GRAPH_ptrTouchService(pos.x,pos.y,1);	break;		// 				... and you must reloaded charts if you have changed CounterBusyBytesForFontsImages before.
			case Touch_Chart_3:	if(GRAPH_IsMemReloaded(2)) FILE_NAME(main)(LoadPartScreen,(char**)ppMain); 	GRAPH_ptrTouchService(pos.x,pos.y,2);	break;
		 */
		return par3;
	case 3:		/* Indirect display option for  2-GRAPH option  */
					/* In Main() set:  USER_GRAPH_PARAM par3 = LCDEXAMPLE_Graph(2, testGraph, Touch_Chart_1, v.FONT_ID_Descr, v.COLOR_BkScreen);  */
					/* and after LCD_Display() put call indirect displaying as:  LCDEXAMPLE_Graph(3,par3,unUsed,unUsed,unUsed) 						*/
		LCDSHAPE_Chart_Indirect(graph);
		return USER_GRAPH_PARAM_Zero;

	default:		/* 3 - minimize displaying GRAPH */
		int memOffsForGraphOwner = 3000000;
		offsMem = 0;  nrChart = 0;		widthChart = 100;
		GRAPHFUNC_SetMemOffsForOwnFunc(memOffsForGraphOwner);
		GRAPH_GetSamplesAndDraw(0, NR_MEM(offsMem,nrChart), LCD_X, XYPOS_YMIN_YMAX(550,350, -100,100), POINTS_STEP_XYSCALE(widthChart,1.0, 1.0,1.0), FUNC_TYPE(Func_owner), LINE_COLOR(WHITE,0,0), AA_VAL(0.0,0.0), DRAW_OPT(Disp_AA,unUsed,unUsed,unUsed,unUsed), GRAD_None, GRAD_COEFF(unUsed,unUsed), 1, CHART_PTR_NONE, GRID_NONE );
		return USER_GRAPH_PARAM_Zero;
	}

	/* Debug Test GRAPH common for 1 and 2 GRAPH option	-  use only if debug is necessary in function FILE_NAME(debugRcvStr)() */
	/* int startFuncType = Func_sin,	stopFuncType = Func_lines6;

	_DBG3_PARAM_NOWRAP("a","A","z","Z",&graph.par.scaleX,_float,_Float(0.1),_Float( 1.5),_Float( 20.0),_Float(1.0),"Test Graph scaleX: ",MainFuncRefresh,NULL)
	_DBG3_PARAM_NOWRAP("s","S","x","X",&graph.par.scaleY,_float,_Float(1.0),_Float(10.0),_Float(100.0),_Float(1.0),"Test Graph scaleY: ",MainFuncRefresh,NULL)

	_DBG_PARAM_NOWRAP("d",&graph.funcType,_uint8,_Incr,_Uint8(1),_Uint8(stopFuncType),"Test Graph funcType: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("c",&graph.funcType,_uint8,_Decr,_Uint8(1),_Uint8(startFuncType),	 "Test Graph funcType: ",MainFuncRefresh)

	_DBG_PARAM_NOWRAP("f",&graph.AAoutCoeff,_float,_Incr,_Float(0.1),_Float(1.0),"Test Graph AA out: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("v",&graph.AAoutCoeff,_float,_Decr,_Float(0.1),_Float(0.0),"Test Graph AA out: ",MainFuncRefresh)

	_DBG_PARAM_NOWRAP("g",&graph.AAinCoeff,_float,_Incr,_Float(0.1),_Float(1.0),"Test Graph AA in: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("b",&graph.AAinCoeff,_float,_Decr,_Float(0.1),_Float(0.0),"Test Graph AA in: ",MainFuncRefresh)

	_DBG_PARAM_WRAP("y",&graph.corr45degAA,_int,_Wrap,_Int(1), _Int(0),_Int(1), "Test Graph AA 45deg: ",MainFuncRefresh)

	_DBG_PARAM_WRAP("q",&graph.grad.bkType,_int,_Wrap,_Int(1), _Int(Grad_YmaxYmin),_Int(Grad_Ycolor), "Test Graph grad type: ",MainFuncRefresh)
	 */

	/* In future:
	 * - conversion factor in displayed value
	 * - scrolling the chart
	 * - opt fast for the chart
	 */
}

void LCDEXAMPLE_CreateLoadDisplayFonts(u32 COLOR_BkScreen)
{
	/* 1. Creating all fonts from file.bmp to compressed file.ccf */
	LCD_CreateFileCFFfromAllFilesBMP();
	LCD_CkeckAllFontFilesCFF();
	LCD_DeleteAllFontAndImages();

	/* 2. Creating selected fonts from file.bmp to compressed file.ccf */
	LCD_DeleteAllFontAndImages();
	if( LCD_CreateFileCFFfromBMP(FONT_72, Comic_Saens_MS, RGB_RGB) < 0)
		_NOP;
	LCD_DeleteAllFontAndImages();
	if( LCD_CreateFileCFFfromBMP(FONT_72_bold, Comic_Saens_MS, RGB_RGB) < 0)
		_NOP;
	LCD_DeleteAllFontAndImages();
	if( LCD_CreateFileCFFfromBMP(FONT_72_italics, Comic_Saens_MS, RGB_RGB) < 0)
		_NOP;
	LCD_DeleteAllFontAndImages();

	/* 3. Loading selected fonts from compressed file.ccf to SDRAM */
	if( LCD_LoadFontFromFileCFF(FONT_72, Comic_Saens_MS, RGB_RGB, fontID_1) < 0)
		_NOP;
	if( LCD_LoadFontFromFileCFF(FONT_72_bold, Comic_Saens_MS, RGB_RGB, fontID_2) < 0)
		_NOP;
	if( LCD_LoadFontFromFileCFF(FONT_72_italics, Comic_Saens_MS, RGB_RGB, fontID_3) < 0)
		_NOP;

	/* 4. Displaying fonts from SDRAM before loading from compressed file.ccf */
	extern uint32_t pLcd[];
	LCD_DisplayTxt(0,pLcd,0,fontID_1, "+23.09 -76.9", 0,0, LCD_GetXSize(),LCD_GetYSize(), 5,0, 	COLOR_BkScreen,YELLOW, 0,NoConstWidth,fullHight,0);
	LCD_DisplayTxt(0,pLcd,0,fontID_2, "+23.09 -76.9", 0,0, LCD_GetXSize(),LCD_GetYSize(), 5,150, COLOR_BkScreen,YELLOW, 0,NoConstWidth,fullHight,0);
	LCD_DisplayTxt(0,pLcd,0,fontID_3, "+23.09 -76.9", 0,0, LCD_GetXSize(),LCD_GetYSize(), 5,300, COLOR_BkScreen,YELLOW, 0,ConstWidth,  fullHight,0);

	LCD_Show();
	while(1);

	/* In future:
	 * - opt fast for displaying or and creating fonts
	 */
}

void LCDEXAMPLE_SpecificCircle(uint32_t posBuff,uint32_t BkpSizeX, uint32_t x,uint32_t y, uint32_t lineLen, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor, int angleInclination, int iterix, float aa)
{
  /* Call as:  LCDEXAMPLE_SpecificCircle(0,LCD_X, 210,400, len_Line, WHITE, unUsed, v.COLOR_BkScreen, 0,deg_Line,AA_Line); */
	structPosition pos={x,y};
	if(aa>=1.0) CorrectLineAA_off();	else  CorrectLineAA_on();
	for(int i=angleInclination; i<360+angleInclination; i+=iterix)
		pos = DrawLine(0,pos.x,pos.y, lineLen, i, FrameColor,BkpSizeX, aa,aa ,BkpColor,BkpColor);

	/* Debug Test 	-  use only if debug is necessary in function FILE_NAME(debugRcvStr)() */
	/* int deg_Line=8, len_Line=12; float AA_Line=0.5;

	_DBG_PARAM_NOWRAP("m",&deg_Line,_uint16,_Incr,_Uint16(1),_Uint16(360),"incr deg: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("n",&deg_Line,_uint16,_Decr,_Uint16(1),_Uint16(1),	"decr deg: ",MainFuncRefresh)

	_DBG_PARAM_NOWRAP("v",&len_Line,_uint16,_Incr,_Uint16(1),_Uint16(100),"incr len: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("b",&len_Line,_uint16,_Decr,_Uint16(1),_Uint16(1),	 "decr len: ",MainFuncRefresh)

	_DBG_PARAM_NOWRAP("x",&AA_Line,_float,_Incr,_Float(0.1),_Float(1.0),"incr AA: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("c",&AA_Line,_float,_Decr,_Float(0.1),_Float(0.0),"decr AA: ",MainFuncRefresh)
	*/
}

void LCDEXAMPLE_BezierCurves(u32 xMidd,u32 yMidd, u32 xRight,u32 yRight, u32 BkpColor)
{
	u32 memOffsForSamples 	 = 2000000;
	u32 memOffsForSamplesAVR = 5000000;

	structPosition pos[3]={{120,250},{xMidd,yMidd},{xRight,yRight}};
	structPosition pos2[3]={0};
	structPosition *posA, *posC;
	int posAi=0, posCi=0;
	u32 *wsk_line1, *wsk_line2;		u32 len_line1, len_line2;
	float stepVal = 0.0;

	extern char* GETVAL_ptr(uint32_t nrVal);

	posA = (structPosition*) GETVAL_ptr (memOffsForSamples);			/* calculated */
	posC = (structPosition*) GETVAL_ptr (memOffsForSamplesAVR);		/* calculated and filtered AVR */

	wsk_line1 = (u32*) GETVAL_ptr (100000);
	wsk_line2 = (u32*) GETVAL_ptr (200000);

	CorrectLineAA_off();

	LCD_Line(0, POS_START_STOP( pos[0], pos[1]), WHITE,LCD_X, AA_OFF ,BKCOLOR_INOUT(BkpColor));		len_line1 = LCD_CopyPosLinePointsToBuff(wsk_line1);
	LCD_Line(0, POS_START_STOP( pos[1], pos[2]), WHITE,LCD_X, AA_OFF ,BKCOLOR_INOUT(BkpColor));		len_line2 = LCD_CopyPosLinePointsToBuff(wsk_line2);

	for(  int i=0; i<1000; ++i)
	{
		pos2[0] = LCD_GetPosLinePointFromBuff(wsk_line1,  VALPERC(len_line1,stepVal), LCD_X );
		pos2[1] = LCD_GetPosLinePointFromBuff(wsk_line2,  VALPERC(len_line2,stepVal), LCD_X );

		LCD_LinePoints(0, POS_START_STOP(pos2[0],pos2[1]), WHITE,LCD_X);			 pos2[2] = LCD_GetPosLinePoint( VALPERC(LCD_GetNmbrLinePoints(),stepVal), LCD_X );

		if(posAi>0 && (posA+posAi-1)->x == pos2[2].x) posAi--;		 /* if the same position X then overwrite previous position */

			  if(posAi>2 && posA[posAi-2].x == pos2[2].x);   /* if position is backward then omitted */
		else if(posAi>3 && posA[posAi-3].x == pos2[2].x);   /* if position is backward then omitted */
		else	posA[posAi++]=pos2[2];

		stepVal+=0.1;
		Dbg(1,"i");
	}


	for(  int i=0; i<posAi; ++i)  	/* Displaying indirect on the screen */
		LCD_Buffer(LCD_X, 200+posA[i].x, posA[i].y, YELLOW);

	for(  int i=0; i<posAi; ++i){  	/* Displaying with GRAPH_function() on the screen */
		posA[i].x -= pos[0].x;
		posA[i].y = pos[0].y - posA[i].y;
	}

	posCi = posAi;

	for(  int i=0; i<posAi;   ++i)  posC[i] = posA[i];
	for(  int i=2; i<posAi-2; ++i)  /* filtering AVR */
		posC[i].y = (posA[i-2].y + posA[i-1].y + posA[i].y + posA[i+1].y + posA[i+2].y ) / 5;

	for(  int i=0; i<posAi;   ++i)  posA[i] = posC[i];
	for(  int i=2; i<posAi-2; ++i)  /* filtering AVR */
		posC[i].y = (posA[i-2].y + posA[i-1].y + posA[i].y + posA[i+1].y + posA[i+2].y ) / 5;


	GRAPHFUNC_SetMemOffsForOwnFunc(memOffsForSamplesAVR);
	GRAPH_GetSamplesAndDraw(0, NR_MEM(0,0), LCD_X, XYPOS_YMIN_YMAX(400+120,240, -240,240), POINTS_STEP_XYSCALE(posCi-3,1.0, 1.0,1.0), FUNC_TYPE(Func_owner), LINE_COLOR(WHITE,0,0), AA_VAL(0.0,0.0), DRAW_OPT(Disp_AA,  unUsed,unUsed,unUsed,unUsed), 	GRAD_None, GRAD_COEFF(unUsed,unUsed), 1, CHART_PTR_NONE, GRID_NONE );

	/* Debug Test 	-  use only if debug is necessary in function FILE_NAME(debugRcvStr)() */
	/* u32 xMidd=250,yMidd=400,   xRight=315,yRight=31;   int distStep=10;

	_DBG_PARAM_NOWRAP("w",&xMidd, _uint32,_Incr,_Uint32(distStep),_Uint32(LCD_X-distStep),"xMidd: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("q",&xMidd, _uint32,_Decr,_Uint32(distStep),_Uint32(distStep), 	  "xMidd: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("r",&xRight,_uint32,_Incr,_Uint32(distStep),_Uint32(LCD_X-distStep),"xRight: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("e",&xRight,_uint32,_Decr,_Uint32(distStep),_Uint32(distStep), 	  "xRight: ",MainFuncRefresh)

	_DBG_PARAM_NOWRAP("z",&yMidd, _uint32,_Incr,_Uint32(distStep),_Uint32(LCD_Y-distStep),"yMidd: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("a",&yMidd, _uint32,_Decr,_Uint32(distStep),_Uint32(distStep), 	  "yMidd: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("s",&yRight,_uint32,_Incr,_Uint32(distStep),_Uint32(LCD_Y-distStep),"yRight: ",MainFuncRefresh)
	_DBG_PARAM_NOWRAP("x",&yRight,_uint32,_Decr,_Uint32(distStep),_Uint32(distStep), 	  "yRight: ",MainFuncRefresh)
	 */
}

void LCDEXAMPLE_RotMovText(int FONT_ID1, int FONT_ID2, int FONT_VAR_Start, u32 COLOR_FillMainFrame, u32 COLOR_Frame, u32 COLOR_BkScreen)
{
	/* Call as:  LCDEXAMPLE_RotMovText(v.FONT_ID_Speed, v.FONT_ID_Descr, fontVar_28, v.COLOR_FillFrame, v.COLOR_Frame, v.COLOR_BkScreen); */
	char txt1[] = "Analizator z serii RSA6000 oferuje wy"ś"wietlanie w wielu oknach, doskona"ł"y poziom szumu "ś"redniego wy"ś"wietlanego (DANL), najwy"ż"sz"ą" klas"ę" szumu fazowego i szeroki zakres dynamiczny";
	char txt2[] = "Stanowi cenne wsparcie zar"ó"wno dla laboratori"ó"w, jak i dla produkcji oraz test"ó"w terenowych";
	char txt3[] = "W komplecie s"ą" dwie oryginalne sondy na 500MHz";
	StructTxtPxlLen lenStr1, lenStr2, lenStr3;
	int fontId1 = FONT_ID1,		fontId2 = FONT_ID1,		fontId3 = FONT_ID2,		margin = 8;
	int widthWin1 = 109,			heightWin1 = LCD_GetFontHeight(fontId1) + margin;
	int widthWin2 = 110,			heightWin2 = LCD_GetFontHeight(fontId2) + margin;
	int widthWin3 = 111,			heightWin3 = LCD_GetFontHeight(fontId3) + margin;

	LCD_RoundRectangle(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(0,lenStr1,SetPos,10 ), LCD_posY(0,lenStr1,SetPos,240), widthWin1,heightWin1, COLOR_GRAY(0xAA),DARKYELLOW,COLOR_BkScreen);		/* lenStr1..3 are used in LCD_Xpos()-LCD_posX() or LCD_Ypos()-LCD_posY only for 'IncPos' */
	LCD_RoundRectangle(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(1,lenStr2,SetPos,130), LCD_posY(1,lenStr2,SetPos,240), widthWin2,heightWin2, COLOR_GRAY(0xAA),DARKBLUE,  COLOR_BkScreen);
	LCD_RoundFrame		(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(2,lenStr3,SetPos,260), LCD_posY(2,lenStr3,SetPos,240), widthWin3,heightWin3, COLOR_GRAY(0xAA),unUsed,	 COLOR_BkScreen);
	LCD_StrMovH( FONT_VAR_Start+0,Rotate_0,0,widthWin1-margin,fontId1, LCD_posX(0,lenStr1,GetPos,+margin/2), LCD_posY(0,lenStr1,GetPos,+margin/2), txt1, fullHight,0, DARKYELLOW,	 	0,NoConstWidth );
	LCD_StrMovH( FONT_VAR_Start+1,Rotate_0,0,widthWin2-margin,fontId2, LCD_posX(1,lenStr2,GetPos,+margin/2), LCD_posY(1,lenStr2,GetPos,+margin/2), txt2, fullHight,0, DARKBLUE,		0,NoConstWidth );
	LCD_StrMovH( FONT_VAR_Start+2,Rotate_0,0,widthWin3-margin,fontId3, LCD_posX(2,lenStr3,GetPos,+margin/2), LCD_posY(2,lenStr3,GetPos,+margin/2), txt3, fullHight,0, COLOR_BkScreen,0,NoConstWidth );		/* READ_BGCOLOR here it does not run */

	LCD_RoundFrame		(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(0,lenStr1,SetPos,130), LCD_posY(0,lenStr1,SetPos,210), widthWin1,heightWin1, COLOR_GRAY(0xAA),unUsed,	 COLOR_BkScreen);
	LCD_RoundRectangle(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(1,lenStr2,SetPos,380), LCD_posY(1,lenStr2,SetPos,210), heightWin2,widthWin2, COLOR_GRAY(0xAA),DARKYELLOW,COLOR_BkScreen);
	LCD_RoundFrame		(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(2,lenStr3,SetPos,415), LCD_posY(2,lenStr3,SetPos,210), heightWin3,widthWin3, COLOR_GRAY(0xAA),unUsed,	 COLOR_BkScreen);
	LCD_StrChangeColorMovH( FONT_VAR_Start+3,Rotate_0,  0,widthWin1-margin,fontId1, LCD_posX(0,lenStr1,GetPos,+margin/2), LCD_posY(0,lenStr1,GetPos,+margin/2), txt1,fullHight,0, COLOR_BkScreen,LIGHTRED,unUsed,NoConstWidth );
	LCD_StrChangeColorMovH( FONT_VAR_Start+4,Rotate_90, 0,widthWin2-margin,fontId2, LCD_posX(1,lenStr2,GetPos,+margin/2), LCD_posY(1,lenStr2,GetPos,+margin/2), txt2,fullHight,0, DARKYELLOW,    DARKBLUE,unUsed,NoConstWidth );
	LCD_StrChangeColorMovH( FONT_VAR_Start+5,Rotate_180,0,widthWin3-margin,fontId3, LCD_posX(2,lenStr3,GetPos,+margin/2), LCD_posY(2,lenStr3,GetPos,+margin/2), txt3,fullHight,0, COLOR_BkScreen,ORANGE,  unUsed,NoConstWidth );

	LCD_RoundFrame		(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(0,lenStr1,SetPos,10 ), LCD_posY(0,lenStr1,SetPos,290), widthWin1,5*heightWin1, COLOR_GRAY(0xAA),unUsed,			  COLOR_BkScreen );
	LCD_RoundRectangle(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(1,lenStr2,SetPos,130), LCD_posY(1,lenStr2,SetPos,290), widthWin2,2*heightWin2, COLOR_GRAY(0xAA),COLOR_GRAY(0x29),COLOR_BkScreen );
	LCD_StrChangeColorMovV( FONT_VAR_Start+6,Rotate_0,0,widthWin1-margin,5*heightWin1-margin,fontId1, LCD_posX(0,lenStr1,GetPos,+margin/2), LCD_posY(0,lenStr1,GetPos,+margin/2), txt1,fullHight,0, COLOR_BkScreen,  LIGHTBLUE, SEL_ITEM(1,BLACK)/*unUsed*/, NoConstWidth );
	LCD_StrMovV           ( FONT_VAR_Start+7,Rotate_0,0,widthWin2-margin,2*heightWin2-margin,fontId2, LCD_posX(1,lenStr2,GetPos,+margin/2), LCD_posY(1,lenStr2,GetPos,+margin/2), txt2,fullHight,0, COLOR_GRAY(0x29),	  	  	    0,  	 							   NoConstWidth );

	LCD_StrChangeColorMovV( FONT_VAR_Start+8,Rotate_90,0,widthWin1,1*heightWin1,FONT_ID2,510,210,txt2,fullHight,0,COLOR_BkScreen,LIGHTMAGENTA, SEL_ITEM(2,GRAY)/*unUsed*/ ,NoConstWidth );
	LCD_StrMovV           ( FONT_VAR_Start+9,Rotate_90,0,widthWin2,5*heightWin2,FONT_ID1,640,210,txt1,fullHight,0,MYGRAY,				  			 0,	 	 						  NoConstWidth );

	LCD_StrChangeColorMovV( FONT_VAR_Start+10,Rotate_180,0,widthWin1,2*heightWin1,FONT_ID2,510,340,txt2,fullHight,0,COLOR_BkScreen,WHITE, 	 SEL_ITEM(3,LIGHTRED)/*unUsed*/ ,NoConstWidth );
	LCD_StrChangeColorMovV( FONT_VAR_Start+11,Rotate_180,0,widthWin2,5*heightWin2,FONT_ID1,640,340,txt1,fullHight,0,LIGHTGRAY,		 DARKBLUE,SEL_ITEM(4,DARKGRAY)/*unUsed*/ ,NoConstWidth );


/* Another method displaying */
/*
	lenStr1 = LCD_StrChangeColorMovV( FONT_VAR_Start+0,Rotate_0, 0, 100,50, fontId1,LCD_posX(0,lenStr1,SetPos,480),LCD_posY(0,lenStr1,SetPos,240), txt1,fullHight,0,COLOR_BkScreen,ORANGE,unUsed,NoConstWidth );
	lenStr2 = LCD_StrMovH			  ( FONT_VAR_Start+1,Rotate_90,0, 109,    fontId2,LCD_posX(1,lenStr2,SetPos,100),LCD_posY(1,lenStr2,SetPos,240), txt2,fullHight,0,COLOR_BkScreen,		  0,  	NoConstWidth);
	LCD_RoundFrame(0,LCD_GetXSize(),LCD_GetYSize(),		LCD_posX(0,lenStr1,GetPos,-margin/2),LCD_posY(0,lenStr1,GetPos,-margin/2), lenStr1.inPixel+margin,lenStr1.height+margin,COLOR_GRAY(0xAA),unUsed,COLOR_BkScreen);
	LCD_RoundFrame(0,LCD_GetXSize(),LCD_GetYSize(),		LCD_posX(1,lenStr2,GetPos,-margin/2),LCD_posY(1,lenStr2,GetPos,-margin/2), lenStr2.inPixel+margin,lenStr2.height+margin,COLOR_GRAY(0xAA),unUsed,COLOR_BkScreen);
*/


/* Refresh Test 	-  use only if cyclic refresh is necessary in function FILE_NAME(timer)() */

/*	if(LCD_IsRefreshScreenTimeout(refresh_1,60)) LCD_StrMovHIndirect(fontVar_28,1);
	if(LCD_IsRefreshScreenTimeout(refresh_2,60)) LCD_StrMovHIndirect(fontVar_29,1);
	if(LCD_IsRefreshScreenTimeout(refresh_3,20)) LCD_StrMovHIndirect(fontVar_30,1);

	if(LCD_IsRefreshScreenTimeout(refresh_4,60)) LCD_StrMovHIndirect(fontVar_31,1);
	if(LCD_IsRefreshScreenTimeout(refresh_5,60)) LCD_StrMovHIndirect(fontVar_32,1);
	if(LCD_IsRefreshScreenTimeout(refresh_6,20)) LCD_StrMovHIndirect(fontVar_33,1);

	if(LCD_IsRefreshScreenTimeout(refresh_7,60)) LCD_StrMovVIndirect(fontVar_34,1);
	if(LCD_IsRefreshScreenTimeout(refresh_8,20)) LCD_StrMovVIndirect(fontVar_35,1);

	if(LCD_IsRefreshScreenTimeout(refresh_9,60))  LCD_StrMovVIndirect(fontVar_36,1);
	if(LCD_IsRefreshScreenTimeout(refresh_10,20)) LCD_StrMovVIndirect(fontVar_37,1);

	if(LCD_IsRefreshScreenTimeout(refresh_11,60)) LCD_StrMovVIndirect(fontVar_38,1);
	if(LCD_IsRefreshScreenTimeout(refresh_12,20)) LCD_StrMovVIndirect(fontVar_39,1);
*/

/* Another method displaying */
/*	if(LCD_IsRefreshScreenTimeout(refresh_1,60)) LCD_StrMovVIndirect(fontVar_28,1);
	if(LCD_IsRefreshScreenTimeout(refresh_2,60)) LCD_StrMovHIndirect(fontVar_29,1);
*/

}

void LCDEXAMPLE_RotVarText(int FONT_ID1, int FONT_ID2, int FONT_VAR_Start, u32 COLOR_FillMainFrame, u32 COLOR_Frame, u32 COLOR_BkScreen)
{
	/* Call as:  LCDEXAMPLE_RotVarText(v.FONT_ID_Speed, v.FONT_ID_Descr, fontVar_28, v.COLOR_FillFrame, v.COLOR_Frame, v.COLOR_BkScreen); */
	char txt1[] = "Analizator z serii RSA6000";
	char txt2[] = "Stanowi cenne wsparcie";
	char txt3[] = "Doskona"ł"y poziom szumu "ś"redniego wy"ś"wietlanego (DANL), najwy"ż"sz"ą" klas"ę" szumu fazowego i szeroki zakres dynamiczny";
	StructTxtPxlLen lenStr1, lenStr2;
	int fontId1 = FONT_ID1,		fontId2 = FONT_ID1,		margin = 8;
	int widthWin1 = LCD_GetWholeStrPxlWidth(fontId1,txt1,0,NoConstWidth) + margin,			heightWin1 = LCD_GetFontHeight(fontId1) + margin;
	int widthWin2 = LCD_GetWholeStrPxlWidth(fontId2,txt2,0,NoConstWidth) + margin,			heightWin2 = LCD_GetFontHeight(fontId2) + margin;
	int widthWin3 = 80;

	LCD_RoundRectangle(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(0,lenStr1,SetPos,10 ), LCD_posY(0,lenStr1,SetPos,240), widthWin1,heightWin1, COLOR_GRAY(0xAA),DARKRED,  COLOR_BkScreen);		/* lenStr1..2 are used in LCD_Xpos()-LCD_posX() or LCD_Ypos()-LCD_posY only for 'IncPos' */
	LCD_RoundRectangle(0,LCD_GetXSize(),LCD_GetYSize(), LCD_posX(1,lenStr2,SetPos,210), LCD_posY(1,lenStr2,SetPos,210), heightWin2,widthWin2, COLOR_GRAY(0xAA),DARKBLUE, COLOR_BkScreen);
	LCD_StrRot				( Rotate_0,	fontId1, LCD_posX(0,lenStr1,GetPos,+margin/2), LCD_posY(0,lenStr1,GetPos,+margin/2), txt1,fullHight,0,DARKRED,		     1,	   NoConstWidth );
	LCD_StrChangeColorRot( Rotate_90,fontId2, LCD_posX(1,lenStr2,GetPos,+margin/2), LCD_posY(1,lenStr2,GetPos,+margin/2), txt2,fullHight,0,DARKBLUE,MAGENTA,unUsed,NoConstWidth );

	lenStr1=LCD_StrChangeColorRotWin( Rotate_0,  widthWin3,fontId1, LCD_posX(0,lenStr1,SetPos,60), LCD_posY(0,lenStr1,SetPos,280), txt3,fullHight,0,COLOR_BkScreen,CYAN,	SEL_ITEM(2,DARKBLUE),NoConstWidth,unUsed );
	lenStr2=LCD_StrRotWin			  ( Rotate_180,widthWin3,fontId2, LCD_posX(1,lenStr2,SetPos,600),LCD_posY(1,lenStr2,SetPos,210), txt3,fullHight,0,COLOR_BkScreen, 		1,	  					   NoConstWidth,unUsed );
	LCD_RoundFrame(0,LCD_GetXSize(),LCD_GetYSize(),	LCD_posX(0,lenStr1,GetPos,-margin/2),LCD_posY(0,lenStr1,GetPos,-margin/2), lenStr1.inPixel+margin,lenStr1.height+margin,COLOR_GRAY(0xAA),unUsed,COLOR_BkScreen);
	LCD_RoundFrame(0,LCD_GetXSize(),LCD_GetYSize(),	LCD_posX(1,lenStr2,GetPos,-margin/2),LCD_posY(1,lenStr2,GetPos,-margin/2), lenStr2.inPixel+margin,lenStr2.height+margin,COLOR_GRAY(0xAA),unUsed,COLOR_BkScreen);

	lenStr1=LCD_StrRotVar			  ( FONT_VAR_Start+0,Rotate_90, fontId1, LCD_posX(0,lenStr1,SetPos,520),LCD_posY(0,lenStr1,SetPos,210), txt1,fullHight,0,COLOR_BkScreen,		   0,NoConstWidth,COLOR_BkScreen );		/* It can be also first LCD_Str..() and than LCD_Round..() */
	lenStr2=LCD_StrChangeColorRotVar( FONT_VAR_Start+1,Rotate_180,fontId2, LCD_posX(1,lenStr2,SetPos,560),LCD_posY(1,lenStr2,SetPos,210), txt2,fullHight,0,COLOR_BkScreen,MAGENTA,0,NoConstWidth,COLOR_BkScreen );
	LCD_RoundFrame(0,LCD_GetXSize(),LCD_GetYSize(),	LCD_posX(0,lenStr1,GetPos,-margin/2),LCD_posY(0,lenStr1,GetPos,-margin/2), lenStr1.inPixel+margin,lenStr1.height+margin,COLOR_GRAY(0xAA),unUsed,COLOR_BkScreen);
	LCD_RoundFrame(0,LCD_GetXSize(),LCD_GetYSize(),	LCD_posX(1,lenStr2,GetPos,-margin/2),LCD_posY(1,lenStr2,GetPos,-margin/2), lenStr2.inPixel+margin,lenStr2.height+margin,COLOR_GRAY(0xAA),unUsed,COLOR_BkScreen);

	/* Refresh Test 	-  use only if cyclic refresh is necessary in function FILE_NAME(timer)() */

/*		  if(LCD_IsRefreshScreenTimeout(refresh_1,2000)){ LCD_StrRotVarIndirect(fontVar_28,INT2STR(6)); 	LCD_RefreshScreenClear(refresh_2); }
	else if(LCD_IsRefreshScreenTimeout(refresh_2,1000))  LCD_StrRotVarIndirect(fontVar_28,"123450000000000000000");

		  if(LCD_IsRefreshScreenTimeout(refresh_3,2000)){ LCD_StrChangeColorRotVarIndirect(fontVar_29,INT2STR(7)); 	 LCD_RefreshScreenClear(refresh_4);}
	else if(LCD_IsRefreshScreenTimeout(refresh_4,1000))  LCD_StrChangeColorRotVarIndirect(fontVar_29,"5432100000000000000");
*/

}


