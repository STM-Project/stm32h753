/*
 * common.h
 *
 *  Created on: 11.04.2021
 *      Author: Elektronika RM
 */

#ifndef COMMON_H_
#define COMMON_H_
#include "string_oper.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define EXAMPLE(x)  TEST_##x

#define GET_DATE_COMPILATION  __DATE__
#define GET_TIME_COMPILATION  __TIME__
#define GET_CODE_FILE  __FILE__
#define GET_CODE_LINE  __LINE__
#define GET_CODE_FUNCTION  __FUNCTION__

#define GET_CODE_PLACE 	StrAll(4,"\r\n", GET_CODE_FILE, ":", GET_CODE_LINE)

#define TEXT2PRINT(message,type)		(type == 0 ? StrAll(9,"\r\n", GET_CODE_FILE, " : ", GET_CODE_FUNCTION, "() : ", Int2Str(GET_CODE_LINE,None,6,Sign_none), " \"", message,"\"") \
															  : (type == 1  ? StrAll(7,"\r\n", GET_CODE_FUNCTION, "() :", Int2Str(GET_CODE_LINE,None,6,Sign_none), " \"", message,"\"") \
																	  	  	  	 : StrAll(3,"\r\n\"", message,"\"") ))

#define getName(var)  #var
#define ABS(X)  ((X) > 0 ? (X) : -(X))
#define TOOGLE(x)  ((x)=1-(x))
#define TOOGLE_BIT(x,bit)  ( (x) = (x)&(bit) ? (x)&(~(bit)) : (x)|(bit) )
#define INCR(x,step,max)  (((x)+(step)>(max)) ? ((x)=(max)) : ((x)+=(step)))
#define DECR(x,step,min)  (((x)-(step)<(min)) ? ((x)=(min)) : ((x)-=(step)))
#define INCR_WRAP(x,step,min,max)  (((x)+(step)>(max)) ? ((x)=(min)) : ((x)+=(step)))
#define DECR_WRAP(x,step,min,max)  (((x)-(step)<(min)) ? ((x)=(max)) : ((x)-=(step)))
#define INTEGER(f)	 ((int)f)
#define FRACTION(f)	 (f-(int)f)
#define ROUND_VAL(val,fraction)	 ((FRACTION((val))>(fraction)) ? (INTEGER((val))+1) : (INTEGER((val))));
#define EVEN_INT(x)   ( (x%2) ? (x+1) : (x) )
#define TANG_ARG(deg) 0.0174532*deg
#define DEG(tang_arg) tang_arg/0.0174532

#define INCR_FLOAT_WRAP(x,step,min,max)  ((x<max-step/2) ? (x+=step) : (x=min))
#define DECR_FLOAT_WRAP(x,step,min,max)  ((x<step/2) ?     (x=max)   : (x-=step))

#define MIDDLE(startPos,widthBk,width)					((startPos)+((widthBk)-(width))/2)
#define MIDDLE_PERC(startPos,widthBk,width,perc)	((startPos)+((perc)*((widthBk)-(width)))/100)
#define IS_RANGE(val,min,max) (((val)>=(min))&&((val)<=(max)))
#define EQUAL2_OR(val,eq1,eq2)		(((eq1)==(val))||((eq2)==(val)))
#define EQUAL2_AND(val,eq1,eq2)		(((eq1)==(val))&&((eq2)==(val)))

#define SET_IN_RANGE(val,min,max) (((val)<(min))?(min):(((val)>(max))?(max):(val)))
#define SET_NEW_RANGE(val,min,max,newMin,newMax) (((val)<(min))?(newMin):(((val)>(max))?(newMax):(val)))

#define STRUCT_TAB_SIZE(_struct) (sizeof(_struct) / sizeof(&_struct[0]))		/* if every element of table or structure is regular */

#define SET_bit(allBits,bitNr)	((allBits) |=   1<<(bitNr))
#define RST_bit(allBits,bitNr)	((allBits) &= ~(1<<(bitNr)))
#define CHECK_bit(allBits,bitNr) (((allBits)>>((bitNr)>31?31:(bitNr))) & 0x00000001)

#define INIT_MAXVAL(ptr,nmb,minVal,maxValCalc)	 int maxValCalc=0; 	for(int i=0,j=(minVal); i<(nmb); (ptr[i]>j ? j=ptr[i] : 0), (maxValCalc)=j, ++i);
#define MAXVAL(ptr,nmb,minVal,maxValCalc)		for(int i=0,j=(minVal); i<(nmb); (ptr[i]>j ? j=ptr[i] : 0), (maxValCalc)=j, ++i);
#define INIT_INCVAL(nmb,val,valinc)		int val=0; for(int i=0; i<nmb; val+=valinc[i++])
#define INCVAL(nmb,val,valinc)				 val=0; for(int i=0; i<nmb; val+=valinc[i++])

#define BKCOPY_VAL(dst,src,val)	dst=src; src=val
#define BKCOPY(dst,src)	dst=src

#define SDRAM 			__attribute__ ((section(".sdram")))
#define SDRAM_ALIGN32 	__attribute__ ((section(".sdram"), aligned(32)));		/* ALIGN_32BYTES() */

#define PTR2CHAR(txt,ptr)	char *ptr
#define PTR2INT(txt,ptr)	int *ptr

#define LCD_BUFF_XSIZE		800
#define LCD_BUFF_YSIZE		480
#define XY(x,y) 	x,y

#define MASK(val,hexMask)		((val)&0x##hexMask)
#define SHIFT_RIGHT(val,shift,hexMask)		(((val)>>(shift))&0x##hexMask)
#define SHIFT_LEFT(val,val2,shift)		((val)|((val2)<<(shift)))
#define DO_NOTHING(x)	((x)=(x))

#define PERCENT_SCALE(val,maxVal)	(((val)*100)/(maxVal))
#define VALPERC(val,perc)		(((perc)*(val))/100)

/* use only in thread 'LCD display' */
#define pCHAR_PLCD(offs)	((char*)(pLcd+(offs)))
#define pUINT8_PLCD(offs)	((uint8_t*)(pLcd+(offs)))
#define pUINT16_PLCD(offs)	((uint16_t*)(pLcd+(offs)))
#define pINT_PLCD(offs)		((int*)(pLcd+(offs)))

#define vCHAR_PLCD(offs)	(*((char*)(pLcd+(offs))))
#define vUINT8_PLCD(offs)	(*((uint8_t*)(pLcd+(offs))))
#define vUINT16_PLCD(offs)	(*((uint16_t*)(pLcd+(offs))))
#define vINT_PLCD(offs)		(*((int*)(pLcd+(offs))))

#define INIT(name,val)	int name=val
#define CONDITION(condition,val1,val2)	((condition) ? (val1) : (val2))
#define MAXVAL2(value1,value2)	((value1) >= (value2) ? (value1) : (value2))
#define MINVAL2(value1,value2)	((value1) <= (value2) ? (value1) : (value2))
#define SET_VAL(val,to1,to2)		to1=val; to2=val

#define STRUCT_SIZE_SHAPE_POS		3

#define FUNC_MAIN_INIT		int argNmb, char **argVal
#define FUNC_MAIN_ARG		argNmb,argVal

#define TXT_CUTTOFF(txt,len)	if(strlen(txt)>len-1) *((txt)+(len-1))=0

#define LOOP_FOR(cnt,max) 		for(int (cnt)=0;(cnt)<(max);++(cnt))
#define LOOP_INIT(cnt,init,max) 	for(int (cnt)=(init);(cnt)<(max);++(cnt))
#define LOOP_FOR2(cnt,max,step) 	for(double (cnt)=0;(cnt)<(max);cnt+=step)

#define PARAM32(a,b,c,d)	((a)<<24|(b)<<16|(c<<8)|(d))

#define U32_TO_FLOAT(val)	(*((float*)(&(val))))			/* problem with -0fast optimize */
#define FLOAT_TO_U32(val)	(*((uint32_t*)(&(val))))

#define U64_TO_DOUBLE(val)	(*((double*)(&(val))))
#define DOUBLE_TO_U64(val)	(*((uint64_t*)(&(val))))

#define _ZERO 0
#define _ZEROS2 _ZERO,_ZERO
#define _ZEROS3 _ZEROS2,_ZERO
#define _ZEROS4 _ZEROS3,_ZERO
#define _ZEROS5 _ZEROS4,_ZERO
#define _ZEROS6 _ZEROS5,_ZERO
#define _ZEROS7 _ZEROS6,_ZERO
#define _ZEROS8 _ZEROS7,_ZERO
#define _ZEROS9 _ZEROS8,_ZERO
#define _ZEROS10 _ZEROS9,_ZERO
#define _ZEROS11 _ZEROS10,_ZERO
#define _ZEROS12 _ZEROS11,_ZERO

#define BIT_1	1<<0
#define BIT_2	1<<1
#define BIT_3	1<<2
#define BIT_4	1<<3
#define BIT_5	1<<4
#define BIT_6	1<<5
#define BIT_7	1<<6
#define BIT_8	1<<7
#define BIT_9	1<<8
#define BIT_10	1<<9
#define BIT_11	1<<10
#define BIT_12	1<<11
#define BIT_13	1<<12
#define BIT_14	1<<13
#define BIT_15	1<<14
#define BIT_16	1<<15

#define _1SPACE 			" "
#define _2SPACE 			"  "
#define TABU_1SPACE 		"	 "
#define TABU_2SPACE 		"	  "

#define _DESCR(txt,var)		var
#define _NOT_USED(var)	var=var

#define OPTIMIZE_FAST	__attribute__ ((optimize("-Ofast")))
#define OPTIMIZE_SIZE	__attribute__ ((optimize("-Os")))
#define OPTIMIZE_DBG		__attribute__ ((optimize("-Og")))

#define ALIGN_TO_32BIT(val)		while(((val)%4)!=0) (val)++;
#define _NOP		asm("nop")

#define _MEGA		1000000

__STATIC_FORCEINLINE uint32_t __get_LR(void){	register uint32_t result;		__ASM volatile ("MOV %0, lr\n" : "=r" (result) );		return(result);	}
__STATIC_FORCEINLINE uint32_t __get_R0(void){	register uint32_t result;		__ASM volatile ("MOV %0, r0\n" : "=r" (result) );		return(result);	}
__STATIC_FORCEINLINE uint32_t __get_R1(void){	register uint32_t result;		__ASM volatile ("MOV %0, r1\n" : "=r" (result) );		return(result);	}
__STATIC_FORCEINLINE uint32_t __get_R2(void){	register uint32_t result;		__ASM volatile ("MOV %0, r2\n" : "=r" (result) );		return(result);	}
__STATIC_FORCEINLINE uint32_t __get_R3(void){	register uint32_t result;		__ASM volatile ("MOV %0, r3\n" : "=r" (result) );		return(result);	}
__STATIC_FORCEINLINE uint32_t __get_R4(void){	register uint32_t result;		__ASM volatile ("MOV %0, r4\n" : "=r" (result) );		return(result);	}
__STATIC_FORCEINLINE uint32_t __get_R15(void){	register uint32_t result;		__ASM volatile ("MOV %0, r15\n" : "=r" (result) );		return(result);	}

typedef enum{
	unUsed,
	_NO,
	_YES
}GENERAL_PARAM;

typedef enum{
	LoadWholeScreen,
	LoadPartScreen,
	LoadUserScreen,
	LoadNoDispScreen
}LOAD_SCREEN;

typedef enum{
	_int,
	_int8,
	_int16,
	_int32,
	_uint8,
	_uint16,
	_uint32,
	_char,
	_float,
	_double,
}DATA_TYPE;

typedef enum{ _Incr,  _Decr,  _Wrap }DATA_ACTION;
typedef enum{ _ON,    _OFF  }ON_OFF;
typedef enum{ _START, _STOP, _RESET, _RESTART }START_STOP;
typedef enum{ _MIN,   _MID,  _MAX }MAX_MIN;
typedef enum{ _GET=-16,_GET1,_GET2,_GET3, _SET,_SET1,_SET2,_SET3, _CALC,_CALC1,_CALC2,_CALC3, _RST,_RST1,_RST2,_RST3 }GET_SET;

typedef enum{
	NoDirect = -1,
	Up,
	Down,
	Down2,
	Right,
	Right2,
	RightDown,
	RightUp,
	Left,
	Left2,
	LeftUp,
	LeftDown,
	outside,
	inside,
	inside2,
	DownUp,
	LeftLeft,
	RightRight,
	Horizontal,
	Vertical,
	Middle,
	Middle2,
	Midd_X,
	Midd_Y,
	Midd_X2,
	Midd_Y2,
	AllEdge,
	AllEdge2,
	Shade,
	Round,
	Center,
}DIRECTIONS;

typedef struct{
	char *pData;
	uint32_t len;
	u8 state;
}DATA_TO_SEND;

typedef struct{ void *min,*max,*div,*mid,*sum,*itr,*avr; }struct_MATH;

typedef struct{ int x; int y; }structPosition;
typedef struct{ u16 x; u16 y; }structPosU16;
typedef struct{ int w; int h; }structSize;
typedef struct{ u16 w; u16 h; }structSizeU16;

typedef struct{
	int frame;
	int fill;
	int bk;
}structColor;

typedef struct{
	structSize 		bkSize;
	structPosition pos[STRUCT_SIZE_SHAPE_POS];
	structSize 		size[STRUCT_SIZE_SHAPE_POS];
	structColor 	color[STRUCT_SIZE_SHAPE_POS];
	uint32_t 		param[STRUCT_SIZE_SHAPE_POS];
	char           *ptr[STRUCT_SIZE_SHAPE_POS];
}SHAPE_PARAMS;

typedef struct{
	structPosition pos;
	structSize 		size;
}POS_SIZE;

extern SHAPE_PARAMS SHAPE_PARAMS_Zero;
extern structPosition structPos_Zero;

/* ----------- Functions Definitions ----------- */
typedef void (*WindowFunc)(uint16_t,uint16_t,uint16_t,uint16_t);

typedef void (*figureShape)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);

typedef int FUNC_Keyboard(uint8_t,uint8_t,figureShape,uint8_t,uint16_t,uint16_t,uint16_t,uint16_t,uint8_t,uint16_t,uint16_t,uint8_t);
#define FUNC1_SET(func,a,b,c,d,e,f,g,h,i,j,k,l)		func,a,b,c,d,e,f,g,h,i,j,k,l
#define FUNC1_DEF(pfunc)		FUNC_Keyboard *pfunc,uint8_t a,uint8_t b,figureShape c,uint8_t d,uint16_t e,uint16_t f,uint16_t g,uint16_t h,uint8_t i,uint16_t j,uint16_t k,uint8_t l
#define FUNC1_ARG		a,b,c,d,e,f,g,h,i,j,k,l

typedef void VOID_FUNCTION(void);
typedef SHAPE_PARAMS (*ShapeFunc)(uint32_t,SHAPE_PARAMS);
typedef void FUNC_MAIN(int, char**);
typedef void TOUCH_FUNC(int);
typedef void TIMER_FUNC(int,int,int);

typedef void* (*VOID_FUNCTION_TEST)(void*,void*);
/* --------- End Functions Definitations ----------- */

int* 			_Int		(int 	val);
uint8_t* 	_Uint8	(uint8_t val);
uint16_t* 	_Uint16	(uint16_t val);
uint32_t* 	_Uint32	(uint32_t val);
int16_t* 	_Int16	(int16_t val);
int32_t* 	_Int32	(int32_t val);
float* 		_Float	(float val);
double* 		_Double	(double val);

int _ReturnVal(int val, int in);
int _ReturnVal2(int val, int in1,int in2);
int _ReturnVal3(int val, int in1,int in2,int in3);

int _RetVal(int val, int in);
int _RetVal2(int val, int in1,int in2);
int _RetVal3(int val, int in1,int in2,int in3);

structPosition RetStructPos(int x, int y);

void Int16ToCharBuff(char* buff, uint16_t val);
void Int32ToCharBuff(char* buff, uint32_t val);
uint16_t CharBuffToInt16(char* buff);
uint32_t CharBuffToInt32(char* buff);
int FV(VARIABLE_ACTIONS type, int nrMem, int val);							/* global access to memory, be careful for multi-threaded systems, for each thread use another nrMem */
int FV2(char* descr, VARIABLE_ACTIONS type, int nrMem, int val);

struct_MATH CALCULATE_MinMaxAvr (GET_SET operType, int nr, void *value, DATA_TYPE dataType);
int 			COMPARE_2Struct	  (void *struct1, void *struct2, int structSize, DATA_TYPE dataType);
void 			SORT_Bubble			  (int tab[], int n);
void 			AVR_Calc				  (int numbersOfSamples, int *pSamplesIn, int *pSamplesOut, int coeff);



#endif /* COMMON_H_ */
