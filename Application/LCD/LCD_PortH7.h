#ifndef LCD_PORT_H7_H
#define LCD_PORT_H7_H
#include <stdint.h>
/* Existing H7 LTDC/SDRAM pin, clock and memory-remap configuration is retained. */
#define LCD_H7_SDRAM_BASE 0x60000000UL
#define LCD_H7_WIDTH 800U
#define LCD_H7_HEIGHT 480U
#define LCD_H7_FRAME_BYTES (LCD_H7_WIDTH*LCD_H7_HEIGHT*3U)
extern volatile struct LCD_H7_Status {
    int initialized,touch_available,last_error;
    uint32_t transfers,dma_errors,timeouts;
} lcdH7Status;
int LCD_H7_Start(void);
void PCF8575_Init(void);
int PCF8575_Write(uint16_t address,uint16_t value);
#endif
