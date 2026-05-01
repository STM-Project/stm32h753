/*
 * debug.c
 *
 *  Created on: 02.04.2021
 *      Author: Elektronika RM
 */

#include <_debug.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"
#include "mini_printf.h"
#include "FreeRTOS.h"

#define RECV_BUFF_SIZE	128
#define DEBUG_DEBUG		1

static char ALIGN_32BYTES(dbgRecvBuffer[RECV_BUFF_SIZE]);

void DEBUG_Init(void)
{
	memset(dbgRecvBuffer,0,RECV_BUFF_SIZE);
	DEBUG_ReceiveStart((uint8_t*)dbgRecvBuffer, RECV_BUFF_SIZE);
}

void Dbg(int on, char *txt)
{
	if(on)
		DEBUG_Send(txt);
}

void DbgMulti(int on, char *startTxt, char *txt, char *endTxt)
{
	if(on)
	{
		DEBUG_Send(startTxt);
		DEBUG_Send(txt);
		DEBUG_Send(endTxt);
	}
}

void DbgVar(int on, unsigned int buffLen, const char *fmt, ...)
{
	if(on)
	{
		char *temp = (char*)pvPortMalloc(buffLen);
		va_list va;
		va_start(va, fmt);
		mini_vsnprintf(temp, buffLen, fmt, va);
		va_end(va);
		DEBUG_Send(temp);
		vPortFree(temp);
	}
}

void DbgVar2(int on, unsigned int buffLen, const char *fmt, ...)
{
	if(on)
	{
		char *temp = (char*)pvPortMalloc(buffLen);
		va_list va;
		va_start(va, fmt);
		vsnprintf(temp,buffLen, fmt, va);
		va_end(va);
		DEBUG_Send(temp);
		vPortFree(temp);
	}
}

static int DEBUG_IsAnythingReceive(void)
{
	if(dbgRecvBuffer[0]>0)
		return 1;
	else
		return 0;
}

static int DEBUG_IsTxtReceive(char *txt)
{
	if(strstr(dbgRecvBuffer,txt))
	{
		DEBUG_ReceiveStop();
		memset(dbgRecvBuffer,0,RECV_BUFF_SIZE);
		DEBUG_ReceiveStart((uint8_t*)dbgRecvBuffer, RECV_BUFF_SIZE);
		return 1;
	}
	return 0;
}

void DEBUG_RxFullBuffService(void)
{
	DEBUG_Send(dbgRecvBuffer);
	memset(dbgRecvBuffer,0,RECV_BUFF_SIZE);
	DEBUG_ReceiveStart((uint8_t*)dbgRecvBuffer, RECV_BUFF_SIZE);
	Dbg(DEBUG_DEBUG,"\r\n -----  DEBUG_RxFullBuffService -------  ");
}

int DEBUG_RcvStr(char *txt)
{
	if(DEBUG_IsAnythingReceive())
	{
	  if(DEBUG_IsTxtReceive(txt))
		  return 1;
	  else
		  return 0;
	}
	else
		return 0;
}

char* _Col(FONT_BKG_COLOR background, uint8_t red, uint8_t green, uint8_t blue)
{
	#define	SIZE_TAB			MAX_SIZE_TXT * 10
	#define	MAX_SIZE_TXT	20

	static char tab[SIZE_TAB]={0};
	static int i=0;
	uint8_t fontBkg;
	int i_copy;

	if(i + MAX_SIZE_TXT >= SIZE_TAB-1)
		i=0;
	i_copy = i;

	switch(background){
		case font:
			fontBkg=38;
			break;
		default:
			fontBkg=48;
			break;
	}
	i += mini_snprintf(&tab[i],MAX_SIZE_TXT,"\x1b[%d;2;%d;%d;%dm",fontBkg,red,green,blue);
	tab[i++]=0;
	return &tab[i_copy];
}

void* DEBUG_TestFunction(void *a, DATA_TYPE dataType, DATA_ACTION dataAction, void *step, void *min, void *max, char *descr, VOID_FUNCTION_TEST xfunc, VOID_FUNCTION_TEST xfunc2){

	#define  _OPERAT(type)\
		if(_Incr==dataAction) INCR( *((type*)a), *((type*)step), *((type*)max));\
		if(_Decr==dataAction) DECR( *((type*)a), *((type*)step), *((type*)max));\
		if(_Wrap==dataAction) INCR_WRAP( *((type*)a), *((type*)step), *((type*)min), *((type*)max));\
		if(_float==dataType||_double==dataType) DbgVar(1,50,"\r\n%s: %s ",descr,Float2Str(*((type*)a),' ',1,Sign_none,1));\
		else 												 DbgVar(1,50,"\r\n%s: %d ",descr, *((type*)a));\
		if(xfunc !=NULL) xfunc (NULL,NULL);	/* in future use */
		if(xfunc2!=NULL) xfunc2(NULL,NULL);

	switch((int)dataType){
		case _int:		_OPERAT(int) 	  break;
		case _int8:		_OPERAT(int8_t)   break;
		case _int16:	_OPERAT(int16_t)  break;
		case _int32:	_OPERAT(int32_t)  break;
		case _uint8:	_OPERAT(uint8_t)  break;
		case _uint16:	_OPERAT(uint16_t) break;
		case _uint32:	_OPERAT(uint32_t) break;
		case _float:	_OPERAT(float)    break;
		case _double:	_OPERAT(double)   break;
	}
	return (void*)(0);

	#undef _OPERAT
}
