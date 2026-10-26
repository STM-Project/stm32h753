#ifndef SCREEN_NAVIGATION_H
#define SCREEN_NAVIGATION_H
#include <stdint.h>
enum { SCREEN_NAV_WIDTH = 30, SCREEN_NAV_TOP = 210, SCREEN_NAV_HEIGHT = 60 };
/* Update the hidden background; return 1 when the transfer needs clipping. */
int SCREEN_NavigationUpdate ( unsigned x, unsigned y, unsigned width, unsigned height, const uint32_t *source, unsigned stride );
/* Called only by the LCD owner task. Edge gestures are consumed until release. */
void SCREEN_NavigationEnter ( uint32_t now, int down );
int SCREEN_NavigationInput ( int down, unsigned x, unsigned y, int *direction );
void SCREEN_NavigationRestore ( void );
void SCREEN_NavigationDraw ( uint32_t now );
int SCREEN_NavigationNext ( int current, int direction );
#endif
