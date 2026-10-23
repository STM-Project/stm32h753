#ifndef SCREEN_TEMPERATURE_PK_H
#define SCREEN_TEMPERATURE_PK_H
#include <stdint.h>

enum { LCD_TEMP_UPDATE = 0, LCD_TEMP_ENTER = 1, LCD_TEMP_EXIT = 2 };

/* One public screen function. Call UPDATE every 20 ms from the GUI task.
 * readTemperature10: sensor callback (255 = 25.5 C), sampled every 500 ms.
 * NULL callback selects an explicitly simulated temperature.
 * pkDemo is caller-owned RAM state; this module never writes relay GPIO.
 * Returns 1 when touch changed pkDemo, 0 otherwise;
 * -1 invalid arguments/mode, -2 resolution, -3 font loading, -4 touch setup.
 * ENTER takes exclusive ownership of the screen and resets screen resources.
 * Fonts required on SD: Arial 16, Arial 24, Arial 28 bold, Arial 72.
 */
int LCD_TemperaturePkPanel(uint8_t mode, int (*readTemperature10)(void), uint8_t *pkDemo);

/* Default screen 7 demonstration, editable in the debugger. */
extern uint8_t lcdPanelPkDemo;
extern volatile int lcdPanelResult;
extern volatile int lcdPanelTemperature10;
extern int (*lcdPanelReadTemperature10)(void);
/* Screen 8: dark dashboard, 120 samples, separate ON/OFF touch buttons.
 * Same modes/return codes/temperature callback contract as screen 7.
 * Fonts on SD: Arial 9, Arial 14 bold, Arial 36, Arial 16.
 * No networking or GPIO. Callback NULL selects sine-wave demo (-50..+90 C, 30 s period), initially 25.5 C.
 */
int LCD_TemperaturePkPanel2(uint8_t mode, int (*readTemperature10)(void), uint8_t *pkDemo);
extern uint8_t lcdPanel2PkDemo;
extern volatile int lcdPanel2Result;
extern volatile int lcdPanel2Temperature10;
extern int (*lcdPanel2ReadTemperature10)(void);
/* Screen 9: three COLD/WARM/HOT gauges, drag each needle with touch.
 * Modes/return codes as above; fonts on SD: Arial 36 and Arial 16.
 * Positions 0..1000 correspond to COLD..HOT; no GPIO or network operations.
 */
int LCD_TemperaturePkPanel3(uint8_t mode);
extern uint16_t lcdGaugePositions[3];
extern volatile int lcdPanel3Result;
#endif
