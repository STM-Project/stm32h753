/*
 * timer.c
 *
 *  Created on: 03.04.2021
 *      Author: Elektronika RM
 */

#include <_debug.h>
#include "timer.h"
#include "timers.h"
#include "semphr.h"

#define MAX_COUNT_TIME		40
#define MAX_MEASURE_TIME	10

#define MAX_ELEMENTS_TIMER_SERVICE	40

/*	-------- Semaphores ------------ */							/* Mutex`s must be in the same task, 	but 	semaphores can be not only in the same task, can be in the interrupt */
osSemaphoreId  LcdUpdateSemaphoreId = 0;
osSemaphoreId	osDma2dSemph = 0;
osMutexId		osDeviceMutex = 0;

static xSemaphoreHandle xSemphr_pLcd;		/* Change semaphores declarations as MACRO  */
static xSemaphoreHandle xSemphr_fontImg;
static xSemaphoreHandle xSemphr_cardSD;

static portTickType timeVar[MAX_COUNT_TIME];
static portTickType measurTime[MAX_MEASURE_TIME];

void InitAllMutex(void){
	xSemphr_pLcd 	 = xSemaphoreCreateMutex();
	xSemphr_fontImg = xSemaphoreCreateMutex();
	xSemphr_cardSD  = xSemaphoreCreateMutex();
}
uint32_t TakeMutex(SEMPHR_ID ID, uint32_t timeout){
	switch((int)ID){
	 case Semphr_sdram:		if(xSemaphoreTake(xSemphr_pLcd,	 timeout)==pdTRUE) return 1; 	else return 0;
	 case Semphr_fontImg:	if(xSemaphoreTake(xSemphr_fontImg,timeout)==pdTRUE) return 1; 	else return 0;
	 case Semphr_cardSD:		if(xSemaphoreTake(xSemphr_cardSD, timeout)==pdTRUE) return 1; 	else return 0;
	 default:
		 return 0;
}}
void GiveMutex(SEMPHR_ID ID){
	switch((int)ID){
	 case Semphr_sdram:		xSemaphoreGive(xSemphr_pLcd); 	break;
	 case Semphr_fontImg:	xSemaphoreGive(xSemphr_fontImg); break;
	 case Semphr_cardSD:		xSemaphoreGive(xSemphr_cardSD);  break;
}}
uint32_t TakeMutex2(SEMPHR_ID ID1,SEMPHR_ID ID2, uint32_t timeout){
	if(TakeMutex(ID1,timeout)&&TakeMutex(ID2,timeout)) return 1;
	return 0;
}
void GiveMutex2(SEMPHR_ID ID1,SEMPHR_ID ID2){
	GiveMutex(ID1);
	GiveMutex(ID2);
}
/*	-------- END Semaphores ------------ */

void AllTimeReset(void)
{
	for(int i=0; i<MAX_COUNT_TIME; ++i)
		timeVar[i] = xTaskGetTickCount();
}
void TimeReset(int nrTime)
{
	if(nrTime<MAX_COUNT_TIME)
		timeVar[nrTime] = xTaskGetTickCount();
}
int IsTimeout(int nrTime, int timeout)
{
	if(nrTime<MAX_COUNT_TIME)
	{
		if((xTaskGetTickCount()-timeVar[nrTime])>timeout)
			return 1;
	}
	return 0;
}

void StartMeasureTime(int nr)
{
	if(nr<MAX_MEASURE_TIME)
		measurTime[nr] = xTaskGetTickCount();
}
uint32_t StopMeasureTime(int nr, char *nameTime)
{
	if(nr<MAX_MEASURE_TIME)
	{
		int measurTimeStop = xTaskGetTickCount();
		if(nameTime[0]>0)
			DbgVar(1,20,"%s%d ",nameTime,measurTimeStop-measurTime[nr]);
		return measurTimeStop-measurTime[nr];
	}
	return 0;
}


int _CheckTickCount(portTickType tim, int timeout){		/* check: configUSE_16_BIT_TICKS in portmacro.h */
	TickType_t countVal = xTaskGetTickCount();
	if((countVal-tim) < 0){
		if(countVal+(65535-tim) > timeout)
			return 1;
	}
	else{
		if((countVal-tim) > timeout)
			return 1;
	}
	return 0;
}

uint16_t vTimerService(int nr, int cmd, int timeout)
{
	static portTickType _timer[MAX_ELEMENTS_TIMER_SERVICE] = {0};

	if(nr >= MAX_ELEMENTS_TIMER_SERVICE) return 0xFFFF;

	switch(cmd)
	{
		case start_time:
			if(_timer[nr] == 0){
				_timer[nr] = xTaskGetTickCount();
				return 0;
			}
			return _timer[nr];

		case restart_time:
			_timer[nr] = xTaskGetTickCount();
			return _timer[nr];

		case get_time:
			return xTaskGetTickCount();

		case check_time:
			if(_timer[nr])
				return _CheckTickCount(_timer[nr],timeout);
			return 0;

		case check_restart_time:
			if(_timer[nr] && _CheckTickCount(_timer[nr],timeout)){
				_timer[nr] = xTaskGetTickCount();
				return 1;
			}
			return 0;

		case check_stop_time:
			if(_timer[nr] && _CheckTickCount(_timer[nr],timeout)){
				_timer[nr] = 0;
				return 1;
			}
			return 0;

		case stop_time:
			if(_timer[nr]){
				int temp= _CheckTickCount(_timer[nr],timeout);
				_timer[nr] = 0;
				return temp;
			}
			return 0;

		case get_startTime:
			return _timer[nr];

		case reset_time:
		default:
			_timer[nr] = 0;
			return 1;
	}
}

