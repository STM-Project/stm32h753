/*
 * ScreenLCDTask.c
 *
 *  Created on: Jun 2, 2024
 *      Author: mgrag
 */
#include "ScreenLcdTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "SCREEN_ReadPanel.h"

xTaskHandle vtask_ScreensSelectLCD_Handle;
xTaskHandle vtask_TEST_Handle;

void vtask_ScreensSelectLCD(void *pvParameters)
{
	while(1)
	{
		SCREEN_ReadPanel();
		vTaskDelay(20);
	}
}
double bufff[110];
void vtask_TEST(void *pvParameters)
{

	double dd=125.0145;


	while(1)
	{
		for(int i=0;i<100;++i){
			bufff[i] = dd*dd+(double)i*25.1236;
		}
		for(int i=0;i<100;++i){
			if(bufff[i]==125.125) bufff[i]=0.0;
		}
		vTaskDelay(20);
	}
}

void Create_ScreensSelectLCD_Task(void)
{
	if(vtask_ScreensSelectLCD_Handle) return;
	xTaskCreate(vtask_ScreensSelectLCD, (char* )"vtask_ScreensSelectLCD", 4024, NULL, (unsigned portBASE_TYPE ) 1, &vtask_ScreensSelectLCD_Handle);
}

void Create_TEST_Task(void)
{
	xTaskCreate(vtask_TEST, (char* )"vtask_TEST", 4048, NULL, (unsigned portBASE_TYPE ) 7, &vtask_TEST_Handle);
}




/* -------------END HTTP ---------------- */
