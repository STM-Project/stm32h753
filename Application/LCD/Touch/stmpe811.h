
#ifndef __STMPE811_H
#define __STMPE811_H

typedef struct
{
  uint16_t TouchDetected;
  uint16_t x;
  uint16_t y;
  uint16_t z;
}TS_StateTypeDef;

extern TS_StateTypeDef  TS_State;

uint8_t BSP_TS_GetState	(TS_StateTypeDef *TS_State);
void 	  BSP_TS_ClearIT	(void);
uint8_t BSP_TS_Init		(uint16_t xSize, uint16_t ySize);

void TOUCH_SetNewResolution(uint8_t resol);
void TOUCH_SetDefaultResolution(void);
uint8_t* TOUCH_GetPtr2Resolution(void);

#endif
