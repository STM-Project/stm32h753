#include "stm32h7xx_hal.h"
#include "LCD_PortH7.h"
#include "LCD_Hardware.h"
#include "LCD_fonts_images.h"
#include "SCREEN_ReadPanel.h"
#include "ScreenLcdTask.h"
#include "TouchLcdTask.h"
#include "stmpe811.h"
#include "touch.h"
#include "lang.h"
#include "timer.h"
#include "ltdc.h"
#include "i2c.h"
#include "_debug.h"
#include <string.h>
/* Linker places the physical LTDC framebuffer at 0x60000000, before .lcd_work. */
static uint8_t framebuffer[LCD_H7_FRAME_BYTES] __attribute__((section(".lcd_framebuffer"),aligned(32),used));
extern uint8_t __lcd_work_start__,__lcd_work_end__;
extern TaskHandle_t vtask_ScreensSelectLCD_Handle,vtask_TouchLcd_Handle;
volatile struct LCD_H7_Status lcdH7Status;
int LCD_H7_Start(void)
{
    if(lcdH7Status.initialized)return 1;
    if(LCD_GetXSize()!=800||LCD_GetYSize()!=480||hltdc.LayerCfg[0].FBStartAdress!=LCD_H7_SDRAM_BASE||hltdc.LayerCfg[0].PixelFormat!=LTDC_PIXEL_FORMAT_RGB888){lcdH7Status.last_error=-1;return 0;}
    osSemaphoreDef(LcdUpdateSemaphore);
    LcdUpdateSemaphoreId=osSemaphoreCreate(osSemaphore(LcdUpdateSemaphore),1);
    osSemaphoreDef(Dma2dSemaphore);
    osDma2dSemph=osSemaphoreCreate(osSemaphore(Dma2dSemaphore),1);
    osMutexDef(DeviceMutex);
    osDeviceMutex=osMutexCreate(osMutex(DeviceMutex));
    if(!LcdUpdateSemaphoreId||!osDma2dSemph||!osDeviceMutex){lcdH7Status.last_error=-2;return 0;}
    osSemaphoreWait(LcdUpdateSemaphoreId,0);osSemaphoreWait(osDma2dSemph,0);
    /* Clear LCD-owned NOLOAD buffers only. Never clear H7 .sdram/RTOS heap. */
    memset(&__lcd_work_start__,0,(size_t)((uintptr_t)&__lcd_work_end__-(uintptr_t)&__lcd_work_start__));
    memset(framebuffer,0,sizeof(framebuffer));
    SCB_CleanDCache_by_Addr((uint32_t*)framebuffer,sizeof(framebuffer));__DSB();
    __HAL_RCC_DMA2D_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA2D_IRQn,6,0);HAL_NVIC_EnableIRQ(DMA2D_IRQn);
    HAL_NVIC_SetPriority(LTDC_IRQn,6,0);HAL_NVIC_EnableIRQ(LTDC_IRQn);
    HAL_LTDC_ProgramLineEvent(&hltdc,hltdc.Init.AccumulatedActiveH+1U);
    for(unsigned attempt=0; attempt<3; ++attempt){
        lcdH7Status.touch_available=BSP_TS_Init(800,480)==0;
        if(lcdH7Status.touch_available)break;
        if(attempt<2)osDelay(100);
    }
    if(!lcdH7Status.touch_available)DbgDma(1,"\r\nLCD: STMPE811 not detected on I2C1; touch task disabled");
    SetLang(2,English);LCD_SetSpacesBetweenFonts();LCD_TOUCH_DeleteAllSetTouch();
    lcdH7Status.initialized=1;
    if(lcdH7Status.touch_available)Create_TouchLcd_Task();
    Create_ScreensSelectLCD_Task();
    if(!vtask_ScreensSelectLCD_Handle){
        if(lcdH7Status.touch_available&&vtask_TouchLcd_Handle)Delete_TouchLcd_Task();
        lcdH7Status.initialized=0;lcdH7Status.last_error=-3;return 0;
    }
    if(lcdH7Status.touch_available&&!vtask_TouchLcd_Handle){
        lcdH7Status.touch_available=0;lcdH7Status.last_error=-4;
        DbgDma(1,"\r\nLCD: touch task allocation failed");
    }
    return 1;
}

/* Radio demonstration screen used these F7 application helpers. The H7
 * I2C4 bus is already initialized by main; do not reconfigure its pins/clock. */
void PCF8575_Init(void){}
int PCF8575_Write(uint16_t address,uint16_t value)
{
    uint8_t bytes[2]={(uint8_t)value,(uint8_t)(value>>8)};
    return HAL_I2C_Master_Transmit(&hi2c4,(uint16_t)(0x40U|(address<<1)),bytes,2,10)==HAL_OK?1:-1;
}
