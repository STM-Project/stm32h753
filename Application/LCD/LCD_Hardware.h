/*
 * LCD_Hardware.h
 *
 *  Created on: 10.04.2021
 *      Author: Elektronika RM
 */

#ifndef LCD_LCD_HARDWARE_H_
#define LCD_LCD_HARDWARE_H_
#include <stdint.h>

uint32_t LCD_GetXSize(void);
uint32_t LCD_GetYSize(void);
void LCD_DisplayBuff(uint32_t Xpos, uint32_t Ypos, uint32_t width, uint32_t height, uint32_t *pbmp);

#endif /* LCD_LCD_HARDWARE_H_ */
