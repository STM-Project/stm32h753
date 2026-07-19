/*
 * wifi.c
 *
 *  Created on: 11.12.2020
 *      Author: RafalMar
 */
#include "esp32wroom.h"
#include "wwwPages.h"
#include <string.h> /* memset */
#include <stdlib.h> /* atoi */
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include "FreeRTOS.h"
#include "task.h"
#include "_debug.h"
#include "mini_printf.h"
#include "main.h"
#include "SD_Card.h"
#include "_smtp.h"
#include "variables.h"
#include "float.h"
//#include "def.h"
#include "sntp_dns.h"
#include "timer.h"
#include "usart.h"

#include "tim.h"

#define PAUSE_BETWEEN_SEND_RECV_MS		3000

#define ESP_RECV_BUFF_SIZE		4096
#define PACKET_SEND_LEN 		2048

#define HTTP_ANSWER_DELAY_MS		500
#define SMTP_CONNECTION_DELAY_MS	15000
#define SMTP_ANSWER_DELAY_MS		10000
#define CONNECTION_TIMEOUT_MS		30000
#define TCP_SERVER_TIMEOUT_S		10
#define SNTP_SERVER_TIMEOUT_MS		5000
#define DNS_SERVER_TIMEOUT_MS		15000
#define SNTP_NMBR_QUERY			5

#define _S_		Ita_ Gr1_
#define _SE_	Ita_ Cya_
#define _E_		_X_

#define ESP_ON 		HAL_GPIO_WritePin(ESP_EN_GPIO_TYPE, ESP_EN_GPIO_PIN, GPIO_PIN_SET)
#define ESP_OFF		HAL_GPIO_WritePin(ESP_EN_GPIO_TYPE, ESP_EN_GPIO_PIN, GPIO_PIN_RESET)

#define HTML_TXT_CODE		"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>ESP32 SSL 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789</h1></body></html>"
#define ESP32_DOMAIN_ERROR 	"\r\nNOT updated email Server IP "
#define TXT_OK				"\r\nOK\r\n"
#define TXT_ERR				"ERROR"
#define EMAIL_ERROR			" --- email ERROR --- "
#define RecvFromEsp(txt)   strstr_(NULL,txt) 		/* alternatywnie memcmp() strnstr()  */

#define _GET_REP_CASE_			CASE_Service(-3,NULL,NULL,0)-1
#define _CLR_REP_CASE_			CASE_Service(-4,NULL,NULL,0)
#define _GET_ACTUAL_CASE_		CASE_Service(-1,NULL,NULL,0)
#define _SET_NEW_CASE_(x)		CASE_Service(x,NULL,NULL,0); _CLR_REP_CASE_			/* _SET_NEW_CASE_() increment repeat case so we must call _CLR_REP_CASE_ after _SET_NEW_CASE_() */
#define _SET_NEXT_CASE_			CASE_Service(-2,NULL,NULL,0)
#define _THE_SAME_CASE_			CASE_Service((_GET_ACTUAL_CASE_-1),NULL,NULL,0)		/* CASE_Service() increment actual case */
#define _GET_ANSW_CASE_			CASE_Service(-5,NULL,NULL,0)
#define _CLR_ACTUAL_CASE_		CASE_Service(-6,NULL,NULL,0)

#define BIT_ESP_SRV			(1<<0)
#define BIT_DBG_SRV			(1<<1)

#define WAIT_ON_SEND_TIMEOUT_TIME_MS 	5000

typedef enum
{
	_OK=1, _ERROR
} ANSWER_TYPE;

typedef enum
{
	INIT_CONNECTION, HTTP_CONNECTION, SMTP_CONNECTION, TEST_CONNECTION
} CONNECTION_TYPE;

typedef enum
{
	ESP_WIFI_CONNECTED,
	ESP_WIFI_DISCONNECT,
	ESP_WIFI_GOT_IP,
	ESP_WIFI_GOT_IPv6_LL,
	ESP_WIFI_GOT_IPv6_GL,
	ESP_WIFI_FORCE_RESTART,
	ESP_PLUS_TIME_UPDATED,
	ESP_READY
}ESP32_FREE_ANSWER;			/* asynchroniczne komunikaty */

typedef enum
{
	ESP_CONNECTION_OK,
	ESP_CONNECTION_TIMEOUT,
	ESP_WRONG_PASSWORD,
	ESP_CANNOT_FIND_THE_TARGET_AP,
	ESP_CONNECTION_FAILED,
	ESP_UNKNOW_ERROR_OCCURRED
}CODES_FOR_ESP_CONNECTION;

typedef enum
{
	WIFI_MODE_DISABLED,
	WIFI_MODE_STA,
	WIFI_MODE_AP,
	WIFI_MODE_AP_STA
}WIFI_MODE;

typedef enum
{
	ESPANSWER_OK=0,
	ESPANSWER_HAL_ERROR_SEND_COMM=1,
	ESPANSWER_HAL_ERROR_SEND_DATA=3,
	ESPANSWER_TIMEOUT_TO_GET_SIGN=2,
	ESPANSWER_TIMEOUT_TO_GET_SEND_OK=4,
	ESPANSWER_ERROR_AFTER_SEND_COMM=6,
	ESPANSWER_ERROR_AFTER_SEND_DATA=8,
	ESPANSWER_CLOSED_AFTER_SEND_COMM=5,
	ESPANSWER_CLOSED_AFTER_SEND_DATA=7,
	ESPANSWER_TIMEOUT_FOR_SMTPCOMM_TO_GET_SEND_OK=104,
	ESPANSWER_TIMEOUT_FOR_SMTPCOMM_TO_GET_SIGN=102

}ESP_ANSWER;

static const char *freeAnswerTypes[] = {
    "WIFI CONNECTED",
    "WIFI GOT IP",
	"WIFI DISCONNECT",
	"Will force to restart",
	"+TIME_UPDATED",
	"+STA_CONNECTED:",		/* +STA_CONNECTED:"a4:45:19:6a:7f:5a" */
	"+DIST_STA_IP:",		/* +DIST_STA_IP:"a4:45:19:6a:7f:5a","192.168.7.2" */
};

const static char txt_OK[]  =TXT_OK;
const static char txt_ERR[] =TXT_ERR;

extern UART_HandleTypeDef ESP_UART_HANDLE;
extern DMA_HandleTypeDef ESP_UART_DMA_RX;

struct HTTP_SEND_TEMP{
	u8 	  chnl;							/* actual channel wait for SEND OK or CLOSED */
	char* ptr[ESP_MAX_HTTP_CONN];		/* ptr to html */
 	u8 	  que[ESP_MAX_HTTP_CONN];		/* buffer of requests to send channel */
 	u16   nr[ESP_MAX_HTTP_CONN];		/* iterix of web HTML */
}httpPar;

TimerHandle_t xWaitOnSendTimeoutTimer;

static char* pMem=NULL;
static int DBG = 1;
static uint8_t connectionType = INIT_CONNECTION;
static xTaskHandle vtaskWifiHandle=NULL;
static int resetDMA=0;

RAM_D2_ALIGN32 static char RecvBuffer[ESP_RECV_BUFF_SIZE];			/* ESP_RECV_BUFF_SIZE musi byc wielokrotnoscia 32 jesli uzylem _ALIGN32 */
RAM_D2_ALIGN32 static char sendBuff[PACKET_SEND_LEN];
static int recvByteFromEsp=0, recvByteFromEsp_copy=0;
static int read_pos=0;
static TickType_t tickCnt=0;

static void UpdateReadPos(void){ read_pos = recvByteFromEsp_copy; 		 }
static void GetNewReadPos(void){ recvByteFromEsp_copy = recvByteFromEsp; }
static void ClearReadPos (void){ recvByteFromEsp_copy=0;  read_pos=0; 	 }
static int  BytesToRead  (void){ return (recvByteFromEsp_copy-read_pos); }

static void  IncPos(int* pos) 		   { *pos=(*pos+1)   &(ESP_RECV_BUFF_SIZE-1); }
static void  DecPos(int* pos) 		   { *pos=(*pos-1)   &(ESP_RECV_BUFF_SIZE-1); }
static void  SetPos(int* pos, int offs){ *pos=(*pos+offs)&(ESP_RECV_BUFF_SIZE-1); }
static u32   GetPos(char* ptr)		   { return (ptr-RecvBuffer); 			  	  }

static char* strstr_(char* pBuff, const char* pattern)							/* Algorytm Knutha-Morrisa-Pratta (KMP) idealnie rozwiązuje problem cofania się w buforze kołowym. Zamiast cofać wskaźnik bufora (read_pos_copy), KMP wykorzystuje tablicę przesunięć (tzw. tablicę LPS – Longest Proper Prefix which is also Suffix).Dzięki temu wskaźnik bufora porusza się tylko do przodu, co czyni algorytm znacznie wydajniejszym i prostszym w obsłudze buforów pierścieniowych */
{
    int i=0, read_pos_copy=CONDITION(pBuff,GetPos(pBuff),read_pos), match_start = read_pos_copy;	char* ptr=NULL;
	while (read_pos_copy != recvByteFromEsp_copy){
        if(RecvBuffer[read_pos_copy]==*(pattern+i)){
        	if(i==0){  match_start=read_pos_copy;  ptr=&RecvBuffer[read_pos_copy]; }
        	i++;
        	if(*(pattern+i)=='\0') return ptr;
        	read_pos_copy=(read_pos_copy+1) & (ESP_RECV_BUFF_SIZE-1)/*% ESP_RECV_BUFF_SIZE*/;
        }
        else {
        	if(i>0){  read_pos_copy=(match_start+1)   & (ESP_RECV_BUFF_SIZE-1)/*% ESP_RECV_BUFF_SIZE*/;  i=0; }
        	else   {  read_pos_copy=(read_pos_copy+1) & (ESP_RECV_BUFF_SIZE-1)/*% ESP_RECV_BUFF_SIZE*/;       }   /* zamien na:  & (ESP_RECV_BUFF_SIZE-1); bedzie szybszy ale ESP_RECV_BUFF_SIZE musi byc potega 2 */
        }
    }
	return NULL;
}

static int strcpy_(char* buff, char* ptr, int offs, char endSign)
{
	int i = GetPos(ptr);
	int j=0;

	if(offs) SetPos(&i,offs);

	while(RecvBuffer[i] != endSign)
	{
		*(buff+j++) = RecvBuffer[i];
		IncPos(&i);
	}
	*(buff+j) = '\0';
	return j;
}

static int strcpy2_(char* buff, char* ptr, int offs, u16 size)
{
	int i = GetPos(ptr);
	int j=0;

	if(offs) SetPos(&i,offs);

	for(j=0; j<size; ++j)
	{
		*(buff+j) = RecvBuffer[i];
		IncPos(&i);
	}
	*(buff+j) = '\0';
	return j;
}

int atoi_(char* ptr, int offs)		/* ptr+offs  musi wskazywac na pierwsza liczbę */
{
	char buff[20]={0};
	int i = GetPos(ptr);
	int j=0;

	if(offs) SetPos(&i,offs);

	while(ISDIGITAL(RecvBuffer[i]))
	{
		buff[j++] = RecvBuffer[i];
		if(j==sizeof(buff)-1) break;
		IncPos(&i);
	}
	*(buff+j) = '\0';
	return atoi(buff);
}
/*
static void TEST_CircBuff(void)
{
	LOOP_FOR(i,2048){
		RecvBuffer[i++]='A';
		RecvBuffer[i++]='B';
		RecvBuffer[i++]='C';
		RecvBuffer[i]='D';
	}

	RecvBuffer[1]='A';
	RecvBuffer[2]='B';
	RecvBuffer[3]='C';
	RecvBuffer[4]='D';
	RecvBuffer[5]='E';

	read_pos=7;
	recvByteFromEsp_copy=6;

	char temp[100]={0}, *ptr=NULL;
	INIT_BUFF(answer,"ABCDE");
	StartMeasureTime_us();
	if((ptr=strstr_(answer))!=NULL)		// przeszukanie calego bufora dla tego wzorca zajmuje max 274 us
	{
		strcpy2_(temp,ptr,mini_strlen(answer),4);
		//int zmmm = atoi_(ptr,mini_strlen(answer));
		asm("nop");
	}
	StopMeasureTime_us("\r\nTEST: ");
}

static void TEST_QueLog(void)
{
	vTaskDelay(1000);
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789a");
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789ab");
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abc");
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcd");
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcde");
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcdef");
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcdefgh");
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcdefghi");
	DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcdefghij");
}

static void TEST_CopyRate(void)
{
	StartMeasureTime_us();
	mini_snprintf(sendBuff,sizeof(sendBuff)-1,HTML_TXT_CODE);	// 170 us
	StopMeasureTime_us("\r\nTEST_1: ");

	StartMeasureTime_us();
	strcpy(sendBuff,HTML_TXT_CODE);		// 13 us
	StopMeasureTime_us("\r\nTEST_2: ");

	StartMeasureTime_us();
	int aaaa = mini_strlen(HTML_TXT_CODE);  // 39 us
	StopMeasureTime_us("\r\nTEST_3: ");

	StartMeasureTime_us();
	LOOP_FOR(i,aaaa) sendBuff[i]='a';
	StopMeasureTime_us("\r\nTEST_4: ");   // 43us

	StartMeasureTime_us();
	memcpy(sendBuff, HTML_TXT_CODE, aaaa);
	StopMeasureTime_us("\r\nTEST_5: ");   //13 us
}
*/
/*
static void PutLog(int on, const char *fmt, ...)
{
	if(on)
	{
		va_list va;
		va_start(va, fmt);
		int len = mini_snprintf(sendBuff,sizeof(sendBuff)-1, fmt, va);		// Niebezpieczne bo w tle DMA moze pobierac dane z sendBuff po wywolaniu SendToEsp32();
		va_end(va);
		DbgDma2(DBG,sendBuff,len);
	}
}
*/
static void DispRecvBuff(int nrItem, ARCHIVING_TYPE archType)			/* DispRecvBuff(-2,arch) - ASYN 	*/
{																		/* DispRecvBuff(<0,arch) - no count */
	char type[20]={0}, typeNm[10]={0};
	if(-2==nrItem) strcpy(typeNm,"ASYN");
	else{  		   switch(connectionType){ case INIT_CONNECTION:strcpy(typeNm,"INIT");break;  case HTTP_CONNECTION:strcpy(typeNm,"HTTP");break;  case SMTP_CONNECTION:strcpy(typeNm,"SMTP");break;  case TEST_CONNECTION:strcpy(typeNm,"TEST");break; }  }

	if(nrItem<0) mini_snprintf(type,sizeof(type)-1,"%s",typeNm);
	else		 mini_snprintf(type,sizeof(type)-1,"%s_%03d",typeNm,nrItem);

	void _DbgDma(void){
		if(read_pos < recvByteFromEsp_copy)  DbgDma2(DBG, &RecvBuffer[read_pos], recvByteFromEsp_copy-read_pos);
		else{								 DbgDma2(DBG, &RecvBuffer[read_pos], ESP_RECV_BUFF_SIZE	 -read_pos);	 if(recvByteFromEsp_copy) DbgDma2(DBG,&RecvBuffer[0],recvByteFromEsp_copy);  }
	}
		 if(arch ==archType){ DbgVarDma(DBG,100,CoG3_"\r\nRECV_%s_START:"_X_,type); _DbgDma();  DbgDma(DBG,CoG3_"\r\nRECV_STOP"_X_); }
	else if(arch2==archType){ DbgDma(DBG,"\r\n"); _DbgDma(); DbgDma(DBG,"\r\n"); }

}

static void ESP32_FreeAnswers(int whereCalled)						/* ESP32_FreeAnswers(1) - display the origin asynch mess only if it`s any freeAnswerType */
{																	/* ESP32_FreeAnswers(0) - no display the origin asynch mess */
	char *ptr=NULL;		int flag=1,len=0;							/* ESP32_FreeAnswers(2) - display always the origin asynch mess regardless if it`s any freeAnswerType */
	if(2==whereCalled) DispRecvBuff(-2,arch);
	LOOP_FOR(i,PTR_TAB_SIZE(freeAnswerTypes)){
		if((ptr=strstr_(NULL,freeAnswerTypes[i]))){	if(flag&&1==whereCalled){ DispRecvBuff(-2,arch); flag=0; }
			if(strstr(freeAnswerTypes[i],"+STA_CONNECTED:")||strstr(freeAnswerTypes[i],"+DIST_STA_IP:")){
				char buf[100]={0};
				len += strcpy_(buf,ptr,0,'\r') + 2;
				DbgVarDma(DBG,150,_SE_"\r\n%s -%d- "_E_,buf,BytesToRead());
			}
			else{ DbgVarDma(DBG,150,_SE_"\r\n%s -%d- "_E_,freeAnswerTypes[i],BytesToRead());   len+=mini_strlen(freeAnswerTypes[i])+2; }
	}}
	if(BytesToRead()==len && len)  read_pos = (read_pos+len) & (ESP_RECV_BUFF_SIZE-1);		/* Zmień pozycje odczytu bufora kołowego jeżeli przed komunikatami asynchronicznymi nie było innego rodzaju komunikatu */
}

static void RstTimeBtwnSendRcv(void){
	tickCnt = xTaskGetTickCount();
}
static int GetTimeBtwnSendRcv(void){
	return CONDITION( xTaskGetTickCount()-tickCnt > PAUSE_BETWEEN_SEND_RECV_MS, 1, 0 );
}

static int SendToEsp32(int len, char *data, ARCHIVING_TYPE archType)								/* if data=NULL we use buffer 'sendBuff' as default. 		if len=0 we calculate length text. */
{
	int len_ = CONDITION( 0==len, mini_strlen(CONDITION(NULL==data,sendBuff,data)), len );
	if(len_ > PACKET_SEND_LEN-1)  len_=PACKET_SEND_LEN-1;
	if(data != sendBuff && data != NULL){ strncpy(sendBuff,data,len_); }	sendBuff[len_]=0;		/* memcpy(sendBuff, data, len_)  jest szybsze niż strncpy */

		 if(arch ==archType){ DbgMultiDma(DBG,CoR2_"\r\nSEND_START: "_X_,sendBuff,CoR2_"SEND_STOP\r\n"_X_); }  //TU TEZ DAJ NUMERACJE SEND_START_001 !!!!!!!!!!!!
	else if(arch2==archType){ DbgMultiDma(DBG,"\r\n",sendBuff,"\r\n"); }

	UpdateReadPos();
/*	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, PACKET_SEND_LEN);	*/							/* SCB_CleanDCache_by_Addr() takes only 4us */		/* Jesli w MPU ustawimy adres bufora 'sendBuff' w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_CleanDCache_by_Addr() nie jest potrzebny */
	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, CACHE_ALLIGN_LEN(len_)); 	 					/* Czyszczenie Cache z rozmiarem zaokrąglonym do pełnych linii 32-bajtowych, czyszczenie tylko tego fragmentu, który faktycznie wysyłamy   CACHE_LINE_BYTES = 32 */
	int result = HAL_UART_Transmit_DMA(&ESP_UART_HANDLE, (uint8_t*) sendBuff, len_);
	if(result != HAL_OK)  DbgVarDma(DBG,100,_SE_"\r\nHAL_ERROR: %d "_E_,result);
	RstTimeBtwnSendRcv();
	//xTimerStart(xWaitOnSendTimeoutTimer, 0);
	return result;
}

static int SendToEsp32_http(int len, char *data, ARCHIVING_TYPE archType)								/* if data=NULL we use buffer 'sendBuff' as default. 		if len=0 we calculate length text. */
{
	int len_ = CONDITION( 0==len, mini_strlen(CONDITION(NULL==data,sendBuff,data)), len );
	if(len_ > PACKET_SEND_LEN-1)  len_=PACKET_SEND_LEN-1;
	if(data != sendBuff && data != NULL){ strncpy(sendBuff,data,len_); }	sendBuff[len_]=0;		/* memcpy(sendBuff, data, len_)  jest szybsze niż strncpy */

		 if(arch ==archType){ DbgMultiDma(DBG,CoR2_"\r\nSEND_START: "_X_,sendBuff,CoR2_"SEND_STOP\r\n"_X_); }  //TU TEZ DAJ NUMERACJE SEND_START_001 !!!!!!!!!!!!
	else if(arch2==archType){ DbgMultiDma(DBG,"\r\n",sendBuff,"\r\n"); }

/*	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, PACKET_SEND_LEN);	*/							/* SCB_CleanDCache_by_Addr() takes only 4us */		/* Jesli w MPU ustawimy adres bufora 'sendBuff' w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_CleanDCache_by_Addr() nie jest potrzebny */
	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, CACHE_ALLIGN_LEN(len_)); 	 					/* Czyszczenie Cache z rozmiarem zaokrąglonym do pełnych linii 32-bajtowych, czyszczenie tylko tego fragmentu, który faktycznie wysyłamy   CACHE_LINE_BYTES = 32 */
	int result = HAL_UART_Transmit_DMA(&ESP_UART_HANDLE, (uint8_t*) sendBuff, len_);
	if(result != HAL_OK)  DbgVarDma(DBG,100,_SE_"\r\nHAL_ERROR: %d "_E_,result);
	RstTimeBtwnSendRcv();
	//xTimerStart(xWaitOnSendTimeoutTimer, 0);
	return result;
}

static int CASE_Service(int nrCase, const char* recv1, const char* recv2, ARCHIVING_TYPE archType)		/* CASE_Service(2,NULL,NULL,0)   set new nr case */
{																										/* CASE_Service(2,"","",0) 	     only return 3 in case 2 and only enter to this case */
	static int actualCase=0, repeatCase=0, flagCase=0;

		 if(-1==nrCase) return actualCase;							/* CASE_Service(-1,NULL,NULL,0)  get number of the actual case */
	else if(-2==nrCase){ repeatCase = 0; return ++actualCase; }		/* CASE_Service(-2,NULL,NULL,0)  set next nr case */
	else if(-3==nrCase) return repeatCase;							/* CASE_Service(-3,NULL,NULL,0)  get repat case for this case (nmbr of repeat this case) */
	else if(-4==nrCase) return (repeatCase=0);						/* CASE_Service(-4,NULL,NULL,0)  clera repeat case */
	else if(-5==nrCase) return flagCase;							/* CASE_Service(-5,NULL,NULL,0)  get last flag case for answer */
	else if(-6==nrCase) return actualCase=0;						/* CASE_Service(-6,NULL,NULL,0)  reset actualCase to '0' */

	int flag=0;
	if( recv1 == NULL && recv2 == NULL ){  actualCase = nrCase;  return ++repeatCase;  }
	if( nrCase == actualCase ){
		int hasRecv1 = (recv1 != NULL) && (strstr_(NULL,recv1) != NULL);
		int hasRecv2 = (recv2 != NULL) && (strstr_(NULL,recv2) != NULL);
	    if (hasRecv1 && hasRecv2){ actualCase++; flag=3; flagCase=3; }
	    if (hasRecv2)  			 { actualCase++; flag=2; flagCase=2; }
	    if (hasRecv1) 			 { actualCase++; flag=1; flagCase=1; }
	    if(flag){ DispRecvBuff(nrCase,archType); ESP32_FreeAnswers(0); }
	}
	return flag;
}

static int WaitForRcvEsp(const char* recv1, const char* recv2)
{
	SCB_InvalidateDCache_by_Addr((uint32_t*)RecvBuffer, ESP_RECV_BUFF_SIZE);
	 	 if( NULL!=RecvFromEsp(recv1) && NULL==RecvFromEsp(recv2) ) return 1;
	else if( NULL==RecvFromEsp(recv1) && NULL!=RecvFromEsp(recv2) ) return 2;
	else if( NULL!=RecvFromEsp(recv1) && NULL!=RecvFromEsp(recv2) ) return 3;
	else 															return 0;		/* if( NULL==RecvFromEsp(recv1) && NULL==RecvFromEsp(recv2) ) */
}

static char* COMMAND_Service(GET_SET type, char* comm)
{
	static char commBuff[200]={0};
	switch((int)type){
		case _SET:
			if(NULL!=comm) strncpy(commBuff,comm,sizeof(commBuff)-1);		/* strcpy(dest,src)   kopiuje cały napis z src do dest, włącznie ze znakiem '\0' i zwraca ptr do dest na src    analogicznie dziala   strncpy(src,dest,n) */
			commBuff[sizeof(commBuff)-1]='\0';								/* na wszelki wypadek gdy sizeof(comm) > sizeof(commBuff)-1 wtedy funkcja nie skopiuje zera do commBuff */
			break;
		case _GET:
			break;
	}
	return commBuff;
}

static int ErrorAnswerService(void)
{
	if(_GET_ANSW_CASE_==_ERROR){  DbgVarDma(DBG,200,_SE_"\r\nCMD_ERROR: %s "_E_,COMMAND_Service(_GET,NULL));  return 1; }
	return 0;
}

void ESP32_Notify2EspThread(int interruptSrc, uint16_t size, long *pxWoken)					/* size: ile zostalo wolnego miejsca w buforze DMA,  size=0 to bufor DMA calkowice zapelniony */
{
/*  vTaskNotifyGiveFromISR(vtaskWifiHandle, pxWoken);	*/									/* Wyślij powiadomienie bezpośrednio do wątku */
	if(vtaskWifiHandle!=NULL){
		if(interruptSrc==0){  recvByteFromEsp = ESP_RECV_BUFF_SIZE - size;
							  xTaskNotifyFromISR( vtaskWifiHandle,BIT_ESP_SRV,eSetBits,pxWoken );  }
		else			   {  xTaskNotifyFromISR( vtaskWifiHandle,BIT_DBG_SRV,eSetBits,pxWoken );  }
	}
}

void DefaultSettingsWIFI(void)
{
	int i;
	for (i=0; i<WIFI_AP_MAX; ++i)
	{
		VAR_SetVal64(Const_wifiAP_mac, i, 0x1122334455);
		VAR_SetTabVal(Const_wifiAP_ip, i, LWIP_MAKEU32(192,168,7,1));
		VAR_SetTabVal(Const_wifiAP_mask, i, LWIP_MAKEU32(255,255,255,0));
		VAR_SetTabVal(Const_wifiAP_gate, i, LWIP_MAKEU32(192,168,7,1));
		VAR_SetTabVal(Const_wifiAP_port, i, 80);  //WYprobowac port 8080 !!!!!
		VAR_SetTabVal(Const_wifiAP_dhcp, i, 1);
		VAR_SetStr(Const_wifiAP_name, i, "NazwaAPPPPX");
		VAR_SetStr(Const_wifiAP_pass, i, "markielowski123");
	}

	for (i=0; i<WIFI_STA_MAX; ++i)
	{
		VAR_SetVal64(Const_wifiSTA_mac, i, 0x0c0078f505);
		VAR_SetTabVal(Const_wifiSTA_ip, i, LWIP_MAKEU32(192,168,2,93));
		VAR_SetTabVal(Const_wifiSTA_mask, i, LWIP_MAKEU32(255,255,255,0));
		VAR_SetTabVal(Const_wifiSTA_gate, i, LWIP_MAKEU32(192,168,2,1));
		VAR_SetTabVal(Const_wifiSTA_port, i, 80);
		VAR_SetTabVal(Const_wifiSTA_dhcp, i, 1);
		VAR_SetStr(Const_wifiSTA_name, i, "T-Mobile_Swiatlowod_8638");
		VAR_SetStr(Const_wifiSTA_pass, i, "03109069984530029251");
//		VAR_SetStr(Const_wifiSTA_name, i, "MetronicAKP");
//		VAR_SetStr(Const_wifiSTA_pass, i, "1qaZ@MetronicZ3");
	}
	VAR_SetTabVal(Const_wifiGeneral_nrAP,NO_TAB,0);
	VAR_SetTabVal(Const_wifiGeneral_nrSTA,NO_TAB,0);
	VAR_SetTabVal(Const_wifiGeneral_mode,NO_TAB,WIFI_MODE_AP_STA);
}

static int GetAnswerDelay(void)
{
	switch (connectionType)
	{
	case SMTP_CONNECTION:
		switch (EmailSendParam.start)
		{
		case 3:
			return SMTP_CONNECTION_DELAY_MS;
		case 4:
		default:
			return SMTP_ANSWER_DELAY_MS;
		}
	case HTTP_CONNECTION:
	default:
		return HTTP_ANSWER_DELAY_MS;
	}
}

static void StartDMA(void)																	/* Jesli w tym momencie przyjdzie jakis komunikat asynchroniczny z ESP32 to NIGDY go nie odczytam bo wyczyszcze go. Rozwiazaniem jest Circle DMA */
{
	memset(RecvBuffer, 0, ESP_RECV_BUFF_SIZE);												/* memset() takes 18us */
	SCB_CleanDCache_by_Addr((uint32_t*)RecvBuffer, ESP_RECV_BUFF_SIZE);						/* Wypchnij bufor RecvBuffer z casha do RAMu by wyczyscic pamiec DMA */
	UART_ClearFlags(&ESP_UART_HANDLE);
	ClearReadPos();
	HAL_StatusTypeDef status = HAL_UART_Receive_DMA(&ESP_UART_HANDLE, (uint8_t*) RecvBuffer, ESP_RECV_BUFF_SIZE);
	if (status != HAL_OK) {
	    asm("BKPT 0");
	}
	UART_ClearFlags2(&ESP_UART_HANDLE);
}

static void RestartDMA(void)
{
	HAL_UART_DMAStop(&ESP_UART_HANDLE);
	StartDMA();
}

static void ChangeUartBuadRate(int baudRate)
{
	HAL_UART_DMAStop(&ESP_UART_HANDLE);

	HAL_UART_MspDeInit(&ESP_UART_HANDLE);
	HAL_UART_MspInit(&ESP_UART_HANDLE);

	ESP_UART_HANDLE.Instance= ESP_UART_TYPE;
	ESP_UART_HANDLE.Init.BaudRate=baudRate;
	if (HAL_UART_Init(&ESP_UART_HANDLE)!=HAL_OK)
	{
		Error_Handler();
	}
	StartDMA();
}

static void DisplayRequestGET(char *pBuf, int bytesDisp)
{
	int offs=10;
	if(bytesDisp+offs<sizeof(sendBuff))
	{
		StrBuffCopylimit(sendBuff, pBuf-offs, bytesDisp+offs);
		Dbg(DBG, "\r\n");
		Dbg(DBG, sendBuff);
	}
}

void vTestATcommand(void)  //+DST do czasu    //AT+SYSTEMP?
{
	SendToEsp("AT+SYSTIMESTAMP?\r\n");  //rozlaczyc riuter i sprawdzic   //AT+MDNS=1,"espressif","_iot",8080
	while (RecvFromEsp("\r\nOK")==0)
		vTaskDelay(1);
	Dbg(DBG, RecvBuffer);
}

void ESP_Send(char *txtAT){ SendToEsp(txtAT);  }
void ESP_Recv(void)		  { Dbg(1,RecvBuffer); }

void vDNSdomain(void)
{
	SendToEsp("AT+CIPDOMAIN=\"smtp.poczta.onet.pl\"\r\n");
	while (RecvFromEsp("\r\nOK")==0)
		vTaskDelay(1);
	Dbg(DBG, RecvBuffer);
}

void vRestoreESP(void)
{
	SendToEsp("AT+RESTORE\r\n");
	while (RecvFromEsp("\r\nOK")==0)
		vTaskDelay(1);
	Dbg(DBG, RecvBuffer);
}

static void GetSizeAndChannel(char *pBuf, int *channel, int *size)
{
	int i=1, j;
	*channel=0;
	*size=0;
	while ((*(pBuf-i)>0x2F)&&(*(pBuf-i)<0x3A))
	{
		switch (i)
		{
		case 2:
			j=10;
			break;
		case 3:
			j=100;
			break;
		case 4:
			j=1000;
			break;
		default:
			j=1;
			break;
		}
		*size+=((*(pBuf-i))&0x0F)*j;
		i++;
	}
	i++;
	*channel=((*(pBuf-i))&0x0F);
}

static int GetDMACountByte(void)
{
	return ESP_RECV_BUFF_SIZE-__HAL_DMA_GET_COUNTER(&ESP_UART_DMA_RX);
}

static int vSendDataPacket(char *pData, int packetLen, int channel)
{
	char tempBuff[32] __attribute__((aligned (32)));
	int itx, commandLen;

	Dbg(DBG, ".");
	commandLen=mini_snprintf(tempBuff, sizeof(tempBuff), "AT+CIPSEND=%d,%d\r\n", channel, packetLen);
	if (HAL_OK!=SendToEsp2(tempBuff, commandLen))
		return 1;

	itx=0;
	while (RecvFromEsp("\r\n>")==0)
	{
		if (RecvFromEsp(",CLOSED"))
			return 5;
		if (RecvFromEsp("ERROR"))
			return 6;

		itx++;
		if (itx>GetAnswerDelay())
			return 2;
		vTaskDelay(1);
	}

	if (HAL_OK!=SendToEsp2(pData, packetLen))
		return 3;

	itx=0;
	while (RecvFromEsp("\r\nSEND OK")==0)
	{
		if (RecvFromEsp(",CLOSED"))
			return 7;
		if (RecvFromEsp("ERROR"))
			return 8;

		itx++;
		if (itx>GetAnswerDelay())
			return 4;
		vTaskDelay(1);
	}
	return 0;
}

static int vCloseConnection(int channel)
{
	int itx=0, commandLen;

	commandLen=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPCLOSE=%d\r\n", channel);
	if (HAL_OK!=SendToEsp2(sendBuff, commandLen))
		return 1;
	while (RecvFromEsp("CLOSED")==0)
	{
		itx++;
		if (itx>GetAnswerDelay())
			return 2;
		vTaskDelay(1);
	}
	return 0;
}

static int vSendData(char *pData, int len, int channel)
{
	int i, partial=0, result;

	while (partial<=len-PACKET_SEND_LEN)
	{
		if(TakeMutex(Semphr_sdram, 1000))
		{
			for (i=0; i<PACKET_SEND_LEN; i++)
				sendBuff[i]=*(pData+i+partial);
			GiveMutex(Semphr_sdram);

			result=vSendDataPacket(sendBuff, PACKET_SEND_LEN, channel);
			if (result>0)
				return result;
			partial+=PACKET_SEND_LEN;
		}
	}
	if (len-partial>0)
	{
		if(TakeMutex(Semphr_sdram, 1000))
		{
			for (i=0; i<len-partial; i++)
				sendBuff[i]=*(pData+i+partial);
			GiveMutex(Semphr_sdram);

			result=vSendDataPacket(sendBuff, len-partial, channel);
			if (result>0)
				return result;
		}

	}
	return 0;
}

static int vSendDataHTTP(char *getHttpRequest, int channel)
{
	int result;
	DATA_TO_SEND *temp=GetPageWWW(getHttpRequest);
	result=vSendData(temp->pData, temp->len, channel);
	vPortFree(temp);
	return result;
}

static int vSendDataSMTP(int id)
{
	int result;
	DATA_TO_SEND *temp=GetDataEmail(id);
	if ((result=vSendData(temp->pData, temp->len, /*ESP_EMAIL_CHANNEL*/4)))
	{
		SetEmailState(SMTP_FAIL);
		SetEmailCode(result);
	}
	vPortFree(temp);
	return result;
}

static int vSendCommandSMTP(char *pCommand, int commandLen)
{
	int result;
	if ((result=vSendDataPacket(pCommand, commandLen, /*ESP_EMAIL_CHANNEL*/4)))
	{
		SetEmailState(SMTP_FAIL);
		SetEmailCode(100+result);
	}
	return result;
}

static int GetRecvCodeEmail(char *pBuf)  // dac jako strcat !!!!!
{
	int i=1, j, code=0;
	char *ptr=pBuf;

	ptr++;
	while ((*ptr>0x2F)&&(*ptr<0x3A))
		ptr++;
	while ((*(ptr-i)>0x2F)&&(*(ptr-i)<0x3A))
	{
		switch (i)
		{
		case 2:
			j=10;
			break;
		case 3:
			j=100;
			break;
		case 4:
			j=1000;
			break;
		default:
			j=1;
			break;
		}
		code+=((*(ptr-i))&0x0F)*j;
		i++;
	}
	return code;
}

static char *vWaitForRecv(int timeoutWait)
{
	int itx=0;
	char *ptr,*ptrP;

	while ((ptr=RecvFromEsp("+IPD,"))==0)
	{
		itx++;
		if (itx>timeoutWait)
			return ptr;
		vTaskDelay(1);
	}
	while ((ptrP=strstr(ptr,":"))==0)
	{
		itx++;
		if (itx>timeoutWait)
			return ptrP;
		vTaskDelay(1);
	}
	ptr=ptrP;
	return ptr;
}

static int vGetEmailRecvCode(int *channel)
{
	char *pRecvBuff;
	int size, codeE, itx=0;

	do
	{
		if ((pRecvBuff=vWaitForRecv(GetAnswerDelay()))==0)
		{
			Dbg(DBG, "\r\nNOTHING RECV ");
			return 0;
		}
		GetSizeAndChannel(pRecvBuff, channel, &size);
		if (itx>0)
		{
			Dbg(DBG, " Repeat email parser ");
			if (itx>5)
				return 20;
		}
		itx++;

	} while (*channel!=/*ESP_EMAIL_CHANNEL*/4);

	while (GetDMACountByte()<size)
		vTaskDelay(1);
	vTaskDelay(1);
	codeE=GetRecvCodeEmail(pRecvBuff);
	Dbg(DBG, "\r\n");
	Dbg(DBG, pRecvBuff);
	return codeE;
}

static void SendDummyData(int delayMs)
{
	vTaskDelay(delayMs);
	SendToEsp2(sendBuff, PACKET_SEND_LEN);
	vTaskDelay(delayMs);
	SendToEsp2(sendBuff, PACKET_SEND_LEN);
	vTaskDelay(delayMs);
	SendToEsp2(sendBuff, PACKET_SEND_LEN);
	Dbg(DBG, "\r\nSEND DUMMY DATA ");
}

static int GetHttpPort(void)
{
	switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
	{
	case WIFI_MODE_AP:
		return VAR_GetTabVal(Const_wifiAP_port,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB));
	default:
		return VAR_GetTabVal(Const_wifiSTA_port,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB));
	}
}

static int vStartServer(void)
{
	int len,itx=0;

	len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSERVER=1,%d\r\n", GetHttpPort());
	SendToEsp2(sendBuff,len);
	while (RecvFromEsp("\r\nOK")==0)
	{
		itx++;
		if (itx>GetAnswerDelay())
			return 1;
		vTaskDelay(1);
	}
	return 0;
}

static int vStopServer(void)
{
	int len,itx=0;

	len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSERVER=0\r\n");
	SendToEsp2(sendBuff,len);
	while (RecvFromEsp("\r\nOK")==0)
	{
		itx++;
		if (itx>GetAnswerDelay())
			return 1;
		vTaskDelay(1);
	}
	return 0;
}

void vRestartWifiServer(void)
{
	vStopServer();
	vTaskDelay(30);
	vStartServer();
}

static bool vCheckEmailAnswer(int emailCode)
{
	int channel, code;

	code=vGetEmailRecvCode(&channel);
	SetEmailCode(code);

	if (code==emailCode)
	{
		SetEmailState(SMTP_INPROGRESS);
		return true;
	}
	else
	{
		SetEmailState(SMTP_FAIL);
		vTaskDelay(200);
		return false;
	}
}

static void EmailSendStart(void)
{
	int len;
	char buftemp[5];

	switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
	{
		case WIFI_MODE_STA:
		case WIFI_MODE_AP_STA:

			if(VAR_GetTabVal(Const_emailSend_IP, EmailSendParam.whichSender))
			{
				connectionType=SMTP_CONNECTION;
				SetEmailState(SMTP_START);
				EmailSendParam.start=3;
				vStopServer();

				if (VAR_GetTabVal(Const_emailSend_useSSL, EmailSendParam.whichSender))
					strcpy(buftemp, "SSL");
				else
					strcpy(buftemp, "TCP");

				len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSTART=%d,\"%s\",\"%s\",%d\r\n",
						/*ESP_EMAIL_CHANNEL*/4,
						buftemp,
						IP2Str(VAR_GetTabVal(Const_emailSend_IP, EmailSendParam.whichSender)),
						VAR_GetTabVal(Const_emailSend_port, EmailSendParam.whichSender));
//				len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSTART=%d,\"%s\",\"213.180.147.145\",%d\r\n",
//						ESP_EMAIL_CHANNEL,
//						buftemp,
//						VAR_GetTabVal(Const_emailSend_port, EmailSendParam.whichSender));
				SendToEsp2(sendBuff, len);
				DbgMulti(DBG,"\r\nSEND_START: ",sendBuff," SEND STOP\r\n");
			}

			break;
	}
}

static void ErrorServiceSMTP(void)
{
	if(GetEmailState()==SMTP_FAIL)
	{
		switch(GetEmailCode())
		{
		case ESPANSWER_TIMEOUT_TO_GET_SEND_OK:
			Dbg(DBG,"\r\nESPANSWER_TIMEOUT_TO_GET_SEND_OK");
			break;
		case ESPANSWER_TIMEOUT_FOR_SMTPCOMM_TO_GET_SEND_OK:
			Dbg(DBG,"\r\nESPANSWER_TIMEOUT_FOR_SMTPCOMM_TO_GET_SEND_OK");
			break;
		case ESPANSWER_CLOSED_AFTER_SEND_DATA:
			Dbg(DBG,"\r\nESPANSWER_CLOSED_AFTER_SEND_DATA");
			break;
		case ESPANSWER_ERROR_AFTER_SEND_DATA:
			Dbg(DBG,"\r\nESPANSWER_ERROR_AFTER_SEND_DATA");
			break;
		case 20:
			break;
		}
	}
}

static int vGetConnectionResultToSTA(void)
{
	int itx=0;
	char *ptr;
	while(1)
	{
		if(RecvFromEsp("\r\nOK"))
		{
			return ESP_CONNECTION_OK;
		}
		else if(RecvFromEsp("ERROR"))
		{
			if ((ptr=RecvFromEsp("+CWJAP:")))
			{
				switch(atoi(ptr+7))
				{
				case 1:
					return ESP_CONNECTION_TIMEOUT;
				case 2:
					return ESP_WRONG_PASSWORD;
				case 3:
					return ESP_CANNOT_FIND_THE_TARGET_AP;
				case 4:
					return ESP_CONNECTION_FAILED;
				default:
					return ESP_UNKNOW_ERROR_OCCURRED;
				}
			}
			else
				return ESP_UNKNOW_ERROR_OCCURRED;
		}
		else
		{
			itx++;
			if(itx>CONNECTION_TIMEOUT_MS)
				return ESP_UNKNOW_ERROR_OCCURRED;
			vTaskDelay(1);
		}
	}
}

static void GetAddressesForConnection(void)
{
	char *ptr=NULL, temp[20]={0};
	char rcv1[]="+CIFSR:APIP,\"";
	char rcv2[]="+CIFSR:APMAC,\"";
	char rcv3[]="+CIFSR:STAIP,\"";
	char rcv4[]="+CIFSR:STAMAC,\"";

	if ((ptr=RecvFromEsp(rcv1))){ strcpy2_(temp,ptr,mini_strlen(rcv1)+1,16); Const.wifiAP[Const.wifiGeneral.nrAP].ip    = IPStr2Int	   (temp); }  //Powinno zapisywac do zmiennych niezapisywalnych a nie di zapisywalnych !!!!
	if ((ptr=RecvFromEsp(rcv2))){ strcpy2_(temp,ptr,mini_strlen(rcv2)+1,16); Const.wifiAP[Const.wifiGeneral.nrAP].mac   = MACStr2Int64 (temp); }
	if ((ptr=RecvFromEsp(rcv3))){ strcpy2_(temp,ptr,mini_strlen(rcv3)+1,16); Const.wifiSTA[Const.wifiGeneral.nrSTA].ip  = IPStr2Int	   (temp); }
	if ((ptr=RecvFromEsp(rcv4))){ strcpy2_(temp,ptr,mini_strlen(rcv4)+1,16); Const.wifiSTA[Const.wifiGeneral.nrSTA].mac = MACStr2Int64 (temp); }
}

static void vQueryAndReplaceEmailAddrName2AddrIP(void)
{
	int i,len,itx;
	char *ptr;

	for(i=0;i<MAX_EMAIL_SENDERS;++i)
	{
		if(*VAR_GetStr(Const_emailSend_server,i))
		{
			len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPDOMAIN=\"%s\"\r\n",VAR_GetStr(Const_emailSend_server,i));
			SendToEsp2(sendBuff,len);  DbgMulti(DBG,"\r\n",sendBuff," ");
			itx=0;
			while (1)
			{
				if (RecvFromEsp("\r\nOK"))
				{
					if ((ptr=RecvFromEsp("+CIPDOMAIN:")))
					{
						VAR_SetTabVal(Const_emailSend_IP,i,IPStr2Int(ptr+12)); //POPRAWIC to '12' !!!!!! dac jako przeszukuje do znaki ":"   +CIPDOMAIN:"213.180.147.145"
						DbgMulti(DBG,"\r\n",ptr,"  ");
						vTaskDelay(50);
						break;
					}
					else
						break;
				}
				else if (RecvFromEsp("ERROR"))
					return;
				else
				{
					itx++;
					if (itx>DNS_SERVER_TIMEOUT_MS)
						return;
					vTaskDelay(1);
				}
			}
		}
	}
}
#include <time.h>
static int vQueryAndLoadTimeFromSNTP(void)
{
	int itx=0;
	char *ptr;
	time_t getTime;

	while (1)
	{
		SendToEsp("AT+SYSTIMESTAMP?\r\n");
		while (1)
		{
//			if (RecvFromEsp("\r\nOK"))
//			{
				if ((ptr=RecvFromEsp("+SYSTIMESTAMP:")))
				{
					getTime=(time_t)atoi(ptr+14);
					if(getTime>1565853509)
					{
						VAR_SetTabVal(Const_sntp_time,NO_TAB,getTime);
						sntpTime=gmtime(&getTime);
						DbgVar(1,50,"\r\nES TIME LOADED %d; %02d-%02d-%02d  %02d:%02d:%02d",
								VAR_GetTabVal(Const_sntp_time,NO_TAB),
								sntpTime->tm_year-100,
								sntpTime->tm_mon+1,
								sntpTime->tm_mday,
								sntpTime->tm_hour,
								sntpTime->tm_min,
								sntpTime->tm_sec);
						return 1;
					}
					Dbg(DBG,"*");
					vTaskDelay(500);
					itx+=500;
					if (itx>SNTP_SERVER_TIMEOUT_MS)
						return 0;
					break;
				}
				else
					return 0;
//			}
//			else if (RecvFromEsp("ERROR"))
//				return 0;

			itx++;
			if (itx>SNTP_SERVER_TIMEOUT_MS)
				return 0;
			vTaskDelay(1);
		}
	}
}

//static void vLoadTime(time_t timeSet)
//{
//	int len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+SYSTIMESTAMP=%d\r\n",timeSet);
//	SendToEsp32(sendBuff,len);
//	while (RecvFromEsp("\r\nOK")==0)
//		vTaskDelay(10);
//}

static bool CheckEmailAnswer(int emailCode)
{
	char *pSmtp = NULL;
	int channel=0, size=0;

	if ((pSmtp=RecvFromEsp(",CONNECT\r\n")))		/* RecvFromEsp("0,CONNECT\r\n")   0-channel */
	{
		if ((pSmtp=RecvFromEsp("+IPD,")))				/* RecvFromEsp("+IPD,0,698:GET /")   0-channel, 698-received bytes */
		{
			if ((pSmtp=RecvFromEsp(":")))
			{
				GetSizeAndChannel(pSmtp, &channel, &size);
				if(channel == /*ESP_EMAIL_CHANNEL*/4)
				{
					int codeE=GetRecvCodeEmail(pSmtp);
					SetEmailCode(codeE);

					if (codeE==emailCode)
					{
						SetEmailState(SMTP_INPROGRESS);
						return true;
					}
					else
					{
						SetEmailState(SMTP_FAIL);
						vTaskDelay(200);
						return false;
					}
				}
			}
		}
	}
}

static void BackFromEmail(int nrInfo)
{
	if(nrInfo) DbgDma(DBG, _S_ EMAIL_ERROR _E_);
	SendToEsp32(0,"AT+CIPCLOSE="ESP_EMAIL_CHANNEL"\r\n",arch);
	COMMAND_Service(_SET,sendBuff);
	_SET_NEW_CASE_(98);
}

void BackToHttpService(int *param){
	UpdateReadPos();
	_SET_NEW_CASE_(0);
	*param=0;
	connectionType = HTTP_CONNECTION;
}

static void GetHTTPpacketParam(char* ptr, int* channel, int* size){			/* +IPD,0,698:GET /... */
	*channel = atoi_(ptr,mini_strlen("+IPD,"));								/* *channel = ESP_HTTP_CHANNEL */
	*size 	 = atoi_(ptr,mini_strlen("+IPD,"ESP_HTTP_CHANNEL","));
}

static void GetSMTPpacketParam(char* ptr, char* answer, int* channel, int* size, int* code){		/* +IPD,4,31:220 smtp.poczta.onet.pl... */
	char temp[50]={0};  strcpy2_(temp,ptr,0,20);													/* *channel = ESP_EMAIL_CHANNEL */
	*channel = STRING_GetInt(temp,',');
	*size 	 = STRING_GetInt(temp+mini_strlen(answer),',');
	*code 	 = STRING_GetInt(temp,':');
}

static int SMTP_SendCmd(int typeSendArch,int len){
	if((pMem = (char*)pvPortMalloc((len+1)*sizeof(char)))){								/* Uruchom vApplicationMallocFailedHook() dla  #define configUSE_MALLOC_FAILED_HOOK 1 */
		strncpy(pMem,sendBuff,len);  *(pMem+len)=0;
		len = mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSEND="ESP_EMAIL_CHANNEL",%d\r\n",len);
		SendToEsp32(len,NULL,typeSendArch);
		COMMAND_Service(_SET,sendBuff);
		return 0;
	}
	else return 1;
}

static int SMTP_SendData(int typeSendArch){
	if(ErrorAnswerService()){  if(pMem) vPortFree(pMem);  return 1;  }
	SendToEsp32(0,pMem,typeSendArch);
	COMMAND_Service(_SET,sendBuff);
	if(pMem){ vPortFree(pMem); pMem=NULL; }
	return 0;
}

static void SMTP_Descr(char* ptr, int typeSendArch, int* channel, int* size, int* code){
	char temp[50]={0};
	if((ptr=RecvFromEsp("\r\nRecv "))){
		strcpy_(temp,ptr,1,'\r');		/* strcpy2_(temp,pHttp,0,20); */
		*size = STRING_GetInt(temp,' ');
		if(typeSendArch!=noArch){
			DbgVarDma(DBG,100,_S_"\r\n%d received bytes by ESP32 "_E_,*size);
			DbgDma(DBG, _S_" --- SEND OK --- "_E_);
	}}
	if ((ptr=RecvFromEsp("+IPD,"))){
		strcpy2_(temp,ptr,0,20);
		*channel = STRING_GetInt(temp,',');					/* *channel = ESP_EMAIL_CHANNEL */
		*size 	 = STRING_GetInt(temp+5,',');				/* 5 = mini_strlen("+IPD,") */
		*code 	 = STRING_GetInt(temp,':');
		if(typeSendArch!=noArch)
			DbgVarDma(DBG,100,_S_"\r\nRecv email data: channel %d  size %d  code %d"_E_,*channel,*size,*code);
	}
}

static int Is_ComplRecvSMTPpacket(void){
	return (RecvFromEsp("\r\nRecv ") && RecvFromEsp("\r\n+IPD,"ESP_EMAIL_CHANNEL));
}

static void vWaitOnSendTimeoutTimerCallback(TimerHandle_t pxTimer)
{
	;
	xTimerStop(pxTimer, 0);
}

static void GoToTest(char* txt){
	connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
	DbgVarDma(DBG,50, _S_"%s"_E_,txt);
}

static int SetRqstToSendChnl(int channel, char* ptr){
	for(int i=0;i<ESP_MAX_HTTP_CONN;++i){	if(httpPar.que[i]==channel){ DbgDma(DBG,_SE_"\r\nQue: its ALREADY "_E_); return 1; }	}
	for(int i=0;i<ESP_MAX_HTTP_CONN;++i){   if(httpPar.que[i]==0xFF){ httpPar.que[i]=channel; httpPar.ptr[i]=ptr; return 0; }   }
	DbgDma(DBG,_SE_"\r\nQue: FULL "_E_);
	return 2;
}

int CheckReadyToSendChnl(void){
	if(httpPar.chnl==0xFF){		/* gdy nie ma w tle oczekiwania na SEND OK */
		LOOP_FOR(nrChnl,ESP_MAX_HTTP_CONN){
			if(httpPar.que[nrChnl]!=0xFF){
				httpPar.chnl = httpPar.que[nrChnl];
				return nrChnl;
	}}}
	return -1;
}

static void InitStructRqstToSendChnl(void){
	httpPar.chnl = 0xFF;
	LOOP_FOR(i,ESP_MAX_HTTP_CONN){ httpPar.ptr[i]=NULL; httpPar.que[i]=0xFF; httpPar.nr[i]=0; }
}

static void HTTP_SendCloseChnlInit(ARCHIVING_TYPE archType){
	int nrQue=CheckReadyToSendChnl();
	if(nrQue>-1)
	{
		httpPar.nr[nrQue]=0;
		if	   (httpPar.ptr[nrQue]==(char*)0x00000001)	SendToEsp32_http( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSEND=%d,%d\r\n",httpPar.chnl,mini_strlen(HTML_TXT_CODE)), NULL, archType);
		else if(httpPar.ptr[nrQue]==(char*)0x00000002)	SendToEsp32_http( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPCLOSE=%d\r\n",httpPar.chnl), 							   NULL, archType);
}}

static void HTTP_SendCloseChnl(ARCHIVING_TYPE archType){
	int nrQue=CheckReadyToSendChnl();
	if(nrQue>-1){
		if(httpPar.nr[nrQue] > 200){  httpPar.nr[nrQue]=0;
			SendToEsp32_http( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPCLOSE=%d\r\n",httpPar.chnl), NULL, archType);		/* Czas wykonania SendToEsp32() to 28us */
		}
		else{   if(archType==noArch) DbgVarDma(1,5,"%d",httpPar.chnl);
			if	   (httpPar.ptr[nrQue]==(char*)0x00000001){  httpPar.nr[nrQue]++;
				SendToEsp32_http( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSEND=%d,%d\r\n",httpPar.chnl,mini_strlen(HTML_TXT_CODE)), NULL, archType );  }
			else if(httpPar.ptr[nrQue]==(char*)0x00000002){  httpPar.nr[nrQue]=0;
				SendToEsp32_http( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPCLOSE=%d\r\n",httpPar.chnl), NULL, archType);  }
}}}

static int HTTP_GetClosedChannel(char* ptr){
	int i=GetPos(ptr);
	DecPos(&i);
	return RecvBuffer[i]&0x0F;
}
/*
static void HTTP_ShowRecvBytes(ARCHIVING_TYPE archType){		// RecvFromEsp("\r\nRecv 88 bytes")   88-received bytes by ESP
	char* ptr=NULL;
	if(archType!=noArch){
		if ((ptr=RecvFromEsp("\r\nRecv "))){	if (strstr_(ptr," bytes\r\n")){	if (strstr_(ptr,"\r\nSEND OK")){
				char temp[50]={0};  strcpy_(temp,ptr,1,'\r');		// strcpy2_(temp,pHttp,0,30);
				int val = STRING_GetInt(temp,' ');					// val = atoi_(pHttp,mini_strlen("\r\nRecv "));
					DbgVarDma(DBG,200,_S_"\r\n%d received bytes by ESP32 "_E_,val);
					DbgDma(DBG, _S_" --- SEND OK --- "_E_);
	}}}}
}
*/
static void HTTP_ShowClosedChannel(u8 channel, ARCHIVING_TYPE archType){
	if(archType!=noArch){
		if(httpPar.chnl==archType) DbgVarDma(DBG,100, _S_" \r\n--- CLOSED,%d --- "_E_,archType);
		else						DbgVarDma(DBG,100, _SE_"\r\n --- CLOSED,%d!=%d asyn --- "_E_,httpPar.chnl,archType);
	}
}

static void HTTP_ShowInitChannel(int channel,int size, ARCHIVING_TYPE archType){
	if(archType!=noArch)  DbgVarDma(DBG,100,_S_"\r\nRecv HTTP data: channel %d  size %d "_E_,channel,size);
}

void vtaskWifi(void *argument)
{
	char *pHttp,*pHttp2,  *ptr;   int lenHTTP=0;
	int channel=0, size=0, code=0, len, result, result2;   int nrHTTPpacket=0;  int nrSMTP=0;
	int j;

	uint32_t ulNotifiedValue;
	u8 activHttp=0;


	int typeSendArch = arch;
	int typeRecvArch = arch;

	RstTimeBtwnSendRcv();
	StartDMA();
	ESP_ON;

	EmailSendParam.start=0;
	DefaultSettingsWIFI();
	DefaultSettingsEmail();
	DefaultSettingsDNS();
	DefaultSettingsSNTP();

	InitStructRqstToSendChnl();


	ResetTestTab(); //Do USUNIECIA !!!

	Dbg(DBG,"\r\nStart vtaskWifi\r\n");   //StartUp aktivity dla tego watki jezeli nie ma odp na AT to innty watek restartuje ten watek

	//xWaitOnSendTimeoutTimer = xTimerCreate("WaitOnSendTimeoutTimer", WAIT_ON_SEND_TIMEOUT_TIME_MS, 0, ( void * ) 0, vWaitOnSendTimeoutTimerCallback);


/*
	AT+CWAUTOCONN=1: Włącza automatyczne łączenie z AP przy starcie (standardowo jest włączone).
	AT+SYSSTORE=1:   Upewnia się, że zmiany w konfiguracji Wi-Fi (jak SSID i hasło) są zapisywane w pamięci flash, aby przetrwały restart.
	AT+SYSMSG:       Pozwala na konfigurację dodatkowych komunikatów systemowych (np. o rozłączeniu), co jest dostępne w nowszych wersjach oprogramowania (powyżej v2.1.0.0).


*/

//	StartMeasureTime_us();
//	StopMeasureTime_us("\r\nTEST_5");



	while(1)
	{

	  if(xTaskNotifyWait(0x00, 0x00, &ulNotifiedValue, portMAX_DELAY) == pdPASS)			/* xTaskNotifyWait(bitmask_na_wejsciu, bitmask_na_wyjsciu, &zmienna, czas)		bitmask: 0x00: Nie czyść nic , 0xFFFFFFFF (Wszystkie bity): Czyści całą wartość powiadomienia po wyjściu.   Problem: Jeśli w tym samym momencie (ułamek sekundy po wybudzeniu zadania, ale przed zakończeniem tej funkcji) inne zadanie lub przerwanie (ISR) przyśle nowy bit, zostanie on bezpowrotnie skasowany i utracony. Dlatego najbezpieczniejsze jest uzycie pod koniec obslugi tego zdarzenia reczne czyszczenie aktualnego bitu funkcją ulTaskNotifyValueClear(NULL, ulNotifiedValue) */
	  {
	 /*	if(ulTaskNotifyTake(pdTRUE,portMAX_DELAY)) */										/* Czekaj na powiadomienie.  Dzięki pdTRUE w pierwszym argumencie, po wyjściu z funkcji wartość powiadomienia zostanie zresetowana do 0 */
		if (ulNotifiedValue & BIT_ESP_SRV)
		{
			GetNewReadPos();		//Zastanowic sie co w przypadku przepelnienia bufora cyklicznego i nie zdarzenie oblugi go
			SCB_InvalidateDCache_by_Addr((uint32_t*)RecvBuffer, ESP_RECV_BUFF_SIZE);		/* Jesli w MPU ustawimy adres bufora w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_InvalidateDCache_by_Addr() nie jest potrzebny */

			switch (connectionType)
			{
				case INIT_CONNECTION:

					if (CASE_Service(0,"ready",NULL,typeRecvArch))			/* Jezeli wchodzimy w dany case to wywolujemy ESP32_FreeAnswers() */
					{
						SendToEsp32(0,"ATE0\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(1,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+UART_CUR=%d,8,1,0,0\r\n",ESP_UART_BUADRATE), NULL, typeSendArch );
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(2,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						vTaskDelay(20);
						ChangeUartBuadRate(ESP_UART_BUADRATE);
						SendToEsp32(0,"AT+GMR\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(3,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						SendToEsp32(mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CWMODE=%d\r\n",Const.wifiGeneral.mode), NULL, typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(4,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						if(WIFI_MODE_DISABLED==Const.wifiGeneral.mode){
							DbgDma(DBG,_SE_"\r\nWifi DISABLED "_E_);
							break;
						}
						SendToEsp32(0,"AT+CWLAPOPT=1,23\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(5,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						SendToEsp32(0,"AT+CIPMUX=1\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(6,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						SendToEsp32(0,"AT+CWDHCP=0,3\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(7,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						if( 0 == Const.wifiAP [ Const.wifiGeneral.nrAP ].dhcp &&
							1 == Const.wifiSTA[ Const.wifiGeneral.nrSTA].dhcp )
						{
							SendToEsp32(0,"AT+CWDHCP=1,1\r\n",typeSendArch);
						}
						else if( 1 == Const.wifiAP [ Const.wifiGeneral.nrAP ].dhcp &&
								 0 == Const.wifiSTA[ Const.wifiGeneral.nrSTA].dhcp )
						{
							SendToEsp32(0,"AT+CWDHCP=1,2\r\n",typeSendArch);
						}
						else if( 1 == Const.wifiAP [ Const.wifiGeneral.nrAP ].dhcp &&
								 1 == Const.wifiSTA[ Const.wifiGeneral.nrSTA].dhcp )
						{
							SendToEsp32(0,"AT+CWDHCP=1,3\r\n",typeSendArch);
						}
						else SendToEsp32(0,"AT\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(8,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						SendToEsp32(0,"AT+CWHOSTNAME=\"Elektronika_STM\"\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(9,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						if( (WIFI_MODE_STA 	  == Const.wifiGeneral.mode ||
							 WIFI_MODE_AP_STA == Const.wifiGeneral.mode) && 0 == Const.wifiSTA[ Const.wifiGeneral.nrSTA ].dhcp )
						{
							len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSTA=\"%s\",\"%s\",\"%s\"\r\n",
									IP2Str(Const.wifiSTA[ Const.wifiGeneral.nrSTA ].ip),
									IP2Str(Const.wifiSTA[ Const.wifiGeneral.nrSTA ].gate),
									IP2Str(Const.wifiSTA[ Const.wifiGeneral.nrSTA ].mask));
							SendToEsp32(len,NULL,typeSendArch);	 	/* SendToEsp32(len,sendBuff) is the same */
						}
						else SendToEsp32(0,"AT\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(10,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						if( (WIFI_MODE_AP 	 == Const.wifiGeneral.mode ||
							WIFI_MODE_AP_STA == Const.wifiGeneral.mode) && 0 == Const.wifiAP[ Const.wifiGeneral.nrAP ].dhcp )
						{
							len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPAP=\"%s\",\"%s\",\"%s\"\r\n",
									IP2Str(Const.wifiAP[ Const.wifiGeneral.nrAP ].ip),
									IP2Str(Const.wifiAP[ Const.wifiGeneral.nrAP ].gate),
									IP2Str(Const.wifiAP[ Const.wifiGeneral.nrAP ].mask));
							SendToEsp32(len,sendBuff,typeSendArch);	 	/* SendToEsp32(len,NULL) is the same */
						}
						else SendToEsp32(0,"AT\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(11,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						if( WIFI_MODE_AP 	 == Const.wifiGeneral.mode ||
							WIFI_MODE_AP_STA == Const.wifiGeneral.mode )
						{
							len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CWSAP=\"%s\",\"%s\",5,3\r\n",
									Const.wifiAP[ Const.wifiGeneral.nrAP ].name,
									Const.wifiAP[ Const.wifiGeneral.nrAP ].pass);
							SendToEsp32(len,NULL,typeSendArch);
						}
						else SendToEsp32(0,"AT\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(12,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						if( WIFI_MODE_STA 	 == Const.wifiGeneral.mode ||
							WIFI_MODE_AP_STA == Const.wifiGeneral.mode )
						{
							len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CWJAP=\"%s\",\"%s\"\r\n",
									Const.wifiSTA[ Const.wifiGeneral.nrSTA ].name,
									Const.wifiSTA[ Const.wifiGeneral.nrSTA ].pass);
							SendToEsp32(len,NULL,typeSendArch);
						}
						else SendToEsp32(0,"AT\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(13,txt_OK,txt_ERR,typeRecvArch))
					{
						if( WIFI_MODE_STA 	 == Const.wifiGeneral.mode ||
							WIFI_MODE_AP_STA == Const.wifiGeneral.mode )
						{
							if(_GET_ANSW_CASE_==_OK) result=ESP_CONNECTION_OK;
							else
							{
								INIT_BUFF(answer,"+CWJAP:");
								if ((ptr=RecvFromEsp(answer))){
									result=0;
									switch(atoi_(ptr,mini_strlen(answer))){
										case 1:  result=ESP_CONNECTION_TIMEOUT; 		break;
										case 2:  result=ESP_WRONG_PASSWORD;				break;
										case 3:  result=ESP_CANNOT_FIND_THE_TARGET_AP;	break;
										case 4:  result=ESP_CONNECTION_FAILED;			break;
										default: result=ESP_UNKNOW_ERROR_OCCURRED;		break;
								}}
								else result=ESP_UNKNOW_ERROR_OCCURRED;
							}
							DbgVarDma2(DBG,100,_S_"\r\nSTA_CONNECTION status: %d\r\n"_E_,result);
						}
						else
						{
							if(ErrorAnswerService()) break;
						}
						SendToEsp32(0,"AT+CIFSR\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(14,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						GetAddressesForConnection();
						SendToEsp32(0,"AT+CIPSERVERMAXCONN=5\r\n",typeSendArch);  //ESP_MAX_HTTP_CONN !!!!
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(15,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPDNS=1,\"%s\",\"%s\",\"%s\"\r\n",
								IP2Str(Const.dns.IP1),
								IP2Str(Const.dns.IP2),
								IP2Str(Const.dns.IP3));
						SendToEsp32(len,NULL,typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(16,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSNTPCFG=1,%d,\"%s\",\"%s\"\r\n",Const.sntp.timezone, Const.sntp.nameServer1, Const.sntp.nameServer2);
						SendToEsp32(len,NULL,typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(17,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						//SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSERVER=1,443,\"SSL\"\r\n"),NULL,typeSendArch );
						SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSERVER=1,%d\r\n",GetHttpPort()),NULL,typeSendArch );
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(18,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSTO=%d\r\n",TCP_SERVER_TIMEOUT_S), NULL, typeSendArch );
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(19,txt_OK,txt_ERR,typeRecvArch))
					{
						_THE_SAME_CASE_;								/* przewidujemy w tym case cykliczne powtarzanie */

						/* Obsługa odpowiedzi */
						{
							if(_GET_REP_CASE_ == 0)
							{
								if(ErrorAnswerService()) break;
							}
							else if(IS_RANGE(_GET_REP_CASE_,1,MAX_EMAIL_SENDERS-1))
							{
								if(ErrorAnswerService()){ ; }   		/* z tym błędem nic nie rob */
								else
								{
									INIT_BUFF(answer,"+CIPDOMAIN:");
									if ((ptr=RecvFromEsp(answer))){  char temp[20]={0};  strcpy2_(temp,ptr,mini_strlen(answer)+1,16);  Const.emailSend[_GET_REP_CASE_-1].IP=IPStr2Int(temp);  }
									else  						  {  DbgDma(DBG,_S_ ESP32_DOMAIN_ERROR);  }
								}
							}
						}

						/* Obsługa wysylania i iteracji */
						{
							if( (WIFI_MODE_STA 	  == Const.wifiGeneral.mode ||
								 WIFI_MODE_AP_STA == Const.wifiGeneral.mode))
							{
								SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPDOMAIN=\"%s\"\r\n",Const.emailSend[_GET_REP_CASE_].server), NULL, typeSendArch );
								if(_GET_REP_CASE_ == MAX_EMAIL_SENDERS-1)
									_SET_NEXT_CASE_;
							}
							else
							{
								SendToEsp32(0,"AT\r\n",typeSendArch);
								_SET_NEXT_CASE_;
							}
							COMMAND_Service(_SET,sendBuff);
						}

					}
					else if (CASE_Service(20,txt_OK,txt_ERR,typeRecvArch))
					{
						if( (WIFI_MODE_STA 	  == Const.wifiGeneral.mode ||
							 WIFI_MODE_AP_STA == Const.wifiGeneral.mode))
						{
							if(ErrorAnswerService()){ ; }   	 		/* z tym błędem nic nie rob */
							else
							{
								INIT_BUFF(answer,"+CIPDOMAIN:");
								if ((ptr=RecvFromEsp(answer))){  char temp[20]={0};  strcpy2_(temp,ptr,mini_strlen(answer)+1,16);  Const.emailSend[MAX_EMAIL_SENDERS-1].IP=IPStr2Int(temp);  }
								else  						  {  DbgDma(DBG,_S_ ESP32_DOMAIN_ERROR);  }
							}
						}
						else
						{
							if(ErrorAnswerService()) break;
						}
						SendToEsp32(0,"AT+SYSTIMESTAMP?\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(21,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						_THE_SAME_CASE_;								/* przewidujemy w tym case cykliczne powtarzanie */
						time_t getTime;
						INIT_BUFF(answer,"+SYSTIMESTAMP:");
						if ((ptr=RecvFromEsp(answer)))
						{
							getTime=(time_t)atoi_(ptr,mini_strlen(answer));
							if(getTime>1565853509)
							{
								gmtime_r(&getTime,sntpTime);			/* lepsze dla wielowatkowosci niz  sntpTime=gmtime(&getTime) */
								Const.sntp.time = getTime;
								DbgVarDma(DBG,200,_S_"\r\nESP32 TIME LOADED %d: %02d-%02d-%02d  %02d:%02d:%02d"_E_,
										Const.sntp.time,
										sntpTime->tm_year-100,
										sntpTime->tm_mon+1,
										sntpTime->tm_mday,
										sntpTime->tm_hour,
										sntpTime->tm_min,
										sntpTime->tm_sec);

								connectionType = HTTP_CONNECTION;   _SET_NEW_CASE_(0);
								UpdateReadPos();				/* Poniewż nie używasz w tym case funkcji SendToEsp32(), w której jest UpdateReadPos() musisz sam wywołać UpdateReadPos() */
							}
							else
							{
								vTaskDelay(2000);    //jezeli nie ma odpowiedzi sntp to trzeba cyklicznie co godzine np pytac
								if(_GET_REP_CASE_ == SNTP_NMBR_QUERY)
								{
									UpdateReadPos();
									connectionType = HTTP_CONNECTION;   _SET_NEW_CASE_(0);
								}
								else
								{
									SendToEsp32(0,"AT+SYSTIMESTAMP?\r\n",typeSendArch);
									COMMAND_Service(_SET,sendBuff);
								}
							}
						}

					}
					else
					{
						ESP32_FreeAnswers(1/*2*/);			/* Jezeli NIE wchodzimy w żaden case to rownież wywolujemy ESP32_FreeAnswers() */
					}
					break;



//				case HTTP_CONNECTION:
//
//					DispRecvBuff(++nrHTTPpacket,typeSendArch);  ESP32_FreeAnswers(0);
//					RstTimeBtwnSendRcv();
//
//					if ((pHttp=strstr_(NULL,"0,CONNECT\r\n")))						/* RecvFromEsp("0,CONNECT\r\n")   0-channel */			/* Równoczesne właczenie różnych przegladarek pod ten sam adres IP powoduje że jedna czeka na zakończenie drugiego, w trakcie połączenia 0,CONNECT nie pojawia sie np 1,CONNECT tylko po zakończeniu 0,CONNECT pojawia sie z drugiej przegladarki rownież 0,CONNECT */
//					{
//						INIT_BUFF(answer, "+IPD,"ESP_HTTP_CHANNEL);
//						if ((pHttp=strstr_(pHttp,answer)))						/* RecvFromEsp("+IPD,0,698:GET /")   0-channel, 698-received bytes */
//						{
//							if ((pHttp2=strstr_(pHttp,":GET / ")))
//							{
//								GetHTTPpacketParam(pHttp,&channel,&size);					/* char temp[20]={0};  strcpy2_(temp,pHttp,0,pHttp2-pHttp);   temp="+IPD,0,698" */
//								if(typeSendArch!=noArch)  DbgVarDma(DBG,100,_S_"\r\nRecv HTTP data: channel %d  size %d "_E_,channel,size);
//								SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,"AT+CIPSEND=%d,%d\r\n",channel,mini_strlen(HTML_TXT_CODE)), NULL, typeSendArch );
//							}
//							else if ((pHttp2=strstr_(pHttp,":GET /favicon.ico")))				/* Każde nowe połączenie generuje 0,CONNECT tj. czeka na zakończenie jednego by 'weszlo' drugie, nie ma przychodzących rownocześnie połączeń */
//							{
//								GetHTTPpacketParam(pHttp,&channel,&size);
//								if(typeSendArch!=noArch)  DbgVarDma(DBG,100,_S_"\r\nRecv HTTP data: channel %d  size %d "_E_,channel,size);
//								SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPCLOSE=%d\r\n",channel), NULL, typeSendArch );
//							}
//							else UpdateReadPos();
//						}
//					}
//					else if (RecvFromEsp("\r\nOK\r\n\r\n>"))
//					{
//					/*	SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,HTML_TXT_CODE), NULL, typeSendArch ); */
//						strcpy(sendBuff,HTML_TXT_CODE);  SendToEsp32(2039,NULL,typeSendArch /*noArch*/ );
//					}
//					else if (RecvFromEsp(",CLOSED\r\n"))
//					{
//						if (RecvFromEsp("\r\nOK\r\n"))
//						{
//							if(typeSendArch!=noArch) DbgDma(DBG, _S_" --- CLOSED --- "_E_);
//							UpdateReadPos();
//						}
//					}
//					else if (RecvFromEsp("ERROR"))
//					{
//						if(typeSendArch!=noArch) DbgDma(DBG, _S_" --- ERROR --- "_E_);
//						UpdateReadPos();
//					}
//					else if ((pHttp=RecvFromEsp("\r\nRecv ")))						/* RecvFromEsp("\r\nRecv 88 bytes")   88-received bytes by ESP */
//					{
//						if (strstr_(pHttp," bytes\r\n")){
//							if (strstr_(pHttp,"\r\nSEND OK"))    //!!!!!!!!!!!!!!!!!!!!!!!!!!!TO tez do jednej funkcjia dac !!!!!!!!!!!!!!!!!
//							{
//								char temp[50]={0};  strcpy_(temp,pHttp,1,'\r');		/* strcpy2_(temp,pHttp,0,30); */
//								int val = STRING_GetInt(temp,' ');					/* val = atoi_(pHttp,mini_strlen("\r\nRecv ")); */
//								if(typeSendArch!=noArch){
//									DbgVarDma(DBG,200,_S_"\r\n%d received bytes by ESP32 "_E_,val);
//									DbgDma(DBG, _S_" --- SEND OK --- "_E_);
//								}
//
//								if(nrPages > 100){  nrPages=0;
//									SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPCLOSE=%d\r\n",channel), NULL, typeSendArch);		/* Czas wykonania SendToEsp32() to 28us */
//								}
//								else{  nrPages++; DbgDma(1,".");
//									SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,"AT+CIPSEND=%d,%d\r\n",channel,mini_strlen(HTML_TXT_CODE)), NULL, typeSendArch );
//								}
//							}
//						}
//					}
//					else
//					{
//						UpdateReadPos();
//					}
//					break;


				case HTTP_CONNECTION:
					int flag=0;
					typeSendArch=noArch;
					/*DispRecvBuff(++nrHTTPpacket,typeSendArch);*/  ESP32_FreeAnswers(0);
					RstTimeBtwnSendRcv();

					if (RecvFromEsp("+IPD,")||RecvFromEsp(",CONNECT"))
					{
						DispRecvBuff(++nrHTTPpacket,arch);		/* Show only GET Http */
						if (RecvFromEsp("+IPD,"))
						{
							char answ[20]={0};
							pHttp = NULL;
							LOOP_FOR(nrChnl,ESP_MAX_HTTP_CONN)
							{
								mini_snprintf(answ,sizeof(answ)-1,"+IPD,%d",nrChnl);

								if ((pHttp2=strstr_(pHttp,answ)))
								{
									if (strstr_(pHttp2,":GET / "))
									{
										GetHTTPpacketParam(pHttp2,&channel,&size);
										SetRqstToSendChnl(channel,(char*)0x00000001);
										HTTP_ShowInitChannel(channel,size,typeSendArch);
									}
									else if (strstr_(pHttp2,":GET /favicon.ico"))
									{
										GetHTTPpacketParam(pHttp2,&channel,&size);
										SetRqstToSendChnl(channel,(char*)0x00000002);
										HTTP_ShowInitChannel(channel,size,typeSendArch);
									}
								}
							}
						}
						HTTP_SendCloseChnlInit(typeSendArch);

					}
					if ((pHttp=RecvFromEsp("\r\nOK\r\n\r\n>")))
					{
					/*	SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,HTML_TXT_CODE), NULL, typeSendArch ); */
						strcpy(sendBuff,HTML_TXT_CODE);  SendToEsp32_http(2039,NULL,noArch/*typeSendArch*/);
						// i tu przesuwam  wskaznik do html   httpPar.ptr[ nrChnl]  o tyle iel wyslalem

					}
					if ((pHttp=RecvFromEsp(",CLOSED\r\n")))
					{
						u8 chnlClose=HTTP_GetClosedChannel(pHttp);
						HTTP_ShowClosedChannel(chnlClose,typeSendArch);

						if(httpPar.chnl == chnlClose)    /* jesli ten Case nastapil po wysylce at+cipsend lub at+cipclose - SYNCHRONICZNY */
						{
							LOOP_FOR(nrChnl,ESP_MAX_HTTP_CONN){
								if(httpPar.que[nrChnl]!=0xFF){   if(httpPar.chnl == httpPar.que[nrChnl]){
									httpPar.que[nrChnl]=0xFF;
									httpPar.nr[nrChnl]=0;
									httpPar.ptr[nrChnl]=NULL;
									break;
							}}}
							httpPar.chnl=0xFF;		/* zezwol na nastepna wysylke */
							HTTP_SendCloseChnl(typeSendArch);
						}
						else /* if(httpPar.chnl != chnlClose) */       /* jesli ten Case nastapil NIE po wysylce at+cipsend lub at+cipclose - ASYNCHRONICZNY */
						{
							LOOP_FOR(nrChnl,ESP_MAX_HTTP_CONN){
								if(httpPar.que[nrChnl]!=0xFF){   if(chnlClose == httpPar.que[nrChnl]){
									httpPar.que[nrChnl]=0xFF;
									httpPar.nr[nrChnl]=0;
									httpPar.ptr[nrChnl]=NULL;
									break;
							}}}
						}

					}
					if ((pHttp=RecvFromEsp("ERROR")))
					{
						if(typeSendArch!=noArch) DbgDma(DBG, _SE_" --- ERROR --- "_E_);

					}
					if ((pHttp=RecvFromEsp("\r\nSEND OK")))
					{
						httpPar.chnl=0xFF;		/* zezwol na nastepna wysylke */
						HTTP_SendCloseChnl(typeSendArch);

					}
					UpdateReadPos();
					break;

//i jesli nie bedzie odpowiedzi po np 30 sekund to timercallbak timeout !!!!!!
//ZROB tablice allokacji !!!!!!!!!!!!!! wyswieltlanie na zadanie
					//SPRAWDZ czy czasem razem nie moze isc HTTP i SMTP !!!!!!!


				case TEST_CONNECTION:
					if (CASE_Service(0,txt_OK,txt_ERR,typeRecvArch))  //DAJ ABY  RECV_START_TEST:  zastanow sie
					{
						DbgVarDma(DBG,50, _SE_"\r\nTest OK "_E_);
						BackToHttpService(&nrHTTPpacket);

					}
					else
					{
						ESP32_FreeAnswers(1/*2*/);			/* Jezeli NIE wchodzimy w żaden case to rownież wywolujemy ESP32_FreeAnswers() - podobnie jak w pozostalych case */
					}
					break;

//DAJ odmierzanie timer po wysylanie jesli po czasie pewnym nie dostanie odpowiedzi zadanej to wywoluje  funkcje colback timer i tam zamyka polaczenie dane czy 0 http czy 4 SMTP !!!!!
//Musisz sprawdzac czy wisi jakies polaczenie za pomoca AT+CIPSTATUS jesi tak to at+cipclose=x
//Send - timer start - timer over: callbackFunction Timer, ktory wysyla status jesli jest cos to zamyka,      KAZDY recv stopTimerCalback




				case SMTP_CONNECTION:
					if (CASE_Service(0,txt_OK,txt_ERR,typeRecvArch))		/* CASE_Service() osluguje ESP32_FreeAnswers() ale tylko wtedy gdy jest ktoras z odpowiedzi */
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSTART="ESP_EMAIL_CHANNEL",\"%s\",\"%s\",%d\r\n",CONDITION(Const.emailSend[EmailSendParam.whichSender].useSSL,"SSL","TCP"), IP2Str(Const.emailSend[EmailSendParam.whichSender].IP), 	 Const.emailSend[EmailSendParam.whichSender].port);
					 /* len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSTART="ESP_EMAIL_CHANNEL",\"%s\",\"%s\",%d\r\n",CONDITION(Const.emailSend[EmailSendParam.whichSender].useSSL,"SSL","TCP"), 		 Const.emailSend[EmailSendParam.whichSender].server, Const.emailSend[EmailSendParam.whichSender].port); */
						SendToEsp32(len,NULL,typeSendArch);
						COMMAND_Service(_SET,sendBuff);
						nrSMTP=0;

					}
					else if (CASE_Service(1, ESP_EMAIL_CHANNEL",CONNECT\r\n"TXT_OK, txt_ERR, typeRecvArch))		 /* "...,CONNECT\r\n\r\nOK\r\n"  a  "\r\n+IPD,..."  jest szczelina czasowa, mozna wydluzyc parametr timeout dla UART6 aby nie generowalo przerwania po 1 czesci ale jednak robimy inaczej: czekamy na calosc 1 czesc i 2 czesc w CASE_Service() */
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }									  /* Details:"4,CONNECT\r\n\r\nOK\r\n\r\n+IPD,4,31:220 smtp.poczta.onet.pl ESMTP\r\n\r\n", '\0' <repeats 1985 times> */
						INIT_BUFF(answer, "\r\n+IPD,"ESP_EMAIL_CHANNEL);										  /* Details:"4,CONNECT\r\n\r\nOK\r\n\r\n+IPD,4,78:421 4.7.0 smtp.poczta.onet.pl Error: too many connections from 46.205.198.71\r\n\r\n", '\0' <repeats 1938 times> */
						if ((ptr=RecvFromEsp(answer)))															  /* Free answer					   	 +IPD,4,55:421 4.4.2 smtp.poczta.onet.pl Error: timeout exceeded */
						{
							GetSMTPpacketParam(ptr,answer,&channel,&size,&code);
							if(typeSendArch!=noArch)  DbgVarDma(DBG,100,_S_"\r\nRecv email data: channel %d  size %d  code %d"_E_,channel,size,code);
							if(code==220)
							{
								DbgDma(DBG, _S_" --- Email 220 --- "_E_);
								len = mini_snprintf(sendBuff, sizeof(sendBuff), "EHLO %s\r\n", Const.emailSend[EmailSendParam.whichSender].name);
								if(SMTP_SendCmd(typeSendArch,len)) BackFromEmail(0);
							}
							else BackFromEmail(1);
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(2,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);

					}
					else if (CASE_Service(3,"\r\nSEND OK",txt_ERR,typeRecvArch)) 		/* Details:"\r\nRecv 20 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,4,169:250-smtp.poczta.onet.pl\r\n250-PIPELINING\r\n250-SIZE 90000000\r\n250-ETRN\r\n250-AUTH PLAIN LOGIN XOAUTH2\r\n250-AUTH=PLAIN LOGIN XOAUTH2\r\n250-ENHANCEDSTATUSCODES\r\n250... */
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						if(Is_ComplRecvSMTPpacket()){
							SMTP_Descr(ptr,typeSendArch,&channel,&size,&code);
							if(250==code){  if(SMTP_SendCmd(typeSendArch,mini_snprintf(sendBuff,sizeof(sendBuff),"AUTH LOGIN\r\n"))) BackFromEmail(0);  }else BackFromEmail(1);
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(4,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);

					}
					else if (CASE_Service(5,"\r\nSEND OK",txt_ERR,typeRecvArch)) 		/* Details:"\r\nRecv 20 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,4,18:334 VXNlcm5hbWU6 */
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						if(Is_ComplRecvSMTPpacket()){
							SMTP_Descr(ptr,typeSendArch,&channel,&size,&code);			/* if(strstr_(ptr,"VXNlcm5hbWU6") */
							if(334==code){  if(SMTP_SendCmd(typeSendArch,mini_snprintf(sendBuff,sizeof(sendBuff),"%s\r\n",base64_enc2(Const.emailSend[EmailSendParam.whichSender].login)))) BackFromEmail(0);  }else BackFromEmail(1);
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(6,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);

					}
					else if (CASE_Service(7,"\r\nSEND OK",txt_ERR,typeRecvArch)) 		/* Details:"\r\nRecv 20 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,4,18:334 UGFzc3dvcmQ6 */
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						if(Is_ComplRecvSMTPpacket()){
							SMTP_Descr(ptr,typeSendArch,&channel,&size,&code);				/* if(strstr_(ptr,"UGFzc3dvcmQ6") */
							if(334==code){  if(SMTP_SendCmd(typeSendArch,mini_snprintf(sendBuff,sizeof(sendBuff),"%s\r\n",base64_enc2(Const.emailSend[EmailSendParam.whichSender].password)))){BackFromEmail(0);break;}  }else{BackFromEmail(1);break;}
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(8,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);

					}
					else if (CASE_Service(9,"\r\nSEND OK",txt_ERR,typeRecvArch))  		/* Details:"\r\nRecv 20 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,4,37:235 2.7.0 Authentication successful */
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						if(Is_ComplRecvSMTPpacket()){
							SMTP_Descr(ptr,typeSendArch,&channel,&size,&code);
							if(235==code){  if(SMTP_SendCmd(typeSendArch,mini_snprintf(sendBuff,sizeof(sendBuff),"MAIL FROM:<%s>\r\n",Const.emailSend[EmailSendParam.whichSender].login))) BackFromEmail(0);  }else BackFromEmail(1);
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(10,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);
						else nrSMTP=0;

					}
					else if (CASE_Service(11,"\r\nSEND OK",txt_ERR,typeRecvArch)) 		/* Details:"\r\nRecv 20 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,4,14:250 2.1.5 Ok*/
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						if(Is_ComplRecvSMTPpacket()){
							int isNextMailTo=0;
							SMTP_Descr(ptr,typeSendArch,&channel,&size,&code);
							if(250==code){
								for(int i=nrSMTP; i<MAX_EMAIL_RECIPIENTS; ++i){	  if((EmailSendParam.recepientsMask>>i)&0x01){ nrSMTP=i+1; isNextMailTo=1; break; }	  }
								if(isNextMailTo){
										if(SMTP_SendCmd(typeSendArch,mini_snprintf(sendBuff,sizeof(sendBuff),"RCPT TO:<%s>\r\n",Const.emailRecv[nrSMTP-1].email))) BackFromEmail(0);
								}
								else{	if(SMTP_SendCmd(typeSendArch,mini_snprintf(sendBuff,sizeof(sendBuff),"DATA\r\n"))) BackFromEmail(0);
										else _SET_NEW_CASE_(13);  }
							}
							else BackFromEmail(1);
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(12,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);
						else _SET_NEW_CASE_(11);

					}
					else if (CASE_Service(13,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);
						 _SET_NEW_CASE_(80);

					}
					else if (CASE_Service(80,"\r\nSEND OK",txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						if(Is_ComplRecvSMTPpacket()){
							SMTP_Descr(ptr,typeSendArch,&channel,&size,&code);
							if(354==code){  if(SMTP_SendCmd(typeSendArch,mini_snprintf(sendBuff,sizeof(sendBuff),"FROM: %s\r\n",Const.emailSend[EmailSendParam.whichSender].login))) BackFromEmail(0);  }else BackFromEmail(1);
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(81,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);
						_SET_NEW_CASE_(14);

					}
					else if (CASE_Service(14,"\r\nSEND OK",txt_ERR,typeRecvArch))		/* +IPD,4,37:354 End data with <CR><LF>.<CR><LF> */
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
/*						if(Is_ComplRecvSMTPpacket()){
							SMTP_Descr(ptr,typeSendArch,&channel,&size,&code);
							if(354==code){ */
								len=mini_snprintf(sendBuff,sizeof(sendBuff),"TO: ");  j=0;
								for (int i=0; i<MAX_EMAIL_RECIPIENTS; ++i){  if((EmailSendParam.recepientsMask>>i)&0x01)  len+=mini_snprintf(sendBuff+len,sizeof(sendBuff)-len,"%s,",Const.emailRecv[i].email);  }
								len+=mini_snprintf(sendBuff+len, sizeof(sendBuff), "\r\n");
								if(SMTP_SendCmd(typeSendArch,len)) BackFromEmail(0);
/*							}
							else BackFromEmail(1);
						}
						else _THE_SAME_CASE_; */

					}
					else if (CASE_Service(15,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);

					}
					else if (CASE_Service(16,"\r\nSEND OK",txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						len=mini_snprintf(sendBuff, 	 PACKET_SEND_LEN,  	  "Content-Transfer-Encoding: 8bit\r\n");
						len+=mini_snprintf(sendBuff+len, PACKET_SEND_LEN-len, "Content-Type: text/html; charset=\"UTF-8\"");
						len+=mini_snprintf(sendBuff+len, PACKET_SEND_LEN-len, "\r\n\r\n");
						if(SMTP_SendCmd(typeSendArch,len)) BackFromEmail(0);

					}
					else if (CASE_Service(17,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);

					}
					else if (CASE_Service(18,"\r\nSEND OK",txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						len=mini_snprintf(sendBuff, 	 PACKET_SEND_LEN, 	  "Rafal Markielowski\r\nAAAAA\r\nBBB");
						len+=mini_snprintf(sendBuff+len, PACKET_SEND_LEN-len, "\r\n\r\n.\r\n");
						if(SMTP_SendCmd(typeSendArch,len)) BackFromEmail(0);

					}
					else if (CASE_Service(19,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);

					}
					else if (CASE_Service(20,"\r\nSEND OK",txt_ERR,typeRecvArch))				/* +IPD,4,40:250 2.0.0 Ok: queued as 4gyw1S00G4zjHD */
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						if(Is_ComplRecvSMTPpacket()){
							SMTP_Descr(ptr,typeSendArch,&channel,&size,&code);
							if(250==code){  if(SMTP_SendCmd(typeSendArch,mini_snprintf(sendBuff,sizeof(sendBuff),"quit\r\n"))) BackFromEmail(0);  }else BackFromEmail(1);
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(21,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(SMTP_SendData(typeSendArch)) BackFromEmail(0);

					}
					else if (CASE_Service(22,"\r\nSEND OK",txt_ERR,typeRecvArch))				/* +IPD,4,15:221 2.0.0 Bye */
					{
						if(ErrorAnswerService()){ BackFromEmail(0); break; }
						if(Is_ComplRecvSMTPpacket()){
							SMTP_Descr(ptr,typeSendArch,&channel,&size,&code);
							if(221==code) BackFromEmail(0); else BackFromEmail(1);
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(98,ESP_EMAIL_CHANNEL",CLOSED\r\n"TXT_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()){ ; }
						SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSERVER=1,443,\"SSL\"\r\n"),NULL,typeRecvArch );
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(99,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()){ ; }
						DbgVarDma(DBG,50, _SE_"\r\nWracma do HTTP "_E_);  BackToHttpService(&nrHTTPpacket);	 EmailSendParam.start=0;	InitStructRqstToSendChnl();

					}
					else
					{
						ESP32_FreeAnswers(1/*2*/);			/* Jezeli NIE wchodzimy w żaden case to rownież wywolujemy ESP32_FreeAnswers() - podobnie jak w pozostalych case */

					}
					break;
			}


			uint32_t ulPoprzedniaWartosc = ulTaskNotifyValueClear(NULL, ulNotifiedValue);
			if ((ulPoprzedniaWartosc & BIT_ESP_SRV) != 0) {												/* Sprawdzenie, czy zgłoszenie w ogóle występowało przed wyczyszczeniem: */

				asm("nop");
			}

		}

		if (ulNotifiedValue & BIT_DBG_SRV)
		{
			DEBUG_InvalidateDCache();

			if(DEBUG_IsTxtReceive("a"))
			{
				int n=mini_snprintf(sendBuff,sizeof(sendBuff),"\r\nHTTP PAR: chnl:%d\r\n",httpPar.chnl);
				LOOP_FOR(i,ESP_MAX_HTTP_CONN){	 n+=mini_snprintf(sendBuff+n,sizeof(sendBuff)-n,"ptr:%d		que:%d		nr:%d\r\n",httpPar.ptr[i], httpPar.que[i], httpPar.nr[i]);	}
				DbgDmaQue(DBG,sendBuff,0);

			}
			else if(DEBUG_IsTxtReceive("s"))
			{
				if(GetTimeBtwnSendRcv() && connectionType==HTTP_CONNECTION)
				{
					SendEmail(2, 1<<1, EMAIL_MEASURE);			/* z interia.pl musi byc ustawione: Główne_Ustawienia -> Parametry -> Korzystam z programu pocztowego aby mogl wysylac */

					if( (WIFI_MODE_STA 	  == Const.wifiGeneral.mode   ||
						 WIFI_MODE_AP_STA == Const.wifiGeneral.mode)  &&  Const.emailSend[ EmailSendParam.whichSender ].IP )
					{
						typeSendArch=arch;
						DbgDma(DBG, _S_"\r\nWysylam email... "_E_);
						_CLR_ACTUAL_CASE_;
						connectionType=SMTP_CONNECTION;
						SendToEsp32(0,"AT+CIPSERVER=0\r\n",typeSendArch);
						COMMAND_Service(_SET,sendBuff);
					}
				}
				else{ DbgDma(DBG, _S_"\r\nESP jest zajety..."_E_); }

			}
			else if(DEBUG_IsTxtReceive("x"))
			{
				connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
				DbgDma(DBG, _S_"\r\nSend Test AT "_E_);
				SendToEsp32(0,"AT+SYSTIMESTAMP?\r\n",arch);
			}


//SendToEsp32(0,"AT+CIPSSLCCONF?\r\n",arch);
//SendToEsp32(0,"AT+CIPSSLCCONF=0,0\r\n",arch);

//			AT+CWLAP
//			AT+CWSAP?
//			AT+CWQAP diconnect z softAP
//			AT+CWLIF
//			AT+CWAPPROTO?
//			AT+CIPSNTPCFG=1,8,"cn.ntp.org.cn","ntp.sjtu.edu.cn"
//			AT+CIPSNTPTIME?
//			AT+CIPSNTPINTV?
//			AT+CIPSNTPINTV=3600   //time every hour
//			AT+CIPSSLCCONF?

//SendToEsp32(0,"AT+CIPSSLCSNI=0,\"smtp.interia.pl\"\r\n",arch);


//			// Single connection: (AT+CIPMUX=0)
//			AT+CIPSSLCCONF=<auth_mode>[,<pki_number>][,<ca_number>]
//			// Multiple connections: (AT+CIPMUX=1)
//			AT+CIPSSLCCONF=<link ID>,<auth_mode>[,<pki_number>][,<ca_number>]



			//AT+CIPSSLCCIPHER?
//			AT+CIPSSLCCIPHER=2,0xC023,0xC0AD       // Single connection: (AT+CIPMUX=0), cipher suites are TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256 and TLS_ECDHE_ECDSA_WITH_AES_256_CCM
//			AT+CIPSSLCCIPHER=0,2,0xC023,0xC0AD    // Multiple connections: (AT+CIPMUX=1), cipher suites are TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256 and TLS_ECDHE_ECDSA_WITH_AES_256_CCM







			//AT+SYSMFG?	   //AT+SYSMFG=<operation>,<"namespace">[,<"key">]


//			// Erase all key-value pairs of client_cert namespace (That is, erase all client␣
//			,→certificates)
//			AT+SYSMFG=0,"client_cert"
//			// Erase the client_cert.0 key-value pair of client_cert namespace (That is, erase␣
//			,→the first client certificate)
//			AT+SYSMFG=0,"client_cert","client_cert.0"

//			// Read all namespaces
//			AT+SYSMFG=1
//			// Read all key-value pairs of client_cert namespace
//			AT+SYSMFG=1,"client_cert"
//			// Read the value of client_cert.0 key in client_cert namespace
//			AT+SYSMFG=1,"client_cert","client_cert.0"
//			// Read the value of client_cert.0 key in client_cert namespace, from offset: 100␣
//			,→place, read 200 bytes
//			AT+SYSMFG=1,"client_cert","client_cert.0",100,200


//			// Write a new value for client_cert.0 key into client_cert namespace (That is,␣
//			,→update the 0th client certificate)
//			AT+SYSMFG=2,"client_cert","client_cert.0",8,1164
//			// Wait until AT command port returns ``>``, and then write 1164 bytes


//SNTP aktualny czas
// CIPSSLCCONF
			//


			else if(DEBUG_IsTxtReceive("0")){ GoToTest(0); SendToEsp32(0,"AT+RFPOWER?\r\n",arch); }
			else if(DEBUG_IsTxtReceive("1")){ GoToTest(0); SendToEsp32(0,"AT+RFCAL\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("2")){ GoToTest(0); SendToEsp32(0,"AT+CWSTATE?\r\n",arch); }
			else if(DEBUG_IsTxtReceive("3")){ GoToTest(0); SendToEsp32(0,"AT+CWLAP\r\n",arch); }
			else if(DEBUG_IsTxtReceive("4")){ GoToTest(0); SendToEsp32(0,"AT+CWSAP?\r\n",arch); }
			else if(DEBUG_IsTxtReceive("5")){ GoToTest(0); SendToEsp32(0,"AT+CWQAP\r\n",arch); }
			else if(DEBUG_IsTxtReceive("6")){ GoToTest(0); SendToEsp32(0,"AT+CWLIF\r\n",arch); }
			else if(DEBUG_IsTxtReceive("7")){ GoToTest(0); SendToEsp32(0,"AT+CWAPPROTO?\r\n",arch); }
			else if(DEBUG_IsTxtReceive("8")){ GoToTest(0); SendToEsp32(0,"AT+CIPSNTPCFG=1,1,\"pool.ntp.org\"\r\n",arch); }   //"AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"\r\n"
			else if(DEBUG_IsTxtReceive("9")){ GoToTest(0); SendToEsp32(0,"AT+CIPSNTPTIME?\r\n",arch); }
			else if(DEBUG_IsTxtReceive("q")){ GoToTest(0); SendToEsp32(0,"AT+CIPSNTPINTV?\r\n",arch); }
			else if(DEBUG_IsTxtReceive("w")){ GoToTest(0); SendToEsp32(0,"AT+CIPSNTPINTV=3600\r\n",arch); }
			else if(DEBUG_IsTxtReceive("e")){ GoToTest(0); SendToEsp32(0,"AT+CIPSSLCCONF?\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("r")){ GoToTest(0); SendToEsp32(0,"AT+CIPSSLCCIPHER?\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("t")){ GoToTest(0); SendToEsp32(0,"AT+CIPSSLCCIPHER=0,2,0xC023,0xC0AD\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("y")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG?\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("i")){ GoToTest(0); SendToEsp32(0,"AT+CIPSSLCCONF=4,2,0,0\r\n",arch);    }  //aktywacja cert SMTP
			else if(DEBUG_IsTxtReceive("d")){ GoToTest(0); SendToEsp32(0,"AT+CIPSSLCCONF=4,0\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("k")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=2,\"client_ca\",\"client_ca.0\",8,807\r\n",arch);    }  //zapisz
			else if(DEBUG_IsTxtReceive("j")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=1,\"client_ca\",\"client_ca.0\"\r\n",arch);    }  //odpytaj sprawdz czy zapisano     //client_ca.1,2,3.. to klejne certy
			else if(DEBUG_IsTxtReceive("h")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=1,\"client_ca\",\"client_ca.1\"\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("g")){ GoToTest(0); SendToEsp32(0,"AT+CIPSSLCSNI=4,\"poczta.interia.pl\"\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("f")){ GoToTest(0); SendToEsp32(0,"AT+CIPSSLCSNI?\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("m")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=1,\"client_ca\",\"client_ca\"\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("n")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=2,\"client_ca\",\"client_ca.1\",8,807\r\n",arch);    }




			else if(DEBUG_IsTxtReceive("?"))
			{
				connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
				DbgDma(DBG, _S_"\r\nWysylam DATA cert "_E_);
				//SendToEsp32(0,"-----BEGIN CERTIFICATE-----MIIDrzCCApegAwIBAgIQCDvgVpBCwQdBmJ8V4vhx7DANBgkqhkiG9w0BAQsFADBhMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBDQTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5jb20xIDAeBgNVBAMMF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1EQmYN5QLuEZDY6ntPEGwihkagMrYueAtvxhgreZdkgy5y5CsBBe3N5nJJyXghQIGMC4gKs8Omc79w4Z89yr3DH4mTJUYisH6g6NnIKVWhm5O1LKQXwpAsBq6t8UR695QnNMc9asj6L4gqq8GAe4uM9N178gKv1Cyg9SxZg6Es747teA9Wh2BKRpoY32SWhF9b8SjK4769PkVqAhDJ1U0Y7FVThB4S960uG6P440dgE2B3S4Y6T6qv7vU76RAfB9CTh9K6GfFED7P1B2b4vP5lBp4mMc36X9U8v5eB6H8K5gJ2779EX0G2uLgWp4f9Jtw1F65A7GwIDAQABo2MwYTAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QnvStFj8Q92bJv8ONM0OD8KgwHwYDVR0jBBgwFoAUA95QnvStFj8Q92bJv8ONM0OD8KgwDQYJKoZIhvcNAQELBQADggEBAF3m3EehIiKNtAsST6VKVDTjCejY373bTIi7P35XmKAVffv2yQth564v96f7y9S41U9fDxU8yX5VfJv+D0P/X7H7gqH5fU1FfZ/pX76V/0j8+y6b0g177p+p7gK4S9P6VfXf9C8=-----END CERTIFICATE-----",arch);  //1108
				SendToEsp32(0,"-----BEGIN CERTIFICATE-----\nMIIDjjCCAnagAwIBAgIQAzrx5W63S0b6AE5BHSTHPTANBgkqhkiG9w0BAQsFADBh\nMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\nd3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\nMjAeFw0zMDA4MDExMjAwMDBaFw01MDA4MDExMjAwMDBaMGExCzAJBgNVBAYTAlVT\nMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWcipWVydC5j\nb20xIDAeBgNVBAMMF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1EQmYqrrXSVzXgS7OwtWc9L\nFBBg6snA5bVvOr4H7PDg3VU8qgifgJuogGxOn4UOFIOMpUMG7up6X8XSpYtH8Wwi\nhy67vvwbFJuINkwXgX6V1E24gwEsPDyDxa6e4T6kKvTOUeNANJMch80wFH57S875\nfA3lyWMa2L6+fThvAt04VIO4u30R73MD4b46p6UoMQk96Sbt8v60pYgCE6EAsvIK\nA67X9M1N9L04wREOcIEX3gS6g7LwK7Bf8XpBfXbSB3o9A7S3XfR7O2EDMA40OpFL\nwX10r8A7E+V2N8H4NURbY6K7J4wNURM9OhSgI==-----END CERTIFICATE-----",arch);   //807
			}
			else if(DEBUG_IsTxtReceive("6"))
			{
				connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
				DbgDma(DBG, _S_"\r\nSend Test AT3 "_E_);
				SendToEsp32(0,"AT+SYSFLASH?\r\n",arch);
			}











			else if(DEBUG_IsTxtReceive("o"))
			{
				connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
				DbgDma(DBG, _S_"\r\nSend Test AT "_E_);
				SendToEsp32(0,"AT+CIPSTATUS\r\n",arch);
			}
			else if(DEBUG_IsTxtReceive("p"))
			{
				connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
				DbgDma(DBG, _S_"\r\nSend Test AT "_E_);
				SendToEsp32(0,"AT+CIPSTATE?\r\n",arch);
			}
//			else if(DEBUG_IsTxtReceive("z"))
//			{
//				DbgDma(DBG, _S_"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"_E_);
//			}
//			else if(DEBUG_IsTxtReceive("q"))
//			{
//				Dbg(DBG, _S_"\r\n uint32_t ulPoprzedniaWartosc = ulTaskNotifyValueClear(NULL, ulNotifiedValue) "_E_);
//			}
//			else if(DEBUG_IsTxtReceive("c"))
//			{
//				DbgDma(DBG, Clr_"\r\n-------------------- Start -------------------- ");
//			}




			uint32_t ulPoprzedniaWartosc = ulTaskNotifyValueClear(NULL, ulNotifiedValue);
			if ((ulPoprzedniaWartosc & BIT_DBG_SRV) != 0) {												/* Sprawdzenie, czy zgłoszenie w ogóle występowało przed wyczyszczeniem: */

				asm("nop");
			}
		}
	  }
	}



//	SEND_START: AT+CIPSTATUS
//	SEND_STOP
//
//	RECV_TEST_000_START:STATUS:3
//	+CIPSTATUS:4,"SSL","213.180.147.145",465,53751,0
//
//	OK
//
//	RECV_STOP
//	Test OK
//	Send Test AT
//	SEND_START: AT+CIPSTATE?
//	SEND_STOP
//
//	RECV_TEST_000_START:+CIPSTATE:4,"SSL","213.180.147.145",465,53751,0
//
//	OK
//
//	RECV_STOP
//	Test OK


/*
	while (1)
	{
		if (true==isAnythingRecv())  //Cykliczne odpytatywanie czy nie zawieszony np AT
		{
			switch (connectionType)
			{
			case HTTP_CONNECTION:
				if ((pHttp=RecvFromEsp(":GET /")))
				{
					GetSizeAndChannel(pHttp, &channel, &size);

					while (GetDMACountByte()<size)
						vTaskDelay(1);

					if (RecvFromEsp(":GET /TME.txt")==0)
						DisplayRequestGET(pHttp, 2000);

					result=vSendDataHTTP(pHttp, channel);
					DbgVar(DBG, 20, "\r\nSend Code: %d ", result);

					result2=vCloseConnection(channel);
					DbgVar(DBG, 20, "\r\nClose Code: %d ", result2);

					if ((result==2)&&(result2==2))
						SendDummyData(100);
					RestartDMA();
				}
				break;

			case SMTP_CONNECTION:
				if (false==vCheckEmailAnswer(220))
					goto GOTO_Email_Quit;
				EmailSendParam.start=4;

				len=mini_snprintf(sendBuff, sizeof(sendBuff), "EHLO %s\r\n", VAR_GetStr(Const_emailSend_name,EmailSendParam.whichSender));
				if (vSendCommandSMTP(sendBuff, len))
					goto GOTO_Email_Quit;
				if (false==vCheckEmailAnswer(250))
					goto GOTO_Email_Quit;

				len=mini_snprintf(sendBuff, sizeof(sendBuff), "AUTH LOGIN\r\n");
				if (vSendCommandSMTP(sendBuff, len))
					goto GOTO_Email_Quit;
				if (false==vCheckEmailAnswer(334))
					goto GOTO_Email_Quit;

				len=mini_snprintf(sendBuff, sizeof(sendBuff), "%s\r\n", base64_enc2(VAR_GetStr(Const_emailSend_login,EmailSendParam.whichSender)));
				if (vSendCommandSMTP(sendBuff, len))
					goto GOTO_Email_Quit;
				if (false==vCheckEmailAnswer(334))
					goto GOTO_Email_Quit;

				len=mini_snprintf(sendBuff, sizeof(sendBuff), "%s\r\n", base64_enc2(VAR_GetStr(Const_emailSend_password,EmailSendParam.whichSender)));
				if (vSendCommandSMTP(sendBuff, len))
					goto GOTO_Email_Quit;
				if (false==vCheckEmailAnswer(235))
					goto GOTO_Email_Quit;

				len=mini_snprintf(sendBuff, sizeof(sendBuff), "MAIL FROM:<%s>\r\n", VAR_GetStr(Const_emailSend_login,EmailSendParam.whichSender));
				if (vSendCommandSMTP(sendBuff, len))
					goto GOTO_Email_Quit;
				if (false==vCheckEmailAnswer(250))
					goto GOTO_Email_Quit;

				j=0;
				for (int i=0; i<MAX_EMAIL_RECIPIENTS; ++i)
				{
					if ((EmailSendParam.recepientsMask>>i)&0x01)
					{
						len=mini_snprintf(sendBuff, sizeof(sendBuff), "RCPT TO:<%s>\r\n", VAR_GetStr(Const_emailRecv_email,j));
						if (vSendCommandSMTP(sendBuff, len))
							goto GOTO_Email_Quit;
						if (false==vCheckEmailAnswer(250))
							goto GOTO_Email_Quit;
					}
					j++;
				}

				len=mini_snprintf(sendBuff, sizeof(sendBuff), "DATA\r\n");
				if (vSendCommandSMTP(sendBuff, len))
					goto GOTO_Email_Quit;
				if (false==vCheckEmailAnswer(354))
					goto GOTO_Email_Quit;

				vTaskDelay(1);
				len=mini_snprintf(sendBuff, sizeof(sendBuff), "FROM: %s\r\nSubject: %s\r\n", VAR_GetStr(Const_emailSend_login,EmailSendParam.whichSender), GetTitleEmail());
				if (vSendCommandSMTP(sendBuff, len))
					goto GOTO_Email_Quit;

				vTaskDelay(1);
				len=mini_snprintf(sendBuff, sizeof(sendBuff), "TO: ");
				j=0;
				for (int i=0; i<MAX_EMAIL_RECIPIENTS; ++i)
				{
					if ((EmailSendParam.recepientsMask>>i)&0x01)
						len+=mini_snprintf(sendBuff+len, sizeof(sendBuff), "%s,", VAR_GetStr(Const_emailRecv_email,j));
					j++;
				}
				len+=mini_snprintf(sendBuff+len, sizeof(sendBuff), "\r\n");
				if (vSendCommandSMTP(sendBuff, len))
					goto GOTO_Email_Quit;
				DbgMulti(DBG,"\r\nRecv mail: ",sendBuff,"\r\n");

				vTaskDelay(1);
				len=mini_snprintf(sendBuff, PACKET_SEND_LEN, "Content-Transfer-Encoding: 8bit\r\n");
				len+=mini_snprintf(sendBuff+len, PACKET_SEND_LEN, "Content-Type: text/html; charset=\"UTF-8\"");
				len+=mini_snprintf(sendBuff+len, PACKET_SEND_LEN, "\r\n\r\n");
				if (vSendCommandSMTP(sendBuff, len))
					goto GOTO_Email_Quit;

				vTaskDelay(1);
				if ((result=vSendDataSMTP(EmailSendParam.id)))
				{
					DbgVar(DBG,50, "\r\nResult Error Send Data: %d ", result);
					goto GOTO_Email_Quit;
				}
				if (false==vCheckEmailAnswer(250))
					goto GOTO_Email_Quit;
				else
					SetEmailState(SMTP_SUCCESS);

				GOTO_Email_Quit:
				if ((RecvFromEsp("closing connection")==0)||(RecvFromEsp(",CLOSED")==0))
				{
					if (vSendDataPacket("quit\r\n", 6, ESP_EMAIL_CHANNEL));
					switch (vGetEmailRecvCode(&channel))
					{
					case 221:
						break;
					default:
						vTaskDelay(200);
						break;
					}
				}
				vCloseConnection(ESP_EMAIL_CHANNEL);

				DbgVar(DBG, 40, "\r\nEMAIL STATUS: %d %s ", GetEmailCode(), GetStrEmailState());
				ErrorServiceSMTP();

				EmailSendParam.start=0;
				connectionType=HTTP_CONNECTION;
				vStartServer();
				RestartDMA();
				break;

			case INIT_CONNECTION:
				if (RecvFromEsp("ready"))
				{
					SendToEsp("ATE0\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);

					mini_snprintf(sendBuff, sizeof(sendBuff), "AT+UART_CUR=%d,8,1,0,0\r\n", ESP_UART_BUADRATE);
					SendToEsp(sendBuff);
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);
					HAL_Delay(10);

					ChangeUartBuadRate(ESP_UART_BUADRATE);
					SendToEsp("AT+GMR\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);
					Dbg(DBG, RecvBuffer);

					len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CWMODE=%d\r\n",VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB));
					SendToEsp2(sendBuff,len);
					DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);

					if(WIFI_MODE_DISABLED==VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
					{
						Dbg(DBG, "\r\nWifi DISABLED ");
						break;
					}

					SendToEsp("AT+CWLAPOPT=1,23\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);

					SendToEsp("AT+CIPMUX=1\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);

					SendToEsp("AT+CWDHCP=0,3\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);

					if(0==VAR_GetTabVal(Const_wifiAP_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB))&&
						1==VAR_GetTabVal(Const_wifiSTA_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)) )
					{
						SendToEsp("AT+CWDHCP=1,1\r\n");
						while (RecvFromEsp("\r\nOK")==0)
							vTaskDelay(10);
					}
					else if(1==VAR_GetTabVal(Const_wifiAP_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB))&&
							  0==VAR_GetTabVal(Const_wifiSTA_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)) )
					{
						SendToEsp("AT+CWDHCP=1,2\r\n");
						while (RecvFromEsp("\r\nOK")==0)
							vTaskDelay(10);
					}
					else if(1==VAR_GetTabVal(Const_wifiAP_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB))&&
							  1==VAR_GetTabVal(Const_wifiSTA_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)) )
					{
						SendToEsp("AT+CWDHCP=1,3\r\n");
						while (RecvFromEsp("\r\nOK")==0)
							vTaskDelay(10);
					}

					SendToEsp("AT+CWHOSTNAME=\"Elektronika_STM\"\r\n");
					while (RecvFromEsp("\r\nOK")==0)  //WProwadzic variable dla hostName
					{
						if (RecvFromEsp("ERROR"))
							break;
						vTaskDelay(10);
					}

					switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
					{
					case WIFI_MODE_STA:
					case WIFI_MODE_AP_STA:
						if(0==VAR_GetTabVal(Const_wifiSTA_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)))
						{
							len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSTA=\"%s\",\"%s\",\"%s\"\r\n",
									IP2Str(VAR_GetTabVal(Const_wifiSTA_ip,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB))),
									IP2Str(VAR_GetTabVal(Const_wifiSTA_gate,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB))),
									IP2Str(VAR_GetTabVal(Const_wifiSTA_mask,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB))));
							SendToEsp2(sendBuff,len);
							DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
							while (RecvFromEsp("\r\nOK")==0)
								vTaskDelay(10);
						}
						break;
					}
					switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
					{
					case WIFI_MODE_AP:
					case WIFI_MODE_AP_STA:
						if(0==VAR_GetTabVal(Const_wifiAP_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB)))
						{
							len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPAP=\"%s\",\"%s\",\"%s\"\r\n",
									IP2Str(VAR_GetTabVal(Const_wifiAP_ip,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB))),
									IP2Str(VAR_GetTabVal(Const_wifiAP_gate,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB))),
									IP2Str(VAR_GetTabVal(Const_wifiAP_mask,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB))));
							SendToEsp2(sendBuff,len);
							DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
							while (RecvFromEsp("\r\nOK")==0)
								vTaskDelay(10);
						}
						break;
					}

					switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
					{
					case WIFI_MODE_AP:
					case WIFI_MODE_AP_STA:
						len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CWSAP=\"%s\",\"%s\",5,3\r\n",
								VAR_GetStr(Const_wifiAP_name,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB)),
								VAR_GetStr(Const_wifiAP_pass,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB)));
						SendToEsp2(sendBuff,len);
						DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
						while (RecvFromEsp("\r\nOK")==0)
							vTaskDelay(10);
						break;
					}
					switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
					{
					case WIFI_MODE_STA:
					case WIFI_MODE_AP_STA:
						len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CWJAP=\"%s\",\"%s\"\r\n",
								VAR_GetStr(Const_wifiSTA_name,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)),
								VAR_GetStr(Const_wifiSTA_pass,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)));  // Timer do logowania !!!!!  i poprawic GetPort !! zamiast port na indeks
						SendToEsp2(sendBuff,len);
						DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
						result=vGetConnectionResultToSTA();
						if(ESP_CONNECTION_OK!=result)
							DbgVar(DBG,30,"\r\nERROR_ESP_CONNECTION: %d\r\n",result);
						break;
					}

					SendToEsp("AT+CIFSR\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);
					GetAddressesForConnection();
					Dbg(DBG, RecvBuffer);

					SendToEsp("AT+CIPSERVERMAXCONN=1\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);

					len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPDNS=1,\"%s\",\"%s\",\"%s\"\r\n",
							IP2Str(VAR_GetTabVal(Const_dns_IP1,NO_TAB)),
							IP2Str(VAR_GetTabVal(Const_dns_IP2,NO_TAB)),
							IP2Str(VAR_GetTabVal(Const_dns_IP3,NO_TAB)));
					SendToEsp2(sendBuff,len);
					DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(1);
					Dbg(DBG, RecvBuffer);

					len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSNTPCFG=1,%d,\"%s\",\"%s\"\r\n",
							VAR_GetTabVal(Const_sntp_timezone,NO_TAB),
							VAR_GetStr(Const_sntp_nameServer1,NO_TAB),
							VAR_GetStr(Const_sntp_nameServer2,NO_TAB));
					SendToEsp2(sendBuff,len);
					DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(1);
					Dbg(DBG, RecvBuffer);

					vStartServer();

					len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSTO=%d\r\n",TCP_SERVER_TIMEOUT_S);
					SendToEsp2(sendBuff,len);
					while (RecvFromEsp("\r\nOK")==0)
						vTaskDelay(10);

					switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
					{
						case WIFI_MODE_STA:
						case WIFI_MODE_AP_STA:
							vQueryAndReplaceEmailAddrName2AddrIP();
							if(0==vQueryAndLoadTimeFromSNTP())
								vLoadTime(VAR_GetTabVal(Const_sntp_time,NO_TAB));
							break;
						default:
							vLoadTime(VAR_GetTabVal(Const_sntp_time,NO_TAB));
							break;
					}

					Dbg(DBG, "\r\nESP INIT OK ");
					RestartDMA();
					connectionType=HTTP_CONNECTION;
				}
				break;
			}
		}

		if (resetDMA==1)
		{
			resetDMA=0;
			Dbg(DBG, "\r\nERROR USART6 ");
			if (EmailSendParam.start==0)
				SendDummyData(200);
			RestartDMA();
			EmailSendParam.start=0;
			vRestartWifiServer();
		}

		if (EmailSendParam.start==1)
			EmailSendParam.start++;
		else if (EmailSendParam.start==2)
			EmailSendStart();

		vTaskDelay(20);
	}*/
}

void CreateWifiTask(void)
{
	xTaskCreate(vtaskWifi, "vtaskWifi", 1024, NULL, (unsigned portBASE_TYPE ) 4, &vtaskWifiHandle);
}

void CloseWifiTask(void)
{
	vTaskDelete(vtaskWifiHandle);	vtaskWifiHandle=NULL;
	ESP_OFF;
	ChangeUartBuadRate(115200);
}

void RestartWifiTask(void)
{
	CloseWifiTask();
	vTaskDelay(200);
	CreateWifiTask();
}

void WIFI_UartErrorService(void)
{
	resetDMA=1;  //Nie tak tylko semafor !!!!!
}

void WIFI_RxCallbackService(void)
{
	Dbg(DBG, "\r\n -----  USART6  HAL_UART_RxCpltCallback -------  ");  //xTaskNotify i do vLogTask !!!!
}

//------------- ATTENTIONS ------------------------------
/* AKTUALIZUJ firmware ESP przez strone ESp Home bo przez esp download tool nie dziala */   /* !!! BOOT pin (IO0) low,  EN high !!!		RXD0,TXDO -> do progr.  IO17,IO18 -> at commands*/
/* https://web.esphome.io/ */
/* https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Binary_Lists/esp_at_binaries.html#firmware-esp32-wroom-32-series */
/* Aby sprawdzic firmawer : at+gmr [enter]+[ctrl+j] */
/*
AT version:2.1.0.0(883f7f2 - Jul 24 2020 11:50:07)
SDK version:v4.0.1-193-ge7ac221
compile time(0ad6331):Jul 28 2020 02:47:21
Bin version:2.1.0(WROOM-32)

OK
+CIFSR:APIP,"192.168.4.1"
+CIFSR:APMAC,"4c:11:ae:f9:45:55"
+CIFSR:STAIP,"192.168.1.71"
+CIFSR:STAMAC,"4c:11:ae:f9:45:54"

OK

AT+CIPSTAMAC="1a:fe:35:99:d5:4A"
AT+CIFSR  //...jesli IP 0.0.0.0 to cyklicznie �aczyc z skoja�on� siecia CWJAP (CWSAP) az do polaczenia
AT+CWJAP?  wyswietla parametry sieci do ktorej jest podlaczony
AT+CWSAP?  wyswietla parametry swojej sieci AP
AT+CWLIF: Obtain IP Address of the Station That Connects to an ESP SoftAP


*/









//----------------------------- Mechanizmy RTOS-owe-----------------------------
/*
 1).......................
if (xReceiverTaskHandle != NULL) {
    xTaskNotifyGive(xReceiverTaskHandle);										// WYSYŁANIE: Zwiększ wartość powiadomienia odbiorcy o 1
    printf("Nadawca: Wysłałem powiadomienie!\n");
}

ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);			// ODBIERANIE: Czekaj (blokuj wątek) aż przyjdzie powiadomienie			pdTRUE oznacza, że po odczycie licznik powiadomień zostanie wyzerowany

if (ulNotificationValue > 0) {
	printf("Odbiorca: Otrzymałem powiadomienie! Wysyłam e-mail...\n");
	// Tutaj wstaw kod do wysyłki e-maila przez komendy AT
}

2).......................

eSetValueWithOverwrite: 			Najczęstszy wybór. Odbiorca zawsze dostanie najnowszą wartość, stara (jeśli nie została odebrana) jest kasowana. Idealne do stanów i pomiarów.
eSetValueWithoutOverwrite:			 Zapisze wartość tylko wtedy, gdy poprzednia została już odebrana. Jeśli nie – funkcja zwróci błąd (pdFAIL).
eSetBits: 								Traktuje powiadomienie jako flagi (bit po bicie). Pozwala ustawić konkretne bity bez zmieniania pozostałych (jak w Event Groups).

   sensorData = 25; // Przykładowa wartość (np. temperatura)
   xTaskNotify(xReceiverTaskHandle, sensorData, eSetValueWithOverwrite);    	 // WYSYŁANIE:

   if (xTaskNotifyWait(0, 0, &receivedValue, portMAX_DELAY) == pdPASS) {		// ODBIERANIE:		xTaskNotifyWait(bitmask_na_wejsciu, bitmask_na_wyjsciu, &zmienna, czas)		bitmask: 0x00: Nie czyść nic , 0xFFFFFFFF (Wszystkie bity): Czyści całą wartość powiadomienia po wyjściu.
       printf("Odebrano wartość: %d\n", receivedValue);
   }


 void ARCHIVE_SendEvent(char* eventDesc)
{
	char* eventMessage = pvPortMalloc(100 * sizeof(char));
	if (NULL != eventMessage)
	{
		strncpy(eventMessage,eventDesc,100);
		if(errQUEUE_FULL == xQueueSend(xMessageQueue, &eventMessage, 200))
			vPortFree(eventMessage);
	}
}

void ARCHIVE_SendServiceEvent(int ID)
{
	int* eventMessage = pvPortMalloc(sizeof(int));
	if (NULL != eventMessage)
	{
		*eventMessage = ID;
		if(NULL == xAuthorizedEventQueue)
			xAuthorizedEventQueue = xQueueCreate(20, sizeof(int));
		if(errQUEUE_FULL == xQueueSend(xAuthorizedEventQueue, &eventMessage, sizeof(int)))
			vPortFree(eventMessage);
	}
}

char archBufferEvent[150]={0};
int queueSize = uxQueueMessagesWaiting(xMessageQueue);  //ile jest w kolejsce elemetow do odczytu
  	for (int i = 0; i < queueSize; i++)
	{
		if (xQueueReceive(xMessageQueue, &(eventArchMessage), portMAX_DELAY))
		{
		   strcat(archBufferEvent,eventArchMessage);

		   vPortFree(eventArchMessage);  // to WAZNE !!!
			memset(archBufferEvent, 0, sizeof(archBufferEvent));
		}
	}

*/
