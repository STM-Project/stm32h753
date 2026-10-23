/*
 * LCD_BasicGaphics.c
 *
 *  Created on: 11.04.2021
 *      Author: Elektronika RM
 */

#include <LCD_BasicGraphics.h>
#include <stdarg.h>
#include <math.h>
#include <stdbool.h>
#include "LCD_Hardware.h"
#include "LCD_Common.h"
#include "rng.h"
#include "double_float.h"

/*	#pragma GCC push_options
	#pragma GCC optimize ("-Ofast")	*/

#define MAX_SIZE_TAB_AA		365
#define MAX_LINE_BUFF_CIRCLE_SIZE  100
#define MAX_DEGREE_CIRCLE  10
#define GRAPH_MAX_SIZE_POSXY	10000
#define SIZE_ONE_CHART		sizeof(structGetSmpl) + GRAPH_MAX_SIZE_POSXY*sizeof(structPosU16) + 2*GRAPH_MAX_SIZE_POSXY*sizeof(structRepPos)
#define CHART_PTR_MEM_SIZE		32*32
#define CHART_RCT_MEM_SIZE		120*60
#define MAX_CHARTS_SIMULTANEOUSLY	8

/* Select one memory type for graphic of chart */
/* #define GRAPH_MEMORY_RAM */
/* #define GRAPH_MEMORY_SDRAM */
#define GRAPH_MEMORY_SDRAM2

#define AA_NO_FRAME	 (1<<28)
#define AA_OUT_OFF  	 (1<<24)
#define AA_IN_OFF  	 (1<<26)
#define READ_BKCOLOR  (1<<25)
#define COLOR_TEST		0x12345678		/* Reserved colors - Not use this color in other applications */
#define COLOR_TEST_1		0x12345677
#define COLOR_TEST_2		0x12345679

#define _IS_NOT_PXL(i,color1,color2,color3,color4)		(pLcd[i]!=color1 && pLcd[i]!=color2 && pLcd[i]!=color3 && pLcd[i]!=color4)
#define _IS_NEXT_PXL(bkX,i,color)	(pLcd[(i)+1]==color || pLcd[(i)-1]==color || pLcd[(i)+bkX]==color || pLcd[(i)-bkX]==color || pLcd[(i)+bkX+1]==color || pLcd[(i)+bkX-1]==color || pLcd[(i)-bkX+1]==color || pLcd[(i)-bkX-1]==color)

#define _PLCD(offs,x,y)	 	pLcd[(offs)+widthBk*(y)+(x)]
#define _K(x,y)	 			widthBk*(y)+(x)

#define _2LOOP(ix,iy,widthX,widthY)								for(int (iy)=0;(iy)<(widthY);(iy)++){ for(int (ix)=0;(ix)<(widthX);(ix)++){
#define _2LOOP_INIT(init,ix,iy,widthX,widthY)				init;  		 _2LOOP(ix,iy,widthX,widthY)
#define _2LOOP_2INIT(init,init2,ix,iy,widthX,widthY)		init; init2; _2LOOP(ix,iy,widthX,widthY)
#define _2LOOP_END		}}
#define _1LOOP_END		}

typedef enum{
	RightUpDir1,
	RightUpDir0,
	RightDownDir0,
	RightDownDir1,
	LeftUpDir1,
	LeftUpDir0,
	LeftDownDir0,
	LeftDownDir1,
	Equal,
	RightUpDownDir1,
	RightDownUpDir1
}GRAPH_FUNCTION_TYPE;

typedef enum{
	DegTo45,
	DegAbove45,
}GRAPH_CORRECT45DEG_AA;

ALIGN_32BYTES(uint32_t pLcd[LCD_BUFF_XSIZE*LCD_BUFF_YSIZE] __attribute__ ((section(".lcd_work"))));

static uint32_t k, kCopy;
static uint32_t buff_AA[MAX_SIZE_TAB_AA];
static uint32_t buff2_AA[MAX_SIZE_TAB_AA];
static structPosition pos,pos_prev;

typedef struct
{	float c1;
	float c2;
}AACoeff_RoundFrameRectangle;

typedef struct
{	uint8_t lineBuff[MAX_LINE_BUFF_CIRCLE_SIZE];
	float outRatioStart;
	float inRatioStart;
	float outRatioStart_prev;
	float inRatioStart_prev;
	uint16_t width;
	uint16_t width_prev;
	uint16_t x0;
	uint16_t y0;
	uint16_t degree[MAX_DEGREE_CIRCLE+1];
	uint32_t degColor[MAX_DEGREE_CIRCLE];
	float tang[MAX_DEGREE_CIRCLE];
	float coeff[MAX_DEGREE_CIRCLE];
	uint8_t rot[MAX_DEGREE_CIRCLE];
	uint16_t correctForWidth;
	uint16_t correctPercDeg[2];
	float errorDecision[2];
}Circle_Param;

typedef struct{
	structPosU16 startXYchart;		/* win of chart position (x,y) starts from middle of Y (not on top Y) */
	u16 yMinMaxchart[2];				/* Y of top win of chart  and  Y of bottom win of chart */
	u16 sizeX;
	structPosU16 pos;			/* calculated position of only pointer */
	structSizeU16 size;		/* calculated size of only pointer */
	structPointParam ptr;
	u8 memInUse;
	u16 chartBkW;
	u32 *ptrMem;
	u32 *rctMem;
	u32 CounterBusyBytesForFontsImages_copy;
}CHART_PTR_PREV;

static AACoeff_RoundFrameRectangle AA;
static uint8_t correctLine_AA=0;
static Circle_Param Circle = {.correctForWidth= 80, .correctPercDeg= {70, 80}, .errorDecision= {0.1, 0.4}};

USER_GRAPH_PARAM USER_GRAPH_PARAM_Zero = {0};

LCD_SDRAM static u32 			chartPtrMem					[ MAX_CHARTS_SIMULTANEOUSLY ][ CHART_PTR_MEM_SIZE ];			/* in future allocate as dynamic memory as for fonts memory */
LCD_SDRAM static u32 			chartRctMem					[ MAX_CHARTS_SIMULTANEOUSLY ][ CHART_RCT_MEM_SIZE ];
static CHART_PTR_PREV 	ptrPrev						[ MAX_CHARTS_SIMULTANEOUSLY ] = {0};
static int 					chartMemOffsForMemNr		[ MAX_CHARTS_SIMULTANEOUSLY ] = {0};

#if defined(GRAPH_MEMORY_RAM)

	static structGetSmpl posXY_par [1]= {0};
	static structPosU16  posXY	  	 [GRAPH_MAX_SIZE_POSXY] = {0};
	static structRepPos 	posXY_rep [GRAPH_MAX_SIZE_POSXY] = {0};

#elif defined(GRAPH_MEMORY_SDRAM)

	LCD_SDRAM static structGetSmpl posXY_par [1];
	LCD_SDRAM static structPosU16  posXY	  	 [GRAPH_MAX_SIZE_POSXY];
	LCD_SDRAM static structRepPos 	posXY_rep [GRAPH_MAX_SIZE_POSXY];

#elif defined(GRAPH_MEMORY_SDRAM2)

	static structGetSmpl *posXY_par = NULL;
	static structPosU16  *posXY 	  = NULL;
	static structRepPos  *posXY_rep = NULL;

#endif

static u32 posLinePoints[LCD_BUFF_XSIZE] = {0};
static u16 nmbrLinePoints = 0;
static u32 memOffsForGraphOwnFunc = 0;

uint16_t* GET_CIRCLE_correctForWidth(void) {	return &Circle.correctForWidth;	  }
uint16_t* GET_CIRCLE_correctPercDeg(int nr){	return &Circle.correctPercDeg[nr]; }
float* 	 GET_CIRCLE_errorDecision(int nr) {	return &Circle.errorDecision[nr];  }

void SET_CIRCLE_errorDecision(int nr, float decis){ Circle.errorDecision[nr]= decis; }

void CIRCLE_errorDecision(int nr, ON_OFF action){
	static float decis[2]= {0.0};
	switch((int)action){
		case _ON:
			Circle.errorDecision[nr]= decis[nr];
			break;
		case _OFF:
			decis[nr]= Circle.errorDecision[nr];
			Circle.errorDecision[nr]= 0.0;
			break;
	}
}

static void Set_AACoeff(int pixelsInOneSide, uint32_t colorFrom, uint32_t colorTo, float ratioStart)
{
	float incr= (1-ratioStart)/pixelsInOneSide;
	buff_AA[0]=pixelsInOneSide;
	for(int i=0;i<pixelsInOneSide;++i){
		if(1+i>=MAX_SIZE_TAB_AA)
			break;
		buff_AA[1+i]= GetTransitionColor(colorFrom,colorTo, ratioStart+i*incr);
}}
static void Set_AACoeff2(int pixelsInOneSide, uint32_t colorFrom, uint32_t colorTo, float ratioStart)
{
	float incr= (1-ratioStart)/pixelsInOneSide;
	buff2_AA[0]=pixelsInOneSide;
	for(int i=0;i<pixelsInOneSide;++i){
		if(1+i>=MAX_SIZE_TAB_AA)
			break;
		buff2_AA[1+i]= GetTransitionColor(colorFrom,colorTo, ratioStart+i*incr);
}}

static void _FillBuff(int itCount, uint32_t color)
{
	if(itCount>10)
	{
		int j=itCount/2;
		int a=j;

		uint64_t *pLcd64=(uint64_t*) (pLcd+k);
		uint64_t color64=(((uint64_t)color)<<32)|((uint64_t)color);

		j--;
		while (j)
			pLcd64[j--]=color64;

		pLcd64[j]=color64;
		k+=a+itCount/2;

		if (itCount%2)
			pLcd[k++]=color;
	}
	else
	{
		for(int i=0;i<itCount;++i)
			pLcd[k++]=color;
	}
}

static void _SetColorToPLCD(uint32_t color){
	pLcd[k++]=color;
}

static void _CopyDrawPos(void){
	kCopy=k;
}
static void _SetCopyDrawPos(void){
	k=kCopy;
}
static void _IncDrawPos(int pos){
	k+=pos;
}

static void _StartDrawLine(uint32_t posBuff,uint32_t BkpSizeX,uint32_t x,uint32_t y){
	k=posBuff+(y*BkpSizeX+x);
}
static uint32_t _GetPosK(uint32_t posBuff,uint32_t BkpSizeX,uint32_t x,uint32_t y){
	return (posBuff+(y*BkpSizeX+x));
}
static uint32_t _GetPosY(uint32_t posBuff,uint32_t BkpSizeX){
	return ((k-posBuff)/BkpSizeX);
}
static uint32_t _GetPosX(uint32_t posBuff,uint32_t BkpSizeX){
	return ((k-posBuff)-_GetPosY(posBuff,BkpSizeX)*BkpSizeX);
}
static structPosition _GetPosXY(uint32_t posBuff, uint32_t BkpSizeX){
	structPosition temp;
	int kOffs = k-posBuff;
	temp.y = (kOffs/BkpSizeX);
	temp.x = (kOffs-temp.y*BkpSizeX);
	return temp;
}
static structPosition _GetPosXY___(uint32_t posBuff, int k, uint32_t BkpSizeX){
	structPosition temp;
	int kOffs = k-posBuff;
	temp.y = (kOffs/BkpSizeX);
	temp.x = (kOffs-temp.y*BkpSizeX);
	return temp;
}
static void _NextDrawLine(uint32_t BkpSizeX,uint32_t width){
	k+=(BkpSizeX-width);
}

static void _DrawRight(int width, uint32_t color)
{
	int j=width;
	uint32_t *p = pLcd+k;
	while(j--)
		*(p++)=color;
	k+=width;
}

static void _DrawLeft(int width, uint32_t color)
{
	int j=width;
	uint32_t *p = pLcd+k;
	while(j--)
		*(p--)=color;
	k-=width;
}

static void _DrawDown(int height, uint32_t color,uint32_t BkpSizeX)
{
	int j=height;
	while(j--)
	{
		pLcd[k]=color;
		k+=BkpSizeX;
	}
}

static void _DrawUp(int height, uint32_t color,uint32_t BkpSizeX)
{
	int j=height;
	while(j--)
	{
		pLcd[k]=color;
		k-=BkpSizeX;
	}
}

static void _DrawRightDown(int width,int height, uint32_t color,uint32_t BkpSizeX)
{
	int j,i,x;

	if(width >= height)
	{
		x=width/height;
		j=height;
		while(j--)
		{
			i=x;
			while(i--)
				pLcd[k++]=color;
			k+=BkpSizeX;
		}
	}
	else
	{
		x=height/width;
		j=width;
		while(j--)
		{
			i=x;
			while(i--)
			{
				pLcd[k]=color;
				k+=BkpSizeX;
			}
			k++;
		}
	}
}

static void _DrawRightDown_AA(int width,int height, uint32_t color,uint32_t BkpSizeX)
{
	int j,i,x;

	if(width >= height)
	{
		x=width/height;
		j=height;
		while(j--)
		{
			i=x;
			while(i--)
				pLcd[k++]=color;

			if(j){
				for(int a=0;a<buff_AA[0];++a)
					pLcd[k+a]=buff_AA[1+a];
			}
			k+=BkpSizeX;

			for(int a=0;a<buff_AA[0];++a)
				pLcd[k-1-a]=buff_AA[1+a];
		}
	}
	else
	{
		x=height/width;
		j=width;
		while(j--)
		{
			i=x;
			while(i--){
				pLcd[k]=color;
				k+=BkpSizeX;
			}

			if(j){
			for(int a=0;a<buff_AA[0];++a)
				pLcd[k+a*BkpSizeX]=buff_AA[1+a];
			}
			k++;

			for(int a=0;a<buff_AA[0];++a)
				pLcd[k-(a+1)*BkpSizeX]=buff_AA[1+a];
		}
	}
}

static void _DrawArrayRightDown(uint32_t color,uint32_t BkpSizeX, int direction, int len, ...)
{
	int j=len,i;
	va_list va;
	va_start(va,0);

	if(0==direction)
	{
		while(j--)
		{
			i=va_arg(va,int);
			while(i--)
				pLcd[k++]=color;
			k+=BkpSizeX;
		}
	}
	else
	{
		while(j--)
		{
			i=va_arg(va,int);
			while(i--){
				pLcd[k]=color;
				k+=BkpSizeX;
			}
			k++;
		}
	}
	va_end(va);
}

static void _DrawArrayRightDown_AA(uint32_t drawColor, uint32_t bkColor, float ratioStart, uint32_t BkpSizeX, int direction, int len, ...)
{
	va_list va;
	va_start(va,0);
	int j=len,start=0, i=va_arg(va,int), i_prev;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--)
				pLcd[k++]=drawColor;

			i=va_arg(va,int);
			if(j){
				Set_AACoeff_Draw(i,drawColor,bkColor,ratioStart);
				for(int a=0;a<i&&ratioStart<1.0;++a)
					pLcd[k+a]=buff_AA[1+a];
			}
			k+=BkpSizeX;
			Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
			for(int a=0;a<i_prev&&ratioStart<1.0;++a)
				pLcd[k-1-a]=buff_AA[1+a];

			if(0==start){  start=1;
				k-=2*BkpSizeX;
				Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
				for(int a=0;a<i_prev&&ratioStart<1.0;++a)
					pLcd[k-i_prev+a]=buff_AA[1+a];
				k+=2*BkpSizeX;
			}
		}
		k-=BkpSizeX;
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){
				pLcd[k]=drawColor;
				k+=BkpSizeX;
			}

			i=va_arg(va,int);
			if(j){
				Set_AACoeff_Draw(i,drawColor,bkColor,ratioStart);
				for(int a=0;a<buff_AA[0]&&ratioStart<1.0;++a)
					pLcd[k+a*BkpSizeX]=buff_AA[1+a];
			}
			k++;

			Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
			for(int a=0;a<i_prev&&ratioStart<1.0;++a)
				pLcd[k-(a+1)*BkpSizeX]=buff_AA[1+a];

			if(0==start){  start=1;
				k-=2;
				Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
				for(int a=0;a<i_prev&&ratioStart<1.0;++a)
					pLcd[k-(i_prev-a)*BkpSizeX]=buff_AA[1+a];
				k+=2;
			}
		}
		k--;
	}
	va_end(va);
}

static void _DrawRightUp(int width,int height, uint32_t color,uint32_t BkpSizeX)
{
	int j,i,x;

	if(width >= height)
	{
		x=width/height;
		j=height;
		while(j--)
		{
			i=x;
			while(i--)
				pLcd[k++]=color;
			k-=BkpSizeX;
		}
	}
	else
	{
		x=height/width;
		j=width;
		while(j--)
		{
			i=x;
			while(i--)
			{
				pLcd[k]=color;
				k-=BkpSizeX;
			}
			k++;
		}
	}
}

static void _DrawRightUp_AA(int width,int height, uint32_t color,uint32_t BkpSizeX)
{
	int j,i,x;

	if(width >= height)
	{
		x=width/height;
		j=height;
		while(j--)
		{
			i=x;
			while(i--)
				pLcd[k++]=color;

			if(j){
				for(int a=0;a<buff_AA[0];++a)
					pLcd[k+a]=buff_AA[1+a];
			}
			k-=BkpSizeX;

			for(int a=0;a<buff_AA[0];++a)
				pLcd[k-1-a]=buff_AA[1+a];
		}
	}
	else
	{
		x=height/width;
		j=width;
		while(j--)
		{
			i=x;
			while(i--){
				pLcd[k]=color;
				k-=BkpSizeX;
			}

			if(j){
				for(int a=0;a<buff_AA[0];++a)
					pLcd[k-a*BkpSizeX]=buff_AA[1+a];
			}
			k++;

			for(int a=0;a<buff_AA[0];++a)
				pLcd[k+(a+1)*BkpSizeX]=buff_AA[1+a];
		}
	}
}

static void _DrawArrayRightUp(uint32_t color,uint32_t BkpSizeX, int direction, int len, ...)
{
	int j=len,i;
	va_list va;
	va_start(va,0);

	if(0==direction)
	{
		while(j--)
		{
			i=va_arg(va,int);
			while(i--)
				pLcd[k++]=color;
			k-=BkpSizeX;
		}
	}
	else
	{
		while(j--)
		{
			i=va_arg(va,int);
			while(i--){
				pLcd[k]=color;
				k-=BkpSizeX;
			}
			k++;
		}
	}
	va_end(va);
}

static void _DrawArrayRightUp_AA(uint32_t drawColor, uint32_t bkColor, float ratioStart, uint32_t BkpSizeX, int direction, int len, ...)
{
	va_list va;
	va_start(va,0);
	int j=len,start=0, i=va_arg(va,int), i_prev;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--)
				pLcd[k++]=drawColor;

			i=va_arg(va,int);
			if(j){
				Set_AACoeff_Draw(i,drawColor,bkColor,ratioStart);
				for(int a=0;a<i&&ratioStart<1.0;++a)
					pLcd[k+a]=buff_AA[1+a];
			}
			k-=BkpSizeX;
			Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
			for(int a=0;a<i_prev&&ratioStart<1.0;++a)
				pLcd[k-1-a]=buff_AA[1+a];

			if(0==start){  start=1;
				k+=2*BkpSizeX;
				Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
				for(int a=0;a<i_prev&&ratioStart<1.0;++a)
					pLcd[k-i_prev+a]=buff_AA[1+a];
				k-=2*BkpSizeX;
			}
		}
		k+=BkpSizeX;
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){
				pLcd[k]=drawColor;
				k-=BkpSizeX;
			}

			i=va_arg(va,int);
			if(j){
				Set_AACoeff_Draw(i,drawColor,bkColor,ratioStart);
				for(int a=0;a<buff_AA[0]&&ratioStart<1.0;++a)
					pLcd[k-a*BkpSizeX]=buff_AA[1+a];
			}
			k++;

			Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
			for(int a=0;a<i_prev&&ratioStart<1.0;++a)
				pLcd[k+(a+1)*BkpSizeX]=buff_AA[1+a];

			if(0==start){  start=1;
				k-=2;
				Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
				for(int a=0;a<i_prev&&ratioStart<1.0;++a)
					pLcd[k+(i_prev-a)*BkpSizeX]=buff_AA[1+a];
				k+=2;
			}
		}
		k--;
	}
	va_end(va);
}

static void _DrawLeftDown(int width,int height, uint32_t color,uint32_t BkpSizeX)
{
	int j,i,x;

	if(width >= height)
	{
		x=width/height;
		j=height;
		while(j--)
		{
			i=x;
			while(i--)
				pLcd[k--]=color;
			k+=BkpSizeX;
		}
	}
	else
	{
		x=height/width;
		j=width;
		while(j--)
		{
			i=x;
			while(i--)
			{
				pLcd[k]=color;
				k+=BkpSizeX;
			}
			k--;
		}
	}
}

static void _DrawLeftDown_AA(int width,int height, uint32_t color,uint32_t BkpSizeX)
{
	int j,i,x;

	if(width >= height)
	{
		x=width/height;
		j=height;
		while(j--)
		{
			i=x;
			while(i--)
				pLcd[k--]=color;

			if(j){
				for(int a=0;a<buff_AA[0];++a)
					pLcd[k-a]=buff_AA[1+a];
			}
			k+=BkpSizeX;

			for(int a=0;a<buff_AA[0];++a)
				pLcd[k+1+a]=buff_AA[1+a];
		}
	}
	else
	{
		x=height/width;
		j=width;
		while(j--)
		{
			i=x;
			while(i--){
				pLcd[k]=color;
				k+=BkpSizeX;
			}

			if(j){
				for(int a=0;a<buff_AA[0];++a)
					pLcd[k+a*BkpSizeX]=buff_AA[1+a];
			}
			k--;

			for(int a=0;a<buff_AA[0];++a)
				pLcd[k-(a+1)*BkpSizeX]=buff_AA[1+a];
		}
	}
}

static void _DrawArrayLeftDown(uint32_t color,uint32_t BkpSizeX, int direction, int len, ...)
{
	int j=len,i;
	va_list va;
	va_start(va,0);

	if(0==direction)
	{
		while(j--)
		{
			i=va_arg(va,int);
			while(i--)
				pLcd[k--]=color;
			k+=BkpSizeX;
		}
	}
	else
	{
		while(j--)
		{
			i=va_arg(va,int);
			while(i--){
				pLcd[k]=color;
				k+=BkpSizeX;
			}
			k--;
		}
	}
	va_end(va);
}

static void _DrawArrayLeftDown_AA(uint32_t drawColor, uint32_t bkColor, float ratioStart, uint32_t BkpSizeX, int direction, int len, ...)
{
	va_list va;
	va_start(va,0);
	int j=len,start=0, i=va_arg(va,int), i_prev;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--)
				pLcd[k--]=drawColor;

			i=va_arg(va,int);
			if(j){
				Set_AACoeff_Draw(i,drawColor,bkColor,ratioStart);
				for(int a=0;a<i&&ratioStart<1.0;++a)
					pLcd[k-a]=buff_AA[1+a];
			}
			k+=BkpSizeX;

			Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
			for(int a=0;a<i_prev&&ratioStart<1.0;++a)
				pLcd[k+1+a]=buff_AA[1+a];

			if(0==start){  start=1;
				k-=2*BkpSizeX;
				Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
				for(int a=0;a<i_prev&&ratioStart<1.0;++a)
					pLcd[k+i_prev-a]=buff_AA[1+a];
				k+=2*BkpSizeX;
			}
		}
		k-=BkpSizeX;
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){
				pLcd[k]=drawColor;
				k+=BkpSizeX;
			}

			i=va_arg(va,int);
			if(j){
				Set_AACoeff_Draw(i,drawColor,bkColor,ratioStart);
				for(int a=0;a<buff_AA[0]&&ratioStart<1.0;++a)
					pLcd[k+a*BkpSizeX]=buff_AA[1+a];
			}
			k--;

			Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
			for(int a=0;a<i_prev&&ratioStart<1.0;++a)
				pLcd[k-(a+1)*BkpSizeX]=buff_AA[1+a];

			if(0==start){  start=1;
				k+=2;
				Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
				for(int a=0;a<i_prev&&ratioStart<1.0;++a)
					pLcd[k-(i_prev-a)*BkpSizeX]=buff_AA[1+a];
				k-=2;
			}
		}
		k++;
	}
	va_end(va);
}

static void _DrawLeftUp(int width,int height, uint32_t color,uint32_t BkpSizeX)
{
	int j,i,x;

	if(width >= height)
	{
		x=width/height;
		j=height;
		while(j--)
		{
			i=x;
			while(i--)
				pLcd[k--]=color;
			k-=BkpSizeX;
		}
	}
	else
	{
		x=height/width;
		j=width;
		while(j--)
		{
			i=x;
			while(i--)
			{
				pLcd[k]=color;
				k-=BkpSizeX;
			}
			k--;
		}
	}
}

static void _DrawLeftUp_AA(int width,int height, uint32_t color,uint32_t BkpSizeX)
{
	int j,i,x;

	if(width >= height)
	{
		x=width/height;
		j=height;
		while(j--)
		{
			i=x;
			while(i--)
				pLcd[k--]=color;

			if(j){
				for(int a=0;a<buff_AA[0];++a)
					pLcd[k-a]=buff_AA[1+a];
			}
			k-=BkpSizeX;

			for(int a=0;a<buff_AA[0];++a)
				pLcd[k+1+a]=buff_AA[1+a];
		}
	}
	else
	{
		x=height/width;
		j=width;
		while(j--)
		{
			i=x;
			while(i--){
				pLcd[k]=color;
				k-=BkpSizeX;
			}

			if(j){
				for(int a=0;a<buff_AA[0];++a)
					pLcd[k-a*BkpSizeX]=buff_AA[1+a];
			}
			k--;

			for(int a=0;a<buff_AA[0];++a)
				pLcd[k+(a+1)*BkpSizeX]=buff_AA[1+a];
		}
	}
}

static void _DrawArrayLeftUp(uint32_t color,uint32_t BkpSizeX, int direction, int len, ...)
{
	int j=len,i;
	va_list va;
	va_start(va,0);

	if(0==direction)
	{
		while(j--)
		{
			i=va_arg(va,int);
			while(i--)
				pLcd[k--]=color;
			k-=BkpSizeX;
		}
	}
	else
	{
		while(j--)
		{
			i=va_arg(va,int);
			while(i--){
				pLcd[k]=color;
				k-=BkpSizeX;
			}
			k--;
		}
	}
	va_end(va);
}

static void _DrawArrayLeftUp_AA(uint32_t drawColor, uint32_t bkColor, float ratioStart, uint32_t BkpSizeX, int direction, int len, ...)
{
	va_list va;
	va_start(va,0);
	int j=len,start=0, i=va_arg(va,int), i_prev;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--)
				pLcd[k--]=drawColor;

			i=va_arg(va,int);
			if(j){
				Set_AACoeff_Draw(i,drawColor,bkColor,ratioStart);
				for(int a=0;a<i&&ratioStart<1.0;++a)
					pLcd[k-a]=buff_AA[1+a];
			}
			k-=BkpSizeX;

			Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
			for(int a=0;a<i_prev&&ratioStart<1.0;++a)
				pLcd[k+1+a]=buff_AA[1+a];

			if(0==start){  start=1;
				k+=2*BkpSizeX;
				Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
				for(int a=0;a<i_prev&&ratioStart<1.0;++a)
					pLcd[k+i_prev-a]=buff_AA[1+a];
				k-=2*BkpSizeX;
			}
		}
		k+=BkpSizeX;
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){
				pLcd[k]=drawColor;
				k-=BkpSizeX;
			}

			i=va_arg(va,int);
			if(j){
				Set_AACoeff_Draw(i,drawColor,bkColor,ratioStart);
				for(int a=0;a<buff_AA[0]&&ratioStart<1.0;++a)
					pLcd[k-a*BkpSizeX]=buff_AA[1+a];
			}
			k--;

			Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
			for(int a=0;a<i_prev&&ratioStart<1.0;++a)
				pLcd[k+(a+1)*BkpSizeX]=buff_AA[1+a];

			if(0==start){  start=1;
				k+=2;
				Set_AACoeff_Draw(i_prev,drawColor,bkColor,ratioStart);
				for(int a=0;a<i_prev&&ratioStart<1.0;++a)
					pLcd[k+(i_prev-a)*BkpSizeX]=buff_AA[1+a];
				k-=2;
			}
		}
		k++;
	}
	va_end(va);
}

static void _Middle_RoundRectangleFrame(int rectangleFrame, int fillHeight, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpSizeX, uint32_t width, uint32_t height){
	int _height = height-fillHeight;
	int _width = width-2;
	if(rectangleFrame)
	{
		for (int j=0; j<_height; j++)
		{
			_FillBuff(1, FrameColor);
			_FillBuff(_width, FillColor);
			_FillBuff(1, FrameColor);
			_NextDrawLine(BkpSizeX,width);
		}
	}
	else
	{
		for (int j=0; j<_height; j++)
		{
			_FillBuff(1, FrameColor);
			k+=_width;
			_FillBuff(1, FrameColor);
			_NextDrawLine(BkpSizeX,width);
		}
	}
}

/* Transparent version of Rectangle-Frame */
static void LCD_DrawRoundRectangleFrameTransp(int rectangleFrame, uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor_, uint32_t FillColor_, uint32_t BkpColor_, float transpCoeff)
{
	#define BkColor    	CONDITION( (0xFFFFFF&BkpColor_)==0, pLcd[k], BkpColor_ )						/* if(BkpColor_!=0) we don`t mixer colors (BkpColor_ with pLcd[k]) via transparent coefficient */
	#define FrameColor 	GetTransitionColor( FrameColor_,pLcd[k],  transpCoeff )
	#define FillColor 	CONDITION( rectangleFrame, GetTransitionColor( FillColor_,pLcd[k],transpCoeff ), pLcd[k] )
	#define i1 				CONDITION( BkpColor_&AA_IN_OFF, FrameColor, GetTransitionColor( FrameColor, FillColor, AA.c1 ) )
	#define i2 				CONDITION( BkpColor_&AA_IN_OFF, pLcd[k], GetTransitionColor( FrameColor, FillColor, AA.c2 ) )
	#define o1 				GetTransitionColor( FrameColor, BkColor,  AA.c1 )
	#define o2 				GetTransitionColor( FrameColor, BkColor,  AA.c2 )
	typedef enum{ frC,i1C,i2C,o1C,o2C,bkC }TypeOfColor;

	u32 outAAoff = BkpColor_ & AA_OUT_OFF;
	u32 noFrame  = BkpColor_ & AA_NO_FRAME;

	void _Fill(int x){
		if(rectangleFrame){
			for(int i=0;i<x;++i)
				_SetColorToPLCD(FillColor);
		}
		else k+=x;
	}

	void A(int itCount, TypeOfColor type){
		switch((int)type){
			case frC:
				for(int i=0;i<itCount;++i){ if(0==noFrame) _SetColorToPLCD(FrameColor); else k++; }
				break;
			case i1C:
				for(int i=0;i<itCount;++i) _SetColorToPLCD(i1);
				break;
			case i2C:
				for(int i=0;i<itCount;++i) _SetColorToPLCD(i2);
				break;
			case o1C:
				for(int i=0;i<itCount;++i) _SetColorToPLCD(o1);
				break;
			case o2C:
				for(int i=0;i<itCount;++i) _SetColorToPLCD(o2);
				break;
			case bkC:
				for(int i=0;i<itCount;++i) _SetColorToPLCD(BkColor);
				break;
		}
	}

	void _Out_AA_left(int stage)
	{
		if(0==outAAoff)
		{	switch(stage)
			{
			case 0:	A(3,bkC); A(1,o2C); A(1,o1C);  break;
			case 1:	A(2,bkC); A(1,o1C);  break;
			case 2:	A(1,bkC); A(1,o1C);  break;
			case 3:	A(1,o2C); break;
			case 4:	A(1,o1C); break;
			}
		}
		else
		{  switch(stage)
			{
			case 0:	k+=5; break;
			case 1:	k+=3; break;
			case 2:	k+=2; break;
			case 3:	k+=1; break;
			case 4:	k+=1; break;
			}
		}
	}

	void _Out_AA_right(int stage)
	{
		if(0==outAAoff)
		{	switch(stage)
			{
			case 0:	A(1,o1C); A(1,o2C); A(3,bkC);  break;
			case 1:	A(1,o1C); A(2,bkC);  break;
			case 2:	A(1,o1C); A(1,bkC);  break;
			case 3:	A(1,o2C); break;
			case 4:	A(1,o1C); break;
			}
		}
		else
		{	switch(stage)
			{
			case 0:	k+=5; break;
			case 1:	k+=3; break;
			case 2:	k+=2; break;
			case 3:	k+=1; break;
			case 4:	k+=1; break;
			}
		}
	}

	_StartDrawLine(posBuff,BkpSizeX,x,y);
	_Out_AA_left(0); A(width-10,frC); _Out_AA_right(0);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(1); A(2,frC); A(1,i1C);A(1,i2C); _Fill(width-14); A(1,i2C);A(1,i1C);A(2,frC); _Out_AA_right(1);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(2); A(1,frC); A(1,i1C); _Fill(width-8); A(1,i1C); A(1,frC); _Out_AA_right(2);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(3); A(1,frC); A(1,i1C); _Fill(width-6); A(1,i1C); A(1,frC); _Out_AA_right(3);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(4); A(1,frC); _Fill(width-4); A(1,frC); _Out_AA_right(4);
	_NextDrawLine(BkpSizeX,width);

	A(1,frC);  A(1,i1C); _Fill(width-4); A(1,i1C); A(1,frC);
	_NextDrawLine(BkpSizeX,width);
	A(1,frC);  A(1,i2C); _Fill(width-4); A(1,i2C); A(1,frC);
	_NextDrawLine(BkpSizeX,width);

	int _height = height-14;
	int _width = width-2;
	if(rectangleFrame){
		for (int j=0; j<_height; j++){	A(1,frC);  for(int i=0;i<_width;++i) _SetColorToPLCD(FillColor);	 A(1,frC); _NextDrawLine(BkpSizeX,width); }
	}
	else{
		for (int j=0; j<_height; j++){	A(1,frC);  k+=_width; 															 A(1,frC); _NextDrawLine(BkpSizeX,width); }
	}

	A(1,frC);  A(1,i2C); _Fill(width-4); A(1,i2C); A(1,frC);
	_NextDrawLine(BkpSizeX,width);
	A(1,frC);  A(1,i1C); _Fill(width-4); A(1,i1C); A(1,frC);
	_NextDrawLine(BkpSizeX,width);

	_Out_AA_left(4); A(1,frC); _Fill(width-4); A(1,frC); _Out_AA_right(4);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(3); A(1,frC); A(1,i1C); _Fill(width-6); A(1,i1C); A(1,frC); _Out_AA_right(3);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(2); A(1,frC); A(1,i1C); _Fill(width-8); A(1,i1C); A(1,frC); _Out_AA_right(2);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(1); A(2,frC); A(1,i1C);A(1,i2C); _Fill(width-14); A(1,i2C);A(1,i1C);A(2,frC); _Out_AA_right(1);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(0); A(width-10,frC); _Out_AA_right(0);

	#undef BkColor
	#undef FrameColor
	#undef FillColor
	#undef  i1
	#undef  i2
	#undef  o1
	#undef  o2
}

static void LCD_DrawRoundRectangleFrame(int rectangleFrame, uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor)
{
	#define A(a,b) 	_FillBuff(a,b)

	uint8_t thickness = BkpColor>>24;
	uint32_t o1,o2;
	uint32_t i1 = GetTransitionColor(FrameColor,FillColor,AA.c1);
	uint32_t i2 = GetTransitionColor(FrameColor,FillColor,AA.c2);

	if((thickness==0)||(thickness==255)){
		o1 = GetTransitionColor(FrameColor,BkpColor,AA.c1);
		o2 = GetTransitionColor(FrameColor,BkpColor,AA.c2);
	}

	void _Fill(int x)
	{
		if(rectangleFrame)
			A(x,FillColor);
		else
			k+=x;
	}

	void _Out_AA_left(int stage)
	{
		if((thickness==0)||(thickness==255))
		{	switch(stage)
			{
			case 0:	A(3,BkpColor); A(1,o2); A(1,o1);  break;
			case 1:	A(2,BkpColor); A(1,o1);  break;
			case 2:	A(1,BkpColor); A(1,o1);  break;
			case 3:	A(1,o2); break;
			case 4:	A(1,o1); break;
			}
		}
		else
		{  switch(stage)
			{
			case 0:	k+=5; break;
			case 1:	k+=3; break;
			case 2:	k+=2; break;
			case 3:	k+=1; break;
			case 4:	k+=1; break;
			}
		}
	}

	void _Out_AA_right(int stage)
	{
		if((thickness==0)||(thickness==255))
		{	switch(stage)
			{
			case 0:	A(1,o1); A(1,o2); A(3,BkpColor);  break;
			case 1:	A(1,o1); A(2,BkpColor);  break;
			case 2:	A(1,o1); A(1,BkpColor);  break;
			case 3:	A(1,o2); break;
			case 4:	A(1,o1); break;
			}
		}
		else
		{	switch(stage)
			{
			case 0:	k+=5; break;
			case 1:	k+=3; break;
			case 2:	k+=2; break;
			case 3:	k+=1; break;
			case 4:	k+=1; break;
			}
		}
	}

	_StartDrawLine(posBuff,BkpSizeX,x,y);
	_Out_AA_left(0); A(width-10,FrameColor); _Out_AA_right(0);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(1); A(2,FrameColor); A(1,i1);A(1,i2); _Fill(width-14); A(1,i2);A(1,i1);A(2,FrameColor); _Out_AA_right(1);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(2); A(1,FrameColor); A(1,i1); _Fill(width-8); A(1,i1); A(1,FrameColor); _Out_AA_right(2);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(3); A(1,FrameColor); A(1,i1); _Fill(width-6); A(1,i1); A(1,FrameColor); _Out_AA_right(3);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(4); A(1,FrameColor); _Fill(width-4); A(1,FrameColor); _Out_AA_right(4);
	_NextDrawLine(BkpSizeX,width);

	A(1,FrameColor);  A(1,i1); _Fill(width-4); A(1,i1); A(1,FrameColor);
	_NextDrawLine(BkpSizeX,width);
	A(1,FrameColor);  A(1,i2); _Fill(width-4); A(1,i2); A(1,FrameColor);
	_NextDrawLine(BkpSizeX,width);

	_Middle_RoundRectangleFrame(rectangleFrame,14,FrameColor,FillColor,BkpSizeX,width,height);

	A(1,FrameColor);  A(1,i2); _Fill(width-4); A(1,i2); A(1,FrameColor);
	_NextDrawLine(BkpSizeX,width);
	A(1,FrameColor);  A(1,i1); _Fill(width-4); A(1,i1); A(1,FrameColor);
	_NextDrawLine(BkpSizeX,width);

	_Out_AA_left(4); A(1,FrameColor); _Fill(width-4); A(1,FrameColor); _Out_AA_right(4);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(3); A(1,FrameColor); A(1,i1); _Fill(width-6); A(1,i1); A(1,FrameColor); _Out_AA_right(3);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(2); A(1,FrameColor); A(1,i1); _Fill(width-8); A(1,i1); A(1,FrameColor); _Out_AA_right(2);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(1); A(2,FrameColor); A(1,i1);A(1,i2); _Fill(width-14); A(1,i2);A(1,i1);A(2,FrameColor); _Out_AA_right(1);
	_NextDrawLine(BkpSizeX,width);
	_Out_AA_left(0); A(width-10,FrameColor); _Out_AA_right(0);

	#undef  A
}

static void PixelCorrect(uint32_t drawColor, uint32_t bkColor, float ratioStart, uint16_t pxlLen, int multiplier){
	Set_AACoeff_Draw(pxlLen,drawColor,bkColor,ratioStart);
	for(int a=0;a<pxlLen;++a)
		pLcd[k+multiplier*a]=buff_AA[1+a];
}

static void InverseAndCopyBuff(uint8_t *buf_inv, uint8_t *buf){
	buf_inv[0]=buf[0];
	for(int i=0;i<buf[0];i++)
		buf_inv[1+i]=buf[buf[0]-i];
}

static void _OffsetRightDown(uint32_t BkpSizeX, int direction, uint8_t *buf){
	int j=buf[0], i=buf[1], p=2;
	if(0==direction)
	{
		while(j--){
			k+=i;
			if(j) i=buf[p++];
			k+=BkpSizeX;
		}
	}
	else
	{
		while(j--){
			k+=i*BkpSizeX;
			if(j) i=buf[p++];
			k++;
		}
	}
}

static void _OffsetLeftDown(uint32_t BkpSizeX, int direction, uint8_t *buf){
	int j=buf[0], i=buf[1], p=2;
	if(0==direction)
	{
		while(j--){
			k-=i;
			if(j) i=buf[p++];
			k+=BkpSizeX;
		}
	}
	else
	{
		while(j--){
			k+=i*BkpSizeX;
			if(j) i=buf[p++];
			k--;
		}
	}
}

static void _OffsetLeftUp(uint32_t BkpSizeX, int direction, uint8_t *buf){
	int j=buf[0], i=buf[1], p=2;
	if(0==direction)
	{
		while(j--){
			k-=i;
			if(j) i=buf[p++];
			k-=BkpSizeX;
		}
	}
	else
	{
		while(j--){
			k-=i*BkpSizeX;
			if(j) i=buf[p++];
			k--;
		}
	}
}

static void LCD_CircleCorrect(void)
{
/*	LCD_OffsCircleLine(1,1);  LCD_OffsCircleLine(2,-1);
	LCD_OffsCircleLine(1,2);  LCD_OffsCircleLine(2,-1); LCD_OffsCircleLine(3,-1);
	LCD_OffsCircleLine(1,4);  LCD_OffsCircleLine(2,3); LCD_OffsCircleLine(3,2);  LCD_OffsCircleLine(3,1); */
}

static int16_t GetDegFromPosK( uint32_t posBuff, uint16_t x0, uint16_t y0, uint32_t BkpSizeX)
{
	pos = _GetPosXY(posBuff,BkpSizeX);
	float deg = DEG(atan2(pos.y-y0, pos.x-x0));
	return (int16_t)deg + 180;	 /* <0 ? 180.0+deg : deg; */
}

static void OffsetKfromLineBuff(int itBuff, uint32_t BkpSizeX){
	uint32_t offs= (itBuff-1)*BkpSizeX;
	for(int i=1;i<itBuff;++i){
		if(i>=MAX_LINE_BUFF_CIRCLE_SIZE) break;
		offs += Circle.lineBuff[i];
	}
	k+=offs;
}

static uint8_t LCD_SearchLinePoints(int startMeasure, int posBuff, int x0,int y0, float deg, uint32_t BkpSizeX)
{
	static float tang, coeff;

	if(startMeasure)
	{
		tang= tan(TANG_ARG(deg));
		coeff = 1/ABS(tang);

		if(deg>0 && deg<=45)
			return 1;
		else if(deg>45 && deg<=90)
			return 2;
		else if(deg>90 && deg<=135)
			return 3;
		else if(deg>135 && deg<=180)
			return 4;
		else if(deg>180 && deg<=225)
			return 5;
		else if(deg>225 && deg<=270)
			return 6;
		else if(deg>270 && deg<=315)
			return 7;
		else if(deg>315 && deg<=360)
			return 8;
	}
	else
	{
   	pos = _GetPosXY(posBuff,BkpSizeX);
   	float xxx = pos.x - x0;
		float yyy = pos.y - y0;
		float _yyy = tang*xxx;

		if(deg>0 && deg<=45)
		{
				  if(yyy<_yyy)	                                return 0;
			else if( (yyy>=_yyy) && (yyy<tang*(xxx+coeff)) )  return 1;
			else															  return 2;
		}
		else if(deg>45 && deg<=90)
		{
			  if(yyy<_yyy)	                                return 0;
		else if( (yyy>=_yyy) && (yyy<tang*(xxx+1)) )      return 1;
		else															  return 2;
		}

		else if(deg>90 && deg<=135)
		{
			  if(yyy>_yyy)	                                return 0;
		else if( (yyy<=_yyy) && (yyy>tang*(xxx+1)) )      return 1;
		else															  return 2;
		}
		else if(deg>135 && deg<=180)
		{
			  if(yyy>_yyy)	                                return 0;
		else if( (yyy<=_yyy) && (yyy>tang*(xxx+coeff)) )  return 1;
		else															  return 2;
		}

		else if(deg>180 && deg<=225)
		{
				  if(yyy<_yyy)	                                return 2;
			else if( (yyy>=_yyy) && (yyy<tang*(xxx+coeff)) )  return 1;
			else															  return 0;
		}
		else if(deg>225 && deg<=270)
		{
			  if(yyy<_yyy)	                                return 2;
		else if( (yyy>=_yyy) && (yyy<tang*(xxx+1)) )      return 1;
		else															  return 0;
		}

		else if(deg>270 && deg<=315)
		{
			  if(yyy>_yyy)	                                return 2;
		else if( (yyy<=_yyy) && (yyy>tang*(xxx+1)) )      return 1;
		else															  return 0;
		}
		else if(deg>315 && deg<=360)
		{
			  if(yyy>_yyy)	                                return 2;
		else if( (yyy<=_yyy) && (yyy>tang*(xxx+coeff)) )  return 1;
		else															  return 0;
		}

		else
			return 3;
	}
	return 0;
}

static uint8_t LCD_SearchRadiusPoints(int posBuff, int nrDeg, uint32_t BkpSizeX)
{
	float tang= Circle.tang[nrDeg];
	float coeff=Circle.coeff[nrDeg];
	uint16_t deg = Circle.degree[1+nrDeg];

   	pos = _GetPosXY(posBuff,BkpSizeX);
   	float xxx = pos.x - Circle.x0;
		float yyy = pos.y - Circle.y0;
		float _yyy = tang*xxx;

		if(deg>0 && deg<=45)
		{
				  if(yyy<_yyy)	                                return 0;
			else if( (yyy>=_yyy) && (yyy<tang*(xxx+coeff)) )  return 1;
			else															  return 2;
		}
		else if(deg>45 && deg<=90)
		{
			  if(yyy<_yyy)	                                return 0;
		else if( (yyy>=_yyy) && (yyy<tang*(xxx+1)) )      return 1;
		else															  return 2;
		}

		else if(deg>90 && deg<=135)
		{
			  if(yyy>_yyy)	                                return 0;
		else if( (yyy<=_yyy) && (yyy>tang*(xxx+1)) )      return 1;
		else															  return 2;
		}
		else if(deg>135 && deg<=180)
		{
			  if(yyy>_yyy)	                                return 0;
		else if( (yyy<=_yyy) && (yyy>tang*(xxx+coeff)) )  return 1;
		else															  return 2;
		}

		else if(deg>180 && deg<=225)
		{
				  if(yyy<_yyy)	                                return 2;
			else if( (yyy>=_yyy) && (yyy<tang*(xxx+coeff)) )  return 1;
			else															  return 0;
		}
		else if(deg>225 && deg<=270)
		{
			  if(yyy<_yyy)	                                return 2;
		else if( (yyy>=_yyy) && (yyy<tang*(xxx+1)) )      return 1;
		else															  return 0;
		}

		else if(deg>270 && deg<=315)
		{
			  if(yyy>_yyy)	                                return 2;
		else if( (yyy<=_yyy) && (yyy>tang*(xxx+1)) )      return 1;
		else															  return 0;
		}
		else if(deg>315 && deg<=360)
		{
			  if(yyy>_yyy)	                                return 2;
		else if( (yyy<=_yyy) && (yyy>tang*(xxx+coeff)) )  return 1;
		else															  return 0;
		}

		else
			return 3;
}

static void _DrawArrayBuffRightDownUp2_AA(DIRECTIONS upDwn, uint32_t drawColor, uint32_t outColor, uint32_t inColor, float outRatioStart, float inRatioStart, uint32_t BkpSizeX, int direction, uint16_t *buf, int corr45degAA)
{
	int j=buf[0], i=buf[1], p=2, i_prev, start=0;   int flagss=0;
	uint32_t _outColor=outColor;
	uint32_t _inColor=inColor;
	u32 k_temp;

	void _ReadBK(int lenAAbuff, int offsKin,int offsKout){
		if(0==inColor) {	_inColor =pLcd[k+offsKin];  Set_AACoeff(lenAAbuff,drawColor,_inColor,inRatioStart);	   }
		if(0==outColor){	_outColor=pLcd[k+offsKout]; Set_AACoeff2(lenAAbuff,drawColor,_outColor,outRatioStart); }
	}

	void _DrawAAfor2pxl(int sign){
		if(0==direction){
			if(p-2>0){
				_ReadBK(buf[p-2], -i_prev-1-buf[p-2],0);
				for(int a=0;a<buff_AA[0];++a){
					pLcd[k-i_prev-1-a]=buff_AA[1+a];
		}}}
		else{
			k-=2*BkpSizeX*sign;
			_ReadBK(2, -2,3);
			pLcd[k-1]=buff_AA[1];   pLcd[k+1]=buff2_AA[1];
			k+=BkpSizeX*sign;
			pLcd[k+1]=buff2_AA[1];	pLcd[k+2]=buff2_AA[1];
			k+=BkpSizeX*sign;
	}}

	void _StartPxlsCorrect(int sign){
		if(0==direction){
			if(0==flagss){
				k-=(i_prev-1); k-=2;
				_ReadBK(3, -1,0);
				pLcd[k]=buff_AA[1+0];
				k+=2;  k+=(i_prev-1);	k-=BkpSizeX*(-1*sign);	k-=(i_prev-1);
				pLcd[k]=_inColor;
				k+=(i_prev-1);  k+=BkpSizeX*(-1*sign);
			}
		}
		else{
			if(0==flagss){
				k-=3*BkpSizeX*sign;	k-=(i_prev-1);  k+=1;
				_ReadBK(3, -1,0);
				pLcd[k]=buff_AA[1];
				k-=1; k+=(i_prev-1);  k+=3*BkpSizeX*sign;
	}}}

	/*	Correct AA for 45deg H pixels */							/*	Correct AA for 45deg V pixels */
	/*	------.															|
				 .															|
			 	  .__  staPxl`s										|
				 	  .	 	 	 	 									 .
				  	   .	 	 	 	 									  .
				   	 .__  stoPxl`s	 	 	 	 						|  staPxl`s
				 	 	 	  .												|
				  	  	  	   .												 .
				   			 ._______									  .
				   			 	 	 	 	 	 	 	 	 	 	 	 	 	.
		 	 	 	      														 |  stoPxl`s
	 	 	 	      															 |
	 	 	 	       	 	 	 	 	 	 	 	 	 	 	 	 	 	 	  .
	 	 	 	        	  	  	  	  	  	  	  	  	  	  	  	  	  	  	   .
	 	 	 	         															 .
	 	 	 	         															  |
	 	 	 	         															  |
	 	 	 	         															  |
	 	 	 	         															  |
	*/

	int __SearchHVpxlsInLine45deg(int staPxl,int stoPxl, int offs){
		if(buf[(p+offs)-1]/*i_prev*/==staPxl){
			for(int a=0;a<j;++a){		/* how many left to iteration indicates 'j' */
				if(buf[(p+offs)+a]!=1){
					if(buf[(p+offs)+a]==stoPxl) return a;
					else return 0;
				}
			}
			return 0;
		}
		return 0;
	}
	int __ArePoints45degInRange(int minNmbr,int maxNmbr, int staPxl,int stoPxl, int offs, int* nmbr45degPoints){
		int temp = __SearchHVpxlsInLine45deg(staPxl,stoPxl,offs);
		if(nmbr45degPoints!=NULL) *nmbr45degPoints=temp;
		if(IS_RANGE(temp,minNmbr,maxNmbr)) return 1;
		else										  return 0;
	}
	int __ArePoints45degInScheme(int staPxl,		int minNr1,int maxNr1, int midPxl, int minNr2,int maxNr2,	  int stoPxl,     int offs, int* nmbr45degPoints1, int* nmbr45degPoints2){
		int scheme1 = __ArePoints45degInRange(minNr1,maxNr1, staPxl,midPxl, offs, 			  nmbr45degPoints1==NULL?NULL:nmbr45degPoints1);
		int scheme2 = __ArePoints45degInRange(minNr2,maxNr2, midPxl,stoPxl, offs+minNr1+1, nmbr45degPoints2==NULL?NULL:nmbr45degPoints2);
		if		 (scheme1==0) 					 return 0;
		else if(scheme1==1 && scheme2==0) return 1;
		else if(scheme1==1 && scheme2==1) return 2;
		return -1;
	}

	int _AAcorrectFor45degH(DIRECTIONS upDwn)
	{
		int len45degLine, sign=CONDITION(upDwn==Down,1,-1);
      int scheme = __ArePoints45degInRange(3,150, 2,2, 0,&len45degLine);
		if(scheme)
		{
			_StartPxlsCorrect(sign);
			_DrawAAfor2pxl(sign);
			for(int a=0;a<len45degLine;++a){  k+=BkpSizeX*sign; _ReadBK(len45degLine,-3,3);  pLcd[k-2]=buff_AA[1+a]; pLcd[k-1]=drawColor;   pLcd[k+1]=buff2_AA[1+(len45degLine-1)-a]; pLcd[k+2]=_outColor;    pLcd[k++]=drawColor;  }
			k+=BkpSizeX*sign;
			p+=len45degLine;  j-=(len45degLine+1);
			i=buf[p++];   flagss=1;
			return 1;
		}
		else
		{
			int len45degLine;
			if(__ArePoints45degInRange(3,150, 2,3, 0,&len45degLine))
			{
				_StartPxlsCorrect(sign);
				_DrawAAfor2pxl(sign);
				for(int a=0;a<len45degLine;++a){  k+=BkpSizeX*sign; _ReadBK(len45degLine,-4,3);   pLcd[k-3]=_inColor; pLcd[k-2]=buff_AA[1+a]; 		pLcd[k-1]=drawColor;   pLcd[k+1]=buff2_AA[1+(len45degLine-1)-a];  pLcd[k+2]=(a==(len45degLine-1)?buff2_AA[1+len45degLine/2]:_outColor);     pLcd[k++]=drawColor;  }
				k+=BkpSizeX*sign;
				p+=len45degLine;  j-=(len45degLine+1);
				i=buf[p++];   flagss=1;
				return 1;
			}
			else
			{
				if(__ArePoints45degInRange(3,150, 3,2, 0,&len45degLine))
				{
					_StartPxlsCorrect(sign);
					_DrawAAfor2pxl(sign);
					for(int a=0;a<len45degLine;++a){  k+=BkpSizeX*sign; _ReadBK(len45degLine,-4,3);   pLcd[k-3]=(a==0?buff_AA[1+len45degLine/2]:_inColor); pLcd[k-2]=buff_AA[1+a]; pLcd[k-1]=drawColor;   pLcd[k+1]=buff2_AA[1+(len45degLine-1)-a]; pLcd[k+2]=_outColor;    pLcd[k++]=drawColor; }
					k+=BkpSizeX*sign;
					p+=len45degLine;  j-=(len45degLine+1);
					i=buf[p++];		flagss=1;
					return 1;
				}
				else
				{
					if(flagss){
						flagss=0;
						_ReadBK(3, -i_prev-2,0);
						pLcd[k-i_prev-1]=buff_AA[1+0];
						k-=BkpSizeX*(-1*sign);	k-=(i_prev-0);
						pLcd[k+0]=buff_AA[1+0];
						pLcd[k+1]=buff_AA[1+1];
						k+=(i_prev-0); k+=BkpSizeX*(-1*sign);
					}
				}
			}
		}
		return 0;
	}

	int _AAcorrectFor45degV(DIRECTIONS upDwn)
	{
		int len45degLine, sign=CONDITION(upDwn==Down,1,-1);
		int scheme = __ArePoints45degInScheme(2,  3,150, 2, 1,2,   2,  0,&len45degLine,NULL);
		if(scheme)
		{
			if(scheme==2) goto GOTO_ToEndAAcorrect45degV;
			_StartPxlsCorrect(sign);
			_DrawAAfor2pxl(sign);
			k++;
			for(int a=0;a<len45degLine;++a){  _ReadBK(len45degLine,-3,3);   pLcd[k+2]=buff2_AA[1+a]; pLcd[k+1]=drawColor;   pLcd[k-1]=buff_AA[1+(len45degLine-1)-a]; pLcd[k-2]=_inColor;    pLcd[k++]=drawColor;  k+=BkpSizeX*sign; }
			p+=len45degLine;  j-=(len45degLine+1);
			i=buf[p++];   flagss=1;
			return 1;
		}
		else
		{
			int len45degLine;
			if(__ArePoints45degInRange(3,150, 2,3, 0,&len45degLine))
			{
				_StartPxlsCorrect(sign);
				_DrawAAfor2pxl(sign);
				k++;
				for(int a=0;a<len45degLine;++a){  _ReadBK(len45degLine,-3,4);    pLcd[k+3]=_outColor; pLcd[k+2]=buff2_AA[1+a]; 		pLcd[k+1]=drawColor;   pLcd[k-1]=buff_AA[1+(len45degLine-1)-a];  pLcd[k-2]=(a==(len45degLine-1)?buff_AA[1+len45degLine/2]:_inColor);     pLcd[k++]=drawColor;   k+=BkpSizeX*sign; }
				p+=len45degLine;  j-=(len45degLine+1);
				i=buf[p++];   flagss=1;
				return 1;
			}
			else
			{
				if(__ArePoints45degInRange(3,150, 3,2, 0,&len45degLine))
				{
					_StartPxlsCorrect(sign);
					_DrawAAfor2pxl(sign);
					k++;
					for(int a=0;a<len45degLine;++a){   _ReadBK(len45degLine,-3,4);    pLcd[k+3]=(a==0?buff2_AA[1+len45degLine/2]:_outColor); pLcd[k+2]=buff2_AA[1+a]; pLcd[k+1]=drawColor;   pLcd[k-1]=buff_AA[1+(len45degLine-1)-a]; pLcd[k-2]=_inColor;     pLcd[k++]=drawColor;   k+=BkpSizeX*sign; }
					p+=len45degLine;  j-=(len45degLine+1);
					i=buf[p++];   flagss=1;
					return 1;
				}
				else
				{
					if(flagss){
						flagss=0;
						k-=2*BkpSizeX*sign;	k-=(i_prev-0);
						_ReadBK(3, 0,0);
						pLcd[k+1]=buff_AA[1+0];
						pLcd[k+1+BkpSizeX*sign]=buff_AA[1+0];
						k+=(i_prev-0); k+=2*BkpSizeX*sign;
					}
				}
			}
		}
		GOTO_ToEndAAcorrect45degV:
		return 0;
	}

	switch((int)upDwn)
	{
	case Down:

		if(0==direction)
		{
			while(j--)
			{
				GOTO_ToDrawAAforH_Down:
				i_prev=i;
				while(i--) pLcd[k++]=drawColor;

				if(corr45degAA && outRatioStart < 1.0 && inRatioStart < 1.0){
					if(_AAcorrectFor45degH(upDwn))
						goto GOTO_ToDrawAAforH_Down;
				}
				i=buf[p++];

				if(outRatioStart<1.0){
					if(0==start){
						k-=BkpSizeX;
						k_temp=k-i_prev;
						if(0==outColor) _outColor=pLcd[k_temp];
						Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
						for(int a=0;a<i_prev;++a){	 	if(0==outColor){ if(pLcd[k_temp+a]!=_outColor){ _outColor=pLcd[k_temp+a]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} }
																if(0==start){start=1; pLcd[k_temp+a-1+BkpSizeX]=drawColor; }	/* add one pixel to full the hole at the beginning of drawing */
																pLcd[k_temp+a]=buff_AA[1+a]; 	  }
						k+=BkpSizeX;
						start=1;
					}

					if(j){
						if(0==outColor) _outColor=pLcd[k];
						Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);
						for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k+a]!=_outColor){ _outColor=pLcd[k+a]; Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);} }
							pLcd[k+a]=buff_AA[1+a];
					}}
				}
				k+=BkpSizeX;

				if(inRatioStart<1.0){
					if(0==inColor) _inColor=pLcd[k-1];
					Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);
					for(int a=0;a<buff_AA[0];++a){	 if(0==inColor){ if(pLcd[k-1-a]!=_inColor){ _inColor=pLcd[k-1-a]; Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);} }
						pLcd[k-1-a]=buff_AA[1+a];  }
				}
			}
			k-=BkpSizeX;
		}
		else
		{
			while(j--)
			{
				GOTO_ToDrawAAforV_Down:
				i_prev=i;
				while(i--){ pLcd[k]=drawColor; k+=BkpSizeX; }

				if(corr45degAA && outRatioStart < 1.0 && inRatioStart < 1.0){
					if(_AAcorrectFor45degV(upDwn))
						goto GOTO_ToDrawAAforV_Down;
				}
				i=buf[p++];

				if(inRatioStart<1.0){
					if(0==start){
						k--;
						if(0==inColor) _inColor=pLcd[k-i_prev*BkpSizeX];
						Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);
						for(int a=0;a<i_prev;++a){	  if(0==inColor){ if(pLcd[k-(i_prev-a)*BkpSizeX]!=_inColor){ _inColor=pLcd[k-(i_prev-a)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);} }
															  if(0==start){start=1; pLcd[k-(i_prev-a)*BkpSizeX+1-BkpSizeX]=drawColor; }	/* add one pixel to full the hole at the beginning of drawing */
															  pLcd[k-(i_prev-a)*BkpSizeX]=buff_AA[1+a]; 	 }
						k++;
						start=1;
					}

					if(j){
						if(0==inColor) _inColor=pLcd[k];
						Set_AACoeff_Draw(i,drawColor,_inColor,inRatioStart);
						for(int a=0;a<buff_AA[0];++a){ 	if(0==inColor){ if(pLcd[k+a*BkpSizeX]!=_inColor){ _inColor=pLcd[k+a*BkpSizeX]; Set_AACoeff_Draw(i,drawColor,_inColor,inRatioStart);} }
							pLcd[k+a*BkpSizeX]=buff_AA[1+a]; }
					}
				}
				k++;

				if(outRatioStart<1.0){
					if(0==outColor) _outColor=pLcd[k-BkpSizeX];
					Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
					for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k-(a+1)*BkpSizeX]!=_outColor){ _outColor=pLcd[k-(a+1)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} }
						pLcd[k-(a+1)*BkpSizeX]=buff_AA[1+a];
					}
				}
			}
			k--;
		}
		break;

	case Up:

		if(0==direction)
		{
			while(j--)
			{
				GOTO_ToDrawAAforH_Up:
				i_prev=i;
				while(i--) pLcd[k++]=drawColor;

				if(corr45degAA && outRatioStart < 1.0 && inRatioStart < 1.0){
					if(_AAcorrectFor45degH(upDwn))
						goto GOTO_ToDrawAAforH_Up;
				}
				i=buf[p++];

				if(inRatioStart<1.0){
					if(0==start){
						k+=BkpSizeX;
						k_temp=k-i_prev;
						if(0==inColor) _inColor=pLcd[k_temp];
						Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);
						for(int a=0;a<i_prev;++a){	 	if(0==inColor){ if(pLcd[k_temp+a]!=_inColor){ _inColor=pLcd[k_temp+a]; Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);} }
																if(0==start){start=1; pLcd[k_temp+a-1-BkpSizeX]=drawColor; }	/* add one pixel to full the hole at the beginning of drawing */
																pLcd[k_temp+a]=buff_AA[1+a]; 	 }
						k-=BkpSizeX;
						start=1;
					}

					if(j){
						if(0==inColor) _inColor=pLcd[k];
						Set_AACoeff_Draw(i,drawColor,_inColor,inRatioStart);
						for(int a=0;a<buff_AA[0];++a){ 	if(0==inColor){ if(pLcd[k+a]!=_inColor){ _inColor=pLcd[k+a]; Set_AACoeff_Draw(i,drawColor,_inColor,inRatioStart);} }
							pLcd[k+a]=buff_AA[1+a];  }
					}
				}

				if(k > BkpSizeX) k -= BkpSizeX;
				{	if(outRatioStart<1.0){
						if(0==outColor) _outColor=pLcd[k-1];
						Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
						for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k-1-a]!=_outColor){ _outColor=pLcd[k-1-a]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} }
							pLcd[k-1-a]=buff_AA[1+a];
						}
				}}
			}
			k+=BkpSizeX;
		}
		else
		{
			while(j--)
			{
				GOTO_ToDrawAAforV_Up:
				i_prev=i;
				while(i--){ pLcd[k]=drawColor; k-=BkpSizeX; }

				if(corr45degAA && outRatioStart < 1.0 && inRatioStart < 1.0){
					if(_AAcorrectFor45degV(upDwn))
						goto GOTO_ToDrawAAforV_Up;
				}

				i=buf[p++];

				if(inRatioStart<1.0){
					if(0==start){
						k--;
						if(0==inColor) _inColor=pLcd[k+i_prev*BkpSizeX];
						Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);
						for(int a=0;a<i_prev;++a){	   if(0==inColor){ if(pLcd[k+(i_prev-a)*BkpSizeX]!=_inColor){ _inColor=pLcd[k+(i_prev-a)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);} }
																if(0==start){start=1; pLcd[k+(i_prev-a)*BkpSizeX+1+BkpSizeX]=drawColor; }	/* add one pixel to full the hole at the beginning of drawing */
																pLcd[k+(i_prev-a)*BkpSizeX]=buff_AA[1+a];   }
						k++;
						start=1;
					}

					if(j){
						if(0==inColor) _inColor=pLcd[k];
						Set_AACoeff_Draw(i,drawColor,_inColor,inRatioStart);
						for(int a=0;a<buff_AA[0];++a){	 if(0==inColor){ if(pLcd[k-a*BkpSizeX]!=_inColor){ _inColor=pLcd[k-a*BkpSizeX]; Set_AACoeff_Draw(i,drawColor,_inColor,inRatioStart);} }
							pLcd[k-a*BkpSizeX]=buff_AA[1+a];
						}
					}
				}
				k++;

				if(outRatioStart<1.0){
					if(0==outColor) _outColor=pLcd[k+BkpSizeX];
					Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
					for(int a=0;a<buff_AA[0];++a){ 	if(0==outColor){ if(pLcd[k+(a+1)*BkpSizeX]!=_outColor){ _outColor=pLcd[k+(a+1)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} }
						pLcd[k+(a+1)*BkpSizeX]=buff_AA[1+a];   }
				}
			}
			k--;
		}
		break;

	default:
		break;
	}
}

static void _DrawArrayBuffLeftDown2_AA(uint32_t drawColor, uint32_t outColor, uint32_t inColor, float outRatioStart, float inRatioStart, uint32_t BkpSizeX, int direction, uint16_t *buf)
{
	int j=buf[0], i=buf[1], p=2, i_prev, start=0;
	uint32_t _outColor=outColor;
	uint32_t _inColor=inColor;
	u32 k_temp;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--) pLcd[k--]=drawColor;
			i=buf[p++];

			if(outRatioStart<1.0){
				if(0==start){  start=1;
					k-=BkpSizeX;
					k_temp=k+i_prev;
					if(0==outColor) _outColor=pLcd[k_temp];
					Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
					for(int a=0;a<i_prev;++a){	  if(0==outColor){ if(pLcd[k_temp-a]!=_outColor){ _outColor=pLcd[k_temp-a]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} }
														  pLcd[k_temp-a]=buff_AA[1+a];   }	  /* Here it is not one pixel of correct to full the hole at the beginning of drawing as in _DrawArrayBuffRightDown2_AA() */
					k+=BkpSizeX;
				}

				if(j){
					if(0==outColor) _outColor=pLcd[k];
					Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);
					for(int a=0;a<buff_AA[0];++a){ 	if(0==outColor){ if(pLcd[k-a]!=_outColor){ _outColor=pLcd[k-a]; Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);} }
						pLcd[k-a]=buff_AA[1+a];  }
				}
			}
			k+=BkpSizeX;

			if(inRatioStart<1.0){
				if(0==inColor) _inColor=pLcd[k+1];
				Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);
				for(int a=0;a<buff_AA[0];++a){	if(0==inColor){ if(pLcd[k+1+a]!=_inColor){ _inColor=pLcd[k+1+a]; Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);} }
					pLcd[k+1+a]=buff_AA[1+a];
				}
			}
		}
		k-=BkpSizeX;
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){  pLcd[k]=drawColor;  k+=BkpSizeX;  }
			i=buf[p++];

			if(outRatioStart<1.0){
				if(0==start){  start=1;
					k++;
					if(0==outColor) _outColor=pLcd[k-i_prev*BkpSizeX];
					Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
					for(int a=0;a<i_prev;++a){	   if(0==outColor){ if(pLcd[k-(i_prev-a)*BkpSizeX]!=_outColor){ _outColor=pLcd[k-(i_prev-a)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} }
															pLcd[k-(i_prev-a)*BkpSizeX]=buff_AA[1+a];   }	 /* Here it is not one pixel of correct to full the hole at the beginning of drawing as in _DrawArrayBuffRightDown2_AA() */
					k--;
				}

				if(j){
					if(0==outColor) _outColor=pLcd[k];
					Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);
					for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k+a*BkpSizeX]!=_outColor){ _outColor=pLcd[k+a*BkpSizeX]; Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);} }
						pLcd[k+a*BkpSizeX]=buff_AA[1+a];
				}}
			}
			k--;

			if(inRatioStart<1.0){
				if(0==inColor) _inColor=pLcd[k-BkpSizeX];
				Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);
				for(int a=0;a<buff_AA[0];++a){ 	 if(0==inColor){ if(pLcd[k-(a+1)*BkpSizeX]!=_inColor){ _inColor=pLcd[k-(a+1)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);} }
					pLcd[k-(a+1)*BkpSizeX]=buff_AA[1+a];  }
			}
		}
		k++;
	}
}

static void _DrawArrayBuffLeftUp2_AA(uint32_t drawColor, uint32_t outColor, uint32_t inColor, float outRatioStart, float inRatioStart, uint32_t BkpSizeX, int direction, uint16_t *buf)
{
	int j=buf[0], i=buf[1], p=2, i_prev, start=0;
	uint32_t _outColor=outColor;
	uint32_t _inColor=inColor;
	u32 k_temp;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--)  pLcd[k--]=drawColor;
			i=buf[p++];

			if(inRatioStart<1.0){
				if(0==start){  start=1;
					k+=BkpSizeX;
					k_temp=k+i_prev;
					if(0==inColor) _inColor=pLcd[k_temp];
					Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);
					for(int a=0;a<i_prev;++a){	  if(0==inColor){ if(pLcd[k_temp-a]!=_inColor){ _inColor=pLcd[k_temp-a]; Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);} }
														  pLcd[k_temp-a]=buff_AA[1+a];   }	 /* Here it is not one pixel of correct to full the hole at the beginning of drawing as in _DrawArrayBuffRightUp2_AA() */
					k-=BkpSizeX;
				}

				if(j){
					if(0==inColor) _inColor=pLcd[k];
					Set_AACoeff_Draw(i,drawColor,_inColor,inRatioStart);
					for(int a=0;a<buff_AA[0];++a){	 if(0==inColor){ if(pLcd[k-a]!=_inColor){ _inColor=pLcd[k-a]; } Set_AACoeff_Draw(i,drawColor,_inColor,inRatioStart);}
						pLcd[k-a]=buff_AA[1+a];
					}
				}
			}
			k-=BkpSizeX;

			if(outRatioStart<1.0){
				if(0==outColor) _outColor=pLcd[k+1];
				Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
				for(int a=0;a<buff_AA[0];++a){ 	if(0==outColor){ if(pLcd[k+1+a]!=_outColor){ _outColor=pLcd[k+1+a]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} }
					pLcd[k+1+a]=buff_AA[1+a];  }
			}
		}
		k+=BkpSizeX;
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){  pLcd[k]=drawColor;  k-=BkpSizeX;  }
			i=buf[p++];

			if(outRatioStart<1.0){
				if(0==start){  start=1;
					k++;
					if(0==outColor) _outColor=pLcd[k+i_prev*BkpSizeX];
					Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
					for(int a=0;a<i_prev;++a){	   if(0==outColor){ if(pLcd[k+(i_prev-a)*BkpSizeX]!=_outColor){ _outColor=pLcd[k+(i_prev-a)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} }
															pLcd[k+(i_prev-a)*BkpSizeX]=buff_AA[1+a];   }	 /* Here it is not one pixel of correct to full the hole at the beginning of drawing as in _DrawArrayBuffRightUp2_AA() */
					k--;
				}

				if(j){
					if(0==outColor) _outColor=pLcd[k];
					Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);
					for(int a=0;a<buff_AA[0];++a){ 	if(0==outColor){ if(pLcd[k-a*BkpSizeX]!=_outColor){ _outColor=pLcd[k-a*BkpSizeX]; Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);} }
						pLcd[k-a*BkpSizeX]=buff_AA[1+a];  }
				}
			}
			k--;

			if(inRatioStart<1.0){
				if(0==inColor) _inColor=pLcd[k+BkpSizeX];
				Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);
				for(int a=0;a<buff_AA[0];++a){	 if(0==inColor){ if(pLcd[k+(a+1)*BkpSizeX]!=_inColor){ _inColor=pLcd[k+(a+1)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_inColor,inRatioStart);} }
					pLcd[k+(a+1)*BkpSizeX]=buff_AA[1+a];
				}
			}
		}
		k++;
	}
}

static void _DrawArrayBuffRightDown_AA(uint32_t _drawColor, uint32_t outColor, uint32_t inColor, float outRatioStart, float inRatioStart, uint32_t BkpSizeX, int direction, uint8_t *buf)		/* ! Attention !  number of pixels in one line H or V must not exceed value 255 because declaration 'uint8_t *buf'. In the future declare 'uint16_t *buf' */
{
	int j=buf[0], i=buf[1], p=2, i_prev;
	uint32_t drawColor=_drawColor;
	uint32_t _outColor=outColor;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;
				pLcd[k++]=drawColor;
			}
			if(0==outColor) _outColor=pLcd[k];
			if(j){
				i=buf[p++];
				Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);
				for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k+a]!=_outColor){ _outColor=pLcd[k+a]; Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);} } else if(pLcd[k+a]==drawColor) break;		/* if(0==outColor){ if(pLcd[k+a] != _outColor) break; } */
					pLcd[k+a]=buff_AA[1+a];
				}
			}
			k+=BkpSizeX;

			Set_AACoeff_Draw(i_prev,drawColor,inColor,inRatioStart);
			for(int a=0;a<buff_AA[0];++a){ 	if(0!=outColor){ if(pLcd[k-1-a]==drawColor) break; }
				pLcd[k-1-a]=buff_AA[1+a];  }
		}
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;
				pLcd[k]=drawColor;
				k+=BkpSizeX;
			}
			if(j){
				i=buf[p++];
				Set_AACoeff_Draw(i,drawColor,inColor,inRatioStart);
				for(int a=0;a<buff_AA[0];++a){ 	if(0!=outColor){ if(pLcd[k+a*BkpSizeX]==drawColor) break; }
					pLcd[k+a*BkpSizeX]=buff_AA[1+a]; }
			}
			k++;
			if(0==outColor) _outColor=pLcd[k-BkpSizeX];
			Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
			for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k-(a+1)*BkpSizeX]!=_outColor){ _outColor=pLcd[k-(a+1)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} } else if(pLcd[k-(a+1)*BkpSizeX]==drawColor) break;		/* if(0==outColor){ if(pLcd[k-(a+1)*BkpSizeX] != _outColor) break; } */
				pLcd[k-(a+1)*BkpSizeX]=buff_AA[1+a];
			}
		}
	}
}
static void OPTIMIZE_FAST _DrawArrayBuffRightDown_linePoints(uint32_t BkpSizeX, int direction, uint8_t *buf)		/* ! Attention !  number of pixels in one line H or V must not exceed value 255 because declaration 'uint8_t *buf'. In the future declare 'uint16_t *buf' */
{
	int j=buf[0], i=buf[1], p=2;

	if(0==direction)
	{
		while(j--)
		{
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;	k++;				}		if(j) i=buf[p++];		k+=BkpSizeX;
		}
	}
	else
	{
		while(j--)
		{
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;	k+=BkpSizeX;	}		if(j) i=buf[p++];		k++;
		}
	}
}

static void _DrawArrayBuffLeftDown_AA(uint32_t drawColor, uint32_t outColor, uint32_t inColor, float outRatioStart, float inRatioStart, uint32_t BkpSizeX, int direction, uint8_t *buf)
{
	int j=buf[0], i=buf[1], p=2, i_prev;
	uint32_t _outColor=outColor;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;
				pLcd[k--]=drawColor;	}

			if(j){
				i=buf[p++];
				Set_AACoeff_Draw(i,drawColor,inColor,inRatioStart);
				for(int a=0;a<buff_AA[0];++a){ 	if(0!=outColor){ if(pLcd[k-a]==drawColor) break; }
					pLcd[k-a]=buff_AA[1+a];  }
			}
			k+=BkpSizeX;
			if(0==outColor) _outColor=pLcd[k];
			Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
			for(int a=0;a<buff_AA[0];++a){	if(0==outColor){ if(pLcd[k+1+a]!=_outColor){ _outColor=pLcd[k+1+a]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} } else if(pLcd[k+1+a]==drawColor) break;		/* if(0==outColor){ if(pLcd[k+1+a] != _outColor) break; } */
				pLcd[k+1+a]=buff_AA[1+a];
			}
		}
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;
				pLcd[k]=drawColor;
				k+=BkpSizeX;
			}
			if(0==outColor) _outColor=pLcd[k];
			if(j){
				i=buf[p++];
				Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);
				for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k+a*BkpSizeX]!=_outColor){ _outColor=pLcd[k+a*BkpSizeX]; Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);} } else if(pLcd[k+a*BkpSizeX]==drawColor) break;		/* if(0==outColor){ if(pLcd[k+a*BkpSizeX] != _outColor) break; }*/
					pLcd[k+a*BkpSizeX]=buff_AA[1+a];
				}
			}
			k--;
			Set_AACoeff_Draw(i_prev,drawColor,inColor,inRatioStart);
			for(int a=0;a<buff_AA[0];++a){ 	if(0!=outColor){ if(pLcd[k-(a+1)*BkpSizeX]==drawColor) break; }
				pLcd[k-(a+1)*BkpSizeX]=buff_AA[1+a];  }
		}
	}
}
static void OPTIMIZE_FAST _DrawArrayBuffLeftDown_linePoints(uint32_t BkpSizeX, int direction, uint8_t *buf)
{
	int j=buf[0], i=buf[1], p=2;

	if(0==direction)
	{
		while(j--)
		{
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;	 k--;				}		if(j) i=buf[p++];		k+=BkpSizeX;
		}
	}
	else
	{
		while(j--)
		{
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;	 k+=BkpSizeX;	}		if(j) i=buf[p++];		k--;
		}
	}
}

static void _DrawArrayBuffLeftUp_AA(uint32_t drawColor, uint32_t outColor, uint32_t inColor, float outRatioStart, float inRatioStart, uint32_t BkpSizeX, int direction, uint8_t *buf)
{
	int j=buf[0], i=buf[1], p=2, i_prev;
	uint32_t _outColor=outColor;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;
				pLcd[k--]=drawColor;	}

			if(0==outColor) _outColor=pLcd[k];
			if(j){
				i=buf[p++];
				Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);
				for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k-a]!=_outColor){ _outColor=pLcd[k-a]; } Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);} else if(pLcd[k-a]==drawColor) break;		/* if(0==outColor){ if(pLcd[k-a] != _outColor) break; } */
					pLcd[k-a]=buff_AA[1+a];
				}
			}
			k-=BkpSizeX;

			Set_AACoeff_Draw(i_prev,drawColor,inColor,inRatioStart);
			for(int a=0;a<buff_AA[0];++a){ 	if(0!=outColor){ if(pLcd[k+1+a]==drawColor) break; }
				pLcd[k+1+a]=buff_AA[1+a];  }
		}
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;
				pLcd[k]=drawColor;
				k-=BkpSizeX;
			}

			if(j){
				i=buf[p++];
				Set_AACoeff_Draw(i,drawColor,inColor,inRatioStart);
				for(int a=0;a<buff_AA[0];++a){ 	if(0!=outColor){ if(pLcd[k-a*BkpSizeX]==drawColor) break; }
					pLcd[k-a*BkpSizeX]=buff_AA[1+a];  }
			}
			k--;
			if(0==outColor) _outColor=pLcd[k+BkpSizeX];
			Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
			for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k+(a+1)*BkpSizeX]!=_outColor){ _outColor=pLcd[k+(a+1)*BkpSizeX]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} } else if(pLcd[k+(a+1)*BkpSizeX]==drawColor) break; 		/* if(0==outColor){ if(pLcd[k+(a+1)*BkpSizeX] != _outColor) break; } */
				pLcd[k+(a+1)*BkpSizeX]=buff_AA[1+a];
			}
		}
	}
}
static void OPTIMIZE_FAST _DrawArrayBuffLeftUp_linePoints(uint32_t BkpSizeX, int direction, uint8_t *buf)
{
	int j=buf[0], i=buf[1], p=2;

	if(0==direction)
	{
		while(j--)
		{
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;		k--;  		}		if(j) i=buf[p++];		k-=BkpSizeX;
		}
	}
	else
	{
		while(j--)
		{
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;		k-=BkpSizeX; }		if(j) i=buf[p++];		k--;
		}
	}
}

static void _DrawArrayBuffRightUp_AA(uint32_t drawColor, uint32_t outColor, uint32_t inColor, float outRatioStart, float inRatioStart, uint32_t BkpSizeX, int direction, uint8_t *buf)
{
	int j=buf[0], i=buf[1], p=2, i_prev;
	uint32_t _outColor=outColor;

	if(0==direction)
	{
		while(j--)
		{
			i_prev=i;
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;
				pLcd[k++]=drawColor;	}

			if(j){
				i=buf[p++];
				Set_AACoeff_Draw(i,drawColor,inColor,inRatioStart);
				for(int a=0;a<buff_AA[0];++a){ 	if(0!=outColor){ if(pLcd[k+a]==drawColor) break; }
					pLcd[k+a]=buff_AA[1+a];  }
			}

			if(k > BkpSizeX) k -= BkpSizeX;
			if(0==outColor) _outColor=pLcd[k-1];
			Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);
			for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k-1-a]!=_outColor){ _outColor=pLcd[k-1-a]; Set_AACoeff_Draw(i_prev,drawColor,_outColor,outRatioStart);} } else if(pLcd[k-1-a]==drawColor) break; 		/* if(0==outColor){ if(pLcd[k-1-a] != _outColor) break; } */ 	if(k < 1+a) break;
				pLcd[k-1-a]=buff_AA[1+a];
			}
		}
	}
	else
	{
		while(j--)
		{
			i_prev=i;
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;
				pLcd[k]=drawColor;
				k-=BkpSizeX;
			}
			if(0==outColor) _outColor=pLcd[k];
			if(j){
				i=buf[p++];
				Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);
				for(int a=0;a<buff_AA[0];++a){	 if(0==outColor){ if(pLcd[k-a*BkpSizeX]!=_outColor){ _outColor=pLcd[k-a*BkpSizeX]; Set_AACoeff_Draw(i,drawColor,_outColor,outRatioStart);} } else if(pLcd[k-a*BkpSizeX]==drawColor) break; 		/* if(0==outColor){ if(pLcd[k-a*BkpSizeX] != _outColor) break; } */
					pLcd[k-a*BkpSizeX]=buff_AA[1+a];
				}
			}
			k++;
			Set_AACoeff_Draw(i_prev,drawColor,inColor,inRatioStart);
			for(int a=0;a<buff_AA[0];++a){ 	if(0!=outColor){ if(pLcd[k+(a+1)*BkpSizeX]==drawColor) break; }
				pLcd[k+(a+1)*BkpSizeX]=buff_AA[1+a];   }
		}
	}
}
static void OPTIMIZE_FAST _DrawArrayBuffRightUp_linePoints(uint32_t BkpSizeX, int direction, uint8_t *buf)
{
	int j=buf[0], i=buf[1], p=2;

	if(0==direction)
	{
		while(j--)
		{
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;	 k++;	 			}		if(j) i=buf[p++];		if(k > BkpSizeX) k -= BkpSizeX;
		}
	}
	else
	{
		while(j--)
		{
			while(i--){		if(nmbrLinePoints < STRUCT_TAB_SIZE(posLinePoints)-1) posLinePoints[nmbrLinePoints++]=k;	 k-=BkpSizeX;	}		if(j) i=buf[p++];		k++;
		}
	}
}

static void LCD_DrawCircle(uint32_t posBuff,uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t __x, uint32_t __y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor, int outColorRead)
{
	int matchWidth=0, circleFlag=0;													/* Circle.degColor[0] - is free space for future variable */			/* 'FrameColor' must not equal 'FillColor' (if equal then AA is zero) */
	uint16_t param =width>>16;
	uint32_t _FillColor, _width=(width&0x0000FFFF)-matchWidth;

	if(FillColor!=TRANSPARENT) _FillColor=FillColor;
	else								_FillColor=BkpColor;

	#define PARAM 		FrameColor, BkpColor, Circle.outRatioStart
	#define PARAM_AA 	FrameColor, BkpColor, _FillColor, Circle.outRatioStart, Circle.inRatioStart, BkpSizeX
	#define buf			Circle.lineBuff

	uint32_t bkX = BkpSizeX;
	uint32_t x=__x, y=__y;

	if((_width==height)&&(_width>0)&&(height>0))
	{
		GOTO_ToCalculateRadius:
	   _width=(width&0x0000FFFF)-matchWidth;

		float R=((float)_width)/2;
		uint32_t pxl_width = R/3;

		x=__x+_width/2 + matchWidth/2;
		if(_width%2) x++;
		y=__y;

		float _x=(float)x, _y=(float)y;
		float x0=(float)x, y0=(float)y+R;

		float param_y = pow(y0-_y,2);
		float param_x = pow(_x-x0,2);
		float decision = pow(R,2);

		int pxl_line=0,i=0;
		uint8_t block=1;

		void _CorrectDecision(void){
			if(block){	 int ni= CONDITION(R<=Circle.correctForWidth,0,1);
				if(i >= VALPERC(pxl_width,Circle.correctPercDeg[ni])){	block=0;   decision= pow(R+Circle.errorDecision[ni],2);  }
		}}

		buf[0]=pxl_width;
		do{
			_x++;  pxl_line++;
			param_x = pow(_x-x0,2);		_CorrectDecision();
			if((param_x+param_y) > decision){
				_y++;
				param_y = pow(y0-_y,2);
				buf[1+i++]=pxl_line-1;
				pxl_line=1;
			}
		}while(i<pxl_width);

		LCD_CircleCorrect();

  	 uint16_t _height=buf[0];
  	 for(int i=0;i<buf[0];++i) _height+=buf[buf[0]-i];
  	 _height = 2*_height;

		if(height>=_height)
			y= y+(height-_height)/2;
		else{
			matchWidth+=1;
			goto GOTO_ToCalculateRadius;
		}

		x-=(x-__x-_height/2);
		y-=(y-__y);

		Circle.width=_height;
		Circle.x0=x;
		Circle.y0=y+Circle.width/2;
		circleFlag=1;
	}
	else if((_width==0)&&(height==0));
	else return;

	uint8_t buf_Inv[buf[0]+1];
	InverseAndCopyBuff(buf_Inv,buf);
	_StartDrawLine(posBuff,bkX,x,y);

	uint32_t BkpColor_copy = BkpColor;
	if(outColorRead) BkpColor=0;
		_DrawArrayBuffRightDown_AA(PARAM_AA,0,buf); 	    _CopyDrawPos(); _IncDrawPos(-bkX); PixelCorrect(PARAM, 1,      1);    _SetCopyDrawPos();
		_DrawArrayBuffRightDown_AA(PARAM_AA,1,buf_Inv);                      				   PixelCorrect(PARAM, buf[1],bkX);   _IncDrawPos(-1);

		_DrawArrayBuffLeftDown_AA (PARAM_AA,1,buf); 		 _CopyDrawPos(); _IncDrawPos(1);    PixelCorrect(PARAM, 1,      1);    _SetCopyDrawPos();
		_DrawArrayBuffLeftDown_AA (PARAM_AA,0,buf_Inv);                      					PixelCorrect(PARAM, buf[1],-1);    _IncDrawPos(-bkX);

		_DrawArrayBuffLeftUp_AA  (PARAM_AA,0,buf);       _CopyDrawPos(); _IncDrawPos(bkX);  PixelCorrect(PARAM, 1,     -bkX);  _SetCopyDrawPos();
		_DrawArrayBuffLeftUp_AA  (PARAM_AA,1,buf_Inv);  						    					PixelCorrect(PARAM, buf[1],-bkX);  _IncDrawPos(1);

		_DrawArrayBuffRightUp_AA (PARAM_AA,1,buf);       _CopyDrawPos(); _IncDrawPos(-1);  	PixelCorrect(PARAM, 1,      1);    _SetCopyDrawPos();
		_DrawArrayBuffRightUp_AA (PARAM_AA,0,buf_Inv);                       					PixelCorrect(PARAM, buf[1], 1);
	if(outColorRead) BkpColor = BkpColor_copy;

		if(FillColor!=TRANSPARENT)
		{
	   	 uint16_t fillWidth, fillHeight=0;

	   	 void _RemoveUnnecessaryDots(int i, int nrDeg){			/* Remove 4 dots in Frame at 45, 135, 225, 315 degree for 'Percent_Circle' */
	   		 switch(nrDeg){
	   		 	 case 0:  if(i==buf[0]-1){ k--; 			 	 _DrawRight(1,FrameColor); 				   } break;
	   		 	 case 1:  if(i==0)		 { k--;  k-=bkX;	 _DrawRight(1,FrameColor); k+=bkX; 		   } break;
	   		 	 case 2:  if(i==buf[0]-1){ k-=2; k-=bkX; 	 _DrawRight(1,FrameColor); k++; k+=bkX;   } break;
	   		 	 case 3:  if(i==0)		 { k-=2; k+=2*bkX; _DrawRight(1,FrameColor); k-=2*bkX; k++; } break;
	   	}}

			_StartDrawLine(posBuff,bkX,x,y);
			fillWidth=0;
			for(int i=0;i<buf[0];++i){
				_IncDrawPos(bkX-buf[i+1]);	_CopyDrawPos(); k+=buf[i+1];	_RemoveUnnecessaryDots(i,0);  if(i>0)_DrawRight(2*fillWidth,FillColor);	  _SetCopyDrawPos();	fillHeight++;
				fillWidth += buf[i+1];
			}

			for(int i=0;i<buf[0];++i){
				fillWidth++;
				_IncDrawPos(-1);
				for(int j=0; j<buf[buf[0]-i]; ++j){
					_IncDrawPos(bkX);	_CopyDrawPos(); if(j==buf[buf[0]-i]-1){k+=1;_DrawRight(2*(fillWidth-1),FillColor); _RemoveUnnecessaryDots(i,1); }else{k+=2;_DrawRight(2*(fillWidth-2),FillColor);}	_SetCopyDrawPos();	fillHeight++;
				}
			}

			fillHeight = 2*fillHeight-1;

			_StartDrawLine(posBuff,bkX,x,y+fillHeight);
			fillWidth=0;
			for(int i=0;i<buf[0];++i){
				_IncDrawPos(-bkX-buf[i+1]); _CopyDrawPos();	k+=buf[i+1];	_RemoveUnnecessaryDots(i,2);  if(i>0)_DrawRight(2*fillWidth,FillColor);	_SetCopyDrawPos();
				fillWidth += buf[i+1];
			}

			for(int i=0;i<buf[0];++i){
				fillWidth++;
				_IncDrawPos(-1);
				for(int j=0; j<buf[buf[0]-i]; ++j){
					_IncDrawPos(-bkX);	_CopyDrawPos();  if(j==buf[buf[0]-i]-1){k+=1;_DrawRight(2*(fillWidth-1),FillColor); _RemoveUnnecessaryDots(i,3); }else{k+=2;_DrawRight(2*(fillWidth-2),FillColor);}	 _SetCopyDrawPos();
				}
			}
		}

	if(param==Percent_Circle && circleFlag)
	{
		int circleLinesLenCorrect;
		uint16_t fillWidth=0, fillHeight=0;

		for(int i=0;i<Circle.degree[0];++i)
		{
			circleLinesLenCorrect=0;
			if(IS_RANGE(Circle.degree[1+i],  34,  56)){
				if(Circle.width/2 < 100)
					circleLinesLenCorrect = 0;
				else
					circleLinesLenCorrect = 2;
			}
			else if(IS_RANGE(Circle.degree[1+i], 122, 149))
			{
				if(Circle.width/2 < 100)
					circleLinesLenCorrect = 0;
				else
					circleLinesLenCorrect = 4;
			}
			else if(IS_RANGE(Circle.degree[1+i], 206, 244)){
				if(Circle.width/2 < 100)
					circleLinesLenCorrect = 2;
				else
					circleLinesLenCorrect = 6;
			}
			else if(IS_RANGE(Circle.degree[1+i], 300, 330))
			{
				if(Circle.width/2 < 100)
					circleLinesLenCorrect = 0;
				else
					circleLinesLenCorrect = 4;
			}

			if(i==0)
				DrawLine(0,Circle.x0,Circle.y0,(Circle.width-4-circleLinesLenCorrect)/2-1,Circle.degree[1+i],FrameColor,BkpSizeX, Circle.outRatioStart,Circle.inRatioStart,_FillColor,Circle.degColor[i+1]);
			else if(i==Circle.degree[0]-1)
				DrawLine(0,Circle.x0,Circle.y0,(Circle.width-4-circleLinesLenCorrect)/2-1,Circle.degree[1+i],FrameColor,BkpSizeX, Circle.outRatioStart,Circle.inRatioStart,Circle.degColor[i],_FillColor);
			else
				DrawLine(0,Circle.x0,Circle.y0,(Circle.width-4-circleLinesLenCorrect)/2-1,Circle.degree[1+i],FrameColor,BkpSizeX, Circle.outRatioStart,Circle.inRatioStart,Circle.degColor[i],Circle.degColor[i+1]);
		}

		void _Fill(int width)
		{
			int j=width;
			uint8_t threshold[Circle.degree[0]];
			int subj=0, deltaDeg;
			uint32_t colorDeg=0;	uint8_t flagPermission=0;

			int _GGGG(void){
				for(int i=0;i<Circle.degree[0]-1;++i){
					deltaDeg=Circle.degree[1+i+1]-Circle.degree[1+i];
					if( ((deltaDeg>0)&&(deltaDeg<=180)) || (deltaDeg<-180) ){
						if((threshold[i]==0)&&(threshold[i+1]==2)){
							colorDeg= Circle.degColor[1+i];
							return 1;
						}
					}
					else{
						if((threshold[i]==0)||(threshold[i+1]==2)){
							colorDeg= Circle.degColor[1+i];
							return 2;
				}}}
				return 0;
			}

			while(j--)
			{
				if(pLcd[k]==FillColor){
					for(int i=0;i<Circle.degree[0];++i)
						threshold[i]=LCD_SearchRadiusPoints(posBuff,i,bkX);
					flagPermission=_GGGG();
				}

				GOTO_ToFillCircle:
				if(pLcd[k]==FillColor)
				{
					if(flagPermission) pLcd[k]= colorDeg;
					k++; subj=1;
				}
				if(pLcd[k]==FillColor){ if(j==0){ k--; break; } j--; subj=0; goto GOTO_ToFillCircle; }
				k++;
				if(subj==1){ subj=0; if(j>0) j--; else{ k--; break; }}
			}
		}

		void _Change_AA(int width)
		{
			int j=width;
			int threshold[Circle.degree[0]];
			int deltaDeg;

			while(j--)
			{
				for(int i=0;i<Circle.degree[0];++i)
					threshold[i]=LCD_SearchRadiusPoints(posBuff,i,bkX);

				for(int i=0;i<Circle.degree[0]-1;++i)
				{
					deltaDeg=Circle.degree[1+i+1]-Circle.degree[1+i];
					if( ((deltaDeg>0)&&(deltaDeg<=180)) || (deltaDeg<-180) )
					{
						if((threshold[i]==0)&&(threshold[i+1]==2)){
							pLcd[k]=GetTransitionColor(FrameColor, Circle.degColor[1+i], GetTransitionCoeff(FrameColor,FillColor,pLcd[k]));	/* 'FrameColor' must not equal 'FillColor' !!! */
							break;
						}
					}
					else
					{
						if((threshold[i]==0)||(threshold[i+1]==2)){
							pLcd[k]=GetTransitionColor(FrameColor, Circle.degColor[1+i], GetTransitionCoeff(FrameColor,FillColor,pLcd[k]));	/* 'FrameColor' must not equal 'FillColor' !!! */
							break;
				}}}
				k++;
		}}

		_StartDrawLine(posBuff,bkX,x,y);
		for(int i=0;i<buf[0];++i){
			_IncDrawPos(bkX-buf[i+1]);	  _CopyDrawPos();   _Change_AA(buf[i+1]); 	if(i>0) _Fill(2*fillWidth); _Change_AA(buf[i+1]); _SetCopyDrawPos();	fillHeight++;
			fillWidth += buf[i+1];
		}
		for(int i=0;i<buf[0];++i){
			fillWidth++;
			_IncDrawPos(-1);
			for(int j=0; j<buf[buf[0]-i]; ++j){
				_IncDrawPos(bkX);	_CopyDrawPos(); if(j==buf[buf[0]-i]-1){ _Change_AA(1); _Fill(2*(fillWidth-1)); _Change_AA(1); }else{  _Change_AA(2); _Fill(2*(fillWidth-2)); _Change_AA(2); }	_SetCopyDrawPos();	fillHeight++;
			}
		}

		fillHeight = 2*fillHeight-1;
		_StartDrawLine(posBuff,bkX,x,y+fillHeight);
		fillWidth=0;
		for(int i=0;i<buf[0];++i){
			_IncDrawPos(-bkX-buf[i+1]);	_CopyDrawPos();	_Change_AA(buf[i+1]); if(i>0) _Fill(2*fillWidth); _Change_AA(buf[i+1]);	_SetCopyDrawPos();
			fillWidth += buf[i+1];
		}

		for(int i=0;i<buf[0];++i){
			fillWidth++;
			_IncDrawPos(-1);
			for(int j=0; j<buf[buf[0]-i]; ++j){
				_IncDrawPos(-bkX);	_CopyDrawPos();  if(j==buf[buf[0]-i]-1){ _Change_AA(1); _Fill(2*(fillWidth-1)); _Change_AA(1);}else{ _Change_AA(2); _Fill(2*(fillWidth-2)); _Change_AA(2);}	 _SetCopyDrawPos();
			}
		}
	}
	#undef PARAM
	#undef PARAM_AA
	#undef buf
}

static void LCD_DrawHalfCircle(uint32_t posBuff,uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t __x, uint32_t __y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor)
{
	int matchWidth=0;
	uint32_t _FillColor, _width=(width&0x0000FFFF)-matchWidth;

	if(FillColor!=TRANSPARENT) _FillColor=FillColor;
	else								_FillColor=BkpColor;

	#define PARAM 		FrameColor, BkpColor, Circle.outRatioStart
	#define PARAM_AA 	FrameColor, BkpColor, _FillColor, Circle.outRatioStart, Circle.inRatioStart, BkpSizeX
	#define buf			Circle.lineBuff

	uint32_t bkX = BkpSizeX;
	uint32_t x=__x, y=__y;

	if((_width==height)&&(_width>0)&&(height>0))
	{
		GOTO_ToCalculateRadius:
	   _width=(width&0x0000FFFF)-matchWidth;

	   float R=((float)_width)/2;
		uint32_t pxl_width = R/3;

		x=__x+_width/2 + matchWidth/2;
		if(_width%2) x++;
		y=__y;

		float _x=(float)x, _y=(float)y;
		float x0=(float)x, y0=(float)y+R;

		float param_y = pow(y0-_y,2);
		float param_x = pow(_x-x0,2);
		float decision = pow(R,2);

		int pxl_line=0,i=0;
		uint8_t block=1;

		void _CorrectDecision(void){
			if(block){	 int ni= CONDITION(R<=Circle.correctForWidth,0,1);
				if(i >= VALPERC(pxl_width,Circle.correctPercDeg[ni])){	block=0;   decision= pow(R+Circle.errorDecision[ni],2);  }
		}}

		buf[0]=pxl_width;
		do{
			_x++;  pxl_line++;
			param_x = pow(_x-x0,2);		_CorrectDecision();
			if((param_x+param_y) > decision){
				_y++;
				param_y = pow(y0-_y,2);
				buf[1+i++]=pxl_line-1;
				pxl_line=1;
			}
		}while(i<pxl_width);

		LCD_CircleCorrect();

  	 uint16_t _height=buf[0];
  	 for(int i=0;i<buf[0];++i) _height+=buf[buf[0]-i];
  	 _height = 2*_height;

		if(height>=_height)
			y= y+(height-_height)/2;
		else{
			matchWidth+=1;
			goto GOTO_ToCalculateRadius;
		}

		x-=(x-__x-_height/2);
		y-=(y-__y);

		Circle.width=_height;

	}
	else if((_width==0)&&(height==0));
	else return;

	uint8_t buf_Inv[buf[0]+1];
	InverseAndCopyBuff(buf_Inv,buf);

	_StartDrawLine(0,bkX,x,y);

	switch(width>>16)
	{
	case Half_Circle_0:
		_DrawArrayBuffRightDown_AA(PARAM_AA,0,buf); 		 _CopyDrawPos(); _IncDrawPos(-bkX); PixelCorrect(PARAM, 1,      1);    _SetCopyDrawPos();
		_DrawArrayBuffRightDown_AA(PARAM_AA,1,buf_Inv);                      				   PixelCorrect(PARAM, buf[1],bkX);   _IncDrawPos(-1);

		_OffsetLeftDown(BkpSizeX,1,buf);
		_OffsetLeftDown(BkpSizeX,0,buf_Inv);  _IncDrawPos(-bkX);

		_OffsetLeftUp(BkpSizeX,0,buf);
		_OffsetLeftUp(BkpSizeX,1,buf_Inv);	_IncDrawPos(1);

		_DrawArrayBuffRightUp_AA (PARAM_AA,1,buf);       _CopyDrawPos(); _IncDrawPos(-1);  	PixelCorrect(PARAM, 1,      1);    _SetCopyDrawPos();
		_DrawArrayBuffRightUp_AA (PARAM_AA,0,buf_Inv);                       					PixelCorrect(PARAM, buf[1], 1);
		break;

	case Half_Circle_90:
		_DrawArrayBuffRightDown_AA(PARAM_AA,0,buf); 		 _CopyDrawPos(); _IncDrawPos(-bkX); PixelCorrect(PARAM, 1,      1);    _SetCopyDrawPos();
		_DrawArrayBuffRightDown_AA(PARAM_AA,1,buf_Inv);                      				   PixelCorrect(PARAM, buf[1],bkX);   _IncDrawPos(-1);

		_DrawArrayBuffLeftDown_AA (PARAM_AA,1,buf); 		 _CopyDrawPos(); _IncDrawPos(1);    PixelCorrect(PARAM, 1,      1);    _SetCopyDrawPos();
		_DrawArrayBuffLeftDown_AA (PARAM_AA,0,buf_Inv);                      					PixelCorrect(PARAM, buf[1],-1);    _IncDrawPos(-bkX);
		break;

	case Half_Circle_180:
		_OffsetRightDown(BkpSizeX,0,buf);
		_OffsetRightDown(BkpSizeX,1,buf_Inv);    _IncDrawPos(-1);

		_DrawArrayBuffLeftDown_AA (PARAM_AA,1,buf); 		 _CopyDrawPos(); _IncDrawPos(1);    PixelCorrect(PARAM, 1,      1);    _SetCopyDrawPos();
		_DrawArrayBuffLeftDown_AA (PARAM_AA,0,buf_Inv);                      					PixelCorrect(PARAM, buf[1],-1);    _IncDrawPos(-bkX);

		_DrawArrayBuffLeftUp_AA  (PARAM_AA,0,buf);       _CopyDrawPos(); _IncDrawPos(bkX);  PixelCorrect(PARAM, 1,     -bkX);  _SetCopyDrawPos();
		_DrawArrayBuffLeftUp_AA  (PARAM_AA,1,buf_Inv);  						    					PixelCorrect(PARAM, buf[1],-bkX);  _IncDrawPos(1);
		break;

	case Half_Circle_270:
		_OffsetRightDown(BkpSizeX,0,buf);
		_OffsetRightDown(BkpSizeX,1,buf_Inv);    _IncDrawPos(-1);

		_OffsetLeftDown(BkpSizeX,1,buf);
		_OffsetLeftDown(BkpSizeX,0,buf_Inv);    _IncDrawPos(-bkX);

		_DrawArrayBuffLeftUp_AA  (PARAM_AA,0,buf);       _CopyDrawPos(); _IncDrawPos(bkX);  PixelCorrect(PARAM, 1,     -bkX);  _SetCopyDrawPos();
		_DrawArrayBuffLeftUp_AA  (PARAM_AA,1,buf_Inv);  						    					PixelCorrect(PARAM, buf[1],-bkX);  _IncDrawPos(1);

		_DrawArrayBuffRightUp_AA (PARAM_AA,1,buf);       _CopyDrawPos(); _IncDrawPos(-1);  	PixelCorrect(PARAM, 1,      1);    _SetCopyDrawPos();
		_DrawArrayBuffRightUp_AA (PARAM_AA,0,buf_Inv);                       					PixelCorrect(PARAM, buf[1], 1);
		break;
	}

	if(FillColor!=TRANSPARENT)
	{
   	 uint16_t fillWidth, fillHeight=0;

   		switch(width>>16)
   		{
   		case Half_Circle_0:
   			_StartDrawLine(0,bkX,x,y);
   			fillWidth=0;
   			for(int i=0;i<buf[0];++i){
   				_IncDrawPos(bkX-buf[i+1]);	_CopyDrawPos(); k+=buf[i+1]; 	if(i>0) _DrawRight(2*fillWidth,FillColor);	_SetCopyDrawPos(); fillHeight++;
   				fillWidth += buf[i+1];
   			}

   			for(int i=0;i<buf[0];++i){
   				fillWidth++;
   				_IncDrawPos(-1);
   				for(int j=0; j<buf[buf[0]-i]; ++j){
   					_IncDrawPos(bkX);	_CopyDrawPos(); if(j==buf[buf[0]-i]-1){if(i<buf[0]-2){k+=1;_DrawRight(2*(fillWidth-1),FillColor);}}else{k+=2;_DrawRight(2*(fillWidth-2),FillColor);}	_SetCopyDrawPos();	fillHeight++;
   				}
   			}
   			break;

   		case Half_Circle_90:
   			_StartDrawLine(0,bkX,x,y);
   			fillWidth=0;
   			for(int i=0;i<buf[0];++i){
   				_IncDrawPos(bkX-buf[i+1]);	_CopyDrawPos(); k+=buf[i+1]+fillWidth; 	if(i>0) _DrawRight(1*fillWidth,FillColor);	_SetCopyDrawPos(); fillHeight++;
   				fillWidth += buf[i+1];
   			}

   			for(int i=0;i<buf[0];++i){
   				fillWidth++;
   				_IncDrawPos(-1);
   				for(int j=0; j<buf[buf[0]-i]; ++j){
   					_IncDrawPos(bkX);	_CopyDrawPos(); if(j==buf[buf[0]-i]-1){k+=1+fillWidth-1;_DrawRight(1*(fillWidth-1),FillColor);}else{k+=fillWidth;_DrawRight(1*(fillWidth-2),FillColor);}	_SetCopyDrawPos();	fillHeight++;
   				}
   			}

   			fillHeight = 2*fillHeight-1;

   			_StartDrawLine(0,bkX,x,y+fillHeight);
   			fillWidth=0;
   			for(int i=0;i<buf[0];++i){
   				_IncDrawPos(-bkX-buf[i+1]);_CopyDrawPos(); k+=buf[i+1]+fillWidth; 	if(i>0) _DrawRight(1*fillWidth,FillColor);  _SetCopyDrawPos();
   				fillWidth += buf[i+1];
   			}

   			for(int i=0;i<buf[0];++i){
   				fillWidth++;
   				_IncDrawPos(-1);
   				for(int j=0; j<buf[buf[0]-i]; ++j){
   					_IncDrawPos(-bkX);	_CopyDrawPos(); if(j==buf[buf[0]-i]-1){k+=1+fillWidth-1;_DrawRight(1*(fillWidth-1),FillColor);}else{k+=fillWidth;_DrawRight(1*(fillWidth-2),FillColor);}	_SetCopyDrawPos();	fillHeight++;
   				}
   			}
   			break;

   			case Half_Circle_180:
   			_StartDrawLine(0,bkX,x,y);
   			fillWidth=0;
   			for(int i=0;i<buf[0];++i){
   				_IncDrawPos(bkX-buf[i+1]);	 fillHeight++;
   				fillWidth += buf[i+1];
   			}

   			for(int i=0;i<buf[0];++i){
   				fillWidth++;
   				_IncDrawPos(-1);
   				for(int j=0; j<buf[buf[0]-i]; ++j){
   					_IncDrawPos(bkX);	 fillHeight++;
   				}
   			}

   			fillHeight = 2*fillHeight-1;

   			_StartDrawLine(0,bkX,x,y+fillHeight);
   			fillWidth=0;
   			for(int i=0;i<buf[0];++i){
   				_IncDrawPos(-bkX-buf[i+1]);	_CopyDrawPos(); k+=buf[i+1]; 	if(i>0) _DrawRight(2*fillWidth,FillColor);	_SetCopyDrawPos();
   				fillWidth += buf[i+1];
   			}

   			for(int i=0;i<buf[0];++i){
   				fillWidth++;
   				_IncDrawPos(-1);
   				for(int j=0; j<buf[buf[0]-i]; ++j){
   					_IncDrawPos(-bkX);	_CopyDrawPos(); if(j==buf[buf[0]-i]-1){if(i<buf[0]-2){k+=1;_DrawRight(2*(fillWidth-1),FillColor);}}else{k+=2;_DrawRight(2*(fillWidth-2),FillColor);}	_SetCopyDrawPos();
   				}
   			}
   			break;

   		case Half_Circle_270:
   			_StartDrawLine(0,bkX,x,y);
   			fillWidth=0;
   			for(int i=0;i<buf[0];++i){
   				_IncDrawPos(bkX-buf[i+1]);	_CopyDrawPos(); k+=buf[i+1]; 	if(i>0) _DrawRight(1*fillWidth,FillColor);	_SetCopyDrawPos(); fillHeight++;
   				fillWidth += buf[i+1];
   			}

   			for(int i=0;i<buf[0];++i){
   				fillWidth++;
   				_IncDrawPos(-1);
   				for(int j=0; j<buf[buf[0]-i]; ++j){
   					_IncDrawPos(bkX);	_CopyDrawPos(); if(j==buf[buf[0]-i]-1){k+=1;_DrawRight(1*(fillWidth-1),FillColor);}else{k+=2;_DrawRight(1*(fillWidth-2),FillColor);}	_SetCopyDrawPos();	fillHeight++;
   				}
   			}

   			fillHeight = 2*fillHeight-1;

   			_StartDrawLine(0,bkX,x,y+fillHeight);
   			fillWidth=0;
   			for(int i=0;i<buf[0];++i){
   				_IncDrawPos(-bkX-buf[i+1]);_CopyDrawPos(); k+=buf[i+1]; 	if(i>0) _DrawRight(1*fillWidth,FillColor);  _SetCopyDrawPos();
   				fillWidth += buf[i+1];
   			}

   			for(int i=0;i<buf[0];++i){
   				fillWidth++;
   				_IncDrawPos(-1);
   				for(int j=0; j<buf[buf[0]-i]; ++j){
   					_IncDrawPos(-bkX);	_CopyDrawPos(); if(j==buf[buf[0]-i]-1){k+=1;_DrawRight(1*(fillWidth-1),FillColor);}else{k+=2;_DrawRight(1*(fillWidth-2),FillColor);}	_SetCopyDrawPos();	fillHeight++;
   				}
   			}
   			break;
   		}
	}

	#undef PARAM
	#undef PARAM_AA
	#undef buf
}

static void LCD_DrawRoundRectangle2(u32 posBuff,int rectangleFrame,u32 BkpSizeX,u32 BkpSizeY,u32 x,u32 y,u32 width,u32 height,u32 FrameColorStart,u32 FrameColorStop,u32 FillColorStart,u32 FillColorStop,u32 BkpColor,float ratioStart,DIRECTIONS direct)
{
	#define A(a,b) 	_FillBuff(a,b)

	int stepGrad = SHIFT_RIGHT(rectangleFrame,24,FF);	/* effect 3D */
	int shapeType = MASK(rectangleFrame,FF);
	uint8_t AAoutOff    = SHIFT_RIGHT(BkpColor,24,1);
	uint8_t readBkColor = SHIFT_RIGHT(BkpColor,25,1);
	uint32_t o1=0,o2=0,  i1=0,i2=0,i3=0,i4=0,	FrameColorTemp=0;
	int iFrameHeight=0, iFillHeight=0;

	if(EQUAL2_OR( SHIFT_RIGHT(BkpColor,24,FF),0,255)){ AAoutOff=0; readBkColor=0; }

	void _CalcInternalTransColor(u32 colorFrame, int offs){
		if(direct==Up || direct==Down){
			i1 = GetTransitionColor(colorFrame,buff_AA[1+iFillHeight],AA.c1);
			i2 = GetTransitionColor(colorFrame,buff_AA[1+iFillHeight],AA.c2);
			i3=i2;
			i4=i1;
		}
		else if(direct==Right){
			i1 = GetTransitionColor(colorFrame,buff_AA[1+offs],AA.c1);
			i2 = GetTransitionColor(colorFrame,buff_AA[1+offs],AA.c2);
			i3 = GetTransitionColor(colorFrame,buff_AA[1+(width-1)-offs],AA.c2);
			i4 = GetTransitionColor(colorFrame,buff_AA[1+(width-1)-offs],AA.c1);
		}
		else if(direct==Left){
			i1 = GetTransitionColor(colorFrame,buff_AA[1+(width-1)-offs],AA.c1);
			i2 = GetTransitionColor(colorFrame,buff_AA[1+(width-1)-offs],AA.c2);
			i3 = GetTransitionColor(colorFrame,buff_AA[1+offs],AA.c2);
			i4 = GetTransitionColor(colorFrame,buff_AA[1+offs],AA.c1);
	}}

	void _Fill(int x,int offs){
		if(shapeType){
			switch((int)direct){
				case Down:
					A(x,buff_AA[1+iFillHeight++]);
					break;
				case Up:
					A(x,buff_AA[1+iFillHeight--]);
					break;
				case Right:
					LOOP_FOR(i,x){ A(1,buff_AA[1+offs+i]); }
					break;
				case Left:
					LOOP_FOR(i,x){ A(1,buff_AA[1+(width-1)-offs-i]); }
					break;
		}}
		else
			k+=x;
	}

	void _Out_AA_left(int stage){		/*FrameColor*/
		#define _A(i)	 if(0==readBkColor) A(i,BkpColor); else k+=i
		if(0==AAoutOff){
			if(readBkColor) BkpColor=pLcd[k-1];
			o1 = GetTransitionColor(FrameColorTemp,BkpColor,AA.c1);		/* _CalcOutTransColor */
			o2 = GetTransitionColor(FrameColorTemp,BkpColor,AA.c2);
		}
		if(0==AAoutOff)
		{	switch(stage)
			{
			case 0:	_A(3); A(1,o2); A(1,o1);  break;
			case 1:	_A(2); A(1,o1);  break;
			case 2:	_A(1); A(1,o1);  break;
			case 3:	A(1,o2); break;
			case 4:	A(1,o1); break;
			}
		}
		else
		{  switch(stage)
			{
			case 0:	k+=5; break;
			case 1:	k+=3; break;
			case 2:	k+=2; break;
			case 3:	k+=1; break;
			case 4:	k+=1; break;
	}}
	#undef _A
	}

	void _Out_AA_right(int stage){		/*FrameColor*/
		#define _A(i)	 if(0==readBkColor) A(i,BkpColor); else k+=i
		if(readBkColor){
			BkpColor=pLcd[k+5];
			o1 = GetTransitionColor(FrameColorTemp,BkpColor,AA.c1);		/* _CalcOutTransColor */
			o2 = GetTransitionColor(FrameColorTemp,BkpColor,AA.c2);
		}
		if(0==AAoutOff)
		{	switch(stage)
			{
			case 0:	A(1,o1); A(1,o2); _A(3);  break;
			case 1:	A(1,o1); _A(2);  break;
			case 2:	A(1,o1); _A(1);  break;
			case 3:	A(1,o2); break;
			case 4:	A(1,o1); break;
			}
		}
		else
		{	switch(stage)
			{
			case 0:	k+=5; break;
			case 1:	k+=3; break;
			case 2:	k+=2; break;
			case 3:	k+=1; break;
			case 4:	k+=1; break;
	}}
	#undef _A
	}

	void _SetFillColor(u32 FillStart, u32 FillStop){
		switch((int)direct){
			case Down: case Up:		Set_AACoeff(height,FillStart,FillStop,ratioStart);  break;
			case Right: case Left:	Set_AACoeff(width, FillStart,FillStop,ratioStart);	 break;
	}}

	Set_AACoeff2(height,FrameColorStart,FrameColorStop,ratioStart);		/* careful for 'height' < MAX_SIZE_TAB_AA */
	_SetFillColor(FillColorStart,FillColorStop);

	switch((int)direct){
		case Down:	iFillHeight=0;				break;
		case Up:		iFillHeight=height-1;	break;
		case Right: case Left:	break;
		}

	_StartDrawLine(posBuff,BkpSizeX,x,y);
	FrameColorTemp=buff2_AA[1+iFrameHeight++]; 															_Out_AA_left(0); 												A(width-10,FrameColorTemp);			 							 	  _Out_AA_right(0);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=buff2_AA[1+iFrameHeight++]; _CalcInternalTransColor(FrameColorTemp,7);	_Out_AA_left(1); A(2,FrameColorTemp); A(1,i1);A(1,i2); _Fill(width-14,7); A(1,i3); A(1,i4); A(2,FrameColorTemp); _Out_AA_right(1);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=buff2_AA[1+iFrameHeight++]; _CalcInternalTransColor(FrameColorTemp,4);	_Out_AA_left(2); A(1,FrameColorTemp); A(1,i1); 			 _Fill(width-8,4); 			  A(1,i4); A(1,FrameColorTemp); _Out_AA_right(2);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=buff2_AA[1+iFrameHeight++]; _CalcInternalTransColor(FrameColorTemp,3);	_Out_AA_left(3); A(1,FrameColorTemp); A(1,i1); 			 _Fill(width-6,3); 			  A(1,i4); A(1,FrameColorTemp); _Out_AA_right(3);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=buff2_AA[1+iFrameHeight++]; _CalcInternalTransColor(FrameColorTemp,2);	_Out_AA_left(4); A(1,FrameColorTemp); 						 _Fill(width-4,2); 			  			  A(1,FrameColorTemp); _Out_AA_right(4);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=buff2_AA[1+iFrameHeight++]; _CalcInternalTransColor(FrameColorTemp,2);						  A(1,FrameColorTemp); A(1,i1); 			 _Fill(width-4,2);  		  	  A(1,i4); A(1,FrameColorTemp);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=buff2_AA[1+iFrameHeight++]; _CalcInternalTransColor(FrameColorTemp,2);						  A(1,FrameColorTemp); 			 A(1,i2); _Fill(width-4,2); 	A(1,i3);			  A(1,FrameColorTemp);
	_NextDrawLine(BkpSizeX,width);

	int _height = height-14;
	int _width = width-2;
	if(shapeType)
	{
		for (int j=0; j<_height; j++){
			FrameColorTemp=buff2_AA[1+iFrameHeight++];
			A(1, FrameColorTemp);
			_Fill(_width,0);
			A(1, BrightDecr(FrameColorTemp,stepGrad));
			_NextDrawLine(BkpSizeX,width);
	}}
	else
	{
		for (int j=0; j<_height; j++){
			FrameColorTemp=buff2_AA[1+iFrameHeight++];
			A(1, FrameColorTemp);
			k+=_width;
			A(1, BrightDecr(FrameColorTemp,stepGrad));
			_NextDrawLine(BkpSizeX,width);
	}}

	if(stepGrad) _SetFillColor(BrightDecr(FillColorStart,stepGrad),BrightDecr(FillColorStop,stepGrad));

	FrameColorTemp=BrightDecr(buff2_AA[1+iFrameHeight++],stepGrad); _CalcInternalTransColor(FrameColorTemp,2);						  A(1,FrameColorTemp);  		 A(1,i2); _Fill(width-4,2); A(1,i3); 			  A(1,FrameColorTemp);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=BrightDecr(buff2_AA[1+iFrameHeight++],stepGrad); _CalcInternalTransColor(FrameColorTemp,2);						  A(1,FrameColorTemp); A(1,i1); 			 _Fill(width-4,2); 			  A(1,i4); A(1,FrameColorTemp);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=BrightDecr(buff2_AA[1+iFrameHeight++],stepGrad); _CalcInternalTransColor(FrameColorTemp,2);	_Out_AA_left(4); A(1,FrameColorTemp); 						 _Fill(width-4,2); 						  A(1,FrameColorTemp); _Out_AA_right(4);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=BrightDecr(buff2_AA[1+iFrameHeight++],stepGrad); _CalcInternalTransColor(FrameColorTemp,3);	_Out_AA_left(3); A(1,FrameColorTemp); A(1,i1); 			 _Fill(width-6,3); 			  A(1,i4); A(1,FrameColorTemp); _Out_AA_right(3);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=BrightDecr(buff2_AA[1+iFrameHeight++],stepGrad); _CalcInternalTransColor(FrameColorTemp,4);	_Out_AA_left(2); A(1,FrameColorTemp); A(1,i1); 			 _Fill(width-8,4); 			  A(1,i4); A(1,FrameColorTemp); _Out_AA_right(2);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=BrightDecr(buff2_AA[1+iFrameHeight++],stepGrad); _CalcInternalTransColor(FrameColorTemp,7);	_Out_AA_left(1); A(2,FrameColorTemp); A(1,i1);A(1,i2); _Fill(width-14,7); A(1,i3); A(1,i4); A(2,FrameColorTemp); _Out_AA_right(1);
	_NextDrawLine(BkpSizeX,width);
	FrameColorTemp=BrightDecr(buff2_AA[1+iFrameHeight++],stepGrad);															_Out_AA_left(0); 												A(width-10,FrameColorTemp); 											  _Out_AA_right(0);

	#undef  A
}

static structK GetPxlAround(u32 k, u32 bkX, int maxPxls, u32 colorPxl){		/* not used */
	structPosition pos={0};
	structK posK={0};
	int radiusMin=2*(maxPxls+1), p=0;
	for(int j=-maxPxls; j<maxPxls+1; ++j){
		p=k+j*bkX;
		for(int i=-maxPxls; i<maxPxls+1; ++i){
			if(pLcd[p+i]==colorPxl){
				if(ABS(i)+ABS(j) < radiusMin){
					radiusMin=ABS(i)+ABS(j);
					pos.x=i;
					pos.y=j;
	}}}}
	posK.k[0]= k +  (pos.y)	 *bkX +  (pos.x);
	posK.k[1]= k +  (pos.y/2)*bkX +  (pos.x/2);
	posK.k[2]= k + (-pos.y/2)*bkX + (-pos.x/2);
	posK.k[3]= k + (-pos.y)	 *bkX + (-pos.x);

	return posK;
}

static int LCD_CIRCLE_GetDegFromPosXY(int x,int y, int x0,int y0)
{
	if(x == x0){
		if(y >= y0) return 90;
		else			return 270;
	}
	if(y == y0){
		if(x >= x0) return 180;
		else			return 0;
	}

	int deg = DEG(atan(ABS((float)y/(float)x)));

		  if(x < x0 && y > y0) return deg;
	else if(x > x0 && y > y0) return 90+(90-deg);
	else if(x > x0 && y < y0) return 180+deg;
	else if(x < x0 && y < y0) return 270+(90-deg);

	return 0;
}

static int LCD_CIRCLE_GetRadiusFromPosXY(int x,int y, int x0,int y0){
	return sqrt(ABS((x-x0)*(x-x0)) + ABS((y-y0)*(y-y0)));
}

static void GRAPH_ClearPosXYpar(void){
	for(int i=0;i<1;i++){ posXY_par[i].startX=0; posXY_par[i].startY=0; posXY_par[i].yMin=0; posXY_par[i].yMax=0; posXY_par[i].nmbrPoints=0; posXY_par[i].precision=0; posXY_par[i].scaleX=0; posXY_par[i].scaleY=0; posXY_par[i].funcPatternType=0; posXY_par[i].len_posXY=0; posXY_par[i].len_posXYrep=0; }
}
static void GRAPH_ClearPosXY(void){
	for(int i=0;i<GRAPH_MAX_SIZE_POSXY;i++){ posXY[i].x=0; posXY[i].y=0; }
}
static void GRAPH_ClearPosXYrep(void){
	for(int i=0;i<GRAPH_MAX_SIZE_POSXY;i++){ posXY_rep[i].x=0; posXY_rep[i].y=0; 	posXY_rep[i].rx=0; posXY_rep[i].ry=0; }
}

static int GRAPH_SetPointers(int offsMem, int nrMem)
{
	#if defined(GRAPH_MEMORY_SDRAM2)
		extern char* GETVAL_ptr(uint32_t nrVal);
		extern uint32_t GETVAL_freeMemSize(uint32_t offs);

		int size_param		= sizeof(structGetSmpl);
		int size_posXY 	= 	 GRAPH_MAX_SIZE_POSXY*sizeof(structPosU16);
		int size_posXYrep = 2*GRAPH_MAX_SIZE_POSXY*sizeof(structRepPos);				/* size of structRepPos is 2 times larger then size of structPosU16,   2 * size of structRepPos is for pixels correct for GRAPH_Display() */

		int ptr2Mem = offsMem + (SIZE_ONE_CHART * nrMem);

		if(GETVAL_freeMemSize(ptr2Mem) > size_param + size_posXY + size_posXYrep){
			posXY_par = (structGetSmpl*) GETVAL_ptr (ptr2Mem);
			posXY 	 = (structPosU16*)  GETVAL_ptr (ptr2Mem + size_param);
			posXY_rep = (structRepPos*)  GETVAL_ptr (ptr2Mem + size_param + size_posXY); 	return 0; }
		else return 1;
	#endif
	return 0;
}

static double GRAPHFUNC_UpDownUp(double posX){
	if(posX <350) 		 return 0;
	else if(posX==350) return 51;
	else if(posX==351) return 10;
	else if(posX==352) return 49;
	else if(posX>352)  return 46;
	return 0;
}
static double GRAPHFUNC_DownUpDown(double posX){
	if(posX <350) 		 return 0;
	else if(posX==350) return -51;
	else if(posX==351) return -10;
	else if(posX==352) return -57;
	else if(posX>352)  return -54;
	else if(posX==354) return 113;
	else if(posX==355) return -113;
	else if(posX>355)  return 0;
	return 0;
}
static double GRAPHFUNC_DownUpDown2(double posX){
	if(posX <350) 		 return 0;
	else if(posX==350) return -51;
	else if(posX==351) return -10;
	else if(posX>351)  return -10;
	return 0;
}
static double GRAPHFUNC_Example1(double posX){
	if(posX<350) 		 return 0;
	else if(posX==350) return 51;
	else if(posX==351) return 10;
	else if(posX>351 ) return 9;
	return 0;
}
static double GRAPHFUNC_Example2(double posX){
	if(posX <350) 						return 0;
	else if(posX==350) 				return -51;
	else if(posX>350 && posX<450) return -51;
	else if(posX==450)				return -101;
	else if(posX>450 && posX<500) return -101;
	else if(posX==500)				return -103;
	else									return -106;
	return 0;
}
static double GRAPHFUNC_Example3(double posX){
/*	if(posX <350) 		 return 0;
	else if(posX==350) return 10;
	else if(posX==351) return 0;
	else if(posX==352) return 8;
	else if(posX==353) return 1;
	else if(posX==354) return 7;
	else if(posX==355) return 2;
	else if(posX==356) return 6;
	else if(posX==357) return 3;
	else if(posX==358) return 44;
	else if(posX>358)  return 41;
	return 0;
*/
/*
	if(posX <350) 		 return 0;
	else if(posX==350) return 10;
	else if(posX==351) return 0;
	else if(posX==352) return 4;
	else if(posX==353) return 2;
	else if(posX>353)  return 41;
	return 0;
*/
	if(posX <350) 		 return 0;
	else if(posX==350) return 4;
	else if(posX==351) return 2;
	else if(posX==352) return 4;
	else if(posX==353) return 2;
	else if(posX>353)  return 41;
	return 0;
}
static double GRAPHFUNC_Square(double posX){
	if		 (IS_RANGE(posX, 0,  50)) return  0;
	else if(IS_RANGE(posX, 51,100)) return  1;
	else if(IS_RANGE(posX,101,150)) return -1;
	else if(IS_RANGE(posX,151,200)) return  1;
	else if(IS_RANGE(posX,201,250)) return -1;
	else if(IS_RANGE(posX,251,300)) return  1;
	else if(IS_RANGE(posX,301,350)) return -1;
	else if(IS_RANGE(posX,351,400)) return  1;
	else if(IS_RANGE(posX,401,450)) return -1;
	else if(IS_RANGE(posX,451,500)) return  1;
	else 									  return  0;
}
static double GRAPHFUNC_Owner(double posX){
	/* return bufferY[posX]; */
	extern char* GETVAL_ptr(uint32_t nrVal);
	structPosition *pos;
	pos = (structPosition*) GETVAL_ptr (memOffsForGraphOwnFunc);
	return (double) (pos[(int)posX].y);		/* return (double) ((pos+(int)posX)->x); */
}

static double GRAPHFUNC_Noise(void){
	static uint32_t aRandom32bit=0;
	static int cnt=0, cnt2=0, flag=0, randMask=0;

	if(flag){	cnt2++;	if(cnt2%5==0) aRandom32bit=300-(cnt2/5);								  }
	else	  {  				if(cnt%2==0)  HAL_RNG_GenerateRandomNumber(&hrng,&aRandom32bit); }

		  if(cnt%50==0) randMask = 0x2F;
	else if(cnt%51==0) randMask = 0x0F;
	else if(cnt%52==0) randMask = 0x1F;

	if(cnt%32==0){	 flag=1-flag;	cnt2=0;  }
	cnt++;
	return aRandom32bit & randMask;
}

static double GRAPHFUNC_Noise2(void){
	static uint32_t aRandom32bit=0;
	HAL_RNG_GenerateRandomNumber(&hrng,&aRandom32bit);
	return aRandom32bit & 0x2F;
}

static double GRAPH_GetFuncPosY(int funcPatternType, double posX){
	switch(funcPatternType){
		case Func_sin:		return sin(TANG_ARG(posX));
		case Func_cos:		return cos(TANG_ARG(posX));
		case Func_sin1:	return (sin(3*TANG_ARG(posX))+cos(2*TANG_ARG(posX)));
		case Func_sin2:	return (sin(TANG_ARG(posX))+0.3*sin(3*TANG_ARG(posX))+cos(2*TANG_ARG(posX))+0.2*cos(20*TANG_ARG(posX)));
		case Func_sin3:	return ABS(sin(TANG_ARG(posX)));
		case Func_log:		return log((sin(3*TANG_ARG(posX))+cos(2*TANG_ARG(posX))));
		case Func_tan:		return tan(TANG_ARG(posX));

		case Func_noise:  return GRAPHFUNC_Noise();
		case Func_noise2: return GRAPHFUNC_Noise2();
		case Func_square: return GRAPHFUNC_Square(posX);

		case Func_lines1: return GRAPHFUNC_UpDownUp(posX);
		case Func_lines2: return GRAPHFUNC_DownUpDown(posX);
		case Func_lines3: return GRAPHFUNC_DownUpDown2(posX);
		case Func_lines4: return GRAPHFUNC_Example1(posX);
		case Func_lines5: return GRAPHFUNC_Example2(posX);
		case Func_lines6: return GRAPHFUNC_Example3(posX);

		case Func_owner:	return GRAPHFUNC_Owner(posX);

		default:
			return 0;
}}

static int GRAPH_GetFuncPosXY(int startX,int startY, int yMin,int yMax, int nmbrPoints,double precision, double scaleX,double scaleY, int funcPatternType)
{
	structPosU16 posXY_prev={0};
	int temp_x, temp_y, diff_Y, delta, n=0;
	double funcVal;

	posXY[n].x = startX;
	posXY[n].y = startY;
	posXY_prev.x = posXY[n].x;
	posXY_prev.y = posXY[n].y;
	n++;

	LOOP_FOR2(i,nmbrPoints,precision)
	{
		funcVal = scaleY * GRAPH_GetFuncPosY(funcPatternType, scaleX*i);
		funcVal *=-1;
		funcVal = SET_IN_RANGE(funcVal,yMin,yMax);

		temp_x = posXY[0].x + (int)i;
		temp_y = posXY[0].y + (int)funcVal;

		if(posXY_prev.x != temp_x)
		{
			if(temp_y > posXY_prev.y +1)
			{
				delta = temp_y - (posXY_prev.y +1);
				diff_Y=0;
				while(1){
					posXY[n].x = temp_x;
					posXY[n].y = diff_Y + (posXY_prev.y +1);
					n++;
					if(n >= GRAPH_MAX_SIZE_POSXY-1) return n;
					delta--; diff_Y++;
					if(delta==0) break;
				}
			}
			else if(temp_y < posXY_prev.y -1)
			{
				delta = (posXY_prev.y -1) - temp_y;
				diff_Y=0;
				while(1){
					posXY[n].x = temp_x;
					posXY[n].y = (posXY_prev.y -1) - diff_Y;
					n++;
					if(n >= GRAPH_MAX_SIZE_POSXY-1) return n;
					delta--; diff_Y++;
					if(delta==0) break;
				}
			}
			posXY[n].x = temp_x;
			posXY[n].y = temp_y;
			posXY_prev.x = temp_x;
			posXY_prev.y = temp_y;
			n++;
			if(n >= GRAPH_MAX_SIZE_POSXY-1) return n;
		}
	}
	return n;
}

static void GRAPH_DispPosXY(int offs_k, int widthBk, int numberOfPoints, u32 color){
	LOOP_FOR(i,numberOfPoints){
		pLcd[offs_k + posXY[i].y * widthBk + posXY[i].x] = color;
}}

static int GRAPH_RepetitionRedundancyOfPosXY(int nmbrPoints)
{
	int j=0,i,prevState=0;

	for(i=0; i<nmbrPoints-1; ++i)
	{
		if(prevState==1)
		{
			if(posXY[i].x+1==posXY[i+1].x && posXY[i].y==posXY[i+1].y);
			else{
				posXY_rep[j].rx=0;
				posXY_rep[j].ry++;
				prevState=0;
				j++;   goto TempEnd_RepetitionRedundancy;
			}
		}
		else if(prevState==2)
		{
			if(posXY[i].x-1==posXY[i+1].x && posXY[i].y==posXY[i+1].y);
			else{
				posXY_rep[j].rx=0;
				posXY_rep[j].ry--;
				prevState=0;
				j++;   goto TempEnd_RepetitionRedundancy;
			}
		}
		else if(prevState==3)
		{
			if(posXY[i].y+1==posXY[i+1].y && posXY[i].x==posXY[i+1].x);
			else{
				posXY_rep[j].rx++;
				posXY_rep[j].ry=0;
				prevState=0;
				j++;   goto TempEnd_RepetitionRedundancy;
			}
		}
		else if(prevState==4)
		{
			if(posXY[i].y-1==posXY[i+1].y && posXY[i].x==posXY[i+1].x);
			else{
				posXY_rep[j].rx--;
				posXY_rep[j].ry=0;
				prevState=0;
				j++;   goto TempEnd_RepetitionRedundancy;
			}
		}

		if(posXY[i].x+1==posXY[i+1].x && posXY[i].y==posXY[i+1].y)
		{
			if(prevState==0){
				prevState=1;
				posXY_rep[j].x = posXY[i].x;
				posXY_rep[j].y = posXY[i].y;
			}
			posXY_rep[j].ry++;
		}
		else if(posXY[i].x-1==posXY[i+1].x && posXY[i].y==posXY[i+1].y)
		{
			if(prevState==0){
				prevState=2;
				posXY_rep[j].x = posXY[i].x;
				posXY_rep[j].y = posXY[i].y;
			}
			posXY_rep[j].ry--;
		}
		else if(posXY[i].y+1==posXY[i+1].y && posXY[i].x==posXY[i+1].x)
		{
			if(prevState==0){
				prevState=3;
				posXY_rep[j].x = posXY[i].x;
				posXY_rep[j].y = posXY[i].y;
			}
			posXY_rep[j].rx++;
		}
		else if(posXY[i].y-1==posXY[i+1].y && posXY[i].x==posXY[i+1].x)
		{
			if(prevState==0){
				prevState=4;
				posXY_rep[j].x = posXY[i].x;
				posXY_rep[j].y = posXY[i].y;
			}
			posXY_rep[j].rx--;
		}
		else if(posXY[i].x+1==posXY[i+1].x && posXY[i].y+1==posXY[i+1].y)
		{
			posXY_rep[j].x = posXY[i].x;
			posXY_rep[j].y = posXY[i].y;
			posXY_rep[j].rx = 1;
			posXY_rep[j].ry = 1;
			j++;
		}
		else if(posXY[i].x-1==posXY[i+1].x && posXY[i].y+1==posXY[i+1].y)
		{
			posXY_rep[j].x = posXY[i].x;
			posXY_rep[j].y = posXY[i].y;
			posXY_rep[j].rx = 1;
			posXY_rep[j].ry = -1;
			j++;
		}
		else if(posXY[i].x+1==posXY[i+1].x && posXY[i].y-1==posXY[i+1].y)
		{
			posXY_rep[j].x = posXY[i].x;
			posXY_rep[j].y = posXY[i].y;
			posXY_rep[j].rx = -1;
			posXY_rep[j].ry = 1;
			j++;
		}
		else if(posXY[i].x-1==posXY[i+1].x && posXY[i].y-1==posXY[i+1].y)
		{
			posXY_rep[j].x = posXY[i].x;
			posXY_rep[j].y = posXY[i].y;
			posXY_rep[j].rx = -1;
			posXY_rep[j].ry = -1;
			j++;
		}
		else
		{
			posXY_rep[j].x = posXY[i].x; 		/* here it is never ? */
			posXY_rep[j].y = posXY[i].y;
			posXY_rep[j].rx = 0;
			posXY_rep[j].ry = 0;
			j++;
		}

		TempEnd_RepetitionRedundancy:
		if(j>=GRAPH_MAX_SIZE_POSXY-1) return j;
	}

	if(prevState==0){
		posXY_rep[j].x = posXY[i].x;
		posXY_rep[j].y = posXY[i].y;
		posXY_rep[j].rx = 1;
		posXY_rep[j].ry = 1;
	}
	j++;

	return j;
}

static void GRAPH_DispPosXYrep(int offs_k, int widthBk, int lenStruct, u32 color){
	LOOP_FOR(a,lenStruct){
		if(posXY_rep[a].ry!=0){
			LOOP_FOR(b,ABS(posXY_rep[a].ry)){
				if(posXY_rep[a].ry > 0)	 pLcd[offs_k + posXY_rep[a].y*widthBk + posXY_rep[a].x + b]=color;
				else							 pLcd[offs_k + posXY_rep[a].y*widthBk + posXY_rep[a].x - b]=color;
		}}
		else if(posXY_rep[a].rx!=0){
			LOOP_FOR(b,ABS(posXY_rep[a].rx)){
				if(posXY_rep[a].rx > 0)	 pLcd[offs_k + (posXY_rep[a].y+b)*widthBk + posXY_rep[a].x]=color;
				else							 pLcd[offs_k + (posXY_rep[a].y-b)*widthBk + posXY_rep[a].x]=color;
		}}
		else{
			if(posXY_rep[a].ry==0 && posXY_rep[a].rx==0){
				pLcd[offs_k + posXY_rep[a].y*widthBk + posXY_rep[a].x]=color;
		}}
}}

static void GRAPH_Display(int offs_k, int widthBk, int lenStruct, u32 color, u32 colorOut, u32 colorIn, float outRatioStart, float inRatioStart, int corr45degAA)
{
	#define NONE_FUNC_TYPE	100
	#define MAX_SIZE_BUFF	widthBk

	#define IS_RightDownDir0		(pos[i].x+ABS(pos[i].ry) == pos[i+1].x  &&  pos[i].y+1==pos[i+1].y)
	#define IS_RightUpDir0			(pos[i].x+ABS(pos[i].ry) == pos[i+1].x  &&  pos[i].y-1==pos[i+1].y)
	#define IS_LeftDownDir0			(pos[i].x-ABS(pos[i].ry) == pos[i+1].x  &&  pos[i].y+1==pos[i+1].y)
	#define IS_LeftUpDir0			(pos[i].x-ABS(pos[i].ry) == pos[i+1].x  &&  pos[i].y-1==pos[i+1].y)

	#define IS_RightDownDir1		(pos[i].y+ABS(pos[i].rx) == pos[i+1].y  &&  pos[i].x+1==pos[i+1].x)
	#define IS_RightUpDir1			(pos[i].y-ABS(pos[i].rx) == pos[i+1].y  &&  pos[i].x+1==pos[i+1].x)
	#define IS_LeftDownDir1			(pos[i].y+ABS(pos[i].rx) == pos[i+1].y  &&  pos[i].x-1==pos[i+1].x)
	#define IS_LeftUpDir1			(pos[i].y-ABS(pos[i].rx) == pos[i+1].y  &&  pos[i].x-1==pos[i+1].x)

	#define IS_RightUpDownDir1		((pos[i].y+1)-(ABS(pos[i].rx)-1) == pos[i+1].y  &&  pos[i].x+1==pos[i+1].x)
   #define IS_RightDownUpDir1		((pos[i].y-1)+(ABS(pos[i].rx)-1) == pos[i+1].y  &&  pos[i].x+1==pos[i+1].x)
	#define IS_RightUpDownDir1_ver2		(pos[i].rx < 0  &&  pos[i+1].rx > 0)
	#define IS_RightDownUpDir1_ver2		(pos[i].rx > 0  &&  pos[i+1].rx < 0)

	structRepPos *pos_beforeCorrect = posXY_rep, 	*pos = posXY_rep + GRAPH_MAX_SIZE_POSXY;
	u16 buff[MAX_SIZE_BUFF];
	u8 functionType = NONE_FUNC_TYPE;
	u16 lastSample = 0;
	int i;

	void _GetSamplesDir0(int sign){
		if(lastSample){
			if(lastSample%2==0){
				buff[1+buff[0]++]=lastSample/2;
				if(functionType==NONE_FUNC_TYPE){ _StartDrawLine(offs_k+sign*(lastSample/2), widthBk, pos[i].x, pos[i].y); }
			}
			else{
				buff[1+buff[0]++]=lastSample/2+1;
				if(functionType==NONE_FUNC_TYPE){ _StartDrawLine(offs_k+sign*(lastSample/2), widthBk, pos[i].x, pos[i].y); }
			}
		}
		else{
			buff[1+buff[0]++]=ABS(pos[i].ry);
			if(functionType==NONE_FUNC_TYPE){ _StartDrawLine(offs_k, widthBk,pos[i].x,pos[i].y); }
		}
		lastSample=0;
	}

	void _GetSamplesDir1(int sign){
		if(lastSample){
			if(lastSample%2==0){
				buff[1+buff[0]++]=lastSample/2;
				if(functionType==NONE_FUNC_TYPE){ _StartDrawLine(offs_k+sign*widthBk*(lastSample/2), widthBk, pos[i].x, pos[i].y); }
			}
			else{
				buff[1+buff[0]++]=lastSample/2+1;
				if(functionType==NONE_FUNC_TYPE){ _StartDrawLine(offs_k+sign*widthBk*(lastSample/2), widthBk, pos[i].x, pos[i].y); }
			}
		}
		else{
			buff[1+buff[0]++]=ABS(pos[i].rx);
			if(functionType==NONE_FUNC_TYPE){ _StartDrawLine(offs_k, widthBk,pos[i].x,pos[i].y); }
		}
		lastSample=0;
	}

	for(i=0; i<lenStruct; ++i) *(pos+i)=*(pos_beforeCorrect+i);

	/*     		 					 _____            ____
	   Pixels correct from:  ___|       to:  ___|			for 'Up' (on this example) and the same for 'Down'
	*/
	for(i=0; i<lenStruct-1; ++i){
			  if((pos[i].x+1==pos[i+1].x) && (pos[i].y+1+pos[i].rx==pos[i+1].y))	 pos[i].rx++;
		else if((pos[i].x+1==pos[i+1].x) && (pos[i].y-1+pos[i].rx==pos[i+1].y))	 pos[i].rx--;
	}

	 /* We have to add one pixel at the end to draw correct last part of graph. Algorithm must detect changes on the last part of graph. */
	i=lenStruct++;
	pos[i].x =pos[i-1].x + pos[i-1].ry;
	pos[i].y =pos[i-1].y+1;
	pos[i].rx=1;
	pos[i].ry=1;


	/* Main operation */
	for(i=0; i<MAX_SIZE_BUFF; ++i) buff[i]=0;
	for(i=0; i<lenStruct; ++i)
	{
		TempEnd_Display:
		if(buff[0]>=MAX_SIZE_BUFF-1)
			return;

		if(IS_RightDownDir0	&& EQUAL2_OR(functionType,NONE_FUNC_TYPE,RightDownDir0) && ABS(pos[i].ry)>0){
			_GetSamplesDir0(1);
			functionType = RightDownDir0;
		}
		else{
			if(functionType == RightDownDir0){
				if(IS_RightUpDir0){
					lastSample=ABS(pos[i].ry);
					buff[1+buff[0]++]=lastSample/2;
				}
				else
					buff[1+buff[0]++]=ABS(pos[i].ry);

				_DrawArrayBuffRightDownUp2_AA(Down,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegTo45, buff,corr45degAA);
				functionType=NONE_FUNC_TYPE;
				buff[0]=0;
				goto TempEnd_Display;
			}
		}

		if(IS_RightUpDir0 && EQUAL2_OR(functionType,NONE_FUNC_TYPE,RightUpDir0) && ABS(pos[i].ry)>0){
			_GetSamplesDir0(1);
			functionType = RightUpDir0;
		}
		else{
			if(functionType == RightUpDir0){
				if(IS_RightDownDir0){
					lastSample=ABS(pos[i].ry);
					buff[1+buff[0]++]=lastSample/2;
				}
				else
					buff[1+buff[0]++]=ABS(pos[i].ry);

				_DrawArrayBuffRightDownUp2_AA(Up,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegTo45, buff,corr45degAA);
				functionType=NONE_FUNC_TYPE;
				buff[0]=0;
				goto TempEnd_Display;
			}
		}

		if(IS_RightDownDir1	&& (functionType==NONE_FUNC_TYPE || functionType==RightDownDir1) && ABS(pos[i].rx)>0){
			_GetSamplesDir1(1);
			functionType = RightDownDir1;
		}
		else{
			if(functionType == RightDownDir1){
				if(IS_LeftDownDir1){
					lastSample=ABS(pos[i].rx);
					buff[1+buff[0]++]=lastSample/2;
				}
				else
					buff[1+buff[0]++]=ABS(pos[i].rx);

				_DrawArrayBuffRightDownUp2_AA(Down,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff,corr45degAA);
				functionType=NONE_FUNC_TYPE;
				buff[0]=0;
				goto TempEnd_Display;
			}
		}

		if(IS_RightUpDir1	&& (functionType==NONE_FUNC_TYPE || functionType==RightUpDir1) && ABS(pos[i].rx)>0){
			_GetSamplesDir1(-1);
			functionType = RightUpDir1;
		}
		else{
			if(functionType == RightUpDir1){
				if(IS_LeftUpDir1){
					lastSample=ABS(pos[i].rx);
					buff[1+buff[0]++]=lastSample/2;
				}
				else
					buff[1+buff[0]++]=ABS(pos[i].rx);

				_DrawArrayBuffRightDownUp2_AA(Up,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff,corr45degAA);
				functionType=NONE_FUNC_TYPE;
				buff[0]=0;
				goto TempEnd_Display;
			}
		}

		if(IS_LeftDownDir0	&& (functionType==NONE_FUNC_TYPE || functionType==LeftDownDir0) && ABS(pos[i].ry)>0){
			_GetSamplesDir0(-1);
			functionType = LeftDownDir0;
		}
		else{
			if(functionType == LeftDownDir0){
				if(IS_LeftUpDir0){
					lastSample=ABS(pos[i].ry);
					buff[1+buff[0]++]=lastSample/2;
				}
				else
					buff[1+buff[0]++]=ABS(pos[i].ry);

				_DrawArrayBuffLeftDown2_AA(color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegTo45, buff);
				functionType=NONE_FUNC_TYPE;
				buff[0]=0;
				goto TempEnd_Display;
			}
		}

		if(IS_LeftUpDir0	&& (functionType==NONE_FUNC_TYPE || functionType==LeftUpDir0) && ABS(pos[i].ry)>0){
			_GetSamplesDir0(-1);
			functionType = LeftUpDir0;
		}
		else{
			if(functionType == LeftUpDir0){
				if(IS_LeftDownDir0){
					lastSample=ABS(pos[i].ry);
					buff[1+buff[0]++]=lastSample/2;
				}
				else
					buff[1+buff[0]++]=ABS(pos[i].ry);

				_DrawArrayBuffLeftUp2_AA(color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegTo45, buff);
				functionType=NONE_FUNC_TYPE;
				buff[0]=0;
				goto TempEnd_Display;
			}
		}

		if(IS_LeftDownDir1	&& (functionType==NONE_FUNC_TYPE || functionType==LeftDownDir1) && ABS(pos[i].rx)>0){
			_GetSamplesDir1(1);
			functionType = LeftDownDir1;
		}
		else{
			if(functionType == LeftDownDir1){
				if(IS_RightDownDir1){
					lastSample=ABS(pos[i].rx);
					buff[1+buff[0]++]=lastSample/2;
				}
				else
					buff[1+buff[0]++]=ABS(pos[i].rx);

				_DrawArrayBuffLeftDown2_AA(color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff);
				functionType=NONE_FUNC_TYPE;
				buff[0]=0;
				goto TempEnd_Display;
			}
		}

		if(IS_LeftUpDir1	&& (functionType==NONE_FUNC_TYPE || functionType==LeftUpDir1) && ABS(pos[i].rx)>0){
			_GetSamplesDir1(-1);
			functionType = LeftUpDir1;
		}
		else{
			if(functionType == LeftUpDir1){
				if(IS_RightUpDir1){
					lastSample=ABS(pos[i].rx);
					buff[1+buff[0]++]=lastSample/2;
				}
				else
					buff[1+buff[0]++]=ABS(pos[i].rx);

				_DrawArrayBuffLeftUp2_AA(color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff);
				functionType=NONE_FUNC_TYPE;
				buff[0]=0;
				goto TempEnd_Display;
			}
		}


		if(IS_RightUpDownDir1_ver2 && (functionType==NONE_FUNC_TYPE || functionType==RightUpDownDir1))
		{
			buff[0]=0;    buff[1+buff[0]++]=ABS(pos[i].rx);
			_StartDrawLine(offs_k, widthBk,pos[i].x,pos[i].y);
			_DrawArrayBuffRightDownUp2_AA(Up,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff,corr45degAA);
			functionType = NONE_FUNC_TYPE;
			buff[0]=0;
		}

		if(IS_RightDownUpDir1_ver2 && (functionType==NONE_FUNC_TYPE || functionType==RightDownUpDir1))
		{
			buff[0]=0;    buff[1+buff[0]++]=ABS(pos[i].rx);
			_StartDrawLine(offs_k, widthBk,pos[i].x,pos[i].y);
			_DrawArrayBuffRightDownUp2_AA(Down,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff,corr45degAA);
			functionType = NONE_FUNC_TYPE;
			buff[0]=0;
		}


		if(IS_RightUpDownDir1	&& (functionType==NONE_FUNC_TYPE || functionType==RightUpDownDir1) && ABS(pos[i].rx)>2){
			_GetSamplesDir1(-1);
			functionType = RightUpDownDir1;
		}
		else if(functionType == RightUpDownDir1)
		{
			_DrawArrayBuffRightDownUp2_AA(Up,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff,corr45degAA);
			buff[0]=0;  buff[1+buff[0]++]=ABS(pos[i].rx);
			_StartDrawLine(offs_k, widthBk, pos[i].x, pos[i].y);
			_DrawArrayBuffRightDownUp2_AA(Down,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff,corr45degAA);
			functionType=NONE_FUNC_TYPE;
			buff[0]=0;
			goto TempEnd_Display;
		}


		if(IS_RightDownUpDir1	&& (functionType==NONE_FUNC_TYPE || functionType==RightDownUpDir1) && ABS(pos[i].rx)>2){
			_GetSamplesDir1(-1);
			functionType = RightDownUpDir1;
		}
		else if(functionType == RightDownUpDir1)
		{
			_DrawArrayBuffRightDownUp2_AA(Down,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff,corr45degAA);
			buff[0]=0;  buff[1+buff[0]++]=ABS(pos[i].rx);
			_StartDrawLine(offs_k, widthBk, pos[i].x, pos[i].y);
			_DrawArrayBuffRightDownUp2_AA(Up,color, colorOut,colorIn, outRatioStart,inRatioStart, widthBk, DegAbove45, buff,corr45degAA);
			functionType=NONE_FUNC_TYPE;
			buff[0]=0;
			goto TempEnd_Display;
		}
	}

	#undef NONE_FUNC_TYPE
	#undef MAX_SIZE_BUFF

	#undef IS_RightDownDir0
	#undef IS_RightUpDir0
	#undef IS_LeftDownDir0
	#undef IS_LeftUpDir0

	#undef IS_RightDownDir1
	#undef IS_RightUpDir1
	#undef IS_LeftDownDir1
	#undef IS_LeftUpDir1

	#undef IS_RightUpDownDir1
	#undef IS_RightDownUpDir1
	#undef IS_RightUpDownDir1_ver2
	#undef IS_RightDownUpDir1_ver2
}

static structPosition OPTIMIZE_FAST DrawLine_linePoints(uint32_t posBuff,uint16_t x0, uint16_t y0, float len, float degree, uint32_t lineColor,uint32_t BkpSizeX)
{	/* here we don`t read BkColor */
	#define LINES_BUFF_SIZE		(u16)len+6

	float degree_copy=degree;
	uint8_t linesBuff[LINES_BUFF_SIZE];
	int k_iteration=0, searchDirection=0;
	int nrPointsPerLine=0, iteration=1;
	int findNoPoint=0, rot;
	float param_y;
	float param_x;
	float decision;

	void _FFFFFFFFFF(void)
	{
		switch(rot)
		{  case 1: switch(searchDirection){ case 0: k_iteration=-1;	 		 break; case 1: k_iteration=-BkpSizeX-1; break; } break;
			case 2: switch(searchDirection){ case 0: k_iteration=-BkpSizeX; break; case 1: k_iteration=-BkpSizeX-1; break; } break;
			case 3: switch(searchDirection){ case 0: k_iteration=-BkpSizeX; break; case 1: k_iteration=-BkpSizeX+1; break; } break;
			case 4: switch(searchDirection){ case 0: k_iteration=1;     	 break; case 1: k_iteration=-BkpSizeX+1; break; } break;
			case 5: switch(searchDirection){ case 0: k_iteration=1;     	 break; case 1: k_iteration= BkpSizeX+1; break; } break;
			case 6: switch(searchDirection){ case 0: k_iteration=BkpSizeX;  break; case 1: k_iteration= BkpSizeX+1; break; } break;
			case 7: switch(searchDirection){ case 0: k_iteration=BkpSizeX;  break; case 1: k_iteration= BkpSizeX-1; break; } break;
			case 8: switch(searchDirection){ case 0: k_iteration=-1;   		 break; case 1: k_iteration= BkpSizeX-1; break; } break;
		}
	}

	if(degree_copy==0)
		degree_copy=360;

	_StartDrawLine(posBuff,BkpSizeX,x0,y0);
	rot=LCD_SearchLinePoints(1,posBuff,x0,y0,degree_copy,BkpSizeX);
	_FFFFFFFFFF();


	do
	{	pos_prev = pos;
		if(LCD_SearchLinePoints(0,posBuff,x0,y0,degree_copy,BkpSizeX)==1)
		{
			nrPointsPerLine++;
		   if(findNoPoint)
		   {
		   	findNoPoint=0;
		   	searchDirection=1-searchDirection;
		   	_FFFFFFFFFF();
		   }
		}
		else
		{
		   k-=k_iteration;
		   searchDirection=1-searchDirection;
		   if(nrPointsPerLine)
		   {
		   	if(iteration<LINES_BUFF_SIZE-2)
		   		linesBuff[iteration++]=nrPointsPerLine;
		   	else break;
		   }
		   nrPointsPerLine=0;
			findNoPoint=1;
			_FFFFFFFFFF();
		}
		k+=k_iteration;
	   pos = _GetPosXY(posBuff,BkpSizeX);

		param_y = pow((float)(y0-pos.y),2);
		param_x = pow((float)(x0-pos.x),2);
		decision = pow(len+1,2);

	}while((param_x+param_y) <= decision);		/* When precision of float 'degree' is greater than 0.1 then 'decision' may have to big decimal value for float for this precision and never return from the loop while() - endless loop */

	if(nrPointsPerLine)
		linesBuff[iteration++]=nrPointsPerLine;
	linesBuff[0]=iteration-1;

	_StartDrawLine(posBuff,BkpSizeX,x0,y0);

		switch(rot)
		{
			case 1:  _DrawArrayBuffLeftUp_linePoints   (BkpSizeX,0,linesBuff); break;
			case 2:  _DrawArrayBuffLeftUp_linePoints   (BkpSizeX,1,linesBuff); break;
			case 3:  _DrawArrayBuffRightUp_linePoints  (BkpSizeX,1,linesBuff); break;
			case 4:  _DrawArrayBuffRightUp_linePoints  (BkpSizeX,0,linesBuff); break;
			case 5:  _DrawArrayBuffRightDown_linePoints(BkpSizeX,0,linesBuff); break;
			case 6:  _DrawArrayBuffRightDown_linePoints(BkpSizeX,1,linesBuff); break;
			case 7:  _DrawArrayBuffLeftDown_linePoints (BkpSizeX,1,linesBuff); break;
			case 8:  _DrawArrayBuffLeftDown_linePoints (BkpSizeX,0,linesBuff); break;
		}

	#undef LINES_BUFF_SIZE
	return pos_prev;
}

/* ################################## -- Global Declarations -- ######################################################### */
void LCD_Buffer(u16 BkSizeX, u16 x,u16 y, u32 color){
	pLcd[BkSizeX*(y)+(x)] = color;
}
float LCD_GetDegFrom2Points(int x,int y, int x0,int y0){
	if(x == x0){
		if(y >= y0) return 270;
		else			return 90;
	}
	else if(y == y0){
		if(x >= x0) return 180;
		else			return 0;
	}
	else{
		float deg = DEG(atan(ABS((float)(y-y0)/(float)(x-x0))));
			  if(x < x0 && y > y0) return 270+(90-deg);
		else if(x > x0 && y > y0) return 180+deg;
		else if(x > x0 && y < y0) return 90+(90-deg);
		else if(x < x0 && y < y0) return deg;
		return 0;
}}
float LCD_GetLenFrom2Points(int x,int y, int x0,int y0){
	return  sqrt( pow(x-x0,2) + pow(y-y0,2) );
}

void LCD_ResetAllBasicGraphicsParams(void){
	LOOP_FOR(i,MAX_CHARTS_SIMULTANEOUSLY){
		chartMemOffsForMemNr[i] = -1;
	}
}
void CorrectLineAA_on(void){
	correctLine_AA=1;
}
void CorrectLineAA_off(void){
	correctLine_AA=0;
}

void Set_AACoeff_RoundFrameRectangle(float coeff_1, float coeff_2){
	AA.c1 = coeff_1;
	AA.c2 = coeff_2;
}

void Set_AACoeff_Draw(int pixelsInOneSide, uint32_t colorFrom, uint32_t colorTo, float ratioStart)
{
/*	int pixelsInOneSide= pixelsInOneSide_==1?3:pixelsInOneSide_;
	int pixelsInOneSide;

	if(pixelsInOneSide_==1) pixelsInOneSide=2;
	else                    pixelsInOneSide=pixelsInOneSide_;
*/
	float incr= (1-ratioStart)/pixelsInOneSide;
	buff_AA[0]=pixelsInOneSide;
	for(int i=0;i<pixelsInOneSide;++i){
		if(1+i>=MAX_SIZE_TAB_AA)
			break;
		buff_AA[1+i]= GetTransitionColor(colorFrom,colorTo, ratioStart+i*incr);
	}
}

void LCD_LineH2(uint32_t posBuff,uint32_t BkpSizeX, uint16_t x, uint16_t y, uint16_t width,  uint32_t color, uint16_t bold){
	_StartDrawLine(posBuff,BkpSizeX, x, y);	_DrawRight(width, color);
	for(int i=0; i<bold; ++i){
		_NextDrawLine(BkpSizeX,width);	_DrawRight(width, color);
}}
void LCD_LineH(uint32_t BkpSizeX, uint16_t x, uint16_t y, uint16_t width,  uint32_t color, uint16_t bold){
	_StartDrawLine(0,BkpSizeX, x, y);	_DrawRight(width, color);
	for(int i=0; i<bold; ++i){
		_NextDrawLine(BkpSizeX,width);	_DrawRight(width, color);
	}
}
void LCD_LineV2(uint32_t posBuff,uint32_t BkpSizeX, uint16_t x, uint16_t y, uint16_t width,  uint32_t color, uint16_t bold){
	_StartDrawLine(posBuff,BkpSizeX, x, y);	_CopyDrawPos();	_DrawDown(width, color, BkpSizeX);
	for(int i=0; i<bold; ++i){
		_SetCopyDrawPos();	_IncDrawPos(1); _CopyDrawPos();  _DrawDown(width, color, BkpSizeX);
}}
void LCD_LineV(uint32_t BkpSizeX, uint16_t x, uint16_t y, uint16_t width,  uint32_t color, uint16_t bold){
	_StartDrawLine(0,BkpSizeX, x, y);	_CopyDrawPos();	_DrawDown(width, color, BkpSizeX);
	for(int i=0; i<bold; ++i){
		_SetCopyDrawPos();	_IncDrawPos(1); _CopyDrawPos();  _DrawDown(width, color, BkpSizeX);
	}
}
void LCD_ErasePrevShape(int posX_prev,int posY_prev, int posX,int posY, int width,int height, u32 *pBuff){
	int diffX = ABS(posX-posX_prev);
	int diffY = ABS(posY-posY_prev);
	if(diffX >= width || diffY >= height)  LCD_DisplayBuff(posX_prev,posY_prev, width,height, pBuff);
	else{
		int offsY = CONDITION(posY<posY_prev,height-diffY,0);
		int offsX = CONDITION(posX<posX_prev,width-diffX,0);
		if(diffY) LCD_DisplayBuff( posX_prev, posY_prev+offsY, width,diffY, pBuff+width*offsY);

		if(diffX){
			_2LOOP_2INIT(int m=0, int n=0, i,j, diffX,height)
				pBuff[m++] = *(pBuff+offsX+n+i);
				_1LOOP_END
				n+=width;
			_1LOOP_END
			LCD_DisplayBuff(posX_prev+offsX,posY_prev, diffX,height, pBuff);
	}}
}
void LCD_CopyBuffers(u32 *pBuff1,u32 offs1,u16 widthBk1,u16 Xpos1,u16 Ypos1,	 u32 *pBuff2,u32 offs2,u16 widthBk2,u16 Xpos2,u16 Ypos2,u16 width2,u16 height2){		/* copy direction:  pBuff1 <= pBuff2 */
	int k2=0, k1=0;
	if(widthBk1 < width2) return;
	if(widthBk2==width2 && Xpos2==0 && Ypos2==0){
		if(widthBk1==width2 && Xpos1==0 && Ypos1==0){
			_2LOOP(i,j,width2,height2)
				pBuff1[offs1+k1++] = pBuff2[offs2+k2++];
			_2LOOP_END
		}
		else{
			_2LOOP(i,j,width2,height2)
				pBuff1[offs1+widthBk1*(Ypos1+j)+Xpos1+i] = pBuff2[offs2+k2++];
			_2LOOP_END
	}}
	else{
		if(widthBk1==width2 && Xpos1==0 && Ypos1==0){
			_2LOOP(i,j,width2,height2)
				pBuff1[offs1+k1++] = pBuff2[offs2+widthBk2*(Ypos2+j)+Xpos2+i];
			_2LOOP_END
		}
		else{
			_2LOOP(i,j,width2,height2)
				pBuff1[offs1+widthBk1*(Ypos1+j)+Xpos1+i] = pBuff2[offs2+widthBk2*(Ypos2+j)+Xpos2+i];
			_2LOOP_END
	}}
}
void LCD_Display(uint32_t posBuff, uint32_t Xpos, uint32_t Ypos, uint32_t width, uint32_t height){
	LCD_DisplayBuff(Xpos,Ypos,width,height,  pLcd+posBuff);
}
void LCD_DisplayPart(uint32_t posBuff, uint32_t Xpos, uint32_t Ypos, uint32_t width, uint32_t height){
    /* One rectangle transfer from the full canvas, not one DMA per scanline.
     * DMA2D FGOR skips pixels outside this rectangle between source rows. */
    if(Xpos>=LCD_X || Ypos>=LCD_Y || !width || !height ||
       width>LCD_X-Xpos || height>LCD_Y-Ypos) return;
    LCD_DisplayBuffStride(Xpos,Ypos,width,height,
                         pLcd+posBuff+LCD_X*Ypos+Xpos,LCD_X);
}

void LCD_Show(void){
	LCD_Display(0,0,0,LCD_X,LCD_Y);
}

void LCD_Shape(uint32_t x,uint32_t y,figureShape pShape,uint32_t width,uint32_t height,uint32_t FrameColor,uint32_t FillColor,uint32_t BkpColor){
	LCD_ShapeWindow(pShape,0,LCD_X,LCD_Y,x,y,width,height,FrameColor,FillColor,BkpColor);
}
void LCD_ShapeWindow(figureShape pShape,uint32_t posBuff,uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	pShape(posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FillColor,BkpColor);
}
void LCD_ShapeIndirect(uint32_t xPos,uint32_t yPos,figureShape pShape, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	pShape(0,width,height,0,0,width,height,FrameColor,FillColor,BkpColor);
	LCD_Display(0,xPos,yPos,width,height);
}
void LCD_ShapeWindowIndirect(uint32_t xPos,uint32_t yPos,figureShape pShape,uint32_t posBuff,uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	pShape(posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FillColor,BkpColor);
	LCD_Display(posBuff,xPos,yPos,BkpSizeX,BkpSizeY);
}

void LCD_Clear(uint32_t color){
	LCD_ShapeWindow(LCD_Rectangle,0,LCD_X,LCD_Y, 0,0, LCD_X, LCD_Y, color,color,color);
}
void LCD_ClearPartScreen(uint32_t posBuff, uint32_t BkpSizeX, uint32_t BkpSizeY, uint32_t color){
	LCD_ShapeWindow(LCD_Rectangle,posBuff,BkpSizeX,BkpSizeY, 0,0, BkpSizeX, BkpSizeY, color,color,color);
}
void LCD_LittleRoundRectangle(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	_StartDrawLine(posBuff,BkpSizeX,x,y);
	_FillBuff(2,BkpColor);	 _FillBuff(width-4,FrameColor);  _FillBuff(2, BkpColor);
	_NextDrawLine(BkpSizeX,width);
	_FillBuff(1,BkpColor);_FillBuff(1,FrameColor);	 _FillBuff(width-4, FillColor);  _FillBuff(1,FrameColor);_FillBuff(1, BkpColor);
	if(height>1)
	{
		_NextDrawLine(BkpSizeX,width);
		for (int j=0; j<height-4; j++)
		{
			if(width>1)
			{
				_FillBuff(1, FrameColor);
				_FillBuff(width-2, FillColor);
				_FillBuff(1, FrameColor);
				_NextDrawLine(BkpSizeX,width);
			}
			else
			{
				_FillBuff(width, FillColor);
				_NextDrawLine(BkpSizeX,width);
			}
		}
		_FillBuff(1,BkpColor);_FillBuff(1,FrameColor);	 _FillBuff(width-4, FillColor);  _FillBuff(1,FrameColor);_FillBuff(1, BkpColor);
		_NextDrawLine(BkpSizeX,width);
		_FillBuff(2, BkpColor);	 _FillBuff(width-4, FrameColor);  _FillBuff(2, BkpColor);
	}
}
void LCD_LittleRoundFrame(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	_StartDrawLine(posBuff,BkpSizeX,x,y);
	_FillBuff(2,BkpColor);	 _FillBuff(width-4,FrameColor);  _FillBuff(2, BkpColor);
	_NextDrawLine(BkpSizeX,width);
	_FillBuff(1,BkpColor);_FillBuff(1,FrameColor);	 k+=width-4; /* _FillBuff(width-4, FillColor); */  _FillBuff(1,FrameColor);_FillBuff(1, BkpColor);
	if(height>1)
	{
		_NextDrawLine(BkpSizeX,width);
		for (int j=0; j<height-4; j++)
		{
			if(width>1)
			{
				_FillBuff(1, FrameColor);
				k+=width-2;			/* _FillBuff(width-2, FillColor); */
				_FillBuff(1, FrameColor);
				_NextDrawLine(BkpSizeX,width);
			}
			else
			{
				k+=width;			/* _FillBuff(width, FillColor); */
				_NextDrawLine(BkpSizeX,width);
			}
		}
		_FillBuff(1,BkpColor);_FillBuff(1,FrameColor);	k+=width-4; /* _FillBuff(width-4, FillColor); */  _FillBuff(1,FrameColor);_FillBuff(1, BkpColor);
		_NextDrawLine(BkpSizeX,width);
		_FillBuff(2, BkpColor);	 _FillBuff(width-4, FrameColor);  _FillBuff(2, BkpColor);
	}
}
void LCD_Rectangle(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	_StartDrawLine(posBuff,BkpSizeX,x,y);
	_FillBuff(width, FrameColor);
	if(height>1)
	{
		_NextDrawLine(BkpSizeX,width);
		for (int j=0; j<height-2; j++)
		{
			_FillBuff(1, FrameColor);
			_FillBuff(width-2, FillColor);
			_FillBuff(1, FrameColor);
			_NextDrawLine(BkpSizeX,width);
		}
		_FillBuff(width, FrameColor);
	}
}

void LCD_Frame(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	_StartDrawLine(posBuff,BkpSizeX,x,y);
	_FillBuff(width, FrameColor);
	if(height>1)
	{
		_NextDrawLine(BkpSizeX,width);
		for (int j=0; j<height-2; j++)
		{
			_FillBuff(1, FrameColor);
			k+=width-2;
			_FillBuff(1, FrameColor);
			_NextDrawLine(BkpSizeX,width);
		}
		_FillBuff(width, FrameColor);
	}
}

void LCD_BoldRectangle(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	uint8_t thickness = FrameColor>>24;
	if((thickness==0)||(thickness==1)||(thickness==2)||(thickness==255))
		thickness=2;
	int thickness2 = 2*thickness;
	int fillHeight = height-thickness2;
	int fillWidth = width-thickness2;

	_StartDrawLine(posBuff,BkpSizeX,x,y);
	_FillBuff(width, FrameColor);
	for(int i=0;i<thickness-1;++i){
		_NextDrawLine(BkpSizeX,width);
		_FillBuff(width, FrameColor);
	}
	_NextDrawLine(BkpSizeX,width);
	if(fillHeight>0)
	{
		for (int j=0; j<fillHeight; j++)
		{	_FillBuff(thickness, FrameColor);
			_FillBuff(fillWidth, FillColor);
			_FillBuff(thickness, FrameColor);
			_NextDrawLine(BkpSizeX,width);
		}
	}
	_FillBuff(width, FrameColor);
	for(int i=0;i<thickness-1;++i){
		_NextDrawLine(BkpSizeX,width);
		_FillBuff(width, FrameColor);
	}
}
void LCD_BoldFrame(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	uint8_t thickness = FrameColor>>24;
	if((thickness==0)||(thickness==1)||(thickness==2)||(thickness==255))
		thickness=2;
	int thickness2 = 2*thickness;
	int fillHeight = height-thickness2;
	int fillWidth = width-thickness2;

	_StartDrawLine(posBuff,BkpSizeX,x,y);
	_FillBuff(width, FrameColor);
	for(int i=0;i<thickness-1;++i){
		_NextDrawLine(BkpSizeX,width);
		_FillBuff(width, FrameColor);
	}
	_NextDrawLine(BkpSizeX,width);
	if(fillHeight>0)
	{
		for (int j=0; j<fillHeight; j++)
		{	_FillBuff(thickness, FrameColor);
			k+=fillWidth;
			_FillBuff(thickness, FrameColor);
			_NextDrawLine(BkpSizeX,width);
		}
	}
	_FillBuff(width, FrameColor);
	for(int i=0;i<thickness-1;++i){
		_NextDrawLine(BkpSizeX,width);
		_FillBuff(width, FrameColor);
	}
}

void LCD_RoundFrame(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	LCD_DrawRoundRectangleFrame(0,posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FillColor,BkpColor);
}
void LCD_RoundRectangle(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	LCD_DrawRoundRectangleFrame(1,posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FillColor,BkpColor);
}

void LCD_BoldRoundRectangle(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	uint8_t thickness = FrameColor>>24;
	if((thickness==0)||(thickness==1)||(thickness==2)||(thickness==255))
		thickness=1;
	else
		thickness--;
	LCD_DrawRoundRectangleFrame(1,posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FrameColor,BkpColor);
	LCD_DrawRoundRectangleFrame(1,posBuff,BkpSizeX,BkpSizeY,x+thickness,y+thickness,width-2*thickness,height-2*thickness,FrameColor,FillColor,AA_OUT_OFF);
}

void LCD_BoldRoundFrame(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	int i,k1,k2;
	uint8_t thickness = FrameColor>>24;
	if((thickness==0)||(thickness==1)||(thickness==2)||(thickness==255))
		thickness=2;
	LCD_DrawRoundRectangleFrame(0,posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FrameColor,BkpColor);
	thickness-=2;
	for(i=0;i<thickness;++i){
		k1=1+i;
		k2=2*k1;
		LCD_DrawRoundRectangleFrame(0,posBuff,BkpSizeX,BkpSizeY,x+k1,y+k1,width-k2,height-k2,FrameColor,FrameColor,AA_OUT_OFF);
	}
	k1=1+i;
	k2=2*k1;
	LCD_DrawRoundRectangleFrame(0,posBuff,BkpSizeX,BkpSizeY,x+k1,y+k1,width-k2,height-k2,FrameColor,FillColor,AA_OUT_OFF);
}

/* Transparent version of Rectangle-Frame */
void LCD_RoundFrameTransp(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor, float transpCoeff){
	LCD_DrawRoundRectangleFrameTransp(0,posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FillColor,BkpColor,transpCoeff);
}
void LCD_RoundRectangleTransp(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor, float transpCoeff){
	LCD_DrawRoundRectangleFrameTransp(1,posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FillColor,BkpColor,transpCoeff);
}

void LCD_BoldRoundRectangleTransp(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor, float transpCoeff){
	uint8_t thickness = FrameColor>>24;
	if((thickness==0)||(thickness==1)||(thickness==2)||(thickness==255))
		thickness=1;
	else
		thickness--;
	LCD_BoldRoundFrameTransp(posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FrameColor,BkpColor,transpCoeff);
	LCD_DrawRoundRectangleFrameTransp(1,posBuff,BkpSizeX,BkpSizeY,x+thickness,y+thickness,width-2*thickness,height-2*thickness,FrameColor,FillColor,AA_OUT_OFF|AA_NO_FRAME,transpCoeff);
}

void LCD_BoldRoundFrameTransp(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor, float transpCoeff){
	int i,k1,k2;
	uint8_t thickness = FrameColor>>24;
	if((thickness==0)||(thickness==1)||(thickness==2)||(thickness==255))
		thickness=2;
	LCD_DrawRoundRectangleFrameTransp(0,posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FrameColor,BkpColor|AA_IN_OFF,transpCoeff);
	thickness-=2;
	for(i=0;i<thickness;++i){
		k1=1+i;
		k2=2*k1;
		LCD_DrawRoundRectangleFrameTransp(0,posBuff,BkpSizeX,BkpSizeY,x+k1,y+k1,width-k2,height-k2,FrameColor,FrameColor,AA_OUT_OFF|AA_IN_OFF,transpCoeff);
	}
	k1=1+i;
	k2=2*k1;
	LCD_DrawRoundRectangleFrameTransp(0,posBuff,BkpSizeX,BkpSizeY,x+k1,y+k1,width-k2,height-k2,FrameColor,FillColor,AA_OUT_OFF,transpCoeff);
}

void LCD_ResetNmbrLinePoints(void){	 		 nmbrLinePoints = 0;	}
u16  LCD_GetNmbrLinePoints	 (void){	return nmbrLinePoints; 		}

structPosition LCD_GetPosLinePoint(u16 nrLinePoint,u32 BkpSizeX){
	int kOffs = posLinePoints[nrLinePoint];
	structPosition temp;
	temp.y = (kOffs/BkpSizeX);
	temp.x = (kOffs-temp.y*BkpSizeX);
	return temp;
}
structPosition LCD_GetPosLinePointFromBuff(u32 *posLineBuff,u16 nrLinePoint,u32 BkpSizeX){
	int kOffs = *(posLineBuff + nrLinePoint);
	structPosition temp;
	temp.y = (kOffs/BkpSizeX);
	temp.x = (kOffs-temp.y*BkpSizeX);
	return temp;
}
u16 LCD_CopyPosLinePointsToBuff(u32 *posLineBuff){
	LOOP_FOR(i,nmbrLinePoints){ *(posLineBuff+i)=posLinePoints[i]; }
	return nmbrLinePoints;
}
void LCD_SetLinePointToBuffLcd(u16 nrLinePoint,u32 pointColor){
	pLcd[ posLinePoints[nrLinePoint] ] = pointColor;
}

structPosition DrawLine(uint32_t posBuff,uint16_t x0, uint16_t y0, float len, float degree, uint32_t lineColor,uint32_t BkpSizeX, float ratioAA1, float ratioAA2 ,uint32_t bk1Color, uint32_t bk2Color)
{	/* here we don`t read BkColor */
	#define LINES_BUFF_SIZE		(u16)len+6

	float degree_copy=degree;
	uint8_t linesBuff[LINES_BUFF_SIZE];
	int k_iteration=0, searchDirection=0;
	int nrPointsPerLine=0, iteration=1;
	int findNoPoint=0, rot;
	float param_y;
	float param_x;
	float decision;

	void _FFFFFFFFFF(void)
	{
		switch(rot)
		{  case 1: switch(searchDirection){ case 0: k_iteration=-1;	 		 break; case 1: k_iteration=-BkpSizeX-1; break; } break;
			case 2: switch(searchDirection){ case 0: k_iteration=-BkpSizeX; break; case 1: k_iteration=-BkpSizeX-1; break; } break;
			case 3: switch(searchDirection){ case 0: k_iteration=-BkpSizeX; break; case 1: k_iteration=-BkpSizeX+1; break; } break;
			case 4: switch(searchDirection){ case 0: k_iteration=1;     	 break; case 1: k_iteration=-BkpSizeX+1; break; } break;
			case 5: switch(searchDirection){ case 0: k_iteration=1;     	 break; case 1: k_iteration= BkpSizeX+1; break; } break;
			case 6: switch(searchDirection){ case 0: k_iteration=BkpSizeX;  break; case 1: k_iteration= BkpSizeX+1; break; } break;
			case 7: switch(searchDirection){ case 0: k_iteration=BkpSizeX;  break; case 1: k_iteration= BkpSizeX-1; break; } break;
			case 8: switch(searchDirection){ case 0: k_iteration=-1;   		 break; case 1: k_iteration= BkpSizeX-1; break; } break;
		}
	}

	if(degree_copy==0)
		degree_copy=360;

	_StartDrawLine(posBuff,BkpSizeX,x0,y0);
	rot=LCD_SearchLinePoints(1,posBuff,x0,y0,degree_copy,BkpSizeX);
	_FFFFFFFFFF();


	do
	{	pos_prev = pos;
		if(LCD_SearchLinePoints(0,posBuff,x0,y0,degree_copy,BkpSizeX)==1)
		{
			nrPointsPerLine++;
		   if(findNoPoint)
		   {
		   	findNoPoint=0;
		   	searchDirection=1-searchDirection;
		   	_FFFFFFFFFF();
		   }
		}
		else
		{
		   k-=k_iteration;
		   searchDirection=1-searchDirection;
		   if(nrPointsPerLine)
		   {
		   	if(iteration<LINES_BUFF_SIZE-2)
		   		linesBuff[iteration++]=nrPointsPerLine;
		   	else break;
		   }
		   nrPointsPerLine=0;
			findNoPoint=1;
			_FFFFFFFFFF();
		}
		k+=k_iteration;
	   pos = _GetPosXY(posBuff,BkpSizeX);

		param_y = pow((float)(y0-pos.y),2);
		param_x = pow((float)(x0-pos.x),2);
		decision = pow(len+1,2);

	}while((param_x+param_y) <= decision);		/* When precision of float 'degree' is greater than 0.1 then 'decision' may have to big decimal value for float for this precision and never return from the loop while() - endless loop */

	if(nrPointsPerLine)
		linesBuff[iteration++]=nrPointsPerLine;
	linesBuff[0]=iteration-1;

	_StartDrawLine(posBuff,BkpSizeX,x0,y0);

	if(correctLine_AA==0)
	{
		switch(rot)
		{
			case 1:  _DrawArrayBuffLeftUp_AA   (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,0,linesBuff); break;
			case 2:  _DrawArrayBuffLeftUp_AA   (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,1,linesBuff); break;
			case 3:  _DrawArrayBuffRightUp_AA  (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,1,linesBuff); break;
			case 4:  _DrawArrayBuffRightUp_AA  (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,0,linesBuff); break;
			case 5:  _DrawArrayBuffRightDown_AA(lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,0,linesBuff); break;
			case 6:  _DrawArrayBuffRightDown_AA(lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,1,linesBuff); break;
			case 7:  _DrawArrayBuffLeftDown_AA (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,1,linesBuff); break;
			case 8:  _DrawArrayBuffLeftDown_AA (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,0,linesBuff); break;
		}
	}
	else
	{
		int  k_p=k;
		int linePointsStructSize;
		int nmbPxl,repeatPxl,repeatPxl_next;
		int direction=0, upDown;

		int _GetNmbPxls(void)
		{
			int j,it=0,count1=0;
			char linesBuff_prev=0;

			linesBuff_prev=linesBuff[1];

			for(j=0;j<linesBuff[0];++j){
				if(linesBuff[1+j]==linesBuff_prev) count1++;
				else{
					it++;
					linesBuff_prev=linesBuff[1+j];
					count1=1;
				}
			}
			if(count1){
				it++;
			}
			return it;
		}

		typedef struct
		{	uint16_t nmbPxl;
			uint8_t nmbPxlTheSame;
		}Struct_LinePoints;
		Struct_LinePoints linePoints[_GetNmbPxls()];

		int Pxl_V(uint8_t nmbPxl_V, uint8_t repeat, uint8_t stepAA, int offs, int direction, int upDown)
		{
			uint32_t bk1Color_, bk2Color_;
			if((degree<=135 && degree>=135-11)||
				(degree<315  && degree>=315-11)){	bk1Color_=bk2Color; bk2Color_=bk1Color;	}
			else{												bk1Color_=bk1Color; bk2Color_=bk2Color;	}

			Set_AACoeff(stepAA,lineColor,bk1Color_,0.0);
			Set_AACoeff2(stepAA,lineColor,bk2Color_,0.0);

			if(repeat>1)
			{
				for(int i=0;i<repeat;++i){
					if(pLcd[k_p-			upDown*BkpSizeX]!=lineColor) pLcd[k_p-			upDown*BkpSizeX]	=	buff_AA[offs+1+i];
					if(pLcd[k_p+upDown*nmbPxl_V*BkpSizeX]!=lineColor) pLcd[k_p+upDown*nmbPxl_V*BkpSizeX]=	buff2_AA[offs+repeat-i];
					k_p=k_p+upDown*nmbPxl_V*BkpSizeX+direction;
				}
			}
			else{
				if(pLcd[k_p-			upDown*BkpSizeX]!=lineColor) pLcd[k_p-			upDown*BkpSizeX]	=	buff_AA[offs];
				if(pLcd[k_p+upDown*nmbPxl_V*BkpSizeX]!=lineColor) pLcd[k_p+upDown*nmbPxl_V*BkpSizeX]=	buff2_AA[offs];
				k_p=k_p+upDown*nmbPxl_V*BkpSizeX+direction;
			}
			return 0;
		}

		int Pxl_H(uint8_t nmbPxl_H, uint8_t repeat, uint8_t stepAA, int offs, int direction, int upDown)
		{
			uint32_t bk1Color_, bk2Color_;
			if((degree>=45-11  && degree<=45) ||
			   (degree>=225-11 && degree<=225)){	bk1Color_=bk2Color; bk2Color_=bk1Color;	}
			else{												bk1Color_=bk1Color; bk2Color_=bk2Color;	}

			Set_AACoeff(stepAA,lineColor,bk1Color_,0.0);
			Set_AACoeff2(stepAA,lineColor,bk2Color_,0.0);

			if(repeat>1)
			{
				for(int i=0;i<repeat;++i){
					if(pLcd[k_p-direction]!=lineColor)			  pLcd[k_p-direction]			=	buff_AA[offs+1+i];
					if(pLcd[k_p+direction*nmbPxl_H]!=lineColor) pLcd[k_p+direction*nmbPxl_H]=	buff2_AA[offs+repeat-i];
					k_p=k_p+upDown*BkpSizeX+direction*nmbPxl_H;
				}
			}
			else{
				if(pLcd[k_p-direction]!=lineColor) 			  pLcd[k_p-direction]			=	buff_AA[offs];						/* problem with -0fast optimize !!! */
				if(pLcd[k_p+direction*nmbPxl_H]!=lineColor) pLcd[k_p+direction*nmbPxl_H]=	buff2_AA[offs];
				k_p=k_p+upDown*BkpSizeX+direction*nmbPxl_H;
			}
			return 0;
		}

		void _GetNmbPxlsAndLoadToBuff(void)
		{
			int j,it=0,count1=0;
			char linesBuff_prev=0;

			linesBuff_prev=linesBuff[1];

			for(j=1;j<linesBuff[0]+1;++j){
				if(linesBuff[j]==linesBuff_prev) count1++;
				else{
					linePoints[it].nmbPxl=linesBuff_prev;
					linePoints[it++].nmbPxlTheSame=count1;
					linesBuff_prev=linesBuff[j];
					count1=1;
				}
			}
			if(count1){
				linePoints[it].nmbPxl=linesBuff_prev;
				linePoints[it++].nmbPxlTheSame=count1;
			}
			linePointsStructSize=it;
		}

		switch(rot)
		{
			case 1:
				if(degree>=45-11 && degree<=45)
					_DrawArrayBuffLeftUp_AA   (lineColor, bk1Color, bk2Color, 1.0, 1.0, BkpSizeX,0,linesBuff);
				else
					_DrawArrayBuffLeftUp_AA   (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,0,linesBuff);
				break;
			case 2:
				if(degree>45 && degree<=45+11)
					_DrawArrayBuffLeftUp_AA   (lineColor, bk1Color, bk2Color, 1.0, 1.0, BkpSizeX,1,linesBuff);
				else
					_DrawArrayBuffLeftUp_AA   (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,1,linesBuff);
			break;

			case 3:
				if(degree<=135 && degree>=135-11)
					_DrawArrayBuffRightUp_AA  (lineColor, bk1Color, bk2Color, 1.0, 1.0, BkpSizeX,1,linesBuff);
				else
					_DrawArrayBuffRightUp_AA  (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,1,linesBuff);
				break;
			case 4:
				if(degree>135 && degree<=135+11)
					_DrawArrayBuffRightUp_AA  (lineColor, bk1Color, bk2Color, 1.0, 1.0, BkpSizeX,0,linesBuff);
				else
					_DrawArrayBuffRightUp_AA  (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,0,linesBuff);
			break;

			case 5:
				if(degree>=225-11 && degree<=225)
					_DrawArrayBuffRightDown_AA(lineColor, bk1Color, bk2Color, 1.0, 1.0, BkpSizeX,0,linesBuff);
				else
					_DrawArrayBuffRightDown_AA(lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,0,linesBuff);
				break;
			case 6:
				if(degree>225 && degree<=225+11)
					_DrawArrayBuffRightDown_AA(lineColor, bk1Color, bk2Color, 1.0, 1.0, BkpSizeX,1,linesBuff);
				else
					_DrawArrayBuffRightDown_AA(lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,1,linesBuff);
				break;

			case 7:
				if(degree<315 && degree>=315-11)
					_DrawArrayBuffLeftDown_AA (lineColor, bk1Color, bk2Color, 1.0, 1.0, BkpSizeX,1,linesBuff);
				else
					_DrawArrayBuffLeftDown_AA (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,1,linesBuff);
				break;
			case 8:
				if(degree>=315 && degree<=315+11)
					_DrawArrayBuffLeftDown_AA (lineColor, bk1Color, bk2Color, 1.0, 1.0, BkpSizeX,0,linesBuff);
				else
					_DrawArrayBuffLeftDown_AA (lineColor, bk1Color, bk2Color, ratioAA1, ratioAA2, BkpSizeX,0,linesBuff);
				break;
		}


		if((degree>=225-11 && degree<=225)||
		   (degree>=315    && degree<=315+11)||
		   (degree>135     && degree<=135+11)||
		   (degree>=45-11  && degree<=45))
		{
			_GetNmbPxlsAndLoadToBuff();

			if((degree>=225-11 && degree<=225)||
				(degree>135     && degree<=135+11)) direction=1;
			else                                   direction=-1;

			if((degree>=225-11 && degree<=225)||
				(degree>=315    && degree<=315+11)) upDown=1;
			else                                   upDown=-1;

			for(int j=0; j<linePointsStructSize; ++j)
			{
				nmbPxl    = linePoints[j].nmbPxl;
				repeatPxl = linePoints[j].nmbPxlTheSame;

				if(repeatPxl>1)
					Pxl_H(nmbPxl,repeatPxl,repeatPxl+2,1,direction,upDown);
				else{
					if(nmbPxl==2)
						Pxl_H(nmbPxl,repeatPxl,4,4,direction,upDown);
					else
						Pxl_H(nmbPxl,repeatPxl,3,2,direction,upDown);
				}
			}
		}
		else if((degree>225 && degree<=225+11)||
			    (degree<315  && degree>=315-11)||
				 (degree<=135 && degree>=135-11)||
				 (degree>45   && degree<=45+11))
		{
			_GetNmbPxlsAndLoadToBuff();

			if((degree>225  && degree<=225+11)||
				(degree<=135 && degree>=135-11)) direction=1;
			else                               direction=-1;

			if((degree>225 && degree<=225+11)||
				(degree<315 && degree>=315-11)) upDown=1;
			else                               upDown=-1;

			if((degree>=225+6 && degree<=225+11)||
				(degree<=315-6 && degree>=315-11)||
				(degree<=135-6 && degree>=135-11)||
				(degree>=45+6  && degree<=45+11))
			{
				for(int j=0; j<linePointsStructSize; ++j)
				{
					nmbPxl    = linePoints[j].nmbPxl;
					repeatPxl = linePoints[j].nmbPxlTheSame;

					if(repeatPxl>1)
					{
						if(repeatPxl==2)
							Pxl_V(nmbPxl,repeatPxl,4,1,direction,upDown);
						else if(repeatPxl>=3)
							Pxl_V(nmbPxl,repeatPxl,repeatPxl+1,0,direction,upDown);
					}
					else
					{
						if(nmbPxl==1)
							Pxl_V(nmbPxl,repeatPxl,3,2,direction,upDown);
						else
							Pxl_V(nmbPxl,repeatPxl,3,3,direction,upDown);
					}
				}
			}
			else
			{
				for(int j=0; j<linePointsStructSize; ++j)
				{
					nmbPxl = linePoints[j].nmbPxl;
					repeatPxl = linePoints[j].nmbPxlTheSame;
					if(j+1<linePointsStructSize)
						repeatPxl_next = linePoints[j+1].nmbPxlTheSame;

					if(repeatPxl>1){
						if(Pxl_V(nmbPxl,repeatPxl,repeatPxl,0,direction,upDown)) break;
					}
					else{
						if(repeatPxl_next<3){
							if(repeatPxl==1 && j==linePointsStructSize-1){
								if(Pxl_V(nmbPxl,repeatPxl,2,2,direction,upDown)) break;
							}
							else{
								if(Pxl_V(nmbPxl,repeatPxl,5,5,direction,upDown)) break;
							}
						}
						else{
							if(Pxl_V(nmbPxl,repeatPxl,repeatPxl_next,repeatPxl_next,direction,upDown)) break;
						}
					}
				}
			}
		}
	}
	#undef LINES_BUFF_SIZE
	return pos_prev;
}
void LCD_Line(uint32_t posBuff, u16 x0,u16 y0, u16 x1,u16 y1, uint32_t lineColor,uint32_t BkpSizeX, float ratioAA1, float ratioAA2 ,uint32_t bk1Color, uint32_t bk2Color){		/* here we don`t read BkColor */
	nmbrLinePoints = 0;
	DrawLine(posBuff, x0, y0, GetNewfloatValue(LCD_GetLenFrom2Points(x1,y1, x0,y0),1),  GetNewfloatValue(LCD_GetDegFrom2Points(x1,y1, x0,y0),1), lineColor,BkpSizeX, ratioAA1, ratioAA2 ,bk1Color, bk2Color);
}
void OPTIMIZE_FAST LCD_LinePoints(uint32_t posBuff, u16 x0,u16 y0, u16 x1,u16 y1, uint32_t lineColor,uint32_t BkpSizeX){
	nmbrLinePoints = 0;
	DrawLine_linePoints(posBuff, x0, y0, GetNewfloatValue(LCD_GetLenFrom2Points(x1,y1, x0,y0),1),  GetNewfloatValue(LCD_GetDegFrom2Points(x1,y1, x0,y0),1), lineColor,BkpSizeX);
}

SHAPE_PARAMS LCD_KeyBackspace(uint32_t posBuff,uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor)
{
	SHAPE_PARAMS params = {.bkSize.w=BkpSizeX, .bkSize.h=BkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=width, .size[0].h=height, .color[0].frame=FrameColor, .color[0].fill=FillColor, .color[0].bk=BkpColor};

	if(ToStructAndReturn == posBuff)
		return params;

	width = MASK(width,FFFF);
	height = MASK(height,FFFF);

	int widthSign=width/2;
	int heightSign=height/2;
	int sizeSignX=height/4;

	if((heightSign%2)!=0) heightSign++;

	 LCD_Rectangle(posBuff,BkpSizeX,BkpSizeY,x,y,width,height,FrameColor,FillColor,BkpColor);

	_StartDrawLine(posBuff,BkpSizeX,x+(width-widthSign)/2,y+height/2);
	_DrawRightUp(heightSign/2,heightSign/2, FrameColor,BkpSizeX);
	_DrawRight(widthSign-heightSign/2,FrameColor);
	_DrawDown(heightSign,FrameColor,BkpSizeX);
	_DrawLeft(widthSign-heightSign/2,FrameColor);
	_DrawLeftUp(heightSign/2,heightSign/2, FrameColor,BkpSizeX);

	_StartDrawLine(posBuff,BkpSizeX,x+(width-widthSign)/2+1,y+height/2);
	_CopyDrawPos();
	_DrawRightUp(heightSign/2,heightSign/2, FrameColor,BkpSizeX);
	_SetCopyDrawPos();
	_DrawRightDown(heightSign/2,heightSign/2, FrameColor,BkpSizeX);

	_StartDrawLine(posBuff,BkpSizeX,x+(width-sizeSignX)/2+sizeSignX/2,y+(height-sizeSignX)/2);
	_DrawRightDown(heightSign/2,heightSign/2, FrameColor,BkpSizeX);
	_StartDrawLine(posBuff,BkpSizeX,x+(width-sizeSignX)/2+sizeSignX/2-1,y+(height-sizeSignX)/2);
	_DrawRightDown(heightSign/2,heightSign/2, FrameColor,BkpSizeX);

	_StartDrawLine(posBuff,BkpSizeX,x+(width-sizeSignX)/2+sizeSignX/2+sizeSignX,y+(height-sizeSignX)/2);
	_DrawLeftDown(heightSign/2,heightSign/2, FrameColor,BkpSizeX);
	_StartDrawLine(posBuff,BkpSizeX,x+(width-sizeSignX)/2+sizeSignX/2+sizeSignX-1,y+(height-sizeSignX)/2);
	_DrawLeftDown(heightSign/2,heightSign/2, FrameColor,BkpSizeX);
	return params;
}
SHAPE_PARAMS LCDSHAPE_KeyBackspace(uint32_t posBuff, SHAPE_PARAMS param){
	return LCD_KeyBackspace(posBuff,param.bkSize.w,param.bkSize.h, param.pos[0].x,param.pos[0].y, param.size[0].w,param.size[0].h, param.color[0].frame, param.color[0].fill, param.color[0].bk);
}

void LCD_SignStar(uint32_t posBuff,uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor)
{																												/*	'width/height' or 'height/width'  must be divisible */
	#define PARAM1			WHITE,MYGRAY
	#define PARAM2		FrameColor,BkpSizeX

	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(20,PARAM1,0.39); _DrawRightDown_AA(100,5,PARAM2);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(5,PARAM1, 0.45);_DrawRightDown_AA(100,20,PARAM2);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(2,PARAM1, 0.39);_DrawRightDown_AA(100,50,PARAM2);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(1,PARAM1, 0.39);_DrawRightDown_AA(100,100,PARAM2);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(2,WHITE,MYGRAY, 0.39);_DrawRightDown_AA(50,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(5,WHITE,MYGRAY, 0.45); _DrawRightDown_AA(20,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(20,WHITE,MYGRAY,0.39);_DrawRightDown_AA(5,100,FrameColor,BkpSizeX);

	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(10,WHITE,MYGRAY,0.39); _DrawRightUp_AA(100,10,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(4,WHITE,MYGRAY, 0.47);_DrawRightUp_AA(100,25,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(2,WHITE,MYGRAY, 0.39); _DrawRightUp_AA(100,50,FrameColor,BkpSizeX);

	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(1,WHITE,MYGRAY, 0.39);_DrawRightUp_AA(100,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(2,WHITE,MYGRAY, 0.39);_DrawRightUp_AA(50,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(4,WHITE,MYGRAY, 0.47); _DrawRightUp_AA(25,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(10,WHITE,MYGRAY,0.39); _DrawRightUp_AA(10,100,FrameColor,BkpSizeX);

	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(10,WHITE,MYGRAY,0.39); _DrawLeftUp_AA(100,10,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(4,WHITE,MYGRAY, 0.47);_DrawLeftUp_AA(100,25,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(2,WHITE,MYGRAY, 0.39);_DrawLeftUp_AA(100,50,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(1,WHITE,MYGRAY, 0.39); _DrawLeftUp_AA(100,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(2,WHITE,MYGRAY, 0.39); _DrawLeftUp_AA(50,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(4,WHITE,MYGRAY, 0.47);_DrawLeftUp_AA(25,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(10,WHITE,MYGRAY,0.39);_DrawLeftUp_AA(10,100,FrameColor,BkpSizeX);

	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(10,WHITE,MYGRAY,0.39);_DrawLeftDown_AA(100,10,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(4,WHITE,MYGRAY, 0.47);_DrawLeftDown_AA(100,25,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(2,WHITE,MYGRAY, 0.39); _DrawLeftDown_AA(100,50,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(1,WHITE,MYGRAY, 0.39); _DrawLeftDown_AA(100,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(2,WHITE,MYGRAY, 0.39);_DrawLeftDown_AA(50,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(4,WHITE,MYGRAY, 0.47);_DrawLeftDown_AA(25,100,FrameColor,BkpSizeX);
	_StartDrawLine(0,BkpSizeX,x,y);  Set_AACoeff_Draw(10,WHITE,MYGRAY,0.39);_DrawLeftDown_AA(10,100,FrameColor,BkpSizeX);

	#undef PARAM1
	#undef PARA2
}

void LCD_SimpleTriangle(uint32_t posBuff,uint32_t BkpSizeX, uint32_t x,uint32_t y, uint32_t halfBaseWidth,uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor, DIRECTIONS direct)
{																										/*	'halfBaseWidth/height' or 'height/halfBaseWidth'  must be divisible */
	int i;
	int coeff = halfBaseWidth > height ? halfBaseWidth/height : height/halfBaseWidth;

	if(FillColor != 0) FrameColor=FillColor;
	Set_AACoeff_Draw(coeff,FrameColor,BkpColor, 0.1);
	_StartDrawLine(0,BkpSizeX,x,y);

	switch((int)direct)
	{
		case Right:
			_DrawRightDown_AA(height,halfBaseWidth,FrameColor,BkpSizeX);
			_DrawLeftDown_AA(height,halfBaseWidth,FrameColor,BkpSizeX);
			if(FillColor != 0)
			{
				if(halfBaseWidth > height)
				{
					for(i=coeff; i<halfBaseWidth; ++i){
						_DrawRight(i/coeff+1,FillColor); _IncDrawPos(-(i/coeff+1)-BkpSizeX);
					}
					for(int j=i; j>0; --j){
						_DrawRight(j/coeff+1,FillColor); _IncDrawPos(-(j/coeff+1)-BkpSizeX);
					}
				}
				else
				{
					for(i=0; i<halfBaseWidth; ++i){
						_DrawRight(coeff*i,FillColor); _IncDrawPos(-(coeff*i)-BkpSizeX);
					}
					for(int j=i; j>0; --j){
						_DrawRight(coeff*j,FillColor); _IncDrawPos(-(coeff*j)-BkpSizeX);
					}
				}
			}
			else _DrawUp(2*halfBaseWidth,FrameColor,BkpSizeX);
			break;

		case Left:
			_DrawLeftDown_AA(height,halfBaseWidth,FrameColor,BkpSizeX);
			_DrawRightDown_AA(height,halfBaseWidth,FrameColor,BkpSizeX);
			if(FillColor != 0)
			{
				if(halfBaseWidth > height)
				{
					for(i=coeff; i<halfBaseWidth; ++i){
						_DrawLeft(i/coeff+1,FillColor); _IncDrawPos((i/coeff+1)-BkpSizeX);
					}
					for(int j=i; j>0; --j){
						_DrawLeft(j/coeff+1,FillColor); _IncDrawPos((j/coeff+1)-BkpSizeX);
					}
				}
				else
				{
					for(i=0; i<halfBaseWidth; ++i){
						_DrawLeft(coeff*i,FillColor); _IncDrawPos((coeff*i)-BkpSizeX);
					}
					for(int j=i; j>0; --j){
						_DrawLeft(coeff*j,FillColor); _IncDrawPos((coeff*j)-BkpSizeX);
					}
				}
			}
			else _DrawUp(2*halfBaseWidth,FrameColor,BkpSizeX);
			break;

		case Up:
			_DrawRightUp_AA(halfBaseWidth,height,FrameColor,BkpSizeX);
			_DrawRightDown_AA(halfBaseWidth,height,FrameColor,BkpSizeX);
			if(FillColor != 0)
			{
				if(halfBaseWidth > height)
				{
					for(i=coeff; i<halfBaseWidth; ++i){
						_DrawUp(i/coeff+1,FillColor,BkpSizeX); _IncDrawPos((i/coeff+1)*BkpSizeX-1);
					}
					for(int j=i; j>0; --j){
						_DrawUp(j/coeff+1,FillColor,BkpSizeX); _IncDrawPos((j/coeff+1)*BkpSizeX-1);
					}
				}
				else
				{
					for(i=0; i<halfBaseWidth; ++i){
						_DrawUp(coeff*i,FillColor,BkpSizeX); _IncDrawPos((coeff*i)*BkpSizeX-1);
					}
					for(int j=i; j>0; --j){
						_DrawUp(coeff*j,FillColor,BkpSizeX); _IncDrawPos((coeff*j)*BkpSizeX-1);
					}
				}
			}
			else _DrawLeft(2*halfBaseWidth,FrameColor);
			break;

		case Down:
			_DrawRightDown_AA(halfBaseWidth,height,FrameColor,BkpSizeX);
			_DrawRightUp_AA(halfBaseWidth,height,FrameColor,BkpSizeX);
			if(FillColor != 0)
			{
				if(halfBaseWidth > height)
				{
					for(i=coeff; i<halfBaseWidth; ++i){
						_DrawDown(i/coeff+1,FillColor,BkpSizeX); _IncDrawPos(-(i/coeff+1)*BkpSizeX-1);
					}
					for(int j=i; j>0; --j){
						_DrawDown(j/coeff+1,FillColor,BkpSizeX); _IncDrawPos(-(j/coeff+1)*BkpSizeX-1);
					}
				}
				else
				{
					for(i=0; i<halfBaseWidth; ++i){
						_DrawDown(coeff*i,FillColor,BkpSizeX); _IncDrawPos(-(coeff*i)*BkpSizeX-1);
					}
					for(int j=i; j>0; --j){
						_DrawDown(coeff*j,FillColor,BkpSizeX); _IncDrawPos(-(coeff*j)*BkpSizeX-1);
					}
				}
			}
			else _DrawLeft(2*halfBaseWidth,FrameColor);
			break;
	}
}

int ChangeElemSliderColor(SLIDER_PARAMS sel, uint32_t color){
	return ((color&0xFFFFFF) | sel<<24);
}
uint32_t ChangeElemSliderSize(uint16_t width, uint8_t coeffHeightTriang, uint8_t coeffLineBold, uint8_t coeffHeightPtr, uint8_t coeffWidthPtr){
	return ((coeffHeightTriang<<28) | (coeffLineBold<<24) | (coeffHeightPtr<<20) | (coeffWidthPtr<<16) | width);		/*	standard set: (1,6,2,1) */
}
uint32_t SetSpaceTriangLineSlider(uint16_t height, uint16_t param){
	return ((height&0xFFFF) | param<<16);
}
uint32_t SetValType(uint16_t slidPos, uint16_t param){
	return ((slidPos&0xFFFF) | param<<16);
}

SHAPE_PARAMS LCD_SimpleSliderH(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t widthParam, uint32_t heightParam, uint32_t ElementsColor, uint32_t LineColor, uint32_t LineSelColor, uint32_t BkpColor, uint32_t slidPos, int elemSel)
{
	#define TRIANG_HEIGHT	(height / heightTriang_coeff)
	#define LINE_BOLD			(height / lineBold_coeff)
	#define PTR_HEIGHT		(height / heightPtr_coeff)
	#define PTR_WIDTH			(PTR_HEIGHT / widthPtr_coeff)

	SHAPE_PARAMS elements;
	int width  = SHIFT_RIGHT(widthParam,0,FFFF);
	int height = SHIFT_RIGHT(heightParam,0,FFFF);
	int spaceTriangLine = CONDITION(  DelTriang==(int8_t)SHIFT_RIGHT(heightParam,16,FF),  0,  SHIFT_RIGHT(heightParam,16,FF)  );

	int heightTriang_coeff = CONDITION(SHIFT_RIGHT(widthParam,28,F), SHIFT_RIGHT(widthParam,28,F), 2);
	int lineBold_coeff 	  = CONDITION(SHIFT_RIGHT(widthParam,24,F), SHIFT_RIGHT(widthParam,24,F), 6);
	int heightPtr_coeff 	  = CONDITION(SHIFT_RIGHT(widthParam,20,F), SHIFT_RIGHT(widthParam,20,F), 4);
	int widthPtr_coeff 	  = CONDITION(SHIFT_RIGHT(widthParam,16,F), SHIFT_RIGHT(widthParam,16,F), 2);

	int triang_Height = CONDITION(DelTriang==(int8_t)SHIFT_RIGHT(heightParam,16,FF), 0, TRIANG_HEIGHT);
	int triang_Width 	= CONDITION(DelTriang==(int8_t)SHIFT_RIGHT(heightParam,16,FF), 0, height);
	int triangRight_posX = x + width - triang_Height;

	int ptr_height = PTR_HEIGHT;
	int ptr_width 	= PTR_WIDTH;

	int line_Bold 	= CONDITION(LINE_BOLD,LINE_BOLD,1);
	int line_width = width-2*triang_Height - 2*spaceTriangLine;
	int lineSel_posX 	= x + triang_Height + spaceTriangLine;

	int width_sel 	= CONDITION(	Percent==SHIFT_RIGHT(slidPos,16,FF), (MASK(slidPos,FFFF)*line_width)/100, SET_IN_RANGE(MASK(slidPos,FFFF)-lineSel_posX,0,triangRight_posX-spaceTriangLine)	);

	int lineSel_width 	= width_sel 					- ptr_width/2;
	int lineUnSel_posX 	= lineSel_posX + width_sel + ptr_width/2;
	int lineUnSel_width 	= line_width 	- width_sel - ptr_width/2;
	int ptr_posX 			= lineSel_posX + width_sel - ptr_width/2;

	ptr_posX  		 = SET_IN_RANGE( ptr_posX, 		 lineSel_posX, 				triangRight_posX-ptr_width-spaceTriangLine-2 );
	lineSel_width 	 = SET_IN_RANGE( lineSel_width,   0, 				 			  	line_width-ptr_width+1 							  	);
	lineUnSel_posX  =	SET_IN_RANGE( lineUnSel_posX,  lineSel_posX+ptr_width-1, triangRight_posX 									  	);
	lineUnSel_width = SET_IN_RANGE( lineUnSel_width, 0, 							  	line_width-ptr_width 								);

	uint32_t elemColor[NMB_SLIDER_ELEMENTS] = { ElementsColor, ElementsColor, ElementsColor };

	switch(elemSel>>24){
		case LeftSel:	elemColor[0] = (elemSel&0xFFFFFF)|0xFF000000;  break;
		case PtrSel:	elemColor[1] = (elemSel&0xFFFFFF)|0xFF000000;  break;
		case RightSel:	elemColor[2] = (elemSel&0xFFFFFF)|0xFF000000;  break;
	}

	elements.pos[0].x = x;
	elements.pos[0].y = y;
	elements.size[0].w = triang_Height;
	elements.size[0].h = triang_Width;

	elements.pos[1].x = lineSel_posX;
	elements.pos[1].y = y;
	elements.size[1].w = line_width;
	elements.size[1].h = height;

	elements.pos[2].x = triangRight_posX;
	elements.pos[2].y = y;
	elements.size[2].w = triang_Height;
	elements.size[2].h = triang_Width;

	elements.param[0] = width_sel;
	elements.param[1] = line_width;
	elements.param[2] = ptr_width/2;

	if(DelTriang!=(int8_t)SHIFT_RIGHT(heightParam,16,FF))
		LCD_SimpleTriangle	(posBuff,BkpSizeX, 				lineSel_posX-spaceTriangLine, MIDDLE(y,height,triang_Width), 	triang_Width/2,    triang_Height, 	elemColor[0], elemColor[0], BkpColor, 	Left);
	LCD_LineH					(			BkpSizeX, 				lineSel_posX+1,					MIDDLE(y,height,line_Bold), 		lineSel_width, 							LineSelColor, 									line_Bold );
	LCD_LittleRoundRectangle(posBuff,BkpSizeX, BkpSizeY, 	ptr_posX+1, 						MIDDLE(y,height,ptr_height)+1,	ptr_width, 		    ptr_height, 		elemColor[1], elemColor[1], BkpColor);
	if(0<lineUnSel_width-2)
		LCD_LineH				(			BkpSizeX, 				ptr_posX+1+ptr_width,			MIDDLE(y,height,line_Bold), 		lineUnSel_width-2, 						LineColor, 										line_Bold );
	if(DelTriang!=(int8_t)SHIFT_RIGHT(heightParam,16,FF))
		LCD_SimpleTriangle	(posBuff,BkpSizeX, 				triangRight_posX-1, 				MIDDLE(y,height,triang_Width), 	triang_Width/2,  	 triang_Height, 	elemColor[2], elemColor[2], BkpColor, 	Right);

	return elements;

	#undef TRIANG_HEIGHT
	#undef LINE_BOLD
	#undef PTR_HEIGHT
	#undef PTR_WIDTH
}

SHAPE_PARAMS LCD_SimpleSliderV(uint32_t posBuff, uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x,uint32_t y, uint32_t widthParam, uint32_t heightParam, uint32_t ElementsColor, uint32_t LineColor, uint32_t LineSelColor, uint32_t BkpColor, uint32_t slidPos, int elemSel)
{
	#define TRIANG_HEIGHT	(height / heightTriang_coeff)
	#define LINE_BOLD			(height / lineBold_coeff)
	#define PTR_HEIGHT		(height / heightPtr_coeff)
	#define PTR_WIDTH			(PTR_HEIGHT / widthPtr_coeff)

	SHAPE_PARAMS elements;
	int width  = SHIFT_RIGHT(widthParam,0,FFFF);
	int height = SHIFT_RIGHT(heightParam,0,FFFF);
	int spaceTriangLine = CONDITION(  DelTriang==(int8_t)SHIFT_RIGHT(heightParam,16,FF),  0,  SHIFT_RIGHT(heightParam,16,FF)  );

	int heightTriang_coeff = CONDITION(SHIFT_RIGHT(widthParam,28,F), SHIFT_RIGHT(widthParam,28,F), 2);
	int lineBold_coeff 	  = CONDITION(SHIFT_RIGHT(widthParam,24,F), SHIFT_RIGHT(widthParam,24,F), 6);
	int heightPtr_coeff 	  = CONDITION(SHIFT_RIGHT(widthParam,20,F), SHIFT_RIGHT(widthParam,20,F), 4);
	int widthPtr_coeff 	  = CONDITION(SHIFT_RIGHT(widthParam,16,F), SHIFT_RIGHT(widthParam,16,F), 2);

	int triang_Height = CONDITION(DelTriang==(int8_t)SHIFT_RIGHT(heightParam,16,FF), 0, TRIANG_HEIGHT);
	int triang_Width 	= CONDITION(DelTriang==(int8_t)SHIFT_RIGHT(heightParam,16,FF), 0, height);
	int triangRight_posY = y + width - triang_Height;

	int ptr_height = PTR_HEIGHT;
	int ptr_width 	= PTR_WIDTH;

	int line_Bold 	= CONDITION(LINE_BOLD,LINE_BOLD,1);
	int line_width = width-2*triang_Height - 2*spaceTriangLine;
	int lineSel_posY 	= y + triang_Height + spaceTriangLine;

	int width_sel 	= CONDITION(	Percent==SHIFT_RIGHT(slidPos,16,FF), (MASK(slidPos,FFFF)*line_width)/100, SET_IN_RANGE(MASK(slidPos,FFFF)-lineSel_posY,0,triangRight_posY-spaceTriangLine)	);

	int lineSel_width 	= width_sel 					- ptr_width/2;
	int lineUnSel_posY 	= lineSel_posY + width_sel + ptr_width/2;
	int lineUnSel_width 	= line_width 	- width_sel - ptr_width/2;
	int ptr_posY 			= lineSel_posY + width_sel - ptr_width/2;

	ptr_posY  		 = SET_IN_RANGE( ptr_posY, 		 lineSel_posY, 				triangRight_posY-ptr_width-spaceTriangLine-2 );
	lineSel_width 	 = SET_IN_RANGE( lineSel_width,   0, 				 			  	line_width-ptr_width+1 							  	);
	lineUnSel_posY  =	SET_IN_RANGE( lineUnSel_posY,  lineSel_posY+ptr_width-1, triangRight_posY 									  	);
	lineUnSel_width = SET_IN_RANGE( lineUnSel_width, 0, 							  	line_width-ptr_width 								);

	uint32_t elemColor[NMB_SLIDER_ELEMENTS] = { ElementsColor, ElementsColor, ElementsColor };

	switch(elemSel>>24){
		case LeftSel:	elemColor[0] = (elemSel&0xFFFFFF)|0xFF000000;  break;
		case PtrSel:	elemColor[1] = (elemSel&0xFFFFFF)|0xFF000000;  break;
		case RightSel:	elemColor[2] = (elemSel&0xFFFFFF)|0xFF000000;  break;
	}

	elements.pos[0].x = x;
	elements.pos[0].y = y;
	elements.size[0].w = triang_Height;
	elements.size[0].h = triang_Width;

	elements.pos[1].x = x;
	elements.pos[1].y = lineSel_posY;
	elements.size[1].w = line_width;
	elements.size[1].h = height;

	elements.pos[2].x = x;
	elements.pos[2].y = triangRight_posY;
	elements.size[2].w = triang_Height;
	elements.size[2].h = triang_Width;

	elements.param[0] = width_sel;
	elements.param[1] = line_width;
	elements.param[2] = ptr_width/2;

	if(DelTriang!=(int8_t)SHIFT_RIGHT(heightParam,16,FF))
		LCD_SimpleTriangle	(posBuff,BkpSizeX, 				MIDDLE(x,height,triang_Width), lineSel_posY-spaceTriangLine, 	triang_Width/2,    triang_Height, 	elemColor[0], elemColor[0], BkpColor, 	Up);
	LCD_LineV					(			BkpSizeX, 				MIDDLE(x,height,line_Bold), 	 lineSel_posY+1,						lineSel_width, 							LineSelColor, 									line_Bold );
	LCD_LittleRoundRectangle(posBuff,BkpSizeX, BkpSizeY, 	MIDDLE(x,height,ptr_height)+1, ptr_posY+1, 					 		ptr_height,			 ptr_width, 		elemColor[1], elemColor[1], BkpColor);
	if(0<lineUnSel_width-2)
		LCD_LineV				(			BkpSizeX, 				MIDDLE(x,height,line_Bold), 	 ptr_posY+1+ptr_width,				lineUnSel_width-2, 						LineColor, 										line_Bold );
	if(DelTriang!=(int8_t)SHIFT_RIGHT(heightParam,16,FF))
		LCD_SimpleTriangle	(posBuff,BkpSizeX, 				MIDDLE(x,height,triang_Width), triangRight_posY-1, 				triang_Width/2,  	 triang_Height, 	elemColor[2], elemColor[2], BkpColor, 	Down);

	return elements;

	#undef TRIANG_HEIGHT
	#undef LINE_BOLD
	#undef PTR_HEIGHT
	#undef PTR_WIDTH
}

uint32_t SetLineBold2Width(uint32_t width, uint8_t bold){
	return SHIFT_LEFT(width,bold,16);
}
uint32_t SetTriangHeightCoeff2Height(uint32_t height, uint8_t coeff){
	return SHIFT_LEFT(height,coeff,16);
}
SHAPE_PARAMS LCD_Arrow(uint32_t posBuff,uint32_t bkpSizeX,uint32_t bkpSizeY, uint32_t x,uint32_t y, uint32_t width,uint32_t height, uint32_t frameColor, uint32_t fillColor, uint32_t bkpColor, DIRECTIONS direct)
{
	SHAPE_PARAMS params = {.bkSize.w=bkpSizeX, .bkSize.h=bkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=width, .size[0].h=height, .color[0].frame=frameColor, .color[0].fill=fillColor, .color[0].bk=bkpColor, .param[0]=direct};

	if(ToStructAndReturn == posBuff)
		return params;

	posBuff = posBuff & 0x7FFFFFFF;

	int heightShape = MASK(height,FFFF);
	int widthShape = MASK(width,FFFF);

	int boldLine = SHIFT_RIGHT(width,16,FF);
	int widthTiang = 0, heightTiang = 0, lenLine = 0;

	void _CalcHeightTriang(void){
		switch(SHIFT_RIGHT(height,16,FF))
		{	default:
			case 0: heightTiang = widthTiang;   break;
			case 1: heightTiang = widthTiang*2; break;
			case 2: heightTiang = widthTiang*3; break;
			case 3: heightTiang = widthTiang/2; break;
			case 4: heightTiang = widthTiang/4; break; }
	}

	switch((int)direct)
	{
		case Right:
			widthTiang = heightShape;
			_CalcHeightTriang();
			lenLine = CONDITION(widthShape>heightTiang, widthShape-heightTiang, 1);
			LCD_LineH(bkpSizeX, x,  MIDDLE(y,widthTiang,boldLine), lenLine,  frameColor, boldLine);
			LCD_SimpleTriangle(posBuff,bkpSizeX, x+lenLine, y, widthTiang/2,heightTiang, frameColor, fillColor, bkpColor, direct);
			break;

		case Left:
			widthTiang = heightShape;
			_CalcHeightTriang();
			lenLine = CONDITION(widthShape>heightTiang, widthShape-heightTiang, 1);
			LCD_LineH(bkpSizeX, x+heightTiang,  MIDDLE(y,widthTiang,boldLine), lenLine,  frameColor, boldLine);
			LCD_SimpleTriangle(posBuff,bkpSizeX, x+heightTiang, y, widthTiang/2,heightTiang, frameColor, fillColor, bkpColor, direct);
			break;

		case Up:
			widthTiang = widthShape;
			_CalcHeightTriang();
			lenLine = CONDITION(heightShape>heightTiang, heightShape-heightTiang, 1);
			LCD_LineV(bkpSizeX, MIDDLE(x,widthTiang,boldLine),  y+heightTiang, lenLine,  frameColor, boldLine);
			LCD_SimpleTriangle(posBuff,bkpSizeX, x, y+heightTiang, widthTiang/2,heightTiang, frameColor, fillColor, bkpColor, direct);
			break;

		case Down:
			widthTiang = widthShape;
			_CalcHeightTriang();
			lenLine = CONDITION(heightShape>heightTiang, heightShape-heightTiang, 1);
			LCD_LineV(bkpSizeX, MIDDLE(x,widthTiang,boldLine),  y, lenLine,  frameColor, boldLine);
			LCD_SimpleTriangle(posBuff,bkpSizeX, x, y+lenLine, widthTiang/2,heightTiang, frameColor, fillColor, bkpColor, direct);
			break;
	}
	return params;
}
void LCD_Arrow_Indirect(uint32_t x,uint32_t y, uint32_t width,uint32_t height, uint32_t frameColor, uint32_t fillColor, uint32_t bkpColor, DIRECTIONS direct){
	uint32_t bkSizeX = MASK(width,FFFF) +1;
	uint32_t bkSizeY = MASK(height,FFFF)+1;
	LCD_ShapeWindow(LCD_Rectangle, 0, bkSizeX,bkSizeY, 0,0, bkSizeX,bkSizeY, bkpColor,bkpColor,bkpColor );
	LCD_Arrow(0,bkSizeX,bkSizeY, 0,0, width,height, frameColor, fillColor, bkpColor, direct);
	LCD_Display(0,x,y,bkSizeX,bkSizeY);
}
SHAPE_PARAMS LCDSHAPE_Arrow(uint32_t posBuff, SHAPE_PARAMS param){
	return LCD_Arrow(posBuff,param.bkSize.w,param.bkSize.h, param.pos[0].x,param.pos[0].y, param.size[0].w,param.size[0].h, param.color[0].frame, param.color[0].fill, param.color[0].bk, param.param[0]);
}
void LCDSHAPE_Arrow_Indirect(SHAPE_PARAMS param){
	return LCD_Arrow_Indirect(param.pos[0].x,param.pos[0].y, param.size[0].w,param.size[0].h, param.color[0].frame, param.color[0].fill, param.color[0].bk, param.param[0]);
}

SHAPE_PARAMS LCD_Enter(uint32_t posBuff,uint32_t bkpSizeX,uint32_t bkpSizeY, uint32_t x,uint32_t y, uint32_t width,uint32_t height, uint32_t frameColor, uint32_t fillColor, uint32_t bkpColor)
{
	SHAPE_PARAMS params = {.bkSize.w=bkpSizeX, .bkSize.h=bkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=width, .size[0].h=height, .color[0].frame=frameColor, .color[0].fill=fillColor, .color[0].bk=bkpColor};

	if(ToStructAndReturn == posBuff)
		return params;

	#define LEN_TAIL	(widthTiang/2)

	posBuff = posBuff & 0x7FFFFFFF;

	int heightShape = MASK(height,FFFF);
	int widthShape = MASK(width,FFFF);

	int boldLine = SHIFT_RIGHT(width,16,FF);
	int widthTiang = 0, heightTiang = 0, lenLine = 0;

	void _CalcHeightTriang(void){
		switch(SHIFT_RIGHT(height,16,FF))
		{	default:
			case 0: heightTiang = widthTiang;   break;
			case 1: heightTiang = widthTiang*2; break;
			case 2: heightTiang = widthTiang*3; break;
			case 3: heightTiang = widthTiang/2; break;
			case 4: heightTiang = widthTiang/4; break; }
	}

	widthTiang = heightShape;
	_CalcHeightTriang();
	lenLine = CONDITION(widthShape>heightTiang, widthShape-heightTiang, 1);
	LCD_LineH(bkpSizeX, x+heightTiang,  		  					MIDDLE(y,widthTiang,(boldLine+1)), 		  				  				 			  lenLine,  frameColor, boldLine);
	LCD_LineV(bkpSizeX, x+heightTiang+lenLine-(boldLine+1),  MIDDLE(y,widthTiang,(boldLine+1))+(boldLine+1)-(LEN_TAIL+(boldLine+1)/2), LEN_TAIL+(boldLine+1)/2,  frameColor, boldLine);
	LCD_SimpleTriangle(posBuff,bkpSizeX, x+heightTiang, y, widthTiang/2,heightTiang, frameColor, fillColor, bkpColor, Left);

	return params;
	#undef LEN_TAIL
}
SHAPE_PARAMS LCDSHAPE_Enter(uint32_t posBuff, SHAPE_PARAMS param){
	return LCD_Enter(posBuff,param.bkSize.w,param.bkSize.h, param.pos[0].x,param.pos[0].y, param.size[0].w,param.size[0].h, param.color[0].frame, param.color[0].fill, param.color[0].bk);
}

SHAPE_PARAMS LCD_Exit(uint32_t posBuff,uint32_t bkpSizeX,uint32_t bkpSizeY, uint32_t x,uint32_t y, uint32_t width,uint32_t height, uint32_t frameColor, uint32_t fillColor, uint32_t bkpColor)
{
	SHAPE_PARAMS params = {.bkSize.w=bkpSizeX, .bkSize.h=bkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=width, .size[0].h=height, .color[0].frame=frameColor, .color[0].fill=fillColor, .color[0].bk=bkpColor};
	if(ToStructAndReturn == posBuff)
		return params;

	int coeff = width > height ? width/height : height/width;
	Set_AACoeff_Draw(coeff,frameColor,bkpColor, 0.39);

	_StartDrawLine(0,bkpSizeX, x ,y-1);		  _DrawRightDown_AA(width,  height,  frameColor,bkpSizeX);
	_StartDrawLine(0,bkpSizeX, x, y);		  _DrawRightDown_AA(width+1,height+1,frameColor,bkpSizeX);
	_StartDrawLine(0,bkpSizeX, x, y+1);		  _DrawRightDown_AA(width,  height,	 frameColor,bkpSizeX);

	_StartDrawLine(0,bkpSizeX, x+width, y-1);  _DrawLeftDown_AA(width,  height,  frameColor,bkpSizeX);
	_StartDrawLine(0,bkpSizeX, x+width, y);	 _DrawLeftDown_AA(width+1,height+1,frameColor,bkpSizeX);
	_StartDrawLine(0,bkpSizeX, x+width, y+1);  _DrawLeftDown_AA(width,  height,  frameColor,bkpSizeX);
	return params;
}
SHAPE_PARAMS LCDSHAPE_Exit(uint32_t posBuff, SHAPE_PARAMS param){
	return LCD_Exit(posBuff,param.bkSize.w,param.bkSize.h, param.pos[0].x,param.pos[0].y, param.size[0].w,param.size[0].h, param.color[0].frame, param.color[0].fill, param.color[0].bk);
}

SHAPE_PARAMS LCDSHAPE_Window(ShapeFunc pShape, uint32_t posBuff, SHAPE_PARAMS param){
	return pShape(posBuff,param);
}

void LCD_SetCircleParam(float outRatio, float inRatio, int len, ...){
	va_list va;
	va_start(va,0);
	if(len<MAX_LINE_BUFF_CIRCLE_SIZE){
		Circle.lineBuff[0]=len;
		for(int i=0;i<len;i++){
			if(i>=MAX_LINE_BUFF_CIRCLE_SIZE) break;
			Circle.lineBuff[1+i]=va_arg(va,int);
		}
		Circle.outRatioStart=outRatio;
		Circle.inRatioStart=inRatio;
	}
	va_end(va);
}
void LCD_SetCircleDegrees(int len, ...){
	int i,deg;
	va_list va;
	va_start(va,0);
	for(i=0;i<len;i++){
		if(i==MAX_DEGREE_CIRCLE) break;
		Circle.degree[1+i]=va_arg(va,int);
	}
	Circle.degree[0]=i;
	va_end(va);

	for(i=0;i<len;i++)
	{
		deg=Circle.degree[1+i];
		Circle.tang[i]= tan(TANG_ARG(deg));
		Circle.coeff[i] = 1/ABS(Circle.tang[i]);

		if(deg>0 && deg<=45)
			Circle.rot[i]=1;
		else if(deg>45 && deg<=90)
			Circle.rot[i]=2;
		else if(deg>90 && deg<=135)
			Circle.rot[i]=3;
		else if(deg>135 && deg<=180)
			Circle.rot[i]=4;
		else if(deg>180 && deg<=225)
			Circle.rot[i]=5;
		else if(deg>225 && deg<=270)
			Circle.rot[i]=6;
		else if(deg>270 && deg<=315)
			Circle.rot[i]=7;
		else if(deg>315 && deg<=360)
			Circle.rot[i]=8;
	}

	for(int i=0;i<Circle.degree[0];++i){
		if(Circle.degree[1+i]==0)
			Circle.degree[1+i]=360;
	}
}
void LCD_SetCircleDegreesBuff(int len, uint16_t *buf){
	int i,deg;
	for(i=0;i<len;i++){
		if(i==MAX_DEGREE_CIRCLE) break;
		Circle.degree[1+i]=buf[i];
	}
	Circle.degree[0]=i;

	for(i=0;i<len;i++)
	{
		deg=Circle.degree[1+i];
		Circle.tang[i]= tan(TANG_ARG(deg));
		Circle.coeff[i] = 1/ABS(Circle.tang[i]);

		if(deg>0 && deg<=45)
			Circle.rot[i]=1;
		else if(deg>45 && deg<=90)
			Circle.rot[i]=2;
		else if(deg>90 && deg<=135)
			Circle.rot[i]=3;
		else if(deg>135 && deg<=180)
			Circle.rot[i]=4;
		else if(deg>180 && deg<=225)
			Circle.rot[i]=5;
		else if(deg>225 && deg<=270)
			Circle.rot[i]=6;
		else if(deg>270 && deg<=315)
			Circle.rot[i]=7;
		else if(deg>315 && deg<=360)
			Circle.rot[i]=8;
	}

	for(int i=0;i<Circle.degree[0];++i){
		if(Circle.degree[1+i]==0)
			Circle.degree[1+i]=360;
	}
}
void LCD_SetCircleDegColors(int len, ...){
	int i;
	va_list va;
	va_start(va,0);
	for(i=0;i<len;i++){
		if(i==MAX_DEGREE_CIRCLE) break;
		Circle.degColor[i]=va_arg(va,int);
	}
	va_end(va);
}
void LCD_SetCircleDegColorsBuff(int len, uint32_t *buf){
	int i;
	for(i=0;i<len;i++){
		if(i==MAX_DEGREE_CIRCLE) break;
		Circle.degColor[i]=buf[i];
	}
}
void LCD_SetCirclePercentParam(int len, uint16_t *degBuff, uint32_t *degColorBuff){
	LCD_SetCircleDegreesBuff(len,degBuff);
	LCD_SetCircleDegColorsBuff(len,degColorBuff);
}
void LCD_SetCircleDegree(uint8_t degNr, uint8_t deg){
	if(degNr<MAX_DEGREE_CIRCLE+1) Circle.degree[degNr]=deg;
}
uint16_t LCD_GetCircleDegree(uint8_t degNr){
	if(degNr<MAX_DEGREE_CIRCLE+1) return Circle.degree[degNr];
	else return 0;
}
uint32_t SetParamWidthCircle(uint16_t param, uint32_t width){
	return (width&0xFFFF)|param<<16;
}
uint16_t CenterOfCircle(uint16_t xy, uint16_t width){
	return xy+width/2;
}
void LCD_SetCircleLine(uint8_t lineNr, uint8_t val){
	Circle.lineBuff[lineNr]=val;
}
void LCD_OffsCircleLine(uint8_t lineNr, int offs){
	Circle.lineBuff[lineNr]+=offs;
}
void LCD_SetCircleAA(float outRatio, float inRetio){
	Circle.outRatioStart=outRatio;
	Circle.inRatioStart=inRetio;
}
void LCD_CopyCircleAA(void){
	Circle.outRatioStart_prev=Circle.outRatioStart;
	Circle.inRatioStart_prev=Circle.inRatioStart;
}
void LCD_SetCopyCircleAA(void){
	Circle.outRatioStart=Circle.outRatioStart_prev;
	Circle.inRatioStart=Circle.inRatioStart_prev;
}

uint16_t LCD_GetCircleWidth(void){
	return Circle.width;
}
void LCD_CopyCircleWidth(void){
	Circle.width_prev=Circle.width;
}
void LCD_SetCopyCircleWidth(void){
	Circle.width=Circle.width_prev;
}

uint16_t LCD_CalculateCircleWidth(uint32_t width)
{
	#define buf	 Circle.lineBuff

	uint32_t x=0, y=0;
	int matchWidth=0;
	uint32_t _width=(width&0x0000FFFF)-matchWidth;
	uint32_t height=_width;

	GOTO_ToCalculateRadius:
	  _width=(width&0x0000FFFF)-matchWidth;

	float R=((float)_width)/2;
	uint32_t pxl_width = R/3;

	x=_width/2 + matchWidth/2;
	if(_width%2) x++;
	y=0;

	float _x=(float)x, _y=(float)y;
	float x0=(float)x, y0=(float)y+R;

	float param_y = pow(y0-_y,2);
	float param_x = pow(_x-x0,2);
	float decision = pow(R,2);

	int pxl_line=0,i=0;
	uint8_t block=1;

	void _CorrectDecision(void){
		if(block){	 int ni= CONDITION(R<=Circle.correctForWidth,0,1);
			if(i >= VALPERC(pxl_width,Circle.correctPercDeg[ni])){	block=0;   decision= pow(R+Circle.errorDecision[ni],2);  }
	}}

	buf[0]=pxl_width;
	do{
		_x++;  pxl_line++;
		param_x = pow(_x-x0,2);		_CorrectDecision();
		if((param_x+param_y) > decision){
			_y++;
			param_y = pow(y0-_y,2);
			buf[1+i++]=pxl_line-1;
			pxl_line=1;
		}
	}while(i<pxl_width);

   uint16_t _height=buf[0];
  	for(int i=0;i<buf[0];++i) _height+=buf[buf[0]-i];
  	_height = 2*_height;

	if(height>=_height)
		y= y+(height-_height)/2;
	else{
		matchWidth+=1;
		goto GOTO_ToCalculateRadius;
	}
	return _height;
	#undef buf
}
uint16_t LCD_IncrCircleBold(uint16_t width, uint16_t bold){
	uint16_t width_temp= width-2*bold, bold_temp= bold, i=0;
	while(LCD_CalculateCircleWidth(width-2*INCR_WRAP(bold_temp,1,0,width/2-1)) == LCD_CalculateCircleWidth(width_temp) && 20>i) i++;
	if(20==i) return bold;
	else		 return bold_temp;
}
uint16_t LCD_DecrCircleBold(uint16_t width, uint16_t bold){
	uint16_t width_temp= width-2*bold, bold_temp= bold, i=0;
	while(LCD_CalculateCircleWidth(width-2*DECR_WRAP(bold_temp,1,0,width/2-1)) == LCD_CalculateCircleWidth(width_temp) && 20>i) i++;
	if(20==i) return bold;
	else		 return bold_temp;
}
uint16_t LCD_IncrWrapPercCircleBold(uint16_t radius, uint16_t bold, uint8_t minPerc, uint8_t maxPerc, uint8_t stepPerc){
	uint16_t _bold= bold;
	INIT(minVal, VALPERC(radius,minPerc));
	INIT(maxVal, VALPERC(radius,maxPerc));
	INIT(stepVal,VALPERC(radius,stepPerc));
	if(minVal >_bold) _bold= minVal;
	else{
		INCR_WRAP( _bold, stepVal, minVal, maxVal);
		if(minVal==_bold) _bold= 0;
	}
	return _bold;
}
uint16_t LCD_GetNextIncrCircleWidth(uint32_t width){
	uint32_t width_start= width;
	for(int i=0;	i<20 && width_start<=width; 	width_start=LCD_CalculateCircleWidth(width+i++));
	return width_start;
}
uint16_t LCD_GetNextDecrCircleWidth(uint32_t width){
	uint32_t width_start= width;
	for(int i=0;   i<20 && width_start>=width; 	width_start=LCD_CalculateCircleWidth(width-i++));
	return width_start;
}

structPosition GetCircleMiddPoint(uint16_t *radius){
	structPosition pos = {Circle.x0, Circle.y0};
	*radius = Circle.width;
	return pos;
}

void LCD_Circle(uint32_t posBuff,uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x, uint32_t y, uint32_t _width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	#define EASY_BOLD_CIRCLE	0==param && thickness
	uint32_t width = _width&0xFFFF;			/* MASK(_width,FFFF) */
	uint16_t param = _width>>16;				/* MSHIFT_RIGHT(_width,16,FFFF) */
	uint8_t thickness = FrameColor>>24;		/* SHIFT_RIGHT(_FrameColor,24,FF) */

	if(EASY_BOLD_CIRCLE) LCD_DrawCircle(posBuff,BkpSizeX,BkpSizeY,x,y, _width,height, FrameColor, FrameColor, BkpColor, 0);
	else						LCD_DrawCircle(posBuff,BkpSizeX,BkpSizeY,x,y, _width,height, FrameColor, FillColor,  BkpColor, 0);

	if(thickness){
		LCD_CopyCircleWidth();
   	uint32_t width_new = width-2*thickness;
		int offs= (Circle.width-LCD_CalculateCircleWidth(width_new))/2;
		if(EASY_BOLD_CIRCLE) LCD_DrawCircle(posBuff,BkpSizeX,BkpSizeY,x+offs,y+offs, width_new,width_new, FrameColor, FillColor, FrameColor, 0);
		else						LCD_DrawCircle(posBuff,BkpSizeX,BkpSizeY,x+offs,y+offs, width_new,width_new, FrameColor, FillColor, FillColor,  1);
		LCD_SetCopyCircleWidth();
	}
	#undef EASY_BOLD_CIRCLE
}

/* LCDSHAPE_Create() is defined for circle now,  in future for other shapes maybe */
SHAPE_PARAMS LCDSHAPE_Create(uint32_t posBuff,uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x, uint32_t y, uint32_t _width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor,u32 selFillColorFrom,u32 selFillColor,u32 selFillColorTo,u16 degree,DIRECTIONS fillDir,u32 outColorRead)
{
	SHAPE_PARAMS params = {.bkSize.w=BkpSizeX, .bkSize.h=BkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=_width, .size[0].h=height, .color[0].frame=FrameColor, .color[1].frame=FillColor, .color[2].frame=BkpColor, .color[0].fill=selFillColorFrom, .color[1].fill=selFillColor, .color[2].fill=selFillColorTo, .param[0]=degree, .param[1]=fillDir, .param[2]=outColorRead };
	if(ToStructAndReturn == posBuff)
		return params;																				/* For 'Percent_Circle'  frameColor != FillColor */

	int scale_x=2, scale_y=1;		/* scale only for 'RightDown' and 'LeftUp' */

	uint32_t width 			= MASK(_width,FFFF);
	int 	  	_outColorRead 	= MASK(outColorRead,1);
	uint16_t param 			= SHIFT_RIGHT(_width,16,FFFF);
	uint8_t 	thickness 		= SHIFT_RIGHT(FrameColor,24,FF);
	int width_max=0, width_min=0;

	if(Percent_Circle==param){
		uint16_t deg[2] = {0, degree };
		uint32_t degColor[2] = {0, COLOR_TEST };
		LCD_SetCirclePercentParam(2,deg,(uint32_t*)degColor);
	}

	if(param) LCD_DrawCircle(posBuff,BkpSizeX,BkpSizeY,x,y, _width,height, FrameColor, FillColor, BkpColor, _outColorRead);
	else		 LCD_DrawCircle(posBuff,BkpSizeX,BkpSizeY,x,y, _width,height, FrameColor, COLOR_TEST,BkpColor, _outColorRead);

	width_max=Circle.width;

	if(thickness)
	{
		LCD_CopyCircleWidth();
   	uint32_t width_new = width-2*thickness;
		int offs= (Circle.width-LCD_CalculateCircleWidth(width_new))/2;
		LCD_DrawCircle(posBuff,BkpSizeX,BkpSizeY,x+offs,y+offs, width_new,width_new, FrameColor, FillColor, unUsed, ReadOutColor);
		width_min=Circle.width;
		LCD_SetCopyCircleWidth();

		int offsCalc=offs, n=0;
		while(offsCalc==offs){
			width_new = width-2*(thickness-n);
			offsCalc= (Circle.width-LCD_CalculateCircleWidth(width_new))/2;
			n++;
		}
		offs=offsCalc;

		params.pos[0].x	= x+offs;
		params.pos[0].y	= y+offs;
		params.size[0].w	= width_new;
		params.size[0].h	= width_new;
	}

	int nmbPxls=0, nmbPxlsHalf=0;
	switch((int)fillDir){
	case Round:  nmbPxls=360; break;
	case Center: nmbPxls=CONDITION(0<thickness,thickness,width_max/2); break;
	default:		 nmbPxls=width_max; break;
	}
	if(nmbPxls>=MAX_SIZE_TAB_AA-1)
		nmbPxls=MAX_SIZE_TAB_AA-2;
	nmbPxlsHalf=nmbPxls/2;

	if(0==selFillColorFrom && 0==selFillColor && 0==selFillColorTo){
		LOOP_FOR(i,nmbPxls){ buff_AA[1+i]=0; }
	}
	else if(0!=selFillColorFrom && 0==selFillColor && 0==selFillColorTo){
		LOOP_FOR(i,nmbPxls){ buff_AA[1+i]=selFillColorFrom; }
	}
	else if(0!=selFillColorFrom && 0!=selFillColor && 0==selFillColorTo){
		switch((int)fillDir){
		case Up: case Left: case LeftUp: case Center: Set_AACoeff(nmbPxls,selFillColor,    selFillColorFrom, 0.0);	break;
		default:				 									 Set_AACoeff(nmbPxls,selFillColorFrom,selFillColor,     0.0);	break;
		}
	}
	else if(0!=selFillColorFrom && 0!=selFillColor && 0!=selFillColorTo){
		switch((int)fillDir){
		case Up: case Left: case Center:
			Set_AACoeff(nmbPxlsHalf, selFillColorTo,selFillColor,   0.0);
			Set_AACoeff2(nmbPxlsHalf,selFillColor,  selFillColorFrom, 0.0);
			LOOP_FOR(i,nmbPxlsHalf){ buff_AA[1+nmbPxlsHalf+i]=buff2_AA[1+i]; }
			break;
		case RightDown: case LeftUp:
			break;
		default:
			Set_AACoeff(nmbPxlsHalf, selFillColorFrom,selFillColor,   0.0);
			Set_AACoeff2(nmbPxlsHalf,selFillColor, 	selFillColorTo, 0.0);
			LOOP_FOR(i,nmbPxlsHalf){ buff_AA[1+nmbPxlsHalf+i]=buff2_AA[1+i]; }
			break;
		}
	}
	else LOOP_FOR(i,nmbPxls){ buff_AA[1+i]=0; }


	switch((int)fillDir)
	{
	case Center:
		int radius_min=width_min/2;
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			LOOP_FOR(i,width_max){
				if(_IS_NOT_PXL(k+i,COLOR_TEST,FrameColor,FillColor,BkpColor)){
					if(_IS_NEXT_PXL(BkpSizeX,k+i,COLOR_TEST)){
						int temp = LCD_CIRCLE_GetRadiusFromPosXY(i-width_max/2, width_max/2 -j, 0,0);
						temp = temp-radius_min;		if(temp>=nmbPxls) temp=nmbPxls-1; else if(temp<0) temp=0;
						pLcd[k+i]=GetTransitionColor(FrameColor, buff_AA[1+temp], GetTransitionCoeff(FrameColor,COLOR_TEST,pLcd[k+i]));
			}}}
			k+=BkpSizeX;
		}
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			LOOP_FOR(i,width_max){
				if(pLcd[k+i]==COLOR_TEST){
					int temp = LCD_CIRCLE_GetRadiusFromPosXY(i-width_max/2, width_max/2 -j, 0,0);
					temp = temp-radius_min;		if(temp>=nmbPxls) temp=nmbPxls-1; else if(temp<0) temp=0;
					pLcd[k+i]= buff_AA[1+temp];
			}}
			k+=BkpSizeX;
		}
		break;

	case Round:
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			LOOP_FOR(i,width_max){
				if(_IS_NOT_PXL(k+i,COLOR_TEST,FrameColor,FillColor,BkpColor)){
					if(_IS_NEXT_PXL(BkpSizeX,k+i,COLOR_TEST)){
						int degg = LCD_CIRCLE_GetDegFromPosXY(i-width_max/2, width_max/2-j, 0,0);
						pLcd[k+i]=GetTransitionColor(FrameColor, buff_AA[1+degg], GetTransitionCoeff(FrameColor,COLOR_TEST,pLcd[k+i]));
			}}}
			k+=BkpSizeX;
		}
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			LOOP_FOR(i,width_max){
				if(pLcd[k+i]==COLOR_TEST){
					int degg = LCD_CIRCLE_GetDegFromPosXY(i-width_max/2, width_max/2-j, 0,0);
					pLcd[k+i]= buff_AA[1+degg];
			}}
			k+=BkpSizeX;
		}
		break;

	case Down: case Up:
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			LOOP_FOR(i,width_max){
				if(_IS_NOT_PXL(k+i,COLOR_TEST,FrameColor,FillColor,BkpColor)){
					if(_IS_NEXT_PXL(BkpSizeX,k+i,COLOR_TEST)){
						pLcd[k+i]=GetTransitionColor(FrameColor, buff_AA[1+j], GetTransitionCoeff(FrameColor,COLOR_TEST,pLcd[k+i]));
			}}}
			k+=BkpSizeX;
		}
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			LOOP_FOR(i,width_max){
				if(pLcd[k+i]==COLOR_TEST)
					pLcd[k+i]= buff_AA[1+j];
			}
			k+=BkpSizeX;
		}
		break;

	case Right: case Left:
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			LOOP_FOR(i,width_max){
				if(_IS_NOT_PXL(k+i,COLOR_TEST,FrameColor,FillColor,BkpColor)){
					if(_IS_NEXT_PXL(BkpSizeX,k+i,COLOR_TEST)){
						pLcd[k+i]=GetTransitionColor(FrameColor, buff_AA[1+i], GetTransitionCoeff(FrameColor,COLOR_TEST,pLcd[k+i]));
			}}}
			k+=BkpSizeX;
		}
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			LOOP_FOR(i,width_max){
				if(pLcd[k+i]==COLOR_TEST)
					pLcd[k+i]= buff_AA[1+i];
			}
			k+=BkpSizeX;
		}
		break;

	case RightDown: case LeftUp:
		int block=0, stepY=-1, _stepY=0, offs=width_max/2-1, arg1AA=0, arg2AA=0, _width_max=width_max/scale_x;
		_StartDrawLine(posBuff,BkpSizeX,x,y);	stepY=-1; block=0;
		LOOP_FOR(j,width_max){	_stepY=j/(2+(scale_y-1));  if(stepY!=_stepY){ block=0; arg1AA=1+_stepY; arg2AA=arg1AA+offs; }
			LOOP_FOR(i,width_max){
				if(_IS_NOT_PXL(k+i,COLOR_TEST,FrameColor,FillColor,BkpColor)){
					if(_IS_NEXT_PXL(BkpSizeX,k+i,COLOR_TEST)){
						if(block==0){ Set_AACoeff2(_width_max, buff_AA[arg1AA], buff_AA[arg2AA], 0.0); block=1; stepY=_stepY; }
						pLcd[k+i]=GetTransitionColor(FrameColor, buff2_AA[1+i/scale_x], GetTransitionCoeff(FrameColor,COLOR_TEST,pLcd[k+i]));
					}
				}
			}
			k+=BkpSizeX;
		}
		_StartDrawLine(posBuff,BkpSizeX,x,y);	stepY=-1; block=0;
		LOOP_FOR(j,width_max){	_stepY=j/(2+(scale_y-1));  if(stepY!=_stepY){ block=0; arg1AA=1+_stepY; arg2AA=arg1AA+offs; }
			LOOP_FOR(i,width_max){
				if(pLcd[k+i]==COLOR_TEST){
					if(block==0){ Set_AACoeff2(_width_max, buff_AA[arg1AA], buff_AA[arg2AA], 0.0); block=1; stepY=_stepY; }
					pLcd[k+i]= buff2_AA[1+i/scale_x];
				}
			}
			k+=BkpSizeX;
		}
		break;

/* Here not optimized option */
/*	case RightDown: case LeftUp:
		int offs=width_max/2-1;
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			Set_AACoeff2(width_max, buff_AA[1+j/2], buff_AA[1+offs+j/2], 0.0);
			LOOP_FOR(i,width_max){
				if(_IS_NOT_PXL(k+i,COLOR_TEST,FrameColor,FillColor,BkpColor)){
					if(_IS_NEXT_PXL(BkpSizeX,k+i,COLOR_TEST)){
						pLcd[k+i]=GetTransitionColor(FrameColor, buff2_AA[1+i], GetTransitionCoeff(FrameColor,COLOR_TEST,pLcd[k+i]));
					}
				}
			}
			k+=BkpSizeX;
		}
		_StartDrawLine(posBuff,BkpSizeX,x,y);
		LOOP_FOR(j,width_max){
			Set_AACoeff2(width_max, buff_AA[1+j/2], buff_AA[1+offs+j/2], 0.0);
			LOOP_FOR(i,width_max){
				if(pLcd[k+i]==COLOR_TEST)
					pLcd[k+i]= buff2_AA[1+i];
			}
			k+=BkpSizeX;
		}
	break;
*/
	}

	return params;

/*	https://dmitrymorozoff.github.io/react-circle-slider/
	https://stackoverflow.com/questions/78482981/custom-circular-slider-with-gradient-colour-bar-swift */
}

void LCD_HalfCircle(uint32_t posBuff,uint32_t BkpSizeX,uint32_t BkpSizeY, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t FrameColor, uint32_t FillColor, uint32_t BkpColor){
	uint8_t thickness = FrameColor>>24;
	if((thickness==0)||(thickness==255))
		thickness=0;
	else
		thickness=3*thickness;

	if(thickness){
		LCD_DrawHalfCircle(posBuff,BkpSizeX,BkpSizeY,x,y, width,height, FrameColor, FrameColor, BkpColor);
		LCD_CopyCircleWidth();

		int whatRotate= width>>16;
   	uint32_t width_new = (width&0x0000FFFF) - 2*thickness;
   	int offs= (Circle.width-LCD_CalculateCircleWidth(width_new))/2;

		LCD_DrawHalfCircle(posBuff,BkpSizeX,BkpSizeY,x+offs,y+offs, SetParamWidthCircle(whatRotate,width_new),width_new, FrameColor, FillColor, FrameColor);
		LCD_SetCopyCircleWidth();
	}
	else LCD_DrawHalfCircle(posBuff,BkpSizeX,BkpSizeY,x,y, width,height, FrameColor, FillColor, BkpColor);
}

SHAPE_PARAMS LCD_RoundRectangle2(u32 posBuff,int rectangleFrame,u32 BkpSizeX,u32 BkpSizeY,u32 x,u32 y,u32 width,u32 height,u32 FrameColorStart,u32 FrameColorStop,u32 FillColorStart,u32 FillColorStop,u32 BkpColor,float ratioStart,DIRECTIONS direct)
{
	SHAPE_PARAMS params = {.bkSize.w=BkpSizeX, .bkSize.h=BkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=width, .size[0].h=height, .color[0].frame=FrameColorStart, .color[1].frame=FrameColorStop, .color[0].fill=FillColorStart, .color[1].fill=FillColorStop, .color[0].bk=BkpColor, .param[0]=direct, .param[1]=FLOAT_TO_U32(ratioStart), .param[2]=rectangleFrame};
	if(ToStructAndReturn == posBuff)
		return params;

	int boldDirect = SHIFT_RIGHT(rectangleFrame,24,FF);
	int boldValue = SHIFT_RIGHT(rectangleFrame,16,FF);
	int stepGrad = SHIFT_RIGHT(rectangleFrame,8,FF);
	int rectFrame= MASK(rectangleFrame,FF);									if(rectFrame==Frame && boldDirect!=Shade && boldDirect!=AllEdge){ FillColorStart=BkpColor; FillColorStop=BkpColor; }
	u32 colorBuff[boldValue];

	if(boldValue > bold0){
		switch(boldDirect){
			case Down2:
				LCD_DrawRoundRectangle2(posBuff,Frame,BkpSizeX,BkpSizeY,x,y,width,height,FrameColorStart,FrameColorStop,FillColorStart,FillColorStop,BkpColor,ratioStart,direct);
				LOOP_FOR(i,boldValue){
					if(i==boldValue-bold1)	LCD_DrawRoundRectangle2(posBuff,rectFrame,BkpSizeX,BkpSizeY,x,y,width,height-(i+1),  FrameColorStart,								BrightIncr(FrameColorStop,i*stepGrad),  FillColorStart,FillColorStop,AA_OUT_OFF,ratioStart,direct);
					else							LCD_DrawRoundRectangle2(posBuff,Frame,		BkpSizeX,BkpSizeY,x,y,width,height-(i+1),  BrightIncr(FrameColorStop,i*stepGrad),BrightIncr(FrameColorStop,i*stepGrad),  FillColorStart,FillColorStop,AA_OUT_OFF,ratioStart,direct); }
				break;
			case Down:
				LCD_DrawRoundRectangle2(posBuff,Frame,BkpSizeX,BkpSizeY,x,y,width,height,FrameColorStart,FrameColorStop,FillColorStart,FillColorStop,BkpColor,ratioStart,direct);
				LOOP_FOR(i,boldValue){
					LCD_DrawRoundRectangle2(posBuff,CONDITION(i==boldValue-bold1,rectFrame,Frame),BkpSizeX,BkpSizeY,x,y,width,height-(i+1),FrameColorStart,FrameColorStop,FillColorStart,FillColorStop,AA_OUT_OFF,ratioStart,direct); }
				break;
			case Up:
				LCD_DrawRoundRectangle2(posBuff,Frame,BkpSizeX,BkpSizeY,x,y,width,height,FrameColorStart,FrameColorStop,FillColorStart,FillColorStop,BkpColor,ratioStart,direct);
				LOOP_FOR(i,boldValue){
					LCD_DrawRoundRectangle2(posBuff,CONDITION(i==boldValue-bold1,rectFrame,Frame),BkpSizeX,BkpSizeY,x,y+(i+1),width,height-(i+1),FrameColorStart,FrameColorStop,FillColorStart,FillColorStop,AA_OUT_OFF,ratioStart,direct); }
				break;
			case AllEdge:
				Set_AACoeff(boldValue,FillColorStart,FillColorStop,ratioStart);	LOOP_FOR(i,boldValue){ colorBuff[i]=buff_AA[1+i]; }
				LCD_DrawRoundRectangle2(posBuff,Frame|(stepGrad<<24),BkpSizeX,BkpSizeY,x,y,width,height,FrameColorStart,FrameColorStop,colorBuff[0],colorBuff[0],BkpColor,ratioStart,direct);
				LOOP_FOR(i,boldValue-1){
					if(i==boldValue-2) LCD_DrawRoundRectangle2(posBuff,rectFrame,				BkpSizeX,BkpSizeY,x+(i+1),y+(i+1),width-2*(i+1),height-2*(i+1), colorBuff[i+1],colorBuff[i+1], colorBuff[i+1],colorBuff[i+1], READ_BKCOLOR, ratioStart,direct);
					else					 LCD_DrawRoundRectangle2(posBuff,Frame|(stepGrad<<24),BkpSizeX,BkpSizeY,x+(i+1),y+(i+1),width-2*(i+1),height-2*(i+1), colorBuff[i+1],colorBuff[i+1], colorBuff[i+1],colorBuff[i+1], AA_OUT_OFF,   ratioStart,direct);
				}
				break;
			case AllEdge2:
				Set_AACoeff(boldValue,FillColorStart,FillColorStop,ratioStart);	LOOP_FOR(i,boldValue){ colorBuff[i]=buff_AA[1+i]; }
				LCD_DrawRoundRectangle2(posBuff,Frame|(stepGrad<<24),BkpSizeX,BkpSizeY,x,y,width,height,FrameColorStart,FrameColorStop,colorBuff[0],colorBuff[0],BkpColor,ratioStart,direct);
				LOOP_FOR(i,boldValue-1){
					if(i==boldValue-2) LCD_DrawRoundRectangle2(posBuff,rectFrame,           BkpSizeX,BkpSizeY,x+(i+1),y+(i+1),width-2*(i+1),height-2*(i+1), BrightIncr(colorBuff[i+1],stepGrad),BrightIncr(colorBuff[i+1],stepGrad),BrightIncr(colorBuff[i+1],stepGrad),BrightIncr(colorBuff[i+1],stepGrad), READ_BKCOLOR, ratioStart,direct);
					else					 LCD_DrawRoundRectangle2(posBuff,Frame|(stepGrad<<24),BkpSizeX,BkpSizeY,x+(i+1),y+(i+1),width-2*(i+1),height-2*(i+1), colorBuff[i+1],					  		 colorBuff[i+1], 					 		 colorBuff[i+1],							 colorBuff[i+1], 							  AA_OUT_OFF, 	 ratioStart,direct);
				}
				break;
			case Shade:
				LOOP_FOR(i,boldValue){
					LCD_DrawRoundRectangle2(posBuff,rectFrame,BkpSizeX,BkpSizeY,x+boldValue-i,y+boldValue-i,width,height,FrameColorStart,FrameColorStop,FillColorStart,FillColorStop,BkpColor,ratioStart,direct);
				}
				break;
	}}
	else LCD_DrawRoundRectangle2(posBuff,rectFrame,BkpSizeX,BkpSizeY,x,y,width,height,FrameColorStart,FrameColorStop,FillColorStart,FillColorStop,BkpColor,ratioStart,direct);
	return params;
}
void LCD_RoundRectangle_Indirect(int rectFrame,u32 x,u32 y, u32 width,u32 height, u32 FrameColorStart,u32 FrameColorStop,u32 FillColorStart,u32 FillColorStop,u32 BkpColor,float ratioStart,DIRECTIONS direct){
	uint32_t bkSizeX = MASK(width,FFFF) +0;
	uint32_t bkSizeY = MASK(height,FFFF)+0;
	LCD_ShapeWindow(LCD_Rectangle, 0, bkSizeX,bkSizeY, 0,0, bkSizeX,bkSizeY, BkpColor,BkpColor,BkpColor );
	LCD_RoundRectangle2(0,rectFrame,bkSizeX,bkSizeY, 0,0, width,height, FrameColorStart,FrameColorStop, FillColorStart, FillColorStop, BkpColor, ratioStart, direct);
	LCD_Display(0,x,y,bkSizeX,bkSizeY);
}
SHAPE_PARAMS LCDSHAPE_RoundRectangle(uint32_t posBuff, SHAPE_PARAMS param){
	return LCD_RoundRectangle2(posBuff, param.param[2], param.bkSize.w, param.bkSize.h, param.pos[0].x, param.pos[0].y, param.size[0].w, param.size[0].h, param.color[0].frame, param.color[1].frame, param.color[0].fill, param.color[1].fill, param.color[0].bk, U32_TO_FLOAT(param.param[1]), param.param[0]);
}
void LCDSHAPE_RoundRectangle_Indirect(SHAPE_PARAMS param){
	LCD_RoundRectangle_Indirect(param.param[2], param.pos[0].x,param.pos[0].y, param.size[0].w,param.size[0].h, param.color[0].frame, param.color[1].frame, param.color[0].fill, param.color[1].fill, param.color[0].bk, U32_TO_FLOAT(param.param[1]), param.param[0]);
}

SHAPE_PARAMS LCD_Rectangle2(u32 posBuff,u32 BkpSizeX,u32 BkpSizeY,u32 x,u32 y,u32 width,u32 height,u32 FrameColorStart,u32 FrameColorStop,u32 FillColorStart,u32 FillColorStop,u32 BkpColor,float ratioStart,DIRECTIONS param)
{
	SHAPE_PARAMS params = {.bkSize.w=BkpSizeX, .bkSize.h=BkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=width, .size[0].h=height, .color[0].frame=FrameColorStart, .color[1].frame=FrameColorStop, .color[0].fill=FillColorStart, .color[1].fill=FillColorStop, .color[0].bk=BkpColor, .param[0]=param, .param[1]=FLOAT_TO_U32(ratioStart)};
	if(ToStructAndReturn == posBuff)
		return params;

	if(AllEdge==param && IS_RANGE(FrameColorStop,1,MINVAL2(width,height)-1)){
		int boldValue=(int)FrameColorStop;
		u32 colorBuff[boldValue];
		Set_AACoeff(boldValue,FillColorStart,FillColorStop,ratioStart);	LOOP_FOR(i,boldValue){ colorBuff[i]=buff_AA[1+i]; }
		LCD_Frame(posBuff, BkpSizeX,BkpSizeY, x,y, width, height, FrameColorStart, colorBuff[0], BkpColor);
		LOOP_FOR(i,boldValue-1){
			if(i==boldValue-2)
				LCD_Rectangle(posBuff, BkpSizeX,BkpSizeY, x+(i+1),y+(i+1),width-2*(i+1),height-2*(i+1), colorBuff[i], colorBuff[i+1], BkpColor);
			else
				LCD_Frame	 (posBuff, BkpSizeX,BkpSizeY, x+(i+1),y+(i+1),width-2*(i+1),height-2*(i+1), colorBuff[i], colorBuff[i+1], BkpColor);
		}
		return params;
	}

	int iFrame=0, iFill=0;
	int maxFramPxl=height, maxFillPxl=height;

	switch((int)param){
		case Down: 	 case Up:		 maxFillPxl= height-2;	 	break;
		case Midd_Y: case Midd_Y2:	 maxFillPxl=(height-2)/2;  break;
		case Right:  case Left:		 maxFillPxl= width-2;	 	break;
		case Midd_X: case Midd_X2:	 maxFillPxl= (width-2)/2; 	break;
	}
	Set_AACoeff2(maxFramPxl,FrameColorStart,FrameColorStop,ratioStart);		/* careful for {maxFramPxl,maxFillPxl} < MAX_SIZE_TAB_AA */
	Set_AACoeff (maxFillPxl,FillColorStart, FillColorStop, ratioStart);

	_StartDrawLine(posBuff,BkpSizeX,x,y);

	_FillBuff(width, buff2_AA[1+iFrame++]);
	if(height>1)
	{
		_NextDrawLine(BkpSizeX,width);
		for (int j=0; j<height-2; j++)
		{
			_FillBuff(1, buff2_AA[1+iFrame]);
			switch((int)param){
				case Down:
					_FillBuff(width-2, buff_AA[1+iFill]);
					break;
				case Up:
					_FillBuff(width-2, buff_AA[1+(maxFillPxl-1)-iFill]);
					break;
				case Midd_Y:
					if(j==maxFillPxl || j==0) iFill=0;
					if(j< maxFillPxl) _FillBuff(width-2, buff_AA[1+iFill]);
					else				   _FillBuff(width-2, buff_AA[1+(maxFillPxl-1)-iFill]);
					break;
				case Midd_Y2:
					if(j==maxFillPxl || j==0) iFill=0;
					if(j< maxFillPxl)	_FillBuff(width-2, buff_AA[1+(maxFillPxl-1)-iFill]);
					else					_FillBuff(width-2, buff_AA[1+iFill]);
					break;
				case Right:
					LOOP_FOR(i,maxFillPxl){ _FillBuff(1, buff_AA[1+i]); }
					break;
				case Left:
					LOOP_FOR(i,maxFillPxl){ _FillBuff(1, buff_AA[1+(maxFillPxl-1)-i]); }
					break;
				case Midd_X:
					LOOP_FOR(i,maxFillPxl){ _FillBuff(1, buff_AA[1+i]); }
					LOOP_FOR(i,maxFillPxl){ _FillBuff(1, buff_AA[1+(maxFillPxl-1)-i]); }
					break;
				case Midd_X2:
					LOOP_FOR(i,maxFillPxl){ _FillBuff(1, buff_AA[1+(maxFillPxl-1)-i]); }
					LOOP_FOR(i,maxFillPxl){ _FillBuff(1, buff_AA[1+i]); }
					break;
				case RightDown:
					if(width>=height){
						int ratio = (width-2)/(height-2);
						Set_AACoeff (width-2,FillColorStart, FillColorStop, ratioStart);
						u32 colornext = buff_AA[1+((width-2)-1)-ratio*((height-2)-1)+ratio*j];
						Set_AACoeff (width-2,FillColorStart, colornext, ratioStart);
						LOOP_FOR(i,width-2){ _FillBuff(1, buff_AA[1+i]); }
					}
					else{
						Set_AACoeff (height-2,FillColorStart, FillColorStop, ratioStart);
						u32 colornext = buff_AA[1+j];
						Set_AACoeff (width-2,FillColorStart, colornext, ratioStart);
						LOOP_FOR(i,width-2){ _FillBuff(1, buff_AA[1+i]); }
					}
					break;
			}
			_FillBuff(1, buff2_AA[1+iFrame]);
			_NextDrawLine(BkpSizeX,width);
			iFill++; iFrame++;
		}
		_FillBuff(width, buff2_AA[1+iFrame]);
	}
	return params;
}
void LCD_Rectangle_Indirect(u32 x,u32 y, u32 width,u32 height, u32 FrameColorStart,u32 FrameColorStop,u32 FillColorStart,u32 FillColorStop,u32 BkpColor,float ratioStart,DIRECTIONS direct){
	uint32_t bkSizeX = MASK(width,FFFF) +0;
	uint32_t bkSizeY = MASK(height,FFFF)+0;
	LCD_ShapeWindow(LCD_Rectangle, 0, bkSizeX,bkSizeY, 0,0, bkSizeX,bkSizeY, BkpColor,BkpColor,BkpColor );
	LCD_Rectangle2(0,bkSizeX,bkSizeY, 0,0, width,height, FrameColorStart,FrameColorStop, FillColorStart, FillColorStop, BkpColor, ratioStart, direct);
	LCD_Display(0,x,y,bkSizeX,bkSizeY);
}
SHAPE_PARAMS LCDSHAPE_Rectangle(uint32_t posBuff, SHAPE_PARAMS param){
	return LCD_Rectangle2(posBuff, param.bkSize.w, param.bkSize.h, param.pos[0].x, param.pos[0].y, param.size[0].w, param.size[0].h, param.color[0].frame, param.color[1].frame, param.color[0].fill, param.color[1].fill, param.color[0].bk, U32_TO_FLOAT(param.param[1]), param.param[0]);
}
void LCDSHAPE_Rectangle_Indirect(SHAPE_PARAMS param){
	LCD_Rectangle_Indirect(param.pos[0].x,param.pos[0].y, param.size[0].w,param.size[0].h, param.color[0].frame, param.color[1].frame, param.color[0].fill, param.color[1].fill, param.color[0].bk, U32_TO_FLOAT(param.param[1]), param.param[0]);
}

int LCD_GradCircButtSlidCorrectXY(SHAPE_PARAMS param, u16 bkWidth){
	return CONDITION( param.bkSize.w==bkWidth, 0, ((int)bkWidth-(int)param.bkSize.w)/2);
}

/* ------------------- CIRCLE BUTTON ------------------------*/
SHAPE_PARAMS LCD_GradientCircleButton(u32 posBuff,u32 BkpSizeX,u32 BkpSizeY,u32 x,u32 y,u32 width,u32 height,u32 FrameColor,u32 FillColorGradStart,u32 FillColorGradStop,u32 BkpColor,u32 outColorRead)
{
	if(x==0){ BkpSizeX+=2; BkpSizeY+=2;	 x=1, y=1; }
	SHAPE_PARAMS params = {.bkSize.w=BkpSizeX, .bkSize.h=BkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=width, .size[0].h=height, .color[0].frame=FrameColor, .color[0].fill=FillColorGradStart, .color[1].fill=FillColorGradStop, .color[0].bk=BkpColor, .param[0]=outColorRead };
	if(ToStructAndReturn == posBuff)
		return params;
	SHAPE_PARAMS par={0};	uint16_t circleWidth;
	par=LCDSHAPE_Create(posBuff,BkpSizeX,BkpSizeY, x,				 y, 				width, 			height, 			FrameColor, 																 						 COLOR_TEST_1, 						 BkpColor, FillColorGradStart,FillColorGradStop,0,unUsed,RightDown,outColorRead);	circleWidth=Circle.width;
		 LCDSHAPE_Create(posBuff,BkpSizeX,BkpSizeY, par.pos[0].x, par.pos[0].y, par.size[0].w, par.size[0].h, SetBold2Color(GetTransitionColor(FillColorGradStart,FillColorGradStop,0.2),0), _DESCR("not used",COLOR_TEST_2), BkpColor, FillColorGradStart,FillColorGradStop,0,unUsed,LeftUp, 	 ReadOutColor);
	Circle.width=circleWidth;
	return params;
}
void LCD_GradientCircleButton_Indirect(												 u32 x,u32 y,u32 width,u32 height,u32 FrameColor,u32 FillColorGradStart,u32 FillColorGradStop,u32 BkpColor,u32 outColorRead){
	uint32_t bkSizeX = MASK(width,FFFF) +2;
	uint32_t bkSizeY = MASK(height,FFFF)+2;
	if(0==MASK(outColorRead,1)) LCD_ShapeWindow(LCD_Rectangle, 0, bkSizeX,bkSizeY, 0,0, bkSizeX,bkSizeY, BkpColor,BkpColor,BkpColor );
	LCD_GradientCircleButton(0,bkSizeX,bkSizeY,1,1,width,height,FrameColor,FillColorGradStart,FillColorGradStop,BkpColor,outColorRead);
	LCD_Display(0,x,y,bkSizeX,bkSizeY);
}
SHAPE_PARAMS LCDSHAPE_GradientCircleButton(uint32_t posBuff, SHAPE_PARAMS param){
	return LCD_GradientCircleButton(posBuff, param.bkSize.w, param.bkSize.h, param.pos[0].x, param.pos[0].y, param.size[0].w, param.size[0].h, param.color[0].frame, param.color[0].fill, param.color[1].fill, param.color[0].bk, param.param[0]);
}
void LCDSHAPE_GradientCircleButton_Indirect(						 SHAPE_PARAMS param){
	LCD_GradientCircleButton_Indirect(param.pos[0].x, param.pos[0].y, param.size[0].w, param.size[0].h, param.color[0].frame, param.color[0].fill, param.color[1].fill, param.color[0].bk, param.param[0]);
}

/* ------------------- CIRCLE SLIDER ------------------------*/
SHAPE_PARAMS LCD_GradientCircleSlider(u32 posBuff,u32 BkpSizeX,u32 BkpSizeY,u32 x,u32 y,u32 width,u32 height,u32 FrameColorSlid,u32 FillColorSlid,u32 GradColorStartSlid,u32 GradColorSlid,u32 GradColorStopSlid,u32 FrameColorButt,u32 FillColorStartButt,u32 FillColorStopButt,u32 BkpColor,u16 degree,DIRECTIONS fillDirSlid,u32 outColorRead)
{
	if(x==0){ BkpSizeX+=2; BkpSizeY+=2;	 x=1, y=1; }
	SHAPE_PARAMS params = {.bkSize.w=BkpSizeX, .bkSize.h=BkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=width, .size[0].h=height, .color[0].frame=FrameColorSlid, .color[1].frame=FrameColorButt, .color[0].fill=GradColorStartSlid, .color[1].fill=GradColorSlid, .color[2].fill=GradColorStopSlid, .color[0].bk=BkpColor, .color[1].bk=FillColorSlid, .param[0]=FillColorStartButt, .param[1]=FillColorStopButt, .color[2].frame=degree, .color[2].bk=fillDirSlid, .param[2]=outColorRead };
	if(ToStructAndReturn == posBuff)
		return params;
	SHAPE_PARAMS par={0};	uint16_t circleWidth;
	par=LCDSHAPE_Create			(posBuff,BkpSizeX,BkpSizeY, x,				y, 			  SetParamWidthCircle(Percent_Circle,width),width, 		  FrameColorSlid,FillColorSlid, 							 	BkpColor, GradColorStartSlid,GradColorSlid,GradColorStopSlid,degree,fillDirSlid,outColorRead);	 circleWidth=Circle.width;
	if(unUsed!=FrameColorButt && unUsed!=FillColorStartButt && unUsed!=FillColorStopButt)
		LCD_GradientCircleButton(posBuff,BkpSizeX,BkpSizeY, par.pos[0].x, par.pos[0].y, par.size[0].w, 									  par.size[0].h, FrameColorButt,FillColorStartButt,FillColorStopButt,BkpColor,																							  ReadOutColor);
	Circle.width=circleWidth;
	return params;
}
void LCD_GradientCircleSlider_Indirect(u32 x,u32 y,u32 width,u32 height,u32 FrameColorSlid,u32 FillColorSlid,u32 GradColorStartSlid,u32 GradColorSlid,u32 GradColorStopSlid,u32 FrameColorButt,u32 FillColorStartButt,u32 FillColorStopButt,u32 BkpColor,u16 degree,DIRECTIONS fillDirSlid,u32 outColorRead){
	uint32_t bkSizeX = MASK(width,FFFF) +2;
	uint32_t bkSizeY = MASK(height,FFFF)+2;
	if(0==MASK(outColorRead,1)) LCD_ShapeWindow(LCD_Rectangle, 0, bkSizeX,bkSizeY, 0,0, bkSizeX,bkSizeY, BkpColor,BkpColor,BkpColor );
	LCD_GradientCircleSlider(0,bkSizeX,bkSizeY,1,1,width,height,FrameColorSlid,FillColorSlid,GradColorStartSlid,GradColorSlid,GradColorStopSlid,FrameColorButt,FillColorStartButt,FillColorStopButt,BkpColor,degree,fillDirSlid,outColorRead);
	LCD_Display(0,x,y,bkSizeX,bkSizeY);
}
SHAPE_PARAMS LCDSHAPE_GradientCircleSlider(uint32_t posBuff, SHAPE_PARAMS param){
	return LCD_GradientCircleSlider(posBuff, param.bkSize.w, param.bkSize.h, param.pos[0].x, param.pos[0].y, param.size[0].w, param.size[0].h, param.color[0].frame, param.color[1].bk, param.color[0].fill, param.color[1].fill, param.color[2].fill, param.color[1].frame, param.param[0], param.param[1], param.color[0].bk, param.color[2].frame, param.color[2].bk, param.param[2]);
}
void LCDSHAPE_GradientCircleSlider_Indirect(SHAPE_PARAMS param){
	LCD_GradientCircleSlider_Indirect(param.pos[0].x, param.pos[0].y, param.size[0].w, param.size[0].h, param.color[0].frame, param.color[1].bk, param.color[0].fill, param.color[1].fill, param.color[2].fill, param.color[1].frame, param.param[0], param.param[1], param.color[0].bk, param.color[2].frame, param.color[2].bk, param.param[2]);
}

/* ---------------------------- GRAPH ------------------------- */
void GRAPHFUNC_SetMemOffsForOwnFunc(u32 memOffs){	memOffsForGraphOwnFunc = memOffs;	}
u32  GRAPHFUNC_GetMemOffsForOwnFunc(void)			{	return memOffsForGraphOwnFunc;		}

int GRAPH_IsIndirect(int nrMem){		if(ptrPrev[nrMem].startXYchart.x != posXY[0].x	&&	 ptrPrev[nrMem].startXYchart.y != posXY[0].y	) return 1;
												else																															  return 0;
}
int GRAPH_IsMemReloaded(int nrMem){
	extern u32 GET_nmbrBytesForFontsImages();
	if(ptrPrev[nrMem].CounterBusyBytesForFontsImages_copy != GET_nmbrBytesForFontsImages()) return 1;
	return 0;
}
int GRAPH_GetNmbrPoints(int nrMem)
{
	#if defined(GRAPH_MEMORY_SDRAM2)
		if(chartMemOffsForMemNr[nrMem]==-1) return 0;
		extern char* GETVAL_ptr(uint32_t nrVal);
		int ptr2Mem = chartMemOffsForMemNr[nrMem] + (SIZE_ONE_CHART * nrMem);
		posXY_par = (structGetSmpl*) GETVAL_ptr (ptr2Mem);
	#endif
	return posXY_par[0].nmbrPoints;
}
							/* 'offsMem', 'nrMem' are used only for GRAPH_MEMORY_SDRAM2 */
int GRAPH_GetSamples(int offsMem,int nrMem, int startX,int startY, int yMin,int yMax, int nmbrPoints,float precision, float scaleX,float scaleY, int funcPatternType)
{
	#if defined(GRAPH_MEMORY_SDRAM2)
		if(GRAPH_SetPointers(offsMem,nrMem)) return 0;
		chartMemOffsForMemNr[nrMem]=offsMem;
	#endif
	extern u32 GET_nmbrBytesForFontsImages();
	GRAPH_ClearPosXYpar();
	GRAPH_ClearPosXY();
	GRAPH_ClearPosXYrep();			/* '.len_posXY' may be equal to 'nmbrPoints' or many times smaller as in square signal */

	posXY_par[0].len_posXY 	  = GRAPH_GetFuncPosXY(startX,startY,yMin,yMax,nmbrPoints,precision,scaleX,scaleY,funcPatternType);		/* len_posXY >> len_posXYrep (many times larger, at least 2 times) */
	posXY_par[0].len_posXYrep = GRAPH_RepetitionRedundancyOfPosXY(posXY_par[0].len_posXY);

	posXY_par[0].startX=startX;	posXY_par[0].startY=startY;	posXY_par[0].yMin=yMin;		posXY_par[0].yMax=yMax;		posXY_par[0].nmbrPoints=nmbrPoints;	  posXY_par[0].precision=precision;	  posXY_par[0].scaleX=scaleX;	 posXY_par[0].scaleY=scaleY;	 posXY_par[0].funcPatternType=funcPatternType;

	ptrPrev[nrMem].startXYchart.x  = startX;
	ptrPrev[nrMem].startXYchart.y  = startY;
	ptrPrev[nrMem].yMinMaxchart[0] = startY + yMin;
	ptrPrev[nrMem].yMinMaxchart[1] = startY + yMax;
	ptrPrev[nrMem].sizeX 			 = nmbrPoints;
	ptrPrev[nrMem].CounterBusyBytesForFontsImages_copy = GET_nmbrBytesForFontsImages();

	if(1 > posXY_par[0].len_posXYrep) return 1;
	else										 return 0;
}

structPointParam GRAPH_SetPtr(u32 fromColorPtr, u32 toColorPtr, u16 sizePtr, 					 u16 posPtr, 				  u8 hideShowPtr, \
									   u32 fromColorRct, u32 toColorRct, u16 xSizeRct,u16 ySizeRct, u16 xPosRct,u16 yPosRct, u8 hideShowRct, int fontID)
{
	structPointParam param = {.fromColorPtr=fromColorPtr, .toColorPtr=toColorPtr, .sizePtr=sizePtr, 				.posPtr=posPtr, 				.hideShowPtr=hideShowPtr, \
									  .fromColorRct=fromColorRct, .toColorRct=toColorRct, .sizeRct={xSizeRct,ySizeRct}, .posRct={xPosRct,yPosRct}, .hideShowRct=hideShowRct, .fontID=fontID};
	return param;
}

int GRAPH_DrawPtr(int nrMem, int posPtr)
{
	if(nrMem >= MAX_CHARTS_SIMULTANEOUSLY 	||  ptrPrev[nrMem].ptr.hideShowPtr == 0) return 0;
	#if defined(GRAPH_MEMORY_SDRAM2)
		if(chartMemOffsForMemNr[nrMem]==-1)	 							return 0;
		if(GRAPH_SetPointers(chartMemOffsForMemNr[nrMem],nrMem)) return 0;
	#endif
	int posBuff = 0;
	int ptrX,ptrY, offsX,offsY;
	int ptrX_prev 		= ptrPrev[nrMem].pos.x;
	int ptrY_prev 		= ptrPrev[nrMem].pos.y;
	u16 corrPtrW 		= ptrPrev[nrMem].size.w+2;		/* '+2' because of bkSizeX for LCD_GradientCircleButton() */
	u16 corrPtrH 		= ptrPrev[nrMem].size.h+2;
	u32 colorTransPtr = GetTransitionColor( ptrPrev[nrMem].ptr.fromColorPtr, ptrPrev[nrMem].ptr.toColorPtr, 0.5);
	int posChartPtr   = 0;

	void __PTR_CopyBitmapToMem_and_PrepareBk(void){
		u32 temp;
		_2LOOP_INIT(int m=0, i,j, corrPtrW,corrPtrH)
			if(m>=CHART_PTR_MEM_SIZE) return;
			temp 				  				= pLcd[posBuff+ptrPrev[nrMem].chartBkW*(ptrPrev[nrMem].pos.y+j)+(ptrPrev[nrMem].pos.x+i)];
			*(ptrPrev[nrMem].ptrMem+m) = temp;		/* write background to memory */
			pLcd[posBuff+m]  				= temp;  	/* prepare new background */
			m++;
		_2LOOP_END
		ptrPrev[nrMem].memInUse=1;
	}
	void __RCT_CopyBitmapToMem_and_PrepareBk(void){
	/* Time execution for none optimization for below code: 800[us]					Time execution for optimization: 400[us] 					Time execute for fast or size optimization: 220[us] but vibration screen are observed ! */
	/*	StartMeasureTime_us();
		u32 temp;
		_2LOOP_INIT(int m=0, i,j, ptrPrev[nrMem].ptr.sizeRct.w, ptrPrev[nrMem].ptr.sizeRct.h)
			temp 								= pLcd[posBuff+ptrPrev[nrMem].chartBkW*(ptrPrev[nrMem].ptr.posRct.y+j)+(ptrPrev[nrMem].ptr.posRct.x+i)];
			*(ptrPrev[nrMem].rctMem+m) = temp;		//write background to memory
			pLcd[posBuff+m]  				= temp;		//prepare new background
			m++;
		_2LOOP_END
	*/
	/* Time execution for none optimization for LCD_CopyBuffers(): 600[us]			Time execute for debug optimization: 270[us]				Time execute for fast or size optimization: 150[us] but vibration screen are observed ! */
		LCD_CopyBuffers( ptrPrev[nrMem].rctMem, 0, 		 ptrPrev[nrMem].ptr.sizeRct.w, 0,									0, \
							  pLcd,						 posBuff, ptrPrev[nrMem].chartBkW, 	 	 ptrPrev[nrMem].ptr.posRct.x, ptrPrev[nrMem].ptr.posRct.y, ptrPrev[nrMem].ptr.sizeRct.w, ptrPrev[nrMem].ptr.sizeRct.h);		/* write background to memory */

		LCD_CopyBuffers( pLcd,						 posBuff, ptrPrev[nrMem].ptr.sizeRct.w, 0,									0, \
							  pLcd,						 posBuff, ptrPrev[nrMem].chartBkW, 	 	 ptrPrev[nrMem].ptr.posRct.x, ptrPrev[nrMem].ptr.posRct.y,  ptrPrev[nrMem].ptr.sizeRct.w, ptrPrev[nrMem].ptr.sizeRct.h);  	/* prepare new background */
		ptrPrev[nrMem].memInUse=1;
	}

	if(GRAPH_IsIndirect(nrMem)){	offsX = ptrPrev[nrMem].startXYchart.x;
											offsY = ptrPrev[nrMem].startXYchart.y;		}
	else{	 	offsX = 0;
			 	offsY = 0;	 }

	int temp2 = offsX+posXY[0].x;
	LOOP_FOR(i, posXY_par[0].len_posXY){	if(temp2 != offsX+posXY[i].x) temp2=offsX+posXY[i].x;		if(temp2>=posPtr){ posChartPtr=i; break; }	}

	if(posChartPtr >= posXY_par[0].len_posXY)	posChartPtr = posXY_par[0].len_posXY - 1;  		/* posXY_par[0].len_posXY >= ptrPrev[nrMem].sizeX */
	if(posChartPtr < 0)								posChartPtr = 0;

/*	if(posXY[posChartPtr].x < ptrPrev[nrMem].startXYchart.x 								 + corrPtrW/2){		while(posXY[++posChartPtr].x < ptrPrev[nrMem].startXYchart.x 								+ corrPtrW/2);		}
	if(posXY[posChartPtr].x > ptrPrev[nrMem].startXYchart.x + ptrPrev[nrMem].sizeX - corrPtrW/2){		while(posXY[--posChartPtr].x > ptrPrev[nrMem].startXYchart.x + ptrPrev[nrMem].sizeX - corrPtrW/2);		}	*/			/* only if pointer has distance from end or start of chart */

	SET_VAL( offsX + posXY[posChartPtr].x - corrPtrW/2, 		ptrX, 	ptrPrev[nrMem].pos.x );
	SET_VAL( offsY + posXY[posChartPtr].y - corrPtrH/2, 		ptrY, 	ptrPrev[nrMem].pos.y );

	if(ptrPrev[nrMem].memInUse) LCD_ErasePrevShape(ptrX_prev,ptrY_prev, ptrX,ptrY, corrPtrW,corrPtrH, ptrPrev[nrMem].ptrMem);
	if(ptrPrev[nrMem].ptr.hideShowRct)
	{
		extern void LCD_TxtInFrame_minimize();
		int rectX, rectY;
		int rectW 		= ptrPrev[nrMem].ptr.sizeRct.w;
		int rectH 		= ptrPrev[nrMem].ptr.sizeRct.h;
		int rectX_prev = ptrPrev[nrMem].ptr.posRct.x;
		int rectY_prev = ptrPrev[nrMem].ptr.posRct.y;
		int offsRctToPtr_x = 0;		/* offset X relative to middle of pointer */
		int offsRctToPtr_y = 0;		/* offset Y relative to edge of pointer 	*/

		switch(ptrPrev[nrMem].ptr.hideShowRct){
			default:
			case 1:
				offsRctToPtr_x = 0;
				offsRctToPtr_y = -15;
				break;

			case 2:
				offsRctToPtr_x = -20;
				offsRctToPtr_y = 15;
				break;
		}
		int xRctStart = ( ptrX - (rectW-corrPtrW)/2 ) + offsRctToPtr_x;
		if(xRctStart < ptrPrev[nrMem].startXYchart.x)  											 xRctStart = ptrPrev[nrMem].startXYchart.x;
		if(xRctStart > ptrPrev[nrMem].startXYchart.x + ptrPrev[nrMem].sizeX - rectW)   xRctStart = ptrPrev[nrMem].startXYchart.x + ptrPrev[nrMem].sizeX - rectW;

		int yRctStart = ptrY + CONDITION(offsRctToPtr_y<0,-rectH,corrPtrH) + offsRctToPtr_y;
		if(yRctStart < ptrPrev[nrMem].yMinMaxchart[0])  			yRctStart = ptrY + corrPtrH + ABS(offsRctToPtr_y);
		if(yRctStart > ptrPrev[nrMem].yMinMaxchart[1] - rectH)   yRctStart = ptrY - rectH 	 - ABS(offsRctToPtr_y);

		SET_VAL( xRctStart, rectX, ptrPrev[nrMem].ptr.posRct.x );
		SET_VAL( yRctStart, rectY, ptrPrev[nrMem].ptr.posRct.y );

		if(ptrPrev[nrMem].memInUse) LCD_ErasePrevShape(rectX_prev,rectY_prev, rectX,rectY, rectW,rectH, ptrPrev[nrMem].rctMem);
		__RCT_CopyBitmapToMem_and_PrepareBk();
		LCD_BoldRoundRectangleTransp(posBuff,  rectW,rectH, 	0,0, 	rectW,rectH, 	SetBold2Color(ptrPrev[nrMem].ptr.fromColorRct,3), ptrPrev[nrMem].ptr.toColorRct, READ_BGCOLOR, 0.5);
		LCD_TxtInFrame_minimize(rectW,rectH, ptrPrev[nrMem].ptr.fontID, -2,0, 	StrAll(3,Int2Str(ptrX+corrPtrW/2,None,3,Sign_none),",",Int2Str(ptrY+corrPtrH/2,None,3,Sign_none)),		ptrPrev[nrMem].ptr.hideShowRct);
		LCD_Display(posBuff, rectX,rectY, rectW,rectH);
	}

	__PTR_CopyBitmapToMem_and_PrepareBk();
	LCD_GradientCircleButton_Indirect(ptrX,ptrY,  corrPtrW-2,corrPtrH-2,  SetBold2Color(colorTransPtr,1), ptrPrev[nrMem].ptr.fromColorPtr, ptrPrev[nrMem].ptr.toColorPtr, 0,ReadOutColor);
	return posChartPtr;
}

int GRAPH_ptrTouchService(u16 touchPosX, u16 touchPosY, int nrChart){
	if( IS_RANGE(touchPosX, ptrPrev[nrChart].startXYchart.x,  ptrPrev[nrChart].startXYchart.x+ptrPrev[nrChart].sizeX ) &&
		 IS_RANGE(touchPosY, ptrPrev[nrChart].yMinMaxchart[0], ptrPrev[nrChart].yMinMaxchart[1]					  		  ) )
	{
		GRAPH_DrawPtr(nrChart, touchPosX);
		return 1;
	}
	return 0;
}
					 /* 'offsMem', 'nrMem' are used only for GRAPH_MEMORY_SDRAM2 */
void GRAPH_Draw(int posBuff,int nrMem, u32 widthBk, u32 colorLineAA, u32 colorOut, u32 colorIn, float outRatioStart, float inRatioStart, \
					DISP_OPTION dispOption, u32 color1, u32 color2, int offsK1, int offsK2, GRADIENT_GRAPH_TYPE bkGradType,u32 gradColor1,u32 gradColor2,u8 gradStripY,float amplTrans,float offsTrans, int corr45degAA, structPointParam chartPtr, u32 gridColor,int gridSizeX,int gridSizeY,int gridType,float gridCoeff)
{
	#if defined(GRAPH_MEMORY_SDRAM2)
		if(chartMemOffsForMemNr[nrMem]==-1)	 							return;
		if(GRAPH_SetPointers(chartMemOffsForMemNr[nrMem],nrMem)) return;
	#endif

	if(!IS_RANGE(posXY_par[0].len_posXYrep,1,GRAPH_MAX_SIZE_POSXY)) return;

	u32 bkColor = 0;
	int transParamSize = 1 + (posXY_par[0].yMax - posXY_par[0].yMin),   posX_prev=0,   distanceY,   n;

	struct COLOR_TRANS_PARAM{
		u32 lineColor;
		u32 bkColor;
		float coeff;
		u32 transColor;
	}TransParam[transParamSize];

	/* Draw gradient background for chart */
	if((int)bkGradType > -1)
	{
		switch((int)bkGradType)
		{
			case Grad_YmaxYmin:
				LOOP_FOR(i,transParamSize){
					TransParam[i].lineColor	 = gradColor1;
					TransParam[i].bkColor	 = 0;
					TransParam[i].coeff		 = (amplTrans * ((float)i)) / (float)transParamSize + offsTrans;
					TransParam[i].transColor = GetTransitionColor(TransParam[i].lineColor, TransParam[i].bkColor, TransParam[i].coeff); }
				break;

			case Grad_Ystrip:
				break;

			case Grad_Ycolor:
				LOOP_FOR(i,transParamSize){
					TransParam[i].coeff		 = (amplTrans * ((float)i)) / (float)transParamSize + offsTrans;
					TransParam[i].lineColor	 = GetTransitionColor(gradColor1, gradColor2, TransParam[i].coeff);
					TransParam[i].bkColor	 = 0;
					TransParam[i].transColor = GetTransitionColor(TransParam[i].lineColor, TransParam[i].bkColor, TransParam[i].coeff); }
				break;
		}

		LOOP_FOR(i,posXY_par[0].len_posXY)
		{
			if(posXY[i].x != posX_prev)
			{
					if(Grad_Ystrip == bkGradType){
						if(gradStripY) distanceY = gradStripY;
						else				distanceY = (posXY_par[0].startY + posXY_par[0].yMax)-(posXY[i].y+1);
						LOOP_FOR(m, distanceY){
							TransParam[m].lineColor	 = gradColor1;
							TransParam[m].bkColor	 = CONDITION(0==colorIn, _PLCD(posBuff,posXY[i].x,posXY[i].y+1+m), colorIn);
							TransParam[m].coeff		 = (amplTrans * ((float)m)) / ((float)distanceY) + offsTrans;
							TransParam[m].transColor = GetTransitionColor(TransParam[m].lineColor, TransParam[m].bkColor, TransParam[m].coeff); }
					}

					LOOP_INIT(j, posXY[i].y+1, posXY_par[0].startY + posXY_par[0].yMax)
					{
						switch((int)bkGradType)
						{
							case Grad_YmaxYmin:
							case Grad_Ycolor:
								if(0==colorIn)	bkColor = _PLCD(posBuff,posXY[i].x, j);
								else				bkColor = colorIn;
								n = j-(posXY_par[0].startY + posXY_par[0].yMin);
								if(TransParam[n].bkColor == bkColor)
									_PLCD(posBuff,posXY[i].x, j) = TransParam[n].transColor;
								else{
									TransParam[n].bkColor 	 = bkColor;
									TransParam[n].transColor = GetTransitionColor(TransParam[n].lineColor, TransParam[n].bkColor, TransParam[n].coeff);
									_PLCD(posBuff,posXY[i].x, j) = TransParam[n].transColor;
								}
								break;

							case Grad_Ystrip:
								n = j - (posXY[i].y+1);
								if(n < distanceY)
									_PLCD(posBuff,posXY[i].x, j) = TransParam[n].transColor;
								break;
					}}
			}
			posX_prev = posXY[i].x;
		}
	}

	/* Draw lines of the chart */
	if((int)dispOption==Disp_AA){
					   GRAPH_Display(posBuff,	    	widthBk, posXY_par[0].len_posXYrep, colorLineAA, colorOut,colorIn, outRatioStart,inRatioStart, corr45degAA);
		if(offsK1){ GRAPH_Display(posBuff+offsK1, widthBk, posXY_par[0].len_posXYrep, color1,		 colorOut,colorIn, outRatioStart,inRatioStart, 0); 		 }
		if(offsK2){ GRAPH_Display(posBuff+offsK2, widthBk, posXY_par[0].len_posXYrep, color2,		 colorOut,colorIn, 1.0,			   1.0,			  unUsed); 	 }
	}
	else{
		if((int)dispOption&Disp_posXY)	 GRAPH_DispPosXY	 (posBuff+offsK1, widthBk, posXY_par[0].len_posXY,	 	color1);
		if((int)dispOption&Disp_posXYrep) GRAPH_DispPosXYrep(posBuff+offsK2, widthBk, posXY_par[0].len_posXYrep, color2);
		if((int)dispOption&Disp_AA)		 GRAPH_Display		 (posBuff,		 	widthBk, posXY_par[0].len_posXYrep, colorLineAA, colorOut,colorIn, outRatioStart,inRatioStart, corr45degAA);
	}

	/* Draw grid of the chart */
	if(Grid_None != gridType)
	{
		int offsX = CONDITION( GRAPH_IsIndirect(nrMem), 0, ptrPrev[nrMem].startXYchart.x  );
		int offsY = CONDITION( GRAPH_IsIndirect(nrMem), 0, ptrPrev[nrMem].yMinMaxchart[0] );
		int SizeY = ptrPrev[nrMem].yMinMaxchart[1] - ptrPrev[nrMem].yMinMaxchart[0];
		_2LOOP(i,j, ptrPrev[nrMem].sizeX, SizeY)
			switch(gridType){
				case Grid_Dots: 		if(i%gridSizeX==0 && j%gridSizeY==0)	_PLCD(posBuff,offsX+i,offsY+j) = GetTransitionColor(_PLCD(posBuff,offsX+i,offsY+j), gridColor,gridCoeff);	break;
				case Grid_Line:  		if(i%gridSizeX==0 || j%gridSizeY==0)	_PLCD(posBuff,offsX+i,offsY+j) = GetTransitionColor(_PLCD(posBuff,offsX+i,offsY+j), gridColor,gridCoeff); 	break;
				default: break;
			}
		_2LOOP_END
	}

	/* Prepare (not display here) pointer of the chart, rest of data is filled in GRAPH_DrawPtr() */
	if(chartPtr.hideShowPtr)
	{
		if(nrMem >= MAX_CHARTS_SIMULTANEOUSLY) return;
		int sizeChartPtr  = CONDITION(chartPtr.sizePtr%2, chartPtr.sizePtr+1, chartPtr.sizePtr);
		ptrPrev[nrMem].size.w  	= sizeChartPtr;
		ptrPrev[nrMem].size.h  	= sizeChartPtr;
		ptrPrev[nrMem].pos.x   	= 0;
		ptrPrev[nrMem].pos.y   	= 0;;
		ptrPrev[nrMem].chartBkW = LCD_X;
		ptrPrev[nrMem].ptr 	 	= chartPtr;
		ptrPrev[nrMem].ptrMem	= chartPtrMem[nrMem];
		ptrPrev[nrMem].rctMem	= chartRctMem[nrMem];
		if(chartPtr.hideShowRct){
			ptrPrev[nrMem].ptr.posRct.x = ptrPrev[nrMem].pos.x;
			ptrPrev[nrMem].ptr.posRct.y = ptrPrev[nrMem].pos.y;
		}
		ptrPrev[nrMem].memInUse=0;
	/* LCD_GradientCircleButton(0,widthBk,unUsed, ptrPrev.pos.x, ptrPrev.pos.y,  ptrPrev.size.w, ptrPrev.size.h,  SetBold2Color(colorTransPtr,1),chartPtr.fromColorPtr,chartPtr.toColorPtr,0,ReadOutColor); */		/* This function is used only in GRAPH_DrawPtr() */
	}
}
									  /* 'offsMem', 'nrMem' are used only for GRAPH_MEMORY_SDRAM2 */
void GRAPH_GetSamplesAndDraw(int posBuff, int offsMem,int nrMem, u32 widthBk, int startX,int startY, int yMin,int yMax, int nmbrPoints,float precision, float scaleX,float scaleY, int funcPatternType, u32 colorLineAA, u32 colorOut, u32 colorIn, float outRatioStart, float inRatioStart, \
								DISP_OPTION dispOption, u32 color1, u32 color2, int offsK1, int offsK2, GRADIENT_GRAPH_TYPE bkGradType,u32 gradColor1,u32 gradColor2,u8 gradStripY,float amplTrans,float offsTrans, int corr45degAA, structPointParam chartPtr, u32 gridColor,int gridSizeX,int gridSizeY,int gridType,float gridCoeff)
{
	if(nrMem >= MAX_CHARTS_SIMULTANEOUSLY) return;
	if(GRAPH_GetSamples(offsMem,nrMem,startX,startY,yMin,yMax,nmbrPoints,precision,scaleX,scaleY,funcPatternType)) return;
	GRAPH_Draw(posBuff,nrMem, widthBk, colorLineAA,colorOut,colorIn, outRatioStart,inRatioStart, dispOption,color1,color2,offsK1,offsK2, bkGradType,gradColor1,gradColor2,gradStripY,amplTrans,offsTrans, corr45degAA, chartPtr, gridColor,gridSizeX,gridSizeY,gridType,gridCoeff);
}

/* ---------------------------- CHART ------------------------- */
													 /* 'offsMem', 'nrMem' are used only for GRAPH_MEMORY_SDRAM2 */
USER_GRAPH_PARAM LCD_Chart(int posBuff,int nrMem, u32 widthBk, u32 colorLineAA, u32 colorOut, u32 colorIn, u32 bkRectColor, float outRatioStart, float inRatioStart, DISP_OPTION dispOption, u32 color1, u32 color2, int offsK1, int offsK2, GRADIENT_GRAPH_TYPE bkGradType,u32 gradColor1,u32 gradColor2,u8 gradStripY,float amplTrans,float offsTrans, int corr45degAA, structPointParam chartPtr, u32 gridColor,int gridSizeX,int gridSizeY,int gridType,float gridCoeff){
	#if defined(GRAPH_MEMORY_SDRAM2)
		if(chartMemOffsForMemNr[nrMem]==-1)  							return USER_GRAPH_PARAM_Zero;
		if(GRAPH_SetPointers(chartMemOffsForMemNr[nrMem],nrMem)) return USER_GRAPH_PARAM_Zero;
	#endif
	USER_GRAPH_PARAM params = {.offsMem=chartMemOffsForMemNr[nrMem], .nrMem=nrMem, .widthBk=widthBk, .lineColor=colorLineAA, .AAoutColor=colorOut, .AAinColor=colorIn, .AAoutCoeff=outRatioStart, .AAinCoeff=inRatioStart, .dispOpt=dispOption, .colorLinePosXY=color1, .colorLinePosXYrep=color2, .KoffsPosXY=offsK1, .KoffsPosXYrep=offsK2, .grad.bkType=bkGradType, .grad.fromColor=gradColor1, .grad.toColor=gradColor2, .grad.stripY=gradStripY, .grad.amplTrans=amplTrans, .grad.offsTrans=offsTrans, .bkRectColor=bkRectColor, .corr45degAA=corr45degAA, .ptr=chartPtr, .grid.color=gridColor, .grid.sizeX=gridSizeX, .grid.sizeY=gridSizeY, .grid.type=gridType, .grid.transCoeff=gridCoeff };

	params.par.startX				= posXY_par[0].startX;
	params.par.startY				= posXY_par[0].startY;
	params.par.yMin				= posXY_par[0].yMin;
	params.par.yMax				= posXY_par[0].yMax,
	params.par.nmbrPoints		= posXY_par[0].nmbrPoints;
	params.par.precision			= posXY_par[0].precision;
	params.par.scaleX				= posXY_par[0].scaleX;
	params.par.scaleY				= posXY_par[0].scaleY;
	params.par.funcPatternType	= posXY_par[0].funcPatternType;
	params.par.len_posXY			= posXY_par[0].len_posXY;
	params.par.len_posXYrep		= posXY_par[0].len_posXYrep;

	if(ToStructAndReturn == posBuff)
		return params;
	if(IS_RANGE(widthBk,10,LCD_X))
		GRAPH_Draw(posBuff,nrMem, widthBk, colorLineAA,colorOut,colorIn, outRatioStart,inRatioStart, dispOption,color1,color2,offsK1,offsK2, bkGradType,gradColor1,gradColor2,gradStripY,amplTrans,offsTrans, corr45degAA, chartPtr, gridColor,gridSizeX,gridSizeY,gridType,gridCoeff);
	return params;
}

void LCD_Chart_Indirect(int nrMem, u32 widthBk, u32 colorLineAA, u32 colorOut, u32 colorIn, u32 bkRectColor, float outRatioStart, float inRatioStart, DISP_OPTION dispOption, u32 color1, u32 color2, int offsK1, int offsK2, GRADIENT_GRAPH_TYPE bkGradType,u32 gradColor1,u32 gradColor2,u8 gradStripY,float amplTrans,float offsTrans, int corr45degAA, structPointParam chartPtr, u32 gridColor,int gridSizeX,int gridSizeY,int gridType,float gridCoeff){
	#if defined(GRAPH_MEMORY_SDRAM2)
		if(chartMemOffsForMemNr[nrMem]==0)	 							return;
		if(GRAPH_SetPointers(chartMemOffsForMemNr[nrMem],nrMem)) return;
	#endif
	int yPosInGetSamples = ptrPrev[nrMem].startXYchart.y;
	int x	 		= MASK(widthBk>>16,FFFF);
	int y			= MASK(widthBk,	 FFFF)-yPosInGetSamples;
	int width  	= posXY_par[0].nmbrPoints;
	int height 	= 1+2*yPosInGetSamples;
	int offsK	= 0;		/* must be multiple of '32' */
	ptrPrev[nrMem].startXYchart.x  = x;
	ptrPrev[nrMem].startXYchart.y  = y;
	ptrPrev[nrMem].yMinMaxchart[0] = MASK(widthBk,FFFF)-yPosInGetSamples;
	ptrPrev[nrMem].yMinMaxchart[1] = MASK(widthBk,FFFF)+yPosInGetSamples;
	if(bkRectColor) LCD_ShapeWindow(LCD_Rectangle, offsK, width,height, 0,0, width,height, RED,bkRectColor,bkRectColor);
	GRAPH_Draw(offsK, nrMem, width, colorLineAA, colorOut, colorIn, outRatioStart, inRatioStart, dispOption, color1, color2, offsK1, offsK2, bkGradType, gradColor1, gradColor2, gradStripY, amplTrans, offsTrans, corr45degAA, chartPtr, gridColor,gridSizeX,gridSizeY,gridType,gridCoeff);
	_2LOOP_INIT(k=width,i,j,width,height)
			if(i==0) 		pLcd[offsK+k+i]=bkRectColor;
			if(i==width-1) pLcd[offsK+k+i]=bkRectColor;
		_1LOOP_END
		k += width;
	_1LOOP_END
	LCD_Display(offsK, x,y, width,height);
	LCD_CopyBuffers(pLcd,0,LCD_X,x,y, pLcd,offsK,width,0,0,width,height);  /* copying buffers is very important only for GRAPH_DrawPtr() */
}
USER_GRAPH_PARAM LCDSHAPE_Chart(uint32_t posBuff, USER_GRAPH_PARAM param){
	return LCD_Chart(posBuff, param.nrMem, param.widthBk, param.lineColor, param.AAoutColor, param.AAinColor, param.bkRectColor, param.AAoutCoeff, param.AAinCoeff, param.dispOpt, param.colorLinePosXY, param.colorLinePosXYrep, param.KoffsPosXY, param.KoffsPosXYrep, param.grad.bkType, param.grad.fromColor, param.grad.toColor, param.grad.stripY, param.grad.amplTrans, param.grad.offsTrans, param.corr45degAA, param.ptr, param.grid.color, param.grid.sizeX, param.grid.sizeY, param.grid.type, param.grid.transCoeff);
}
void LCDSHAPE_Chart_Indirect(USER_GRAPH_PARAM param){
	LCD_Chart_Indirect(		  param.nrMem, param.widthBk, param.lineColor, param.AAoutColor, param.AAinColor, param.bkRectColor, param.AAoutCoeff, param.AAinCoeff, param.dispOpt, param.colorLinePosXY, param.colorLinePosXYrep, param.KoffsPosXY, param.KoffsPosXYrep, param.grad.bkType, param.grad.fromColor, param.grad.toColor, param.grad.stripY, param.grad.amplTrans, param.grad.offsTrans, param.corr45degAA, param.ptr, param.grid.color, param.grid.sizeX, param.grid.sizeY, param.grid.type, param.grid.transCoeff);
}









/*------------------- Example Shape Outline (only for more advanced graphics - not for basic) -------------------------------------
SHAPE_PARAMS LCD_XXX(u32 posBuff,u32 BkpSizeX,u32 BkpSizeY,u32 x,u32 y,u32 width,u32 height,u32 FrameColorStart,u32 FrameColorStop,u32 FillColorStart,u32 FillColorStop,u32 BkpColor,float ratioStart,DIRECTIONS param)
{
	SHAPE_PARAMS params = {.bkSize.w=BkpSizeX, .bkSize.h=BkpSizeY, .pos[0].x=x, .pos[0].y=y, .size[0].w=width, .size[0].h=height, .color[0].frame=FrameColorStart, .color[1].frame=FrameColorStop, .color[0].fill=FillColorStart, .color[1].fill=FillColorStop, .color[0].bk=BkpColor, .param[0]=param, .param[1]=FLOAT_TO_U32(ratioStart)};
	if(ToStructAndReturn == posBuff)
		return params;
	;

	return params;
}
void LCD_XXX_Indirect(u32 x,u32 y, u32 width,u32 height, u32 FrameColorStart,u32 FrameColorStop,u32 FillColorStart,u32 FillColorStop,u32 BkpColor,float ratioStart,DIRECTIONS direct){
	uint32_t bkSizeX = MASK(width,FFFF) +0;
	uint32_t bkSizeY = MASK(height,FFFF)+0;
	LCD_ShapeWindow(LCD_Rectangle, 0, bkSizeX,bkSizeY, 0,0, bkSizeX,bkSizeY, BkpColor,BkpColor,BkpColor );
	LCD_XXX(0,bkSizeX,bkSizeY, 0,0, width,height, FrameColorStart,FrameColorStop, FillColorStart, FillColorStop, BkpColor, ratioStart, direct);
	LCD_Display(0,x,y,bkSizeX,bkSizeY);
}
SHAPE_PARAMS LCDSHAPE_XXX(uint32_t posBuff, SHAPE_PARAMS param){
	return LCD_XXX(posBuff, param.bkSize.w, param.bkSize.h, param.pos[0].x, param.pos[0].y, param.size[0].w, param.size[0].h, param.color[0].frame, param.color[1].frame, param.color[0].fill, param.color[1].fill, param.color[0].bk, U32_TO_FLOAT(param.param[1]), param.param[0]);
}
void LCDSHAPE_XXX_Indirect(SHAPE_PARAMS param){
	LCD_XXX_Indirect(param.pos[0].x,param.pos[0].y, param.size[0].w,param.size[0].h, param.color[0].frame, param.color[1].frame, param.color[0].fill, param.color[1].fill, param.color[0].bk, U32_TO_FLOAT(param.param[1]), param.param[0]);
}
------------------- END Example Shape Outline ------------------------------------- */


/* #pragma GCC pop_options */
