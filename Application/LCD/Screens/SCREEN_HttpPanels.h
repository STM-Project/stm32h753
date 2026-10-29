#ifndef SCREEN_HTTP_PANELS_H
#define SCREEN_HTTP_PANELS_H
#include <stdint.h>
/* Same ENTER/UPDATE/EXIT modes as the existing temperature screens.
 * settings: 0=/main, 1=/set, 2=white /nowa thermometer panel. */
int LCD_HttpPanel ( uint8_t mode, int settings );
#endif
