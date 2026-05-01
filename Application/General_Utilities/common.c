/*
 * common.c
 *
 *  Created on: Aug 31, 2024
 *      Author: maraf
 */

#include "stm32h7xx_hal.h"
#include "common.h"

#define _BUFF_VAL_SIZE		10
#define _FV_MEMORY_SIZE		30

SHAPE_PARAMS SHAPE_PARAMS_Zero={0};
structPosition structPos_Zero = {0};

int* 			_Int		(int val)		{ static int 		_intVal[_BUFF_VAL_SIZE]={0};	  static int _incIntBuffVal=-1; 	   _incIntBuffVal+1	 >=_BUFF_VAL_SIZE ? _incIntBuffVal=0	 :_incIntBuffVal++;    _intVal[_incIntBuffVal]=val; 		 return &_intVal[_incIntBuffVal]; 		 };
uint8_t*		_Uint8	(uint8_t val)	{ static uint8_t 	_uint8val[_BUFF_VAL_SIZE]={0};  static int _incUint8BuffVal=-1;   _incUint8BuffVal+1 >=_BUFF_VAL_SIZE ? _incUint8BuffVal=0  :_incUint8BuffVal++;  _uint8val[_incUint8BuffVal]=val; 	 return &_uint8val[_incUint8BuffVal]; 	 };
uint16_t*	_Uint16	(uint16_t val)	{ static uint16_t _uint16val[_BUFF_VAL_SIZE]={0}; static int _incUint16BuffVal=-1;  _incUint16BuffVal+1>=_BUFF_VAL_SIZE ? _incUint16BuffVal=0 :_incUint16BuffVal++; _uint16val[_incUint16BuffVal]=val; return &_uint16val[_incUint16BuffVal]; };
uint32_t* 	_Uint32	(uint32_t val)	{ static uint32_t _uint32val[_BUFF_VAL_SIZE]={0}; static int _incUint32BuffVal=-1;  _incUint32BuffVal+1>=_BUFF_VAL_SIZE ? _incUint32BuffVal=0 :_incUint32BuffVal++; _uint32val[_incUint32BuffVal]=val; return &_uint32val[_incUint32BuffVal]; };
int16_t*		_Int16	(int16_t val)	{ static int16_t  _int16val[_BUFF_VAL_SIZE]={0};  static int _incInt16BuffVal=-1;   _incInt16BuffVal+1 >=_BUFF_VAL_SIZE ? _incInt16BuffVal=0  :_incInt16BuffVal++;  _int16val[_incInt16BuffVal]=val; 	 return &_int16val[_incInt16BuffVal]; 	 };
int32_t* 	_Int32	(int32_t val)	{ static int32_t  _int32val[_BUFF_VAL_SIZE]={0};  static int _incInt32BuffVal=-1;   _incInt32BuffVal+1 >=_BUFF_VAL_SIZE ? _incInt32BuffVal=0  :_incInt32BuffVal++;  _int32val[_incInt32BuffVal]=val; 	 return &_int32val[_incInt32BuffVal]; 	 };
float*		_Float	(float val)		{ static float 	_floatVal[_BUFF_VAL_SIZE]={0};  static int _incFloatBuffVal=-1;   _incFloatBuffVal+1 >=_BUFF_VAL_SIZE ? _incFloatBuffVal=0  :_incFloatBuffVal++;  _floatVal[_incFloatBuffVal]=val; 	 return &_floatVal[_incFloatBuffVal]; 	 };
double*		_Double	(double val)	{ static double 	_doubleVal[_BUFF_VAL_SIZE]={0}; static int _incDoubleBuffVal=-1;  _incDoubleBuffVal+1 >=_BUFF_VAL_SIZE ? _incDoubleBuffVal=0:_incDoubleBuffVal++; _doubleVal[_incDoubleBuffVal]=val; return &_doubleVal[_incDoubleBuffVal]; };

int _ReturnVal (int val, int in)							{ return val; };
int _ReturnVal2(int val, int in1, int in2)			{ return val; };
int _ReturnVal3(int val, int in1, int in2,int in3)	{ return val; };

int _RetVal	(int in, 						int val)	{ return val; };
int _RetVal2(int in1,int in2, 			int val)	{ return val; };
int _RetVal3(int in1,int in2,	int in3, int val)	{ return val; };

structPosition RetStructPos(int x, int y){  structPosition temp={x,y};  return temp;  }

void Int16ToCharBuff(char* buff, uint16_t val){
	buff[0] = val>>8;
	buff[1] = val;
}
void Int32ToCharBuff(char* buff, uint32_t val){
	buff[0] = val>>24;
	buff[1] = val>>16;
	buff[2] = val>>8;
	buff[3] = val;
}
uint16_t CharBuffToInt16(char* buff){
	return ((buff[0]<<8) | buff[1]);
}
uint32_t CharBuffToInt32(char* buff){
	return ((buff[0]<<24) | (buff[1]<<16) | (buff[2]<<8) | buff[3]);
}

int FV(VARIABLE_ACTIONS type, int nrMem, int val){
	static int mem[_FV_MEMORY_SIZE];
	switch((int)type){
		case SetVal:
			mem[nrMem]=val;
			return mem[nrMem];
		case GetVal:
			return mem[nrMem];
		default:
			return 0;
}}
int FV2(char* descr, VARIABLE_ACTIONS type, int nrMem, int val){
	static int mem[_FV_MEMORY_SIZE];
	switch((int)type){
		case SetVal:
			mem[nrMem]=val;
			return mem[nrMem];
		case GetVal:
			return mem[nrMem];
		default:
			return 0;
}}

struct_MATH CALCULATE_MinMaxAvr(GET_SET operType, int nr, void *value, DATA_TYPE dataType)
{
	#define _SIZE_STRUCT	10
	static struct_MATH temp = {NULL};

	#define _COPY_STRUCT_TEMP(name,nr)\
		temp.max = (void*)(&name[nr].max);\
		temp.min = (void*)(&name[nr].min);\
		temp.div = (void*)(&name[nr].div);\
		temp.mid = (void*)(&name[nr].mid);\
		temp.sum = (void*)(&name[nr].sum);\
		temp.itr = (void*)(&name[nr].itr);\
		temp.avr = (void*)(&name[nr].avr)

	#define _CALC_MIN_MAX_AVR(name,nr,typData)\
		name[nr].max = MAXVAL2(name[nr].max,*((typData*)value));\
		name[nr].min = MINVAL2(name[nr].min,*((typData*)value));\
		name[nr].div = name[nr].max-name[nr].min;\
		name[nr].mid = name[nr].min+(name[nr].max-name[nr].min)/2;\
		name[nr].sum += *((typData*)value);\
		name[nr].itr += 1;\
		name[nr].avr = name[nr].sum/name[nr].itr

	#define _SET_MAX(name,nr,maxVal)\
		name[nr].max = maxVal
	#define _SET_MIN(name,nr,minVal)\
		name[nr].min = minVal

	#define _OPERAT(name,typData)\
		static struct struct_##name{ typData min,max,div,mid,sum,itr,avr; }name[_SIZE_STRUCT] = {0};\
		if(_RST==operType){\
			for(int i=0; i<_SIZE_STRUCT; ++i){\
				name[i].min = 0;\
				name[i].max = 0;\
				name[i].div = 0;\
				name[i].mid = 0;\
				name[i].sum = 0;\
				name[i].itr = 0;\
				name[i].avr = 0;\
			}\
			_COPY_STRUCT_TEMP(name,0);\
		}\
		else if(_CALC==operType){\
			_CALC_MIN_MAX_AVR(name,nr,typData);\
			_COPY_STRUCT_TEMP(name,nr);\
		}\
		else if(_GET==operType){\
			_COPY_STRUCT_TEMP(name,nr);\
		}\
		else if(_SET1==operType){\
			_SET_MIN(name,nr,*((typData*)value));\
		}\
		else if(_SET2==operType){\
			_SET_MAX(name,nr,*((typData*)value));\
		}

	switch((int)dataType){
		case _int:		_OPERAT(par1,int) 	  break;
		case _int8:		_OPERAT(par2,int8_t)   break;
		case _int16:	_OPERAT(par3,int16_t)  break;
		case _int32:	_OPERAT(par4,int32_t)  break;
		case _uint8:	_OPERAT(par5,uint8_t)  break;
		case _uint16:	_OPERAT(par6,uint16_t) break;
		case _uint32:	_OPERAT(par7,uint32_t) break;
		case _float:	_OPERAT(par8,float)    break;
		case _double:	_OPERAT(par9,double)   break;
	}
	return temp;

	#undef _SIZE_STRUCT
	#undef _COPY_STRUCT_TEMP
	#undef _CALC_MIN_MAX_AVR
	#undef _OPERAT
}

int COMPARE_2Struct(void *struct1, void *struct2, int structSize, DATA_TYPE dataType)
{
	#define _OPERAT(typData)\
		typData *ptr1##typData = (typData*)(struct1);\
		typData *ptr2##typData = (typData*)(struct2);\
		for(int i = 0; i<structSize; ++i){\
			if(*(ptr1##typData+i) != *(ptr2##typData+i))\
				return 1;\
		}\
		return 0;

	switch((int)dataType){
		case _int:		_OPERAT(int); 	 	 break;
		case _int8:		_OPERAT(int8_t);   break;
		case _int16:	_OPERAT(int16_t);  break;
		case _int32:	_OPERAT(int32_t);  break;
		case _uint8:	_OPERAT(uint8_t);  break;
		case _uint16:	_OPERAT(uint16_t); break;
		case _uint32:	_OPERAT(uint32_t); break;
		case _char:		_OPERAT(char);   	 break;
		case _float:	_OPERAT(float);    break;
		case _double:	_OPERAT(double);   break;
		default: return 0;
	}
	#undef _OPERAT
}

void SORT_Bubble(int tab[], int n){
	int i, j, temp;
	for (i = 0; i < n - 1; i++) {
		for (j = 0; j < n - i - 1; j++) {
			if (tab[j] > tab[j + 1]) {
				temp = tab[j];
				tab[j] = tab[j + 1];
				tab[j + 1] = temp;
	}}}
}

void AVR_Calc(int numbersOfSamples, int *pSamplesIn, int *pSamplesOut, int coeff){	/* 'coeff' must be odd number */
	int temp;
	if(coeff%2 == 0) coeff++;
	temp = coeff/2;
	for(int i=temp; i<numbersOfSamples-temp; ++i){
		*(pSamplesOut+i) = 0;
		for(int j=-temp; j<=temp; ++j)
			*(pSamplesOut+i) += *(pSamplesIn+i+j);
		*(pSamplesOut+i) /= coeff;
	}
}

