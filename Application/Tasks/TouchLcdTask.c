/*
 * TouchTask.c
 *
 *  Created on: Apr 10, 2024
 *      Author: maraf
 */
#include "TouchLcdTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "touch.h"
#include "stmpe811.h"
#include "stm32h7xx_hal.h"
#include "LCD_PortH7.h"
#include "_debug.h"

xTaskHandle vtask_TouchLcd_Handle;

static void LogTouchFault(const char *phase)
{
	const struct STMPE811_InitDiagnostic *d = BSP_TS_GetInitDiagnostic();
	DbgVarDma(1, 240, "\r\nLCD touch %s: addr=0x%02X ID=0x%04X stage=%u TSC=0x%02X op=%c reg=0x%02X HAL=%u err=0x%08X SDA(PB7)=%u SCL(PB8)=%u INT(PB4)=%u",
		phase, (unsigned)d->address, (unsigned)d->id, (unsigned)d->stage,
		(unsigned)d->ctrl, d->operation ? d->operation : '-',
		(unsigned)d->reg, (unsigned)d->status, (unsigned)d->hal_error,
		(unsigned)d->sda, (unsigned)d->scl,
		(unsigned)HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4));
}

void vtask_TouchLcd(void *pvParameters)
{
	(void)pvParameters;
	unsigned failedAttempts = 0;
	lcdH7Status.touch_available = 0;
	vTaskDelay(pdMS_TO_TICKS(500));

	for (;;)
	{
		if (!lcdH7Status.touch_available)
		{
			if (BSP_TS_Init(800, 480) != 0)
			{
				if (failedAttempts++ % 10 == 0)
					LogTouchFault("init failed");
				vTaskDelay(pdMS_TO_TICKS(500));
				continue;
			}
			failedAttempts = 0;
			lcdH7Status.touch_available = 1;
			DbgVarDma(1, 100, "\r\nLCD touch ready: STMPE811 at 0x%02X",
				(unsigned)BSP_TS_GetInitDiagnostic()->address);
		}

		if (!LCD_TOUCH_Service())
		{
			LogTouchFault("read failed, retrying");
			lcdH7Status.touch_available = 0;
			vTaskDelay(pdMS_TO_TICKS(500));
			continue;
		}
		vTaskDelay(pdMS_TO_TICKS(SERVICE_TOUCH_PROB_TIME_MS));
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
