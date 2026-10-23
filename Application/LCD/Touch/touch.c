
#include <LCD_BasicGraphics.h>
#include <LCD_Hardware.h>
#include <string.h>
#include <stdio.h>

#include "touch.h"
#include "stmpe811.h"
#include "timer.h"
#include "_debug.h"

#define MAX_OPEN_TOUCH_SIMULTANEOUSLY	 180
#define BUF_LCD_TOUCH_SIZE		100
#define MAX_NUMBER_PIONTS_TOUCH	 2
#define AB_COEFF_TAB_SIZE	4

#define TIME_700_MS_TO_NUMBER_PROBE	  	(700 / SERVICE_TOUCH_PROB_TIME_MS)
#define TIME_5000_MS_TO_NUMBER_PROBE	(5000 / SERVICE_TOUCH_PROB_TIME_MS / Touch[i].param)

#define SCROLL_SEL__MAX_NUMBER			10
#define SCROLL_SEL__NUMBER_PROBE2SEL	5
#define SCROLL_SEL__SEL_SPREAD			7

typedef struct{
  uint8_t press;
  uint8_t idx;
  XY_Touch_Struct  pos[BUF_LCD_TOUCH_SIZE];
}Service_lcd_Touch_Struct;

typedef struct
{
  uint16_t id;
  uint16_t index;
  uint8_t param;
  uint8_t flags1;
  uint8_t flags2;
  XY_Touch_Struct  pos[MAX_NUMBER_PIONTS_TOUCH];
}Touch_Struct;

static int userStatusTouch=0;
static Service_lcd_Touch_Struct  ServiceTouch = {.idx=0};
static Touch_Struct  Touch[MAX_OPEN_TOUCH_SIMULTANEOUSLY];
static uint16_t susspendTouch[MAX_OPEN_TOUCH_SIMULTANEOUSLY];

XY_Touch_Struct  touchTemp[MAX_NUMBER_PIONTS_TOUCH] = {0};
XY_Touch_Struct  structXY_Zero = {0};

static uint8_t Calibration_Done = 0;

static int32_t  A1[AB_COEFF_TAB_SIZE]={0}, 	_A1 = Coeff_A1;
static int32_t  B1[AB_COEFF_TAB_SIZE]={0}, 	_B1 = Coeff_B1;

static int32_t  A2[AB_COEFF_TAB_SIZE]={0}, 	_A2 = Coeff_A2;
static int32_t  B2[AB_COEFF_TAB_SIZE]={0}, 	_B2 = Coeff_B2;

static int32_t aLogX[2*AB_COEFF_TAB_SIZE]={0};
static int32_t aLogY[2*AB_COEFF_TAB_SIZE]={0};

static int32_t aPhysX[2*AB_COEFF_TAB_SIZE]={0};
static int32_t aPhysY[2*AB_COEFF_TAB_SIZE]={0};

uint8_t touchDetect;

void WaitForTouchState(uint8_t Pressed)
{
	int i=0;
	TS_StateTypeDef  State;
	do{
	   BSP_TS_GetState(&State);
	   HAL_Delay(10);
	   if (State.TouchDetected == Pressed)
	   {
	   	i=0;
	   	do {
	   		BSP_TS_GetState(&State);
	   		HAL_Delay(10);
	   		if (State.TouchDetected != Pressed)
	   			break;
	   		else if (i > 0)
	   			return;
	   		i++;
	   	 }
	   	 while (1);
	   }
	}
	while (1);
}

void SetLogXY(XY_Touch_Struct *pos, uint16_t *width, int maxSize)
{
   for(int i=0; i<maxSize; i++){
   	aLogX[i] = pos->x + *width/2;
      aLogY[i] = pos->y + *width/2;
   	if(i == 2*AB_COEFF_TAB_SIZE-1)
   		break;
      pos++;
      width++;
   }
}

void SetPhysXY(XY_Touch_Struct *pos, int maxSize)
{
   for(int i=0; i<maxSize; i++){
   	aPhysX[i] = pos->x;
   	aPhysY[i] = pos->y;
   	if(i == 2*AB_COEFF_TAB_SIZE-1)
   		break;
      pos++;
   }
}

int LCD_TOUCH_CalcCoeffCalibr(int maxSize)
{
	int j=0;

	void _average_AB(int max)
	{
		_A1 =0;  _A2 =0;
		_B1 =0;  _B2 =0;

		for(int i=0; i<max; i++){
			_A1 += A1[i];
			_A2 += A2[i];

			_B1 += B1[i];
			_B2 += B2[i];
		}
		_A1=_A1/max;
		_A2=_A2/max;

		_B1=_B1/max;
		_B2=_B2/max;
	}

	if(maxSize%2)
		maxSize--;

	for(int i=0; i<maxSize; i+=2)
	{
		if(aPhysX[i] != aPhysX[i+1] && aPhysY[i] != aPhysY[i+1])
		{
			A1[j] = (1000 * ( aLogX[i+1] - aLogX[i]))/ ( aPhysX[i+1] - aPhysX[i]);
			B1[j] = (1000 * aLogX[i]) - A1[j] * aPhysX[i];

			A2[j] = (1000 * ( aLogY[i+1] - aLogY[i]))/ ( aPhysY[i+1] - aPhysY[i]);
			B2[j] = (1000 * aLogY[i]) - A2[j] * aPhysY[i];
			j++;

			if(j == AB_COEFF_TAB_SIZE)
				break;
		}
		else
			return 1;
	}

	if(maxSize){
		_average_AB(j);
		return 0;
	}
	else
		return 1;
}

void DisplayCoeffCalibration(void){
	 DbgVar(1,250,"\r\nCoeff Calibration: A1=%d B1=%d   A2=%d B2=%d ",_A1,_B1,_A2,_B2);
}

void DisplayTouchPosXY(int touchIdx, XY_Touch_Struct pos, char *txt){
	if(GetTouchToTemp(touchIdx))
		DbgVar(1,100,"\r\nPos (%d--"Cya_"%d"_X_"--%d)  (%d--"Cya_"%d"_X_"--%d) %s",touchTemp[0].x, pos.x, touchTemp[1].x,	touchTemp[0].y, pos.y, touchTemp[1].y, txt);
}
void DisplayAnyTouchPosXY(void){
	if(press == ServiceTouch.press)
		DbgVar(1,20,"\r\nPos %d:  %d %d ",ServiceTouch.idx, ServiceTouch.pos[ServiceTouch.idx-1].x, ServiceTouch.pos[ServiceTouch.idx-1].y);
}

static uint16_t Calibration_GetX(uint32_t x)
{
	int32_t temp= (((_A1 * x) + _B1)/1000);

	if(temp<0)
		temp=0;
	else if(temp>LCD_GetXSize())
		temp=LCD_GetXSize();

	return (uint16_t)temp;
}

static uint16_t Calibration_GetY(uint32_t y)
{
	int32_t temp= (((_A2 * y) + _B2)/1000);

	if(temp<0)
		temp=0;
	else if(temp>LCD_GetYSize())
		temp=LCD_GetYSize();

	return (uint16_t)temp;
}

uint8_t IsCalibrationDone(void)
{
  return (Calibration_Done);
}

void CalibrationWasDone(void)
{
	Calibration_Done = 1;
}

static uint8_t GetTouchPos(XY_Touch_Struct *pos)
{
  if(touchDetect)		/* if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_4)==0) */
  {
	 touchDetect=0;
	 BSP_TS_ClearIT();
	 BSP_TS_GetState(&TS_State);

	 if(TS_State.TouchDetected)
	 {
		 pos->x = Calibration_GetX(TS_State.x);
	    pos->y = Calibration_GetY(TS_State.y);
	    return 1;
	 }
  }
  return 0;
}

static uint16_t GetTouchType(int *param)
{
	*param=0;
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i)
	{
		if(0 < Touch[i].index)
		{
			switch(Touch[i].id)
			{
				case ID_TOUCH_POINT:
					if( pressRelease == Touch[i].param ? 1 : Touch[i].param == ServiceTouch.press)
					{
						for(int j=0; j<ServiceTouch.idx; j++){
							if((ServiceTouch.pos[j].x >= Touch[i].pos[0].x) && (ServiceTouch.pos[j].x < Touch[i].pos[1].x) &&
								(ServiceTouch.pos[j].y >= Touch[i].pos[0].y) && (ServiceTouch.pos[j].y < Touch[i].pos[1].y))
							{
								if(press == ServiceTouch.press)
								{
									if(Touch[i].flags1 == 0){
										Touch[i].flags1 = 1;
										*param=j;
										return Touch[i].index;
									}
								}
								else{
									*param=j;
									return Touch[i].index;
								}
						}}
					}
					break;

				case ID_TOUCH_POINT_WITH_HOLD:
					if(ServiceTouch.press == press)
					{
						if(IS_RANGE(ServiceTouch.idx-1, 0, BUF_LCD_TOUCH_SIZE-1)){
						if((ServiceTouch.pos[ServiceTouch.idx-1].x >= Touch[i].pos[0].x) && (ServiceTouch.pos[ServiceTouch.idx-1].x < Touch[i].pos[1].x) &&
							(ServiceTouch.pos[ServiceTouch.idx-1].y >= Touch[i].pos[0].y) && (ServiceTouch.pos[ServiceTouch.idx-1].y < Touch[i].pos[1].y))
						{
							if(ServiceTouch.idx > Touch[i].param){
								if(0==Touch[i].flags1){
									Touch[i].flags1 = 1;
									*param = ServiceTouch.idx-1;
									return Touch[i].index;
								}
							}
						}
						else Touch[i].flags1 = 1;
						}
					}
					break;

				case ID_TOUCH_POINT_RELEASE_WITH_HOLD:
					if(ServiceTouch.press == press)
					{
						if(IS_RANGE(ServiceTouch.idx-1, 0, BUF_LCD_TOUCH_SIZE-1)){
						if((ServiceTouch.pos[ServiceTouch.idx-1].x >= Touch[i].pos[0].x) && (ServiceTouch.pos[ServiceTouch.idx-1].x < Touch[i].pos[1].x) &&
							(ServiceTouch.pos[ServiceTouch.idx-1].y >= Touch[i].pos[0].y) && (ServiceTouch.pos[ServiceTouch.idx-1].y < Touch[i].pos[1].y))
						{
								if(ServiceTouch.idx < Touch[i].param){
									if(!Touch[i].flags1)
										Touch[i].flags1 = 1;
								}
								else	Touch[i].flags1 = 2;
						}}
					}
					else{
							if(ServiceTouch.idx < Touch[i].param && 1 == Touch[i].flags1){
								*param = ServiceTouch.idx-1;
								return Touch[i].index;
							}
					}
					break;

				case ID_TOUCH_GET_ANY_POINT:
					if(ServiceTouch.press == press)
					{
						if(IS_RANGE(ServiceTouch.idx-1, 0, BUF_LCD_TOUCH_SIZE-1)){
						if((ServiceTouch.pos[ServiceTouch.idx-1].x >= Touch[i].pos[0].x) && (ServiceTouch.pos[ServiceTouch.idx-1].x < Touch[i].pos[1].x) &&
							(ServiceTouch.pos[ServiceTouch.idx-1].y >= Touch[i].pos[0].y) && (ServiceTouch.pos[ServiceTouch.idx-1].y < Touch[i].pos[1].y))
						{
							if(0 == (ServiceTouch.idx % Touch[i].param)){
								*param = ServiceTouch.idx-1;
								return Touch[i].index;
							}
						}}
					}
					break;

				case ID_TOUCH_GET_ANY_POINT_WITH_WAIT:
					if(ServiceTouch.press == press)
					{
						if(IS_RANGE(ServiceTouch.idx-1, 0, BUF_LCD_TOUCH_SIZE-1)){
						if((ServiceTouch.pos[ServiceTouch.idx-1].x >= Touch[i].pos[0].x) && (ServiceTouch.pos[ServiceTouch.idx-1].x < Touch[i].pos[1].x) &&
							(ServiceTouch.pos[ServiceTouch.idx-1].y >= Touch[i].pos[0].y) && (ServiceTouch.pos[ServiceTouch.idx-1].y < Touch[i].pos[1].y))
						{
							switch(Touch[i].flags1)
							{
								case 0:
									Touch[i].flags1 = 1;
									*param = ServiceTouch.idx-1;
									return Touch[i].index;

								case 1:
									if(ServiceTouch.idx > TIME_700_MS_TO_NUMBER_PROBE)
										Touch[i].flags1 = 2;
									break;

								case 2:
									if(0 == (ServiceTouch.idx % Touch[i].param)){
										Touch[i].flags2++;
										if(Touch[i].flags2 > TIME_5000_MS_TO_NUMBER_PROBE)
											Touch[i].flags1 = 3;
										*param = ServiceTouch.idx-1;
										return Touch[i].index;
									}
									break;

								case 3:
									if(0 == (ServiceTouch.idx % Touch[i].param/2)){
										*param = ServiceTouch.idx-1;
										return Touch[i].index;
									}
									break;
							}
						}}
					}
					break;

				case ID_TOUCH_MOVE_LEFT:
					if(ServiceTouch.press == Touch[i].param)
					{
						if(2 < ServiceTouch.idx)
						{
							if( ServiceTouch.pos[0].x >= Touch[i].pos[1].x
								&& ServiceTouch.pos[0].y >= Touch[i].pos[0].y && ServiceTouch.pos[0].y <= Touch[i].pos[1].y )
							{
								for(int j=1; j<ServiceTouch.idx; ++j)
								{
									if( ServiceTouch.pos[j].x <= Touch[i].pos[0].x
										&& ServiceTouch.pos[j].y >= Touch[i].pos[0].y && ServiceTouch.pos[j].y <= Touch[i].pos[1].y ){
										*param = j;
										return Touch[i].index;
									}
								}
							}
						}
					}
					break;

				case ID_TOUCH_MOVE_RIGHT:
					if(ServiceTouch.press == Touch[i].param)
					{
						if(2 < ServiceTouch.idx)
						{
							if( ServiceTouch.pos[0].x <= Touch[i].pos[0].x
								&& ServiceTouch.pos[0].y >= Touch[i].pos[0].y && ServiceTouch.pos[0].y <= Touch[i].pos[1].y )
							{
								for(int j=1; j<ServiceTouch.idx; ++j)
								{
									if( ServiceTouch.pos[j].x >= Touch[i].pos[1].x
										&& ServiceTouch.pos[j].y >= Touch[i].pos[0].y && ServiceTouch.pos[j].y <= Touch[i].pos[1].y ){
										*param = j;
										return Touch[i].index;
									}
								}
							}
						}
					}
					break;

				case ID_TOUCH_MOVE_UP:
					if(ServiceTouch.press == Touch[i].param)
					{
						if(2 < ServiceTouch.idx)
						{
							if( ServiceTouch.pos[0].y >= Touch[i].pos[1].y
								&& ServiceTouch.pos[0].x >= Touch[i].pos[0].x && ServiceTouch.pos[0].x <= Touch[i].pos[1].x )
							{
								for(int j=1; j<ServiceTouch.idx; ++j)
								{
									if( ServiceTouch.pos[j].y <= Touch[i].pos[0].y
										&& ServiceTouch.pos[j].x >= Touch[i].pos[0].x && ServiceTouch.pos[j].x <= Touch[i].pos[1].x ){
										*param = j;
										return Touch[i].index;
									}
								}
							}
						}
					}
					break;

				case ID_TOUCH_MOVE_DOWN:
					if(ServiceTouch.press == Touch[i].param)
					{
						if(2 < ServiceTouch.idx)
						{
							if( ServiceTouch.pos[0].y <= Touch[i].pos[0].y
								&& ServiceTouch.pos[0].x >= Touch[i].pos[0].x && ServiceTouch.pos[0].x <= Touch[i].pos[1].x )
							{
								for(int j=1; j<ServiceTouch.idx; ++j)
								{
									if( ServiceTouch.pos[j].y >= Touch[i].pos[1].y
										&& ServiceTouch.pos[j].x >= Touch[i].pos[0].x && ServiceTouch.pos[j].x <= Touch[i].pos[1].x ){
										*param = j;
										return Touch[i].index;
									}
								}
							}
						}
					}
					break;

				default:
					break;
			}
		}
	}
	return 0;
}
/* Poniższa funkcja jest wywoływana w jednym wątku i zapisuje do zmiennej: ServiceTouch.idx i .flags1  ale tylko gdy jest RELEASE  -> Wiec konfliktu nie ma */
void LCD_TOUCH_Service(void)
{
	XY_Touch_Struct pos = {0};

	if(GetTouchPos(&pos))
	{
		if(release == ServiceTouch.press){
			ServiceTouch.press = press;
			ServiceTouch.idx = 0;
			for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
				Touch[i].flags1 = 0;
				Touch[i].flags2 = 0;
			}
		}
		if(BUF_LCD_TOUCH_SIZE <= ServiceTouch.idx) ServiceTouch.idx = 0;

		ServiceTouch.pos[ServiceTouch.idx].x = pos.x;
		ServiceTouch.pos[ServiceTouch.idx++].y = pos.y;
	}
	else
		ServiceTouch.press = release;
}
/* Poniższa funkcja jest wywoływana w drugim wątku i zapisuje też do zmiennej: ServiceTouch.idx  i .flags1 , ale tylko gdy jest PRESS  -> Wiec konfliktu nie ma */
uint16_t LCD_TOUCH_GetTypeAndPosition(XY_Touch_Struct *posXY)
{
	uint16_t touchRecognize = 0;
	int param = 0;

	if(0 < ServiceTouch.idx)
	{
		touchRecognize = GetTouchType(&param);
		if( 0 != touchRecognize || ((release == ServiceTouch.press) && (0 == touchRecognize)) )
			ServiceTouch.idx = 0;
	}

	posXY->x = ServiceTouch.pos[param].x;
	posXY->y = ServiceTouch.pos[param].y;

	return touchRecognize;
}

static void clearTouchTemp(void)
{
	for(int i=0; i<MAX_NUMBER_PIONTS_TOUCH; ++i){
		touchTemp[i].x = 0;
		touchTemp[i].y = 0;
	}
}

static int isTouchTemp(void)
{
	if((touchTemp[0].x != touchTemp[1].x) || (touchTemp[0].y != touchTemp[1].y))
		return 1;
	else
		return 0;
}

int LCD_TOUCH_isPress(void){ return ServiceTouch.press; }

XY_Touch_Struct LCD_TOUCH_GetPos(void){
	XY_Touch_Struct pos = {0};
	int i = ServiceTouch.idx > 0 ? ServiceTouch.idx-1 : 0;
	pos.x = ServiceTouch.pos[i].x;
	pos.y = ServiceTouch.pos[i].y;
	return pos;
}

uint8_t LCD_TOUCH_testFunc(GET_SET action){
	static uint8_t testTouchResolutionFlag=0;
	switch((int)action){
	case _RST:
		testTouchResolutionFlag=0;
		Dbg(1,"\r\nSTOP touch test ");
		break;
	case _SET:
		CALCULATE_MinMaxAvr(_RST,unUsed,NULL,							_uint32);
		CALCULATE_MinMaxAvr(_SET1,0,_Uint32(VALPERC(LCD_X,120)), _uint32);
		CALCULATE_MinMaxAvr(_SET2,0,_Uint32(0), 						_uint32);
		CALCULATE_MinMaxAvr(_SET1,1,_Uint32(VALPERC(LCD_Y,120)), _uint32);
		CALCULATE_MinMaxAvr(_SET2,1,_Uint32(0), 						_uint32);
		testTouchResolutionFlag=1;
		Dbg(1,"\r\n"CoR_"START touch test "_X_);
		break;
	case _CALC:
		XY_Touch_Struct posTest = LCD_TOUCH_GetPos();
		CALCULATE_MinMaxAvr(_CALC,0, _Uint32(posTest.x), _uint32);
		CALCULATE_MinMaxAvr(_CALC,1, _Uint32(posTest.y), _uint32);
		break;
	case _GET:
		#define I(var) 	*((uint32_t*)var)
		struct_MATH x = CALCULATE_MinMaxAvr(_GET,0,NULL,_uint32);		/* uint32_t mid = *((uint32_t*)(CALCULATE_MinMaxAvr(_GET,0,NULL,_uint32).mid)); */
		struct_MATH y = CALCULATE_MinMaxAvr(_GET,1,NULL,_uint32);
		int size = (sizeof(x) / sizeof(x.min));
		int L[size];
		void* p1 = x.min;
		void* p2 = y.min;
		char buf[150]={0};
		char *t1[size], *t2[size];

		for(int i=0;i<size;++i)
			L[i] = strlen(DispLongNmb(MAXVAL2(I(p1+i),I(p2+i)),NULL));

		for(int i=0,n=0; i<size; ++i, n+=L[i-1]+1){
			DispLongNmb(I(p1+i), t1[i]=&buf[ 0+n]);
			DispLongNmb(I(p2+i), t2[i]=&buf[70+n]);
		}
/*		int L1 = strlen(DispLongNmb(MAXVAL2(I(x.min),I(y.min)),NULL));
		int L2 = strlen(DispLongNmb(MAXVAL2(I(x.max),I(y.max)),NULL));
		int L3 = strlen(DispLongNmb(MAXVAL2(I(x.div),I(y.div)),NULL));
		int L4 = strlen(DispLongNmb(MAXVAL2(I(x.mid),I(y.mid)),NULL));
		int L5 = strlen(DispLongNmb(MAXVAL2(I(x.sum),I(y.sum)),NULL));
		int L6 = strlen(DispLongNmb(MAXVAL2(I(x.itr),I(y.itr)),NULL));
		int L7 = strlen(DispLongNmb(MAXVAL2(I(x.avr),I(y.avr)),NULL));

		char Bx1[4];	DispLongNmb(I(x.min),Bx1);
		char Bx2[4];	DispLongNmb(I(x.max),Bx2);
		char Bx3[4];	DispLongNmb(I(x.div),Bx3);
		char Bx4[4];	DispLongNmb(I(x.mid),Bx4);
		char Bx5[30];  DispLongNmb(I(x.sum),Bx5);
		char Bx6[20];  DispLongNmb(I(x.itr),Bx6);
		char Bx7[4];	DispLongNmb(I(x.avr),Bx7);

		char By1[4];	DispLongNmb(I(y.min),By1);
		char By2[4];	DispLongNmb(I(y.max),By2);
		char By3[4];	DispLongNmb(I(y.div),By3);
		char By4[4];	DispLongNmb(I(y.mid),By4);
		char By5[30];  DispLongNmb(I(y.sum),By5);
		char By6[20];  DispLongNmb(I(y.itr),By6);
		char By7[4];	DispLongNmb(I(y.avr),By7);
*/
		DbgVar2(1,300,"\r\n"CoGr_"PosX min:"_X_"%*s "CoGr_"max:"_X_"%*s "CoGr_"div:"_X_"%*s "CoGr_"mid:"_X_"%*s "CoGr_"sum:"_X_"%*s "CoGr_"itr:"_X_"%*s "CoGr_"avr:"_X_"%*s",L[0],t1[0],L[1],t1[1],L[2],t1[2],L[3],t1[3],L[4],t1[4],L[5],t1[5],L[6],t1[6]);
		DbgVar2(1,300,"\r\n"CoGr_"PosY min:"_X_"%*s "CoGr_"max:"_X_"%*s "CoGr_"div:"_X_"%*s "CoGr_"mid:"_X_"%*s "CoGr_"sum:"_X_"%*s "CoGr_"itr:"_X_"%*s "CoGr_"avr:"_X_"%*s",L[0],t2[0],L[1],t2[1],L[2],t2[2],L[3],t2[3],L[4],t2[4],L[5],t2[5],L[6],t2[6]);
		break;
	}
	return testTouchResolutionFlag;
	#undef I
	#undef SUM_STR
}

void LCDTOUCH_testFunc(void){
	static uint8_t block=0;
	if(press==LCD_TOUCH_isPress()){
		if(0==block){ LCD_TOUCH_testFunc(_SET); block=1; }
						  LCD_TOUCH_testFunc(_CALC);
	}
	else{
		if(1==block){ LCD_TOUCH_testFunc(_GET);
						  LCD_TOUCH_testFunc(_RST); block=0; }
}}

int LCD_TOUCH_Set(uint16_t ID, uint16_t idx, uint8_t param)
{
  if(isTouchTemp())
  {
	  for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i)
	  {
		  if(Touch[i].index == idx)
		  {
			  clearTouchTemp();
			  return -2;
		  }
	  }
	  for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i)
	  {
		  if(0 == Touch[i].index && 0 == susspendTouch[i])
		  {
			  Touch[i].id = ID;
			  Touch[i].index = idx;
			  Touch[i].param = param;
			  Touch[i].flags1 = 0;
			  Touch[i].flags2 = 0;

			  for(int j=0; j<MAX_NUMBER_PIONTS_TOUCH; ++j)
				  Touch[i].pos[j] = touchTemp[j];

			  clearTouchTemp();
			  return i;
		  }
	  }
  }
  clearTouchTemp();
  return -1;
}

int LCD_TOUCH_Update(uint16_t ID, uint16_t idx, uint8_t param)
{
  if(isTouchTemp())
  {
	  for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i)
	  {
		  if(Touch[i].index == idx || susspendTouch[i] == idx)
		  {
			  Touch[i].id = ID;
			  Touch[i].param = param;
			  Touch[i].flags1 = 0;
			  Touch[i].flags2 = 0;

			  for(int j=0; j<MAX_NUMBER_PIONTS_TOUCH; ++j)
				  Touch[i].pos[j] = touchTemp[j];

			  clearTouchTemp();
			  return i;
		  }
	  }
  }
  clearTouchTemp();
  return -1;
}

int GetTouchToTemp(uint16_t idx)
{
	int i;
	for(i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i)
	{
		if(Touch[i].index==idx)
		{
			touchTemp[0].x= Touch[i].pos[0].x;
			touchTemp[0].y= Touch[i].pos[0].y;
			touchTemp[1].x= Touch[i].pos[1].x;
			touchTemp[1].y= Touch[i].pos[1].y;
			return 1;
		}
	}
	return 0;
}

int LCD_TOUCH_SetNewPos(uint16_t idx, uint16_t x, uint16_t y, uint16_t xLen, uint16_t yLen)
{
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i)
	{
		if(Touch[i].index==idx)
		{
			Touch[i].pos[0].x = x;
			Touch[i].pos[0].y = y;
			Touch[i].pos[1].x = x + xLen;
			Touch[i].pos[1].y = y + yLen;
			return 1;
		}
	}
	return 0;
}

void LCD_TOUCH_DeleteAllSetTouch(void){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		Touch[i].index=0;
		susspendTouch[i]=0;
	}
}
void LCD_TOUCH_DeleteSelectTouch(uint16_t idx){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(Touch[i].index==idx){
			Touch[i].index=0;
			susspendTouch[i]=0;
	}}
}
void DeleteAllTouchWithout(uint16_t idx){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(Touch[i].index!=idx){
			Touch[i].index=0;
			susspendTouch[i]=0;
	}}
}

void LCD_TOUCH_SusspendAllTouchsWithout(uint16_t start_idx, uint16_t stop_idx){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(!IS_RANGE(Touch[i].index, start_idx, stop_idx)){
			if(0 < Touch[i].index && 0 == susspendTouch[i]){
				susspendTouch[i] = Touch[i].index;
				Touch[i].index=0;
}}}}
void LCD_TOUCH_SusspendAllTouchs(void){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(0 < Touch[i].index && 0 == susspendTouch[i]){
			susspendTouch[i] = Touch[i].index;
			Touch[i].index=0;
}}}
void LCD_TOUCH_RestoreAllSusspendedTouchs(void){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(0 < susspendTouch[i] && 0 == Touch[i].index){
			Touch[i].index = susspendTouch[i];
			susspendTouch[i]=0;
}}}
void LCD_TOUCH_SusspendTouch(uint16_t idx){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(Touch[i].index == idx){
			if(0 < Touch[i].index && 0 == susspendTouch[i]){
				susspendTouch[i] = Touch[i].index;
				Touch[i].index=0;
}}}}
void LCD_TOUCH_SusspendTouchs2(uint16_t idx1,uint16_t idx2,uint16_t idx3,uint16_t idx4,uint16_t idx5,uint16_t idx6,uint16_t idx7,uint16_t idx8,uint16_t idx9,uint16_t idx10){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(Touch[i].index==idx1||Touch[i].index==idx2||Touch[i].index==idx3||Touch[i].index==idx4||Touch[i].index==idx5||Touch[i].index==idx6||Touch[i].index==idx7||Touch[i].index==idx8||Touch[i].index==idx9||Touch[i].index==idx10){
			if(0 < Touch[i].index && 0 == susspendTouch[i]){
				susspendTouch[i] = Touch[i].index;
				Touch[i].index=0;
}}}}
void LCD_TOUCH_SusspendNmbTouch(int nmb, uint16_t start_idx){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		for(int j=0; j<nmb; ++j){
		if(Touch[i].index == start_idx+j){
			if(0 < Touch[i].index && 0 == susspendTouch[i]){
				susspendTouch[i] = Touch[i].index;
				Touch[i].index=0;
				break;
}}}}}
void LCD_TOUCH_RestoreSusspendedTouch(uint16_t idx){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(susspendTouch[i] == idx && 0 < idx && 0 == Touch[i].index){
			Touch[i].index = susspendTouch[i];
			susspendTouch[i]=0;
			break;
}}}
void LCD_TOUCH_RestoreSusspendedTouchs2(uint16_t idx1,uint16_t idx2,uint16_t idx3,uint16_t idx4,uint16_t idx5,uint16_t idx6,uint16_t idx7,uint16_t idx8,uint16_t idx9,uint16_t idx10){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(0 < susspendTouch[i]){
			if(susspendTouch[i]==idx1||susspendTouch[i]==idx2||susspendTouch[i]==idx3||susspendTouch[i]==idx4||susspendTouch[i]==idx5||susspendTouch[i]==idx6||susspendTouch[i]==idx7||susspendTouch[i]==idx8||susspendTouch[i]==idx9||susspendTouch[i]==idx10){
				if(0 == Touch[i].index){
					Touch[i].index = susspendTouch[i];
					susspendTouch[i]=0;
}}}}}
void LCD_TOUCH_RestoreSusspendedTouchs(uint16_t start_idx, uint16_t stop_idx){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(IS_RANGE(susspendTouch[i], start_idx, stop_idx) && 0 == Touch[i].index){
			Touch[i].index = susspendTouch[i];
			susspendTouch[i]=0;
}}}
void LCD_TOUCH_SusspendTouchs(uint16_t start_idx, uint16_t stop_idx){
	for(int i=0; i<MAX_OPEN_TOUCH_SIMULTANEOUSLY; ++i){
		if(IS_RANGE(Touch[i].index, start_idx, stop_idx)){
			if(0 < Touch[i].index && 0 == susspendTouch[i]){
				susspendTouch[i] = Touch[i].index;
				Touch[i].index=0;
}}}}
void LCDTOUCH_ActiveOnly(uint16_t idx1,uint16_t idx2,uint16_t idx3,uint16_t idx4,uint16_t idx5,uint16_t idx6,uint16_t idx7,uint16_t idx8,uint16_t idx9,uint16_t idx10, uint16_t start_idx, uint16_t stop_idx){
	LCD_TOUCH_SusspendAllTouchs();
	/* is the same as: if(NoTouch==idx1) */
	if(idx1)	LCD_TOUCH_RestoreSusspendedTouch(idx1);
	if(idx2)	LCD_TOUCH_RestoreSusspendedTouch(idx2);
	if(idx3)	LCD_TOUCH_RestoreSusspendedTouch(idx3);
	if(idx4)	LCD_TOUCH_RestoreSusspendedTouch(idx4);
	if(idx5)	LCD_TOUCH_RestoreSusspendedTouch(idx5);
	if(idx6)	LCD_TOUCH_RestoreSusspendedTouch(idx6);
	if(idx7)	LCD_TOUCH_RestoreSusspendedTouch(idx7);
	if(idx8)	LCD_TOUCH_RestoreSusspendedTouch(idx8);
	if(idx9)	LCD_TOUCH_RestoreSusspendedTouch(idx9);
	if(idx10)	LCD_TOUCH_RestoreSusspendedTouch(idx10);
	LCD_TOUCH_RestoreSusspendedTouchs(start_idx, stop_idx);
}
void LCD_TOUCH_DeleteSelectAndSusspendTouch(uint16_t idx){
	LCD_TOUCH_RestoreSusspendedTouch(idx);
	LCD_TOUCH_DeleteSelectTouch(idx);
}

uint16_t LCD_TOUCH_SetTimeParam_ms(uint16_t time){
	return time/SERVICE_TOUCH_PROB_TIME_MS;
}

/* ------------- Touch Scroll Service -----------*/
int LCD_TOUCH_ScrollSel_Service(uint8_t nr, uint8_t pressRelease, uint16_t *y, uint8_t rollRateCoeff)
{
	static struct SCROLL_SEL{
		uint16_t posY;
		uint16_t prevY[SCROLL_SEL__NUMBER_PROBE2SEL];
		uint16_t countTouchProbe2Sel;
		int delta;
		int delta_copy;
		uint8_t stateTouch;
		uint8_t itx;
		uint8_t selPoint;
	}roll[SCROLL_SEL__MAX_NUMBER]={0};

	switch(pressRelease)
	{
	case press:
		roll[nr].stateTouch = pressRelease;
		if( (roll[nr].posY > 0) && (roll[nr].countTouchProbe2Sel >= SCROLL_SEL__NUMBER_PROBE2SEL) )
		{
			roll[nr].delta = (int)roll[nr].posY - *y;
			if(ABS(roll[nr].delta) > SCROLL_SEL__SEL_SPREAD)
				roll[nr].selPoint = 1;
		}
		else roll[nr].selPoint = 0;

		roll[nr].posY = *y;
		roll[nr].prevY[ (roll[nr].itx < SCROLL_SEL__NUMBER_PROBE2SEL-1) ? roll[nr].itx++ : roll[nr].itx ] = *y;
		roll[nr].countTouchProbe2Sel++;

		roll[nr].delta *= rollRateCoeff;
		return roll[nr].delta;

	case release:
		roll[nr].stateTouch = pressRelease;
		roll[nr].posY = 0;
		roll[nr].delta_copy = roll[nr].delta;
		roll[nr].delta = 0;
		roll[nr].itx = 0;
		roll[nr].countTouchProbe2Sel=0;

		if(roll[nr].selPoint == 0)
			return roll[nr].prevY[0];
		else
			return 0;

	case checkPress:
		*y = roll[nr].stateTouch;
		switch(roll[nr].stateTouch){
			case press:		return roll[nr].delta;
			case release:	return roll[nr].prevY[0];
		}
		break;

	case neverMind:
		return roll[nr].delta_copy;
	}

	return 0;
}

static int LCD_TOUCH_ScrollSel_stateRoll(int action){
	static int actRoll=_MID;
	if(_GET==action) return actRoll;
	return actRoll=(int)action;
}

int LCD_TOUCH_ScrollSel_SetCalculate(uint8_t nr, uint16_t *offsWin, uint16_t *selWin, uint16_t WinposY, uint16_t heightAll, uint16_t heightKey, uint16_t heightWin)
{
	static struct SCROLL_SEL{
		uint16_t offsWin;
		uint16_t selWin;
		uint16_t rateCoeff;
	}roll[SCROLL_SEL__MAX_NUMBER]={0};

	if(NULL == offsWin && NULL == selWin){
		if(WinposY)	return roll[nr].rateCoeff;
		else			return roll[nr].selWin;
	}

	if(0 == WinposY && 0 == heightAll && 0 == heightKey){
		roll[nr].offsWin = *offsWin;
		roll[nr].selWin = *selWin;
		roll[nr].rateCoeff = heightWin;			/*	'heightWin' as param RateCoeff to write setup */
		return 1;
	}

	void _refreshWin(void){
		*offsWin = roll[nr].offsWin;
		*selWin = roll[nr].selWin;
	}

	uint16_t statePress;
	int val = LCD_TOUCH_ScrollSel_Service(nr,checkPress, &statePress,0);
	LCD_TOUCH_ScrollSel_stateRoll(_MID);
	switch(statePress)
	{
		case press:
			if(val > 0){
				if(roll[nr].offsWin < ((heightAll-heightWin)+1 - val) )
					roll[nr].offsWin += val;
				else{
					roll[nr].offsWin = heightAll-heightWin;
					LCD_TOUCH_ScrollSel_stateRoll(_MIN); }
			}
			else{
				val *= -1;
				if(roll[nr].offsWin > val)
					roll[nr].offsWin -= val;
				else{
					roll[nr].offsWin = 0;
					LCD_TOUCH_ScrollSel_stateRoll(_MAX); }
			}
			_refreshWin();
			return statePress;

		case release:
			roll[nr].selWin  = (val + roll[nr].offsWin - WinposY) / (heightKey-1);
			_refreshWin();
			return statePress;
	}
	return neverMind;
}

int LCD_TOUCH_ScrollSel_GetSel(uint8_t nr){
	return LCD_TOUCH_ScrollSel_SetCalculate(nr, NULL, NULL, 0,0,0,0);
}
int LCD_TOUCH_ScrollSel_GetRateCoeff(uint8_t nr){
	return LCD_TOUCH_ScrollSel_SetCalculate(nr, NULL, NULL, 1,0,0,0);
}

void LCD_TOUCH_ScrollSel_FreeRolling(uint8_t nr, FUNC1_DEF(pFunc), VOID_FUNCTION *pfuncBlocking)
{
	int delta = LCD_TOUCH_ScrollSel_Service(nr,neverMind,NULL,0);
	if(delta!=0)
	{
		int paramCoeff[3] = {150,40,0};
		int temp = 0, temp2 = 0;
		uint16_t val = 0;

		/* DbgVar(1,7,BkR_" %d "_X,delta); */
		if(delta > 0)
		{
			delta *= paramCoeff[0];
			val = delta;
			while(1)
			{
				if(NULL!=pfuncBlocking) pfuncBlocking();

				if(LCD_TOUCH_ScrollSel_Service(nr,press, &val,1))
					pFunc(FUNC1_ARG);

				if(_MIN==LCD_TOUCH_ScrollSel_stateRoll(_GET))
					break;

				if(press == LCD_TOUCH_isPress())
					break;

				paramCoeff[2] += 2;
				temp += (paramCoeff[1]+paramCoeff[2]);
				temp2 = delta/temp;

				if(temp2 > 0)
				{
					if(val > temp2)
						val -= temp2;
					else
						break;
				}
				else
					break;
			}
		}
		else
		{
			delta = ABS(delta)*paramCoeff[0];
			val = 1;
			while(1)
			{
				if(NULL!=pfuncBlocking) pfuncBlocking();

				if(LCD_TOUCH_ScrollSel_Service(nr,press, &val,1))
					pFunc(FUNC1_ARG);

				if(_MAX==LCD_TOUCH_ScrollSel_stateRoll(_GET))
					break;

				if(press == LCD_TOUCH_isPress())
					break;

				paramCoeff[2] += 2;
				temp += (paramCoeff[1]+paramCoeff[2]);
				temp2 = delta/temp;

				if(temp2 > 0)
				{
					if(val < delta-temp2)
						val += temp2;
					else
						break;
				}
				else
					break;
			}
		}
	}

	LCD_TOUCH_ScrollSel_Service(nr,release,NULL,0);
}

uint8_t LCD_TOUCH_ScrollSel_DetermineRateRoll(uint8_t nr, uint16_t touchState, uint16_t xPos){
	 return (GetTouchToTemp(touchState) && IS_RANGE(xPos, touchTemp[0].x+3*(touchTemp[1].x-touchTemp[0].x)/4, touchTemp[1].x)) ? LCD_TOUCH_ScrollSel_GetRateCoeff(nr) : 1;
}
int LCDTOUCH_IsScrollPress(int IDroll, uint16_t state, XY_Touch_Struct *pos, int timID){
	vTimerService(timID,start_time,noUse);
	return LCD_TOUCH_ScrollSel_Service(IDroll,press, &(pos->y), LCD_TOUCH_ScrollSel_DetermineRateRoll(IDroll,state,pos->x));
}
int LCDTOUCH_IsScrollRelease(uint8_t IDroll, FUNC1_DEF(pFunc), VOID_FUNCTION *pfuncBlocking, int timID){
	if(LCD_TOUCH_ScrollSel_Service(IDroll,release,NULL,0) && 0==vTimerService(timID,stop_time,1000)){
		pFunc(FUNC1_ARG);
		return LCD_TOUCH_ScrollSel_GetSel(IDroll);
	}
	else{	LCD_TOUCH_ScrollSel_FreeRolling(IDroll, pFunc,FUNC1_ARG, pfuncBlocking);
			vTimerService(timID,reset_time,noUse);
			return -1;
}}
/* ------------- End Touch Scroll Service -----------*/

int LCDTOUCH_Set(uint16_t startX, uint16_t startY, uint16_t width, uint16_t height, uint16_t ID, uint16_t idx, uint8_t param){
 	touchTemp[0].x= startX;
 	touchTemp[0].y= startY;
 	touchTemp[1].x= touchTemp[0].x+width;
 	touchTemp[1].y= touchTemp[0].y+height;
 	return LCD_TOUCH_Set(ID,idx,param);
}
int LCDTOUCH_Update(uint16_t startX, uint16_t startY, uint16_t width, uint16_t height, uint16_t ID, uint16_t idx, uint8_t param){
 	touchTemp[0].x= startX;
 	touchTemp[0].y= startY;
 	touchTemp[1].x= touchTemp[0].x+width;
 	touchTemp[1].y= touchTemp[0].y+height;
 	return LCD_TOUCH_Update(ID,idx,param);
}

int LCDTOUCH_UserStatus(int nr){
	if(nr!=_GET) return (userStatusTouch=nr);
	else			 return userStatusTouch;
}

/* ------------- Touch Main Screen Service -----------*/
#define TOUCH_POINTS 		int
#define KEYBOARD_TYPES 		int
#define SELECT_PRESS_BLOCK int
static uint16_t touchFlag=0;
static uint16_t screenTouchStatePrev=0, screenTouchStatePrev2=0;
XY_Touch_Struct screenTouchPos;
uint16_t 		 screenTouchState;

int BlockTouchForTime(int action, int timerID){
	static int _blokTouchForTime= 0;
	switch(action){
		case _ON:  { _blokTouchForTime= 1;	vTimerService(timerID,restart_time,noUse); break; }
		case _OFF: { _blokTouchForTime= 0; break; }
		case _GET: { break; }
	}
	return _blokTouchForTime;
}
int CheckTouchForTime(uint16_t touchName, int timerID){
	return CONDITION(BlockTouchForTime(_GET,timerID),touchName,0/*NO_TOUCH*/);
}

void SetTouchFlag		(void){ touchFlag=1; }
void TouchScreenInit	(void){ touchFlag=0; }
int  IsSetTouchFlag	(void){ if(touchFlag){ touchFlag=0; return 1; } 	return 0; }
void TOUCHSCREEN_Exit(void){ screenTouchStatePrev=0; screenTouchStatePrev2=0; }

void _SaveState (void){ screenTouchStatePrev =screenTouchState; }		//zmienic na lepsze nazwy
/*	void _RstState	 (void){ screenTouchStatePrev =0; } */
void _SaveState2(void){ screenTouchStatePrev2=screenTouchState; }
void _RstState2 (void){ screenTouchStatePrev2=0; 		}

int _WasState(int point){
	if(release==LCD_TOUCH_isPress() && point==screenTouchStatePrev){
		screenTouchStatePrev = screenTouchState;
		return 1;
	}
	else return 0;
}
int _WasStateRange(int point1, int point2){
	if(release==LCD_TOUCH_isPress() && IS_RANGE(screenTouchStatePrev,point1,point2)){
		screenTouchStatePrev = screenTouchState;
		return 1;
	}
	else return 0;
}
int _WasStatePrev(int rangeMin,int rangeMax){
	return (IS_RANGE(screenTouchStatePrev,rangeMin,rangeMax) && screenTouchStatePrev!=screenTouchState);
}
void _RestoreSusspendedTouchsByAnotherClickItem(TOUCH_POINTS prev,TOUCH_POINTS prevStart,TOUCH_POINTS prevStop, 	TOUCH_POINTS not1,TOUCH_POINTS not2,TOUCH_POINTS not3,TOUCH_POINTS not4,TOUCH_POINTS not5,TOUCH_POINTS not6,TOUCH_POINTS not7,TOUCH_POINTS not8,TOUCH_POINTS not9,TOUCH_POINTS not10, 		TOUCH_POINTS unblock1,TOUCH_POINTS unblock2,TOUCH_POINTS unblock3,TOUCH_POINTS unblock4,TOUCH_POINTS unblock5,TOUCH_POINTS unblock6,TOUCH_POINTS unblock7,TOUCH_POINTS unblock8,TOUCH_POINTS unblock9,TOUCH_POINTS unblock10){
	if(screenTouchState){
		if((prev==screenTouchStatePrev2 || IS_RANGE(screenTouchStatePrev2,prevStart,prevStop)) && (prev!=screenTouchState && !IS_RANGE(screenTouchState,prevStart,prevStop)) && (not1!=screenTouchState && not2!=screenTouchState && not3!=screenTouchState && not4!=screenTouchState && not5!=screenTouchState && not6!=screenTouchState && not7!=screenTouchState && not8!=screenTouchState && not9!=screenTouchState && not10!=screenTouchState)){
			LCD_TOUCH_RestoreSusspendedTouchs2(unblock1,unblock2,unblock3,unblock4,unblock5,unblock6,unblock7,unblock8,unblock9,unblock10);
			screenTouchStatePrev2=0;
}}}

#undef TOUCH_POINTS
#undef KEYBOARD_TYPES
#undef SELECT_PRESS_BLOCK
/* ------------- End Main Screen Service -----------*/

