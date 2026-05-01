/*
 * debug.h
 *
 *  Created on: 02.04.2021
 *      Author: Elektronika RM
 */

#ifndef GENERAL_UTILITIES__DEBUG_H_
#define GENERAL_UTILITIES__DEBUG_H_

#include "stm32h7xx_hal.h"
#include "common.h"

#define _DBG_PARAM_NOWRAP(dbgStr,ptrData,typeData,dataAct,step,minMax,descr,func)	else if(DEBUG_RcvStr(dbgStr)){ DEBUG_TestFunction(ptrData,typeData,dataAct,step,NULL,minMax,descr,func,NULL); }
#define _DBG_PARAM_WRAP(dbgStr,ptrData,typeData,dataAct,step,min,max,descr,func)		else if(DEBUG_RcvStr(dbgStr)){ DEBUG_TestFunction(ptrData,typeData,dataAct,step,min,max,descr,func,NULL); }

#define _DBG2_PARAM_NOWRAP(dbgStr,dbgStr2,ptrData,typeData,dataAct,step,step2,minMax,descr,func,func2)	else if(DEBUG_RcvStr(dbgStr)){ DEBUG_TestFunction(ptrData,typeData,dataAct,step,NULL,minMax,descr,func,func2); }\
																																			else if(DEBUG_RcvStr(dbgStr2)){ DEBUG_TestFunction(ptrData,typeData,dataAct,step2,NULL,minMax,descr,func,func2); }

#define _DBG3_PARAM_NOWRAP(dbgStrIncr,dbgStrIncr2,dbgStrDecr,dbgStrDecr2,ptrData,typeData,step,step2,max,min,descr,func,func2)  _DBG2_PARAM_NOWRAP(dbgStrIncr,dbgStrIncr2,ptrData,typeData,_Incr,step,step2,max,descr,func,func2)\
																												   										 	  	  	  _DBG2_PARAM_NOWRAP(dbgStrDecr,dbgStrDecr2,ptrData,typeData,_Decr,step,step2,min,descr,func,func2)

typedef enum{
	font,
	bkg
}FONT_BKG_COLOR;

#define Clr_	"\x1B\x5B\x32\x4A"

#define Red_	"\x1b[31m"
#define Gre_	"\x1b[32m"
#define Yel_	"\x1b[33m"
#define Blu_	"\x1b[34m"
#define Mag_	"\x1b[35m"
#define Cya_	"\x1b[36m"

#define CoBl_	"\x1b[38;2;0;0;0m"
#define CoGr_	"\x1b[38;2;140;140;140m"

#define CoR_	"\x1b[38;2;255;130;110m"
#define CoG_	"\x1b[38;2;0;205;115m"
#define CoG2_	"\x1b[38;2;100;255;100m"
#define CoB_	"\x1b[38;2;50;190;255m"

#define BkR_	"\x1b[48;2;160;70;45m"
#define BkW_	"\x1b[48;2;255;255;255m"

#define Bol_	"\x1b[1m"
#define Ita_	"\x1b[4m"
#define _X_		"\x1b[0m"

/* ---- User Colors Definitions -----*/
#define Co1_	"\x1b[38;2;255;255;255m"
#define Co2_	"\x1b[38;2;255;255;255m"
#define Co3_	"\x1b[38;2;255;255;255m"

#define Bk1_	"\x1b[48;2;255;255;255m"
#define Bk2_	"\x1b[48;2;255;255;255m"
#define Bk3_	"\x1b[48;2;255;255;255m"
/* ----------------------------------*/

#define Col_	"%s"
char* _Col(FONT_BKG_COLOR background, uint8_t red, uint8_t green, uint8_t blue);

void DEBUG_Init(void);
void Dbg(int on, char *txt);
void DbgMulti(int on, char *startTxt, char *txt, char *endTxt);
void DbgVar(int on, unsigned int buffLen, const char *fmt, ...);
void DbgVar2(int on, unsigned int buffLen, const char *fmt, ...);
void DEBUG_RxFullBuffService(void);
int DEBUG_RcvStr(char *txt);

void* DEBUG_TestFunction(void *a, DATA_TYPE dataType, DATA_ACTION dataAction, void *step, void *min, void *max, char *descr, VOID_FUNCTION_TEST xfunc, VOID_FUNCTION_TEST xfunc2);


#endif /* GENERAL_UTILITIES__DEBUG_H_ */
