/*
 * wwwPages.c
 *
 *  Created on: 23.12.2020
 *      Author: RafalMar
 */

#include "wwwPages.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include "SD_Card.h"
#include "mini_printf.h"
#include "common.h"
#include "_debug.h"
#include "variables.h"
#include "FreeRTOS.h"
#include "ff.h"
#include "timer.h"

#define MAX_SIZE_ASP_NAME	100
#define MAX_SIZE_CGI_NAME	100
#define MAX_SIZE_CGI_VAL	100
#define CGI_GET_STRING	"GET /lpc.cgi/"

extern char* GETVAL_ptr();
extern char buff[];

//SDRAM char dataBufferHTTP[HTTP_BUFFER_SIZE];
//SDRAM char dataBufferHTTP_pre[HTTP_BUFFER_SIZE_PRE];

typedef struct
{
	char a1[100];
}aaa;
SDRAM static aaa bbb[40];

void ResetTestTab(void)  //do usuniecia !!!
{
//	for(int i=0;i<40;i++)
//	{
//		bbb[i].a1[0]=0;
//	}
}

int IsValueInRangeLowHigh(int val, int low, int high)
{
	if((val>low)&&(val<high))
		return 1;
	else
		return 0;
}



static char *ParserPageHTTP(char *pName)
{
	int i;

	if((strstr(pName,"v_emailSendLogin_")))
	{
		i=atoi(pName+17);
		if(IsValueInRangeLowHigh(i,-1,40))
		{
			return bbb[i].a1;
		}
	}

	return 0;
}

static void ParserQueryHTTP(char *name, char *val)
{
	int i;
	char *pV;

	if((pV=strstr(name,"n_GPIO_val_")))
	{
		i=atoi(pV+11)-1;
		if((i>-1)&&(i<40))
		{
			mini_snprintf(bbb[i].a1,60,"Rafal %02d-%s Mark",atoi("_12_"),val);
		}
	}
	else if((pV=strstr(name,"n_109_zapisz")))
	{
		Dbg(1,"\r\n---ZAPISZ do pamieci----");
	}
}

static uint32_t ASP_Callback( char *dataIn, uint32_t dataInLen, char *dataOut)
{
	char *pS, *pE, *pD=dataIn, *pV, aspName[MAX_SIZE_ASP_NAME];
	int i,j,l,valLen,diff;
	uint32_t n=0;

	for(i=0;i<dataInLen;i++)
	{
		if((*(pD+i)=='[')&&((*(pD+i+1)=='#')))
		{
			pS=pD+i+2;
			if((pE=strstr(pS,"#]")))
			{
				diff=pE-pS;
				if(IsValueInRangeLowHigh(diff,0,MAX_SIZE_ASP_NAME))
				{
					for(j=0;j<diff;j++)
						aspName[j]=*(pS+j);
					aspName[j]=0;

					pV=ParserPageHTTP(aspName);
					if(pV!=0)
					{
						valLen=mini_strlen(pV);
						for(l=0;l<valLen;l++)
							dataOut[n++]=*(pV+l);
						i+=diff+3;
					}
					else
						dataOut[n++]=*(pD+i);
				}
				else
					dataOut[n++]=*(pD+i);
			}
			else
				dataOut[n++]=*(pD+i);
		}
		else
			dataOut[n++]=*(pD+i);
	}
	dataOut[n]=0;
	return n;
}

static void CGI_Callback (char *data)
{
	int j,diff;
	char *pS,*pE, cgiName[MAX_SIZE_CGI_NAME], cgiVal[MAX_SIZE_CGI_VAL];

	if((pS=strstr(data,CGI_GET_STRING)))
	{
		pS += mini_strlen(CGI_GET_STRING);
		while(1)
		{
			if((pE=strstr(pS,"=")))
			{
				diff=pE-pS;
				if(IsValueInRangeLowHigh(diff,0,MAX_SIZE_CGI_NAME))
				{
					for(j=0;j<diff;j++)
						cgiName[j]=*(pS+j);
					cgiName[j]=0;

					pS=pE+1;
					if((pE=strstr(pS,"&")))
					{
						diff=pE-pS;
						if(IsValueInRangeLowHigh(diff,0,MAX_SIZE_CGI_VAL))
						{
							for(j=0;j<diff;j++)
								cgiVal[j]=*(pS+j);
							cgiVal[j]=0;

							ParserQueryHTTP(cgiName,cgiVal);
							pS=pE+1;
						}
						else
							break;
					}
					else
					{
						ParserQueryHTTP(cgiName,"");
						break;
					}
				}
				else
					break;
			}
			else
				break;
		}
	}
}

DATA_TO_SEND* GetPageWWW(char *getHttpRequest)
{
	DATA_TO_SEND *temp = (DATA_TO_SEND*)pvPortMalloc(sizeof(DATA_TO_SEND));
	//char *pIn=dataBufferHTTP_pre;

	temp->state=0;

//	if (STARTUP_WaitForBits(0x0010))  //!!!!!!!!!!!!!! ACCESS_TO_SDCARD !!!!!!!!!!!!!
//	{
//		 STARTUP_ClaerBits(0x0010);
	//if(TakeMutex(Semphr_cardSD, 1000))  //!!!!!!!!! dla Semphr_sdram zawiesza sie lubi !!!!!!!!!!!!!!
	//if(TakeMutex2(Semphr_sdram, Semphr_cardSD, 1000))
	//{

//		temp->pData=GETVAL_ptr(0+HTTP_BUFFER_SIZE);
//
//		if(strstr(getHttpRequest,"GET / ")||strstr(getHttpRequest,"GET /main")){
//
//			if(TakeMutex2(Semphr_sdram, Semphr_cardSD, 1000))
//			//if(TakeMutex(Semphr_cardSD, 1000)
//			{
//				SDCardFileOpen(0,"aaa.htm",FA_READ);
//				temp->len = SDCardFileRead(0, GETVAL_ptr(0), HTTP_BUFFER_SIZE);   // SDCard_ReadFile("aaa.htm", pIn, HTTP_BUFFER_SIZE);  //website/log.htm      //MAX SIZ   ff. FIL INFO!!!!!
//				SDCardFileClose(0);
//				temp->len = ASP_Callback(GETVAL_ptr(0), temp->len, temp->pData);
//				temp->state=1;
//				GiveMutex(Semphr_cardSD);
//				GiveMutex(Semphr_sdram);
//			}
//
//		}

//		else if(strstr(getHttpRequest,"GET /favicon")){
//			SDCardFileOpen(0,"website/favicon.png",FA_READ);
//			temp->len=SDCardFileRead(0, temp->pData, HTTP_BUFFER_SIZE);//SDCard_ReadFile("website/favicon.png", temp->pData, HTTP_BUFFER_SIZE);
//			SDCardFileClose(0);
//		}

//		else if(strstr(getHttpRequest,"GET /logo.png"))
//		{
//			if(strstr(getHttpRequest,"If-None-Match: \"533225b1-12341\""))
//			{
//				Dbg(1,"\r\nXX1111");
//				temp->len = mini_snprintf(temp->pData, 110, "HTTP/1.0 304 Not Modified\r\nCache-Control: max-age=31536000\r\n\r\n");
//			}
//			else
//			{
//				temp->len = mini_snprintf(temp->pData, 110, "HTTP/1.0 200 OK\r\nEtag: \"533225b1-12341\"\r\nCache-Control: max-age=31536000\r\n\r\n");
//				SDCardFileOpen(0,"website/logo.png",FA_READ);
//				temp->len += SDCardFileRead(0, temp->pData+temp->len, HTTP_BUFFER_SIZE);//SDCard_ReadFile("website/logo.png", temp->pData+temp->len, HTTP_BUFFER_SIZE);
//				SDCardFileClose(0);
//			}
//		}
//
//		else if(strstr(getHttpRequest,"GET /flags.png"))
//		{
//			if(strstr(getHttpRequest,"If-None-Match: \"533225b1-12341\""))
//			{
//				Dbg(1,"\r\nXX2222");
//				temp->len = mini_snprintf(temp->pData, 110, "HTTP/1.0 304 Not Modified\r\nCache-Control: max-age=31536000\r\n\r\n");
//			}
//			else
//			{
//				temp->len = mini_snprintf(temp->pData, 110, "HTTP/1.0 200 OK\r\nEtag: \"533225b1-12341\"\r\nCache-Control: max-age=31536000\r\n\r\n");
//				SDCardFileOpen(0,"website/flags.png",FA_READ);
//				temp->len += SDCardFileRead(0, temp->pData+temp->len, HTTP_BUFFER_SIZE);//SDCard_ReadFile("website/flags.png", temp->pData+temp->len, HTTP_BUFFER_SIZE);
//				SDCardFileClose(0);
//			}
//		}

//		else if(strstr(getHttpRequest,"GET /TME.txt"))
//		{
//			if(TakeMutex(Semphr_sdram, 1000))
//			{
//				temp->len=mini_snprintf(temp->pData,1500,"19:22:05 &nbsp;19/06/13--0000000000000 111111111111111111  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0  --.-0 --a0       7.9       7.7   + 244    1+ 389    1+ 356    1+ 356    1+ 718    1+ 820    1+ 483    1+ 447    1   ---   0   ---   0+ 460    1   ---   0   ---   0   ---   0 V     V              0.001   KWh            0.001  0          0.001  0          0.001  0          0.001  0          0.001   KWh            0.001  0          0.001  0          0.001  0          0.001  0          0.001   KWh            0.001  0          0.001  0          0.001  0          0.001  0          0.001   KWh            0.001  0          0.001  0          0.001  0          0.001  0 ");
//				temp->state=1;
//				GiveMutex(Semphr_sdram);
//			}
//		}
//		else{
//			temp=NULL;
//		}
//		else if(strstr(getHttpRequest,CGI_GET_STRING))
//		{
//			CGI_Callback(getHttpRequest);
//			temp->len = mini_snprintf(temp->pData, 200, "HTTP/1.0 200 OK\r\nElektronika RM\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
//			temp->len += mini_snprintf(temp->pData+temp->len, 500, "<html><head><link rel=\"shortcut icon\" href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABGdBTUEAALGPC/xhBQAAACRJREFUOE9jZGj4z0AaAGogCZGmGuQckowf1UBkcI0GKxEBBQA55GigBl4jdAAAAABJRU5ErkJggg==\"><meta http-equiv=\"refresh\" content=\"0;url=../\"></head><body bgcolor=\"000000\"></body></html>");
//		}
//
//		else if(strstr(getHttpRequest,"GET /LOGIN "))
//			temp->len=mini_snprintf(temp->pData,20,"LOGIN");


		//	STARTUP_SetBits(0x0010);

//	}
//	else
//	{
//		temp->len=0;
//	}



	return temp;
}
