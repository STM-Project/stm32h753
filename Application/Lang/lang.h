/*
 * lang.h
 *
 *  Created on: 01.02.2021
 *      Author: RafalMar
 */

#ifndef LANG_H_
#define LANG_H_

//#define Polish(x)		2*(x)
//#define English(x)	2*(x)+1

typedef enum{
	Polish,
	English
}lang;

typedef struct
{
  int howMuch;
  int what;
}Language;

extern Language Lang;

void SetLang(int howMuch, int what);
char* GetSelTxt(int nrBuff, const char* txt, int line);
int GetNumberAllLinesSelTxt(const char* txt);

#endif /* LANG_H_ */
