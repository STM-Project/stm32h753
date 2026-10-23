/*
 * LCD_Common.c
 *
 *  Created on: 11.04.2021
 *      Author: Elektronika RM
 */

#include "stm32h7xx_hal.h"
#include "LCD_Common.h"
#include "common.h"
#include "LCD_Hardware.h"
#include "timer.h"
#include "task.h"

#define MAX_ELEMENTS_REFRESH_IN_SCREEN	40

portTickType refreshScreenVar[MAX_ELEMENTS_REFRESH_IN_SCREEN];

void LCD_AllRefreshScreenClear(void){
	for(int i=0; i<MAX_ELEMENTS_REFRESH_IN_SCREEN; ++i)
		refreshScreenVar[i] = xTaskGetTickCount();
}
void LCD_RefreshScreenClear(int nrRefresh){
	refreshScreenVar[nrRefresh] = xTaskGetTickCount();
}
int LCD_IsRefreshScreenTimeout(int nrRefresh, int timeout){
	int temp = _CheckTickCount(refreshScreenVar[nrRefresh],timeout);
	if(temp)
		LCD_RefreshScreenClear(nrRefresh);
	return temp;
}

//float dfeee = GetTransitionCoeff(WHITE, uint32_t colorTo, GetTransitionColor(WHITE, MYGRAY, 0.43));

uint32_t GetTransitionColor(uint32_t colorFrom, uint32_t colorTo, float transCoeff)
{
		  if(1.0<=transCoeff) return colorTo;
	else if(0.0>=transCoeff) return colorFrom;

	uint8_t R_1= (colorFrom>>16)&0x000000FF;
	uint8_t G_1= (colorFrom>>8) &0x000000FF;
	uint8_t B_1=  colorFrom     &0x000000FF;

	uint8_t R_2= (colorTo>>16)&0x000000FF;
	uint8_t G_2= (colorTo>>8) &0x000000FF;
	uint8_t B_2=  colorTo     &0x000000FF;

	uint8_t R_delta= ABS((int)R_1-(int)R_2);
	uint8_t G_delta= ABS((int)G_1-(int)G_2);
	uint8_t B_delta= ABS((int)B_1-(int)B_2);

	uint8_t R_coeff=  (uint8_t)((float)R_delta*transCoeff);
	uint8_t G_coeff=  (uint8_t)((float)G_delta*transCoeff);
	uint8_t B_coeff=  (uint8_t)((float)B_delta*transCoeff);

	uint8_t R_out = R_1>R_2 ? R_1-R_coeff : R_1+R_coeff;
	uint8_t G_out = G_1>G_2 ? G_1-G_coeff : G_1+G_coeff;
	uint8_t B_out = B_1>B_2 ? B_1-B_coeff : B_1+B_coeff;

	return RGB2INT(R_out,G_out,B_out);
}

float GetTransitionCoeff(uint32_t colorFrom, uint32_t colorTo, uint32_t colorTrans)
{
	int div=0;
	float average=0;

	uint8_t R_1= (colorFrom>>16)&0x000000FF;
	uint8_t G_1= (colorFrom>>8) &0x000000FF;
	uint8_t B_1=  colorFrom     &0x000000FF;

	uint8_t R_2= (colorTo>>16)&0x000000FF;
	uint8_t G_2= (colorTo>>8) &0x000000FF;
	uint8_t B_2=  colorTo     &0x000000FF;

	uint8_t R_tr= (colorTrans>>16)&0x000000FF;
	uint8_t G_tr= (colorTrans>>8) &0x000000FF;
	uint8_t B_tr=  colorTrans     &0x000000FF;

	float R_delta= ABS((int)R_1-(int)R_2);	 if(R_delta>0) div++;
	float G_delta= ABS((int)G_1-(int)G_2);	 if(G_delta>0) div++;
	float B_delta= ABS((int)B_1-(int)B_2);	 if(B_delta>0) div++;

	if(R_delta>0) average += (float)ABS(R_tr-R_1)/R_delta;
	if(G_delta>0) average += (float)ABS(G_tr-G_1)/G_delta;
	if(B_delta>0) average += (float)ABS(B_tr-B_1)/B_delta;

	return (average/div);
}

void CorrectPosIfOutRange(int16_t *pos)
{
	if(*pos<0)
		*pos=0;
	else if(*pos>LCD_GetXSize())
		*pos=LCD_GetXSize();
}

int BrightDecr(int color, int val)
{
	uint8_t _R = R_PART(color);
	uint8_t _G = G_PART(color);
	uint8_t _B = B_PART(color);

	return RGB2INT(DECR(_R,val,0), DECR(_G,val,0), DECR(_B,val,0));
}

int BrightIncr(int color, int val)
{
	uint8_t _R = R_PART(color);
	uint8_t _G = G_PART(color);
	uint8_t _B = B_PART(color);

	return RGB2INT(INCR(_R,val,255), INCR(_G,val,255), INCR(_B,val,255));
}

void CallFunc(VOID_FUNCTION *pfunc1, VOID_FUNCTION *pfunc2, VOID_FUNCTION *pfunc3, VOID_FUNCTION *pfunc4, VOID_FUNCTION *pfunc5){
	pfunc1(); pfunc2(); pfunc3(); pfunc4(); pfunc5();
}
