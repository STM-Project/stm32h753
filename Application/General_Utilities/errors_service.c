/*
 * errors.c
 *
 *  Created on: 03.04.2021
 *      Author: Elektronika RM
 */

#include <_debug.h>
#include "errors_service.h"
#include <string.h>
#include "ff.h"

#define DEBUG_ERRORS_SERVICES		1

static char codeBuff[50];

static char* CODESTR_SdCard(int code)
{
	char* CodeStr=codeBuff;

	switch(code)
	{
	case FR_OK:
		strcpy(CodeStr,"FR_OK");
		break;
	case FR_DISK_ERR:
		strcpy(CodeStr,"FR_DISK_ERR");
		break;
	case FR_INT_ERR:
		strcpy(CodeStr,"FR_INT_ERR");
		break;
	case FR_NOT_READY:
		strcpy(CodeStr,"FR_NOT_READY");
		break;
	case FR_NO_FILE:
		strcpy(CodeStr,"FR_NO_FILE");
		break;
	case FR_NO_PATH:
		strcpy(CodeStr,"FR_NO_PATH");
		break;
	case FR_INVALID_NAME:
		strcpy(CodeStr,"FR_INVALID_NAME");
		break;
	case FR_DENIED:
		strcpy(CodeStr,"FR_DENIED");
		break;
	case FR_EXIST:
		strcpy(CodeStr,"FR_EXIST");
		break;
	case FR_INVALID_OBJECT:
		strcpy(CodeStr,"FR_INVALID_OBJECT");
		break;
	case FR_WRITE_PROTECTED:
		strcpy(CodeStr,"FR_WRITE_PROTECTED");
		break;
	case FR_INVALID_DRIVE:
		strcpy(CodeStr,"FR_INVALID_DRIVE");
		break;
	case FR_NOT_ENABLED:
		strcpy(CodeStr,"FR_NOT_ENABLED");
		break;
	case FR_NO_FILESYSTEM:
		strcpy(CodeStr,"FR_NO_FILESYSTEM");
		break;
	case FR_MKFS_ABORTED:
		strcpy(CodeStr,"FR_MKFS_ABORTED");
		break;
	case FR_TIMEOUT:
		strcpy(CodeStr,"FR_TIMEOUT");
		break;
	case FR_LOCKED:
		strcpy(CodeStr,"FR_LOCKED");
		break;
	case FR_NOT_ENOUGH_CORE:
		strcpy(CodeStr,"FR_NOT_ENOUGH_CORE");
		break;
	case FR_TOO_MANY_OPEN_FILES:
		strcpy(CodeStr,"FR_TOO_MANY_OPEN_FILES");
		break;
	case FR_INVALID_PARAMETER:
		strcpy(CodeStr,"FR_INVALID_PARAMETER");
		break;
	}
	return CodeStr;
}

void ERROR_SDcardMount(int code){
	DbgVar(DEBUG_ERRORS_SERVICES,50,"\r\nERROR_SDcardMount code: %s",CODESTR_SdCard(code));
}
void ERROR_SDcardOpen(int code){
	DbgVar(DEBUG_ERRORS_SERVICES,50,"\r\nERROR_SDcardOpen code: %s",CODESTR_SdCard(code));
}
void ERROR_SDcardRead(int code){
	DbgVar(DEBUG_ERRORS_SERVICES,50,"\r\nERROR_SDcardRead code: %s",CODESTR_SdCard(code));
}
void ERROR_SDcardWrite(int code){
	DbgVar(DEBUG_ERRORS_SERVICES,50,"\r\nERROR_SDcardWrite code: %s",CODESTR_SdCard(code));
}
void ERROR_SDcardClose(int code){
	DbgVar(DEBUG_ERRORS_SERVICES,50,"\r\nERROR_SDcardClose code: %s",CODESTR_SdCard(code));
}
void ERROR_SDcardLseek(int code){
	DbgVar(DEBUG_ERRORS_SERVICES,50,"\r\nERROR_SDcardLseek code: %s",CODESTR_SdCard(code));
}
void ERROR_SDcardInfo(int code){
	DbgVar(DEBUG_ERRORS_SERVICES,50,"\r\nERROR_SDcardInfo code: %s",CODESTR_SdCard(code));
}

void ERROR_NMIHandler(void){
	Dbg(DEBUG_ERRORS_SERVICES,"\r\nERROR_NMIHandler ");
	//HAL_NVIC_SystemReset();
}
void ERROR_HardFaulHandler(void){
	Dbg(DEBUG_ERRORS_SERVICES,"\r\nERROR_HardFaulHandler ");
	//HAL_NVIC_SystemReset();
}
void ERROR_MemManageHandler(void){
	Dbg(DEBUG_ERRORS_SERVICES,"\r\nERROR_MemManageHandler ");
	//HAL_NVIC_SystemReset();
}
void ERROR_BusFaultHandler(void){
	Dbg(DEBUG_ERRORS_SERVICES,"\r\nERROR_BusFaultHandler ");
	//HAL_NVIC_SystemReset();
}
void ERROR_UsageFaultHandler(void){
	Dbg(DEBUG_ERRORS_SERVICES,"\r\nERROR_UsageFaultHandler ");
	//HAL_NVIC_SystemReset();
}
void ERROR_DebugMonHandler(void){
	Dbg(DEBUG_ERRORS_SERVICES,"\r\nERROR_DebugMonHandler ");
	//HAL_NVIC_SystemReset();
}

void ERROR_Sdram(void){
	Dbg(1,"\r\nERROR_Sdram ");
}
void ERROR_StrChangeColor(void){
	Dbg(1,"\r\nBuffChangeColor size overrun !!! ");
}
