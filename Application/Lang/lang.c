/*
 * lang.c
 *
 *  Created on: 01.02.2021
 *      Author: RafalMar
 */
#include "lang.h"
#include <string.h>

//############# -- ReadPanel -- ###################################


//####################################################################

#define MAX_BUFTEMP	500

static char bufTemp[MAX_BUFTEMP];

Language Lang;

void SetLang(int howMuch, int what)
{
	Lang.howMuch=howMuch;
	Lang.what=what;
}

char* GetSelTxt(int nrBuff, const char* txt, int line)
{
	int i, j;
	const char *ptr=txt;

	for (i=0; i<line; ++i)
	{
		for (j=0; j<Lang.howMuch; ++j)
			ptr=strstr(ptr, ",")+1;
	}
	for (i=0; i<Lang.what; ++i)
		ptr=strstr(ptr, ",")+1;

	i=0;
	while (ptr[i]!=',')
	{
		if (nrBuff+i>MAX_BUFTEMP-1)
			break;
		bufTemp[nrBuff+i]=ptr[i];
		i++;
	}
	bufTemp[nrBuff+i]=0;
	return &bufTemp[nrBuff];
}

int GetNumberAllLinesSelTxt(const char* txt)
{
	int i, k=0, len=strlen(txt);

	for (i=0; i<len; ++i)
	{
		if (txt[i]==',')
			k++;
	}
	return k/Lang.howMuch;
}
