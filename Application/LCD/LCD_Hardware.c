/* H753 application adapter: DMA2D ARGB8888 -> existing LTDC RGB888.
 * No STM32 HAL driver changes or additional vendor source required.
 */
#include "stm32h7xx_hal.h"
#include "LCD_Hardware.h"
#include "LCD_PortH7.h"
#include "SCREEN_Navigation.h"
#include "ltdc.h"
#include "timer.h"
#include <stddef.h>
static volatile uint32_t dmaResult;
uint32_t LCD_GetXSize ( void )
{
	return hltdc.LayerCfg[0].ImageWidth;
}
uint32_t LCD_GetYSize ( void )
{
	return hltdc.LayerCfg[0].ImageHeight;
}
void DMA2D_IRQHandler ( void )
{
	uint32_t flags = DMA2D->ISR;
	DMA2D->IFCR = flags & 0x3FU;
	if ( flags & ( DMA2D_ISR_TEIF | DMA2D_ISR_CEIF | DMA2D_ISR_TCIF ) )
	{
		dmaResult = flags;
		if ( osDma2dSemph )
			osSemaphoreRelease ( osDma2dSemph );
	}
}
void LTDC_IRQHandler ( void )
{
	HAL_LTDC_IRQHandler ( &hltdc );
}
void HAL_LTDC_LineEventCallback ( LTDC_HandleTypeDef *handle )
{
	if ( LcdUpdateSemaphoreId )
		osSemaphoreRelease ( LcdUpdateSemaphoreId );
	HAL_LTDC_ProgramLineEvent ( handle, handle->Init.AccumulatedActiveH + 1U );
}
static void LCD_DisplayRect ( uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t *source, uint32_t stride )
{
	if ( !lcdH7Status.initialized || !source || !width || !height || x >= 800 || y >= 480 || width > 800 - x || height > 480 - y )
	{
		lcdH7Status.last_error = -10;
		return;
	}
	if ( osMutexWait ( osDeviceMutex, osWaitForever ) != osOK )
	{
		lcdH7Status.last_error = -11;
		return;
	}
	if ( !TakeMutex ( Semphr_sdram, osWaitForever ) )
	{
		osMutexRelease ( osDeviceMutex );
		lcdH7Status.last_error = -12;
		return;
	}
	while ( osSemaphoreWait ( osDma2dSemph, 0 ) == osOK )
	{
	}
	uintptr_t start = (uintptr_t) source, aligned = start & ~(uintptr_t) 31U;
	size_t length = ( ( start - aligned ) + ( ( height - 1U ) * stride + width ) * 4U + 31U ) & ~(size_t) 31U;
	SCB_CleanDCache_by_Addr ( (uint32_t *) aligned, (int32_t) length );
	__DSB ();
	DMA2D->CR = 0;
	DMA2D->IFCR = 0x3FU;
	DMA2D->FGMAR = (uint32_t) source;
	DMA2D->FGOR = stride - width;
	DMA2D->FGPFCCR = 0; /* foreground ARGB8888 */
	DMA2D->OPFCCR = 1;	/* output RGB888: unchanged H7 LTDC pixel format */
	DMA2D->OMAR = LCD_H7_SDRAM_BASE + 3U * ( y * 800U + x );
	DMA2D->OOR = 800U - width;
	DMA2D->NLR = ( width << 16 ) | height;
	dmaResult = 0;
	DMA2D->CR = ( 1U << 16 ) | DMA2D_CR_TCIE | DMA2D_CR_TEIE | DMA2D_CR_CEIE;
	__DSB ();
	DMA2D->CR |= DMA2D_CR_START;
	int32_t result = osSemaphoreWait ( osDma2dSemph, 1000 );
	if ( result != osOK || !dmaResult )
	{
		DMA2D->CR |= DMA2D_CR_ABORT;
		NVIC_ClearPendingIRQ ( DMA2D_IRQn );
		lcdH7Status.timeouts++;
		lcdH7Status.last_error = -13;
	}
	else if ( dmaResult & ( DMA2D_ISR_TEIF | DMA2D_ISR_CEIF ) )
	{
		lcdH7Status.dma_errors++;
		lcdH7Status.last_error = -14;
	}
	else
	{
		lcdH7Status.transfers++;
		lcdH7Status.last_error = 0;
	}
	GiveMutex ( Semphr_sdram );
	osMutexRelease ( osDeviceMutex );
}

/* Leave the visible navigation rectangles untouched by panel redraws.
 * At most three DMA rectangles are needed; the caller's source is never edited. */
void LCD_DisplayBuffStride ( uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t *source, uint32_t stride )
{
	if ( !lcdH7Status.initialized || !source || !width || !height || stride < width || x >= 800 || y >= 480 || width > 800 - x || height > 480 - y )
	{
		lcdH7Status.last_error = -10;
		return;
	}
	if ( !SCREEN_NavigationUpdate ( x, y, width, height, source, stride ) )
	{
		LCD_DisplayRect ( x, y, width, height, source, stride );
		return;
	}
	uint32_t top = y > SCREEN_NAV_TOP ? y : SCREEN_NAV_TOP;
	uint32_t bottom = y + height < SCREEN_NAV_TOP + SCREEN_NAV_HEIGHT ? y + height : SCREEN_NAV_TOP + SCREEN_NAV_HEIGHT;
	if ( y < top )
		LCD_DisplayRect ( x, y, width, top - y, source, stride );
	uint32_t left = x > SCREEN_NAV_WIDTH ? x : SCREEN_NAV_WIDTH;
	uint32_t right = x + width < LCD_H7_WIDTH - SCREEN_NAV_WIDTH ? x + width : LCD_H7_WIDTH - SCREEN_NAV_WIDTH;
	if ( left < right )
		LCD_DisplayRect ( left, top, right - left, bottom - top, source + ( top - y ) * stride + left - x, stride );
	if ( bottom < y + height )
		LCD_DisplayRect ( x, bottom, width, y + height - bottom, source + ( bottom - y ) * stride, stride );
}

void LCD_DisplayBuff ( uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t *source )
{
	LCD_DisplayBuffStride ( x, y, width, height, source, width );
}
