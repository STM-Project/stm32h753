#ifndef SCREEN_NAVIGATION_H
#define SCREEN_NAVIGATION_H
#include <stdint.h>
/* Called only by the LCD owner task. Edge gestures are consumed until release. */
void SCREEN_NavigationEnter ( uint32_t now, int down );
int SCREEN_NavigationInput ( int down, unsigned x, unsigned y, int *direction );
void SCREEN_NavigationRestore ( void );
void SCREEN_NavigationDraw ( uint32_t now );
int SCREEN_NavigationNext ( int current, int direction );
#endif
