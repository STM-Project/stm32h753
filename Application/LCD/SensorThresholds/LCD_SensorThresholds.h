#ifndef LCD_SENSOR_THRESHOLDS_H
#define LCD_SENSOR_THRESHOLDS_H
#include <stdint.h>
#define LCD_THRESHOLD_PORTS 20
/* Units and fields match stm32h753/Application/HTTP/http_sensor_settings.h. */
typedef struct {
    char name[31];
    int16_t max1,max2,min1,min2,hysteresis,correction;
    uint16_t delay_seconds;
} LCD_SensorThreshold;
enum { LCD_THRESH_UPDATE=0,LCD_THRESH_ENTER=1,LCD_THRESH_EXIT=2 };
/* Called before committing BOTH sensors. Return 1 only after successful save.
 * NULL callback selects explicit RAM-only storage. Call from GUI task only.
 */
extern int (*lcdThresholdSave)(unsigned port,const LCD_SensorThreshold sensors[2]);
/* Read committed local RAM values; NULL before entry or for invalid indices. */
const LCD_SensorThreshold *LCD_SensorThresholdGet(unsigned port,unsigned humidity);
extern unsigned lcdThresholdPort; /* 1..20, default 1 */
extern volatile int lcdThresholdResult;
/* Defaults initialized once, then edits retained in RAM. No HTTP/GPIO changes.
 * Return 1 for successful Save, 0 otherwise; -1 mode, -2 LCD resolution,
 * -3 SD font, -4 touch registration. Font files: Arial 12, 14, 18 bold.
 */
int LCD_SensorThresholds(uint8_t mode);
/* Strict parser shared by editing and host tests; field 0=name, 1..7 numeric.
 * Accepts comma or dot for decimals. Does not change sensor on failure.
 * Delay editor accepts integer seconds (displayed as mm:ss on the card).
 */
int LCD_SensorThresholdSetField(LCD_SensorThreshold *sensor,unsigned humidity,
                              unsigned field,const char *text);
#endif
