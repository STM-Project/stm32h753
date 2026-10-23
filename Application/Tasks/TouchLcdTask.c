/*
 * TouchTask.c
 *
 *  Created on: Apr 10, 2024
 *      Author: maraf
 */
#include "TouchLcdTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>
#include "touch.h"

xTaskHandle vtask_TouchLcd_Handle;

void vtask_TouchLcd(void *pvParameters)
{
	portTickType xLastExecutionTime;

	xLastExecutionTime = xTaskGetTickCount();
	vTaskDelayUntil(&xLastExecutionTime, 100);

	while(1)
	{
		LCD_TOUCH_Service();
		vTaskDelayUntil(&xLastExecutionTime, SERVICE_TOUCH_PROB_TIME_MS);
	}
}

void Create_TouchLcd_Task(void)
{
	if(vtask_TouchLcd_Handle) return;
	xTaskCreate(vtask_TouchLcd, (char* )"vtask_TouchLcd", 2048, NULL, (unsigned portBASE_TYPE ) 1, &vtask_TouchLcd_Handle);
}

void Delete_TouchLcd_Task(void)
{
	if(!vtask_TouchLcd_Handle) return;
	vTaskDelete(vtask_TouchLcd_Handle);
	vtask_TouchLcd_Handle = NULL;
	/* vTaskSuspend(vtask_TouchLcd_Handle); */
	/* xTaskResumeFromISR(vtask_TouchLcd_Handle); */
}
