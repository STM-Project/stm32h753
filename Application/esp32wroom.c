/*
 * wifi.c
 *
 *  Created on: 11.12.2020
 *      Author: RafalMar
 */
#include "esp32wroom.h"
#include "wwwPages.h"
#include "HTTP/http_sensor_page.h"
#include "HTTP/http_sensor_settings.h"
#include "HTTP/http_sensor_storage.h"
#include "HTTP/http_settings_model.h"
#include "HTTP/http_settings_page.h"
#include "HTTP/http_settings_storage.h"
#include "HTTP/http_settings_bindings.h"
#include "HTTP/http_request_stream.h"
#include "HTTP/http_output_control.h"
#include "LPC2148/http_lpc_panel.h"
#include "LPC2148/lpc_pages.h"
#include "relay_states.h"
#include "http_sensor_lcd.h"
#include "google_cloud.h"
#include "google_cloud_config.h"
#include "google_cloud_at.h"
#include "google_cloud_protocol.h"
#include "google_cloud_roots.h"
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
#include "sd_card.h"
#include "_smtp.h"
#include "variables.h"
#include "float.h"
#include "sntp_dns.h"
#include "timer.h"
#include "usart.h"
#include <time.h>
#include "tim.h"

#define TIME_AFTER_RELOAD_HTTP_MS		10000

#define ESP_RECV_BUFF_SIZE		4096
#define PACKET_SEND_LEN 		2048

#define ETH_PACKET_LEN 			2039

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

#define WAIT_ON_SEND_TIMEOUT_TIME_MS 	20000

typedef enum
{
	_OK=1, _ERROR
} ANSWER_TYPE;

typedef enum
{
	INIT_CONNECTION,
	HTTP_CONNECTION,
	HTTP_CONNECTION_2,
	SMTP_CONNECTION,
	TEST_CONNECTION,
	GOOGLE_CLOUD_CONNECTION
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

typedef enum
{
	WWW_MAIN_SETTING,
	WWW_MAIN_READ,
	WWW_DATA_REFRESH,
	WWW_LPC_ASSET,
	WWW_LPC_DATA,
	WWW_LPC_SD,
	WWW_LPC_CONFIG,
	WWW_LPC_SAVE,
	WWW_LPC_CATALOG,
	WWW_LPC_LIVE,
	WWW_OUTPUT_CONTROL,
	WWW_FAVICON,
	WWW_SENSOR_SETTING,
	WWW_OTHER_SETTING,
	WWW_HTTP_ERROR
}WEB_WWW;

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

#define MAX_HTML_WEBs		10

struct HTTP_SEND_TEMP{
	char* web[ESP_MAX_HTTP_CONN][MAX_HTML_WEBs];	/* ptr`s to html_web`s */
 	u32   siz[ESP_MAX_HTTP_CONN][MAX_HTML_WEBs];	/* size html_web`s */
	u8	  nrWeb[ESP_MAX_HTTP_CONN];					/* actual nr html_web to send at web[][] */
	u8 	  chnl;										/* actual channel wait for SEND OK or CLOSED */
	char* ptr[ESP_MAX_HTTP_CONN];					/* ptr to actual html_web */
 	u8 	  que[ESP_MAX_HTTP_CONN];					/* buffer of requests to send channel */
 	u32   nr[ESP_MAX_HTTP_CONN];					/* packet iterix of actual html_web */
 	u32   len[ESP_MAX_HTTP_CONN];					/* actual packet len to send */
}httpPar;

TimerHandle_t xWaitOnSendTimeoutTimer=NULL;

/* Queued pages must retain their contents until that slot finishes sending. */
SDRAM static char httpPageBuff[ESP_MAX_HTTP_CONN][100000];
SDRAM static char httpLpcSdBuff[ESP_MAX_HTTP_CONN][LPC_PAGE_BUFFER_SIZE] __attribute__ ( ( aligned ( 32 ) ) );
SDRAM static char httpSensorBuff[ESP_MAX_HTTP_CONN][131072];
SDRAM static HTTP_RequestStream httpRequestStream;
static char httpResponseHeader[ESP_MAX_HTTP_CONN][192];
static char httpCleanAddress[ESP_MAX_HTTP_CONN][384];
static char httpStorageNotice[ESP_MAX_HTTP_CONN][320];
static unsigned httpSensorPort[ESP_MAX_HTTP_CONN];
static const char *httpSensorNotice[ESP_MAX_HTTP_CONN];
static int httpResponseStatus[ESP_MAX_HTTP_CONN];
static const HTTP_SettingPage *httpSettingsPage[ESP_MAX_HTTP_CONN];
static uint16_t httpSettingsInstance[ESP_MAX_HTTP_CONN];
static const HTTP_LpcAsset *httpLpcAsset[ESP_MAX_HTTP_CONN];
/* WiFi task owns all transmit state. 1=prompt pending, 2=SEND OK pending,
 * 3=close pending. Offset counts bytes, not issued CIPSEND commands. */
static uint8_t httpTxPhase[ESP_MAX_HTTP_CONN];
static uint32_t httpTxOffset[ESP_MAX_HTTP_CONN];
static char* pMem=NULL;
static int DBG = 1;
static uint8_t connectionType = INIT_CONNECTION;
static xTaskHandle vtaskWifiHandle=NULL;
static volatile uint8_t httpResetRequested;

RAM_D2_ALIGN32 static char RecvBuffer[ESP_RECV_BUFF_SIZE];			/* ESP_RECV_BUFF_SIZE musi byc wielokrotnoscia 32 jesli uzylem _ALIGN32 */
RAM_D2_ALIGN32 static char sendBuff[PACKET_SEND_LEN];
/* Google owns link 3 only while WWW is idle/paused; SMTP keeps link 4. */
volatile GoogleCloudStatus googleCloudStatus;
static CloudAt cloudAt;
static CloudHttpResponse cloudResponse;
static char cloudRequest[4096], cloudRedirect[2048], cloudNonce[33];
static size_t cloudRequestLength, cloudOffset, cloudPacketLength;
static uint32_t cloudStarted, cloudLastAttempt, cloudCleanupTick;
static int cloudRedirected, cloudResult;
static size_t cloudCertOffset;

static volatile int recvByteFromEsp = 0; /* Updated by UART ISR. */
static int recvByteFromEsp_copy = 0;
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

static void InitReloadHttpTime(void){
	tickCnt = xTaskGetTickCount();
}
static int IsTimeoutForReloadHttp(void){
	return CONDITION( xTaskGetTickCount()-tickCnt > TIME_AFTER_RELOAD_HTTP_MS, 1, 0 );
}

static void InitStructRqstToSendChnl(void){
	httpPar.chnl = 0xFF;
	LOOP_FOR ( i, ESP_MAX_HTTP_CONN )
	{
		httpTxPhase[i] = 0;
		httpTxOffset[i] = 0;
		httpPar.ptr[i] = NULL;
		httpPar.que[i] = 0xFF;
		httpPar.nr[i] = 0;
		httpPar.len[i] = 0;
		httpPar.nrWeb[i] = 0;
		LOOP_FOR ( j, MAX_HTML_WEBs )
		{
			httpPar.web[i][j] = NULL;
			httpPar.siz[i][j] = 0;
		}
	}
}

static int IsActiveAnyRqstToSendChnl(void){
	LOOP_FOR(i,ESP_MAX_HTTP_CONN){ if(httpPar.que[i]!=0xFF) return 1; }
	return 0;
}

static void HTTP_ShowHttpStruct(void){
	LOOP_FOR(i,ESP_MAX_HTTP_CONN){	 DbgVarDma2(1,200,"que:%*d 	nr:%*d 	 ptr:%*d   len:%*d\r\n", -3,httpPar.que[i], -3,httpPar.nr[i], -17,httpPar.ptr[i], -17,httpPar.len[i] );	}
}

static void BackFromEmail(int nrInfo);

static void vWaitOnSendTimeoutTimerCallback(TimerHandle_t pxTimer)
{
	DbgDma(DBG,_SE_"\r\nTIMEOUT RECV "_E_);		DbgVarDma(DBG,100,_S_"connType:%d "_E_,connectionType);
	switch(connectionType){
	case HTTP_CONNECTION:
	case HTTP_CONNECTION_2:
		/* Never clear pointers underneath the WiFi task's packet copy. */
		httpResetRequested = 1;
		if ( vtaskWifiHandle )
			xTaskNotify ( vtaskWifiHandle, BIT_ESP_SRV, eSetBits );
		break;
	case INIT_CONNECTION:
		DbgDma ( DBG, _SE_ "\r\nINIT_ESP !!!" _E_ );
		break;
	case SMTP_CONNECTION:
		BackFromEmail ( 1 );
		break;
	}
	xTimerStop(pxTimer, 0);   /* RcvTimeoutTimerStop() */
}
static int RcvTimeoutTimerInit(void){
	xWaitOnSendTimeoutTimer = xTimerCreate("WaitOnSendTimeoutTimer", WAIT_ON_SEND_TIMEOUT_TIME_MS, 0, ( void * ) 0, vWaitOnSendTimeoutTimerCallback);
	if(NULL!=xWaitOnSendTimeoutTimer) return 1; else return 0;
}
static int RcvTimeoutTimerStart(void){
	if(NULL!=xWaitOnSendTimeoutTimer){  if(pdPASS==xTimerStart(xWaitOnSendTimeoutTimer, 0)) return 1;  }
	return 0;
}
static int RcvTimeoutTimerStop(void){
	if(NULL!=xWaitOnSendTimeoutTimer){  if(pdPASS==xTimerStop(xWaitOnSendTimeoutTimer, 0)) return 1;  }
	return 0;
}

static int SendToEsp32(int len, char *data, ARCHIVING_TYPE archType)								/* if data=NULL we use buffer 'sendBuff' as default. 		if len=0 we calculate length text. */
{
	int len_ = CONDITION( 0==len, mini_strlen(CONDITION(NULL==data,sendBuff,data)), len );
	if(len_ > PACKET_SEND_LEN-1)  len_=PACKET_SEND_LEN-1;
	if(data != sendBuff && data != NULL){ strncpy(sendBuff,data,len_); }	sendBuff[len_]=0;		/* memcpy(sendBuff, data, len_)  jest szybsze niż strncpy */

		 if(arch ==archType){ DbgMultiDma(DBG,CoR2_"\r\nSEND_START: "_X_,sendBuff,CoR2_"SEND_STOP\r\n"_X_); }
	else if(arch2==archType){ DbgMultiDma(DBG,"\r\n",sendBuff,"\r\n"); }

	UpdateReadPos();
/*	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, PACKET_SEND_LEN);	*/							/* SCB_CleanDCache_by_Addr() takes only 4us */		/* Jesli w MPU ustawimy adres bufora 'sendBuff' w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_CleanDCache_by_Addr() nie jest potrzebny */
	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, CACHE_ALLIGN_LEN(len_)); 	 					/* Czyszczenie Cache z rozmiarem zaokrąglonym do pełnych linii 32-bajtowych, czyszczenie tylko tego fragmentu, który faktycznie wysyłamy   CACHE_LINE_BYTES = 32 */
	int result = HAL_UART_Transmit_DMA(&ESP_UART_HANDLE, (uint8_t*) sendBuff, len_);
	if(result != HAL_OK)  DbgVarDma(DBG,100,_SE_"\r\nHAL_ERROR: %d "_E_,result);
	RcvTimeoutTimerStart();
	return result;
}

static int SendToEsp32_http(int len, char *data, ARCHIVING_TYPE archType)								/* if data=NULL we use buffer 'sendBuff' as default. 		if len=0 we calculate length text. */
{
	int len_ = CONDITION( 0==len, mini_strlen(CONDITION(NULL==data,sendBuff,data)), len );
	if(len_ > PACKET_SEND_LEN-1)  len_=PACKET_SEND_LEN-1;
	if(data != sendBuff && data != NULL){ strncpy(sendBuff,data,len_); }	sendBuff[len_]=0;		/* memcpy(sendBuff, data, len_)  jest szybsze niż strncpy */

		 if(arch ==archType){ DbgMultiDma(DBG,CoR2_"\r\nSEND_START: "_X_,sendBuff,CoR2_"SEND_STOP\r\n"_X_); }
	else if(arch2==archType){ DbgMultiDma(DBG,"\r\n",sendBuff,"\r\n"); }

/*	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, PACKET_SEND_LEN);	*/							/* SCB_CleanDCache_by_Addr() takes only 4us */		/* Jesli w MPU ustawimy adres bufora 'sendBuff' w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_CleanDCache_by_Addr() nie jest potrzebny */
	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, CACHE_ALLIGN_LEN(len_)); 	 					/* Czyszczenie Cache z rozmiarem zaokrąglonym do pełnych linii 32-bajtowych, czyszczenie tylko tego fragmentu, który faktycznie wysyłamy   CACHE_LINE_BYTES = 32 */
	int result = HAL_UART_Transmit_DMA(&ESP_UART_HANDLE, (uint8_t*) sendBuff, len_);
	if(result != HAL_OK)  DbgVarDma(DBG,100,_SE_"\r\nHAL_ERROR: %d "_E_,result);
	RcvTimeoutTimerStart();
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
		int hasRecv1 = connectionType == GOOGLE_CLOUD_CONNECTION ? CloudAt_Has ( &cloudAt, recv1 ) : ( recv1 != NULL ) && ( strstr_ ( NULL, recv1 ) != NULL );
		int hasRecv2 = connectionType == GOOGLE_CLOUD_CONNECTION ? CloudAt_Has ( &cloudAt, recv2 ) : ( recv2 != NULL ) && ( strstr_ ( NULL, recv2 ) != NULL );
		if (hasRecv1 && hasRecv2){ actualCase++; flag=3; flagCase=3; }
	    if (hasRecv2)  			 { actualCase++; flag=2; flagCase=2; }
	    if (hasRecv1) 			 { actualCase++; flag=1; flagCase=1; }
		if ( flag )
		{
			if ( connectionType == GOOGLE_CLOUD_CONNECTION )
				cloudAt.events = 0;
			else
			{
				DispRecvBuff ( nrCase, archType );
				ESP32_FreeAnswers ( 0 );
			}
			RcvTimeoutTimerStop ();
		}
	}
	return flag;
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
		VAR_SetTabVal(Const_wifiSTA_ip, i, LWIP_MAKEU32(192,168,1,94));
		VAR_SetTabVal(Const_wifiSTA_mask, i, LWIP_MAKEU32(255,255,255,0));
		VAR_SetTabVal(Const_wifiSTA_gate, i, LWIP_MAKEU32(192,168,1,1));
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
/*
static void GoToTest(char* txt){
	connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
	DbgVarDma(DBG,50, _S_"%s"_E_,txt);
}
*/
static char* HTTP_SetWeb(int channel, int nrWWW)
{
	LOOP_FOR(i,MAX_HTML_WEBs){  httpPar.web[channel][i] = NULL;		httpPar.siz[channel][i] = 0;  }
	int nrPartWWW = 0;
	char *httpBuff = httpPageBuff[channel];
	char *ptr = httpBuff;
	switch(nrWWW){
	case WWW_LPC_SD:
		DbgVarDma ( 1, 120, "\r\nHTTP LPC: TCP=%u /nowa%u odczyt SD...\r\n", (unsigned) httpPar.que[channel] + 1u, httpSensorPort[channel] );
		httpPar.web[channel][nrPartWWW] = httpLpcSdBuff[channel];
		httpPar.siz[channel][nrPartWWW++] = (u32) LPC_StorageReadPage ( httpSensorPort[channel], httpLpcSdBuff[channel], sizeof httpLpcSdBuff[channel] );
		if ( !httpPar.siz[channel][0] )
		{
			httpResponseStatus[channel] = 503;
			snprintf ( httpBuff, sizeof httpPageBuff[channel], "<!doctype html><meta charset='UTF-8'><p>Nie wczytano strony z karty SD.</p><p>%s</p><p>Rozpakuj sdcard.zip i skopiuj folder LPC do katalogu glownego karty.</p><a href='/lpc'>Lista stron</a> | <a href='/nowa'>Panel we Flash</a>", LPC_StoragePageError () );
			DbgVarDma ( 1, 200, "\r\nHTTP LPC: /nowa%u BLAD: %s\r\n", httpSensorPort[channel], LPC_StoragePageError () );
			httpPar.web[channel][0] = httpBuff;
			httpPar.siz[channel][0] = (u32) strlen ( httpBuff );
		}
		else
		{
			DbgVarDma ( 1, 128, "\r\nHTTP LPC: /nowa%u SD OK, %u bajtow do wyslania\r\n", httpSensorPort[channel], (unsigned) httpPar.siz[channel][0] );
		}
		break;
	case WWW_LPC_CONFIG:
		httpPar.web[channel][nrPartWWW] = httpLpcSdBuff[channel];
		httpPar.siz[channel][nrPartWWW++] = (u32) LPC_SettingsJson ( httpSensorPort[channel], httpLpcSdBuff[channel], sizeof httpLpcSdBuff[channel] );
		if ( !httpPar.siz[channel][0] )
			httpResponseStatus[channel] = 500;
		break;
	case WWW_LPC_SAVE:
		snprintf ( httpBuff, sizeof ( httpPageBuff[channel] ), "{\"ok\":%s,\"message\":\"%s\"}", httpResponseStatus[channel] == 200 ? "true" : "false",
				   httpResponseStatus[channel] == 200 ? "Zapisano ustawienia LPC na karcie SD." : httpResponseStatus[channel] == 503 ? "Nie zapisano. Sprawdz karte SD i pakiet LPC."
																							  : httpResponseStatus[channel] == 501	 ? "Ta akcja sprzetowa LPC2148 nie jest dostepna w sterowniku STM32H753."
																																	 : "Nie zapisano. Sprawdz komplet i wartosci formularza." );
		httpPar.web[channel][nrPartWWW] = httpBuff;
		httpPar.siz[channel][nrPartWWW++] = (u32) strlen ( httpBuff );
		break;
	case WWW_LPC_CATALOG:
		httpPar.web[channel][nrPartWWW] = httpBuff;
		httpPar.siz[channel][nrPartWWW++] = (u32) LPC_CatalogRender ( httpBuff, sizeof ( httpPageBuff[channel] ) );
		break;
	case WWW_LPC_LIVE:
		httpPar.web[channel][nrPartWWW] = httpBuff;
		httpPar.siz[channel][nrPartWWW++] = (u32) HTTP_LpcLegacyJson ( httpBuff, sizeof ( httpPageBuff[channel] ), HttpRefr_sdram, strlen ( HttpRefr_sdram ) );
		break;
	case WWW_LPC_ASSET:
		httpPar.web[channel][nrPartWWW] = (char *) httpLpcAsset[channel]->data;
		httpPar.siz[channel][nrPartWWW++] = (u32) httpLpcAsset[channel]->length;
		break;
	case WWW_LPC_DATA:
		httpPar.web[channel][nrPartWWW] = httpBuff;
		httpPar.siz[channel][nrPartWWW++] = (u32) HTTP_LpcPanelJsonFromRefresh ( httpBuff, sizeof ( httpPageBuff[channel] ), HttpRefr_sdram, strlen ( HttpRefr_sdram ) );
		if ( !httpPar.siz[channel][0] )
			httpResponseStatus[channel] = 500;
		break;
	case WWW_MAIN_READ:
		httpPar.web[channel][nrPartWWW] = (char*)HttpStyle;					httpPar.siz[channel][nrPartWWW++] = mini_strlen(HttpStyle);
		httpPar.web[channel][nrPartWWW] = (char*)HttpMainMenu;				httpPar.siz[channel][nrPartWWW++] = mini_strlen(HttpMainMenu);

		{
			size_t length = HTTP_MainReadPanelRender ( httpBuff, sizeof ( httpPageBuff[channel] ) );
			if ( !length )
			{
				httpResponseStatus[channel] = 500;
				strcpy ( httpBuff, "<p>Nie mozna wygenerowac strony glownej.</p>" );
				length = strlen ( httpBuff );
			}
			httpPar.web[channel][nrPartWWW] = httpBuff;
			httpPar.siz[channel][nrPartWWW++] = (u32) length;
		}

		httpPar.web[channel][nrPartWWW] = (char*)HttpStructMainReadPanel;	httpPar.siz[channel][nrPartWWW++] = mini_strlen(HttpStructMainReadPanel);
		break;
	case WWW_DATA_REFRESH:
		/* Retain this slot's snapshot and overlay requested/reported output
		 * states in the legacy protocol's 13-byte output block. */
		strcpy ( httpBuff, HttpRefr_sdram );
		for ( unsigned i = 0; i < RELAY_OUTPUT_COUNT && strlen ( httpBuff ) > 25u + i; i++ )
		{
			int state = RELAY_GetDisplayState ( RELAY_GetStates ()->outputs[i].port );
			if ( state >= 0 )
				httpBuff[25u + i] = (char) ( '0' + state );
		}
		httpPar.web[channel][nrPartWWW] = httpBuff;
		httpPar.siz[channel][nrPartWWW++] = (u32) strlen ( httpBuff );
		break;
	case WWW_OUTPUT_CONTROL:
	{
		const RelayState *state = RELAY_GetState ( httpSensorPort[channel] );
		if ( httpSensorNotice[channel] )
			snprintf ( httpBuff, sizeof ( httpPageBuff[channel] ), "%u", state->pwm_percent );
		else
			snprintf ( httpBuff, sizeof ( httpPageBuff[channel] ),
					   "<font color='#000'>Stan %s</font>", state->requested_on ? "ON" : "OFF" );
		httpPar.web[channel][nrPartWWW] = httpBuff;
		httpPar.siz[channel][nrPartWWW++] = (u32) strlen ( httpBuff );
	}
		break;
	case WWW_MAIN_SETTING:
		httpPar.web[channel][nrPartWWW] = (char *) HttpStyle;
		httpPar.siz[channel][nrPartWWW++] = mini_strlen ( HttpStyle );
		httpPar.web[channel][nrPartWWW] = (char *) HttpMainMenu;
		httpPar.siz[channel][nrPartWWW++] = mini_strlen ( HttpMainMenu );
		if ( httpSensorNotice[channel] )
		{
			httpPar.web[channel][nrPartWWW] = (char *) httpSensorNotice[channel];
			httpPar.siz[channel][nrPartWWW++] = mini_strlen ( httpSensorNotice[channel] );
		}
		ptr = HTTP_TEMPLATE_2_NamesBegin ( ptr );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 1, HTTP_SettingsPortName ( 1, "Nazwa portu nr 1" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 2, HTTP_SettingsPortName ( 2, "Nazwa portu nr 2" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 3, HTTP_SettingsPortName ( 3, "Nazwa portu nr 3" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 4, HTTP_SettingsPortName ( 4, "Nazwa portu nr 4" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 5, HTTP_SettingsPortName ( 5, "Nazwa portu nr 5" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 6, HTTP_SettingsPortName ( 6, "Nazwa portu nr 6" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 7, HTTP_SettingsPortName ( 7, "Nazwa portu nr 7" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 8, HTTP_SettingsPortName ( 8, "Nazwa portu nr 8" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 9, HTTP_SettingsPortName ( 9, "Nazwa portu nr 9" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 10, HTTP_SettingsPortName ( 10, "Nazwa portu nr 10" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 11, HTTP_SettingsPortName ( 11, "Nazwa portu nr 11" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 12, HTTP_SettingsPortName ( 12, "Nazwa portu nr 12" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 13, HTTP_SettingsPortName ( 13, "Nazwa portu nr 13" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 14, HTTP_SettingsPortName ( 14, "Nazwa portu nr 14" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 15, HTTP_SettingsPortName ( 15, "Nazwa portu nr 15     0-10V" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 16, HTTP_SettingsPortName ( 16, "Nazwa portu nr 16     0-10V" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 17, HTTP_SettingsPortName ( 17, "Nazwa portu nr 17     E1" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 18, HTTP_SettingsPortName ( 18, "Nazwa portu nr 18     E2" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 19, HTTP_SettingsPortName ( 19, "Nazwa portu nr 19     E3" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 20, HTTP_SettingsPortName ( 20, "Nazwa portu nr 20     E4" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 21, HTTP_SettingsPortName ( 21, "Nazwa portu nr 21     PW1" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 22, HTTP_SettingsPortName ( 22, "Nazwa portu nr 22     PW2" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 23, HTTP_SettingsPortName ( 23, "Nazwa portu nr 23     PW3" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 24, HTTP_SettingsPortName ( 24, "Nazwa portu nr 24     OC1" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 25, HTTP_SettingsPortName ( 25, "Nazwa portu nr 25     OC2" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 26, HTTP_SettingsPortName ( 26, "Nazwa portu nr 26     OC3" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 27, HTTP_SettingsPortName ( 27, "Nazwa portu nr 27     OC4" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 28, HTTP_SettingsPortName ( 28, "Nazwa portu nr 28     PK1" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 29, HTTP_SettingsPortName ( 29, "Moj Przekaznik" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 30, HTTP_SettingsPortName ( 30, "Nazwa portu nr 30     PK3" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 31, HTTP_SettingsPortName ( 31, "Nazwa portu nr 31     PK4" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 32, HTTP_SettingsPortName ( 32, "Nazwa portu nr 32     PK5" ) );
		ptr = HTTP_TEMPLATE_2_Name ( ptr, 33, HTTP_SettingsPortName ( 33, "Nazwa portu nr 33     PK6" ) );
		ptr = HTTP_TEMPLATE_2_NamesEnd ( ptr );
		ptr = HTTP_TEMPLATE_2_ModesBegin ( ptr );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 1, HTTP_SettingsPortMode ( 1 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 2, HTTP_SettingsPortMode ( 2 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 3, HTTP_SettingsPortMode ( 3 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 4, HTTP_SettingsPortMode ( 4 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 5, HTTP_SettingsPortMode ( 5 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 6, HTTP_SettingsPortMode ( 6 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 7, HTTP_SettingsPortMode ( 7 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 8, HTTP_SettingsPortMode ( 8 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 9, HTTP_SettingsPortMode ( 9 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 10, HTTP_SettingsPortMode ( 10 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 11, HTTP_SettingsPortMode ( 11 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 12, HTTP_SettingsPortMode ( 12 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 13, HTTP_SettingsPortMode ( 13 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 14, HTTP_SettingsPortMode ( 14 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 15, HTTP_SettingsPortMode ( 15 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 16, HTTP_SettingsPortMode ( 16 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 17, HTTP_SettingsPortMode ( 17 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 18, HTTP_SettingsPortMode ( 18 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 19, HTTP_SettingsPortMode ( 19 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 20, HTTP_SettingsPortMode ( 20 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 21, HTTP_SettingsPortMode ( 21 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 22, HTTP_SettingsPortMode ( 22 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 23, HTTP_SettingsPortMode ( 23 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 24, HTTP_SettingsPortMode ( 24 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 25, HTTP_SettingsPortMode ( 25 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 26, HTTP_SettingsPortMode ( 26 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 27, HTTP_SettingsPortMode ( 27 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 28, HTTP_SettingsPortMode ( 28 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 29, HTTP_SettingsPortMode ( 29 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 30, HTTP_SettingsPortMode ( 30 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 31, HTTP_SettingsPortMode ( 31 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 32, HTTP_SettingsPortMode ( 32 ) );
		ptr = HTTP_TEMPLATE_2_Mode ( ptr, 33, HTTP_SettingsPortMode ( 33 ) );
		ptr = HTTP_TEMPLATE_2_ModesEnd ( ptr );
		ptr = HTTP_TEMPLATE_2_PanelBegin ( ptr );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 1, 1, "<font color='#000'>   --.- °C</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 1, 1, " --- °C", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 1, 1, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 1, 1, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 1, 1, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 1 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 2, 2, "<font color='#000'>   --.- °C</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 2, 2, " --- °C", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 2, 2, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 2, 2, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 2, 2, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 2 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 3, 3, "<font color='#000'>   --.- °C</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 3, 3, " --- °C", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 3, 3, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 3, 3, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 3, 3, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 3 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 4, 4, "<font color='#000'>   --.- °C</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 4, 4, " --- °C", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 4, 4, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 4, 4, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 4, 4, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 4 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 5, 5, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 5, 5, " --- °C", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 5, 5, "<font color='#000'>   --.- °C</font>", "<font color='#000'>  -- %</font>", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 5, 5, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 5, 5, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 5 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 6, 6, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 6, 6, " --- °C", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 6, 6, "<font color='#000'>   --.- °C</font>", "<font color='#000'>  -- %</font>", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 6, 6, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 6, 6, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 6 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 7, 7, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 7, 7, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 7, 7, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 7, 7, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 7, 7, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 7 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 8, 8, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 8, 8, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 8, 8, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 8, 8, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 8, 8, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 8 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 9, 9, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 9, 9, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 9, 9, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 9, 9, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 9, 9, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 9 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 10, 10, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 10, 10, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 10, 10, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 10, 10, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 10, 10, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 10 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 11, 11, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 11, 11, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 11, 11, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 11, 11, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 11, 11, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 11 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 12, 12, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 12, 12, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 12, 12, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 12, 12, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 12, 12, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 12 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 13, 13, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 13, 13, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 13, 13, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 13, 13, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 13, 13, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 13 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 14, 14, "<font color='#000'>   --.- °C</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_PT1000, 14, 14, " --- °C", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 14, 14, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 14, 14, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_EOL, 14, 14, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 14 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_ADC, 15, 1, "     7.5   V", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 15 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_ADC, 16, 2, "     6.7   V", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 16 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 17, 17, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 17, 17, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 17, 15, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Impl ( ptr, HTTP_MODE_COUNTER, 17, 1, "0.000 KWh", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 17 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 18, 18, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 18, 18, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 18, 16, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Impl ( ptr, HTTP_MODE_COUNTER, 18, 2, "0.000 KWh", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 18 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 19, 19, "<font color='#000'>   --.- °C</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 19, 19, " --.-", " -- ", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 19, 17, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Impl ( ptr, HTTP_MODE_COUNTER, 19, 3, "0.000 KWh", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 19 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP, 20, 20, " --.-", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_AdcTempRhu ( ptr, HTTP_MODE_TEMP_RHU, 20, 20, "<font color='#000'>   --.- °C</font>", "<font color='#000'>  -- %</font>", 0 );
		ptr = HTTP_TEMPLATE_2_In ( ptr, HTTP_MODE_INPUT, 20, 18, "<font color='#000'>Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Impl ( ptr, HTTP_MODE_COUNTER, 20, 4, "0.000 KWh", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 20 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 21, 11, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 21, 11, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 21, 11, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_PWM, 21, 0, NULL, NULL, 50 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_IR, 21, 11, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 21 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 22, 12, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 22, 12, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 22, 12, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_PWM, 22, 0, NULL, NULL, 18 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_IR, 22, 12, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 22 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 23, 13, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 23, 13, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 23, 13, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_PWM, 23, 0, NULL, NULL, 50 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_IR, 23, 13, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 23 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 24, 7, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 24, 7, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 24, 7, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_IR, 24, 7, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 24 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 25, 8, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 25, 8, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 25, 8, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_IR, 25, 8, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 25 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 26, 9, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 26, 9, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 26, 9, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_IR, 26, 9, "<font color=\"#fff\">Stan ON</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 26 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 27, 10, NULL, NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 27, 10, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 27, 10, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_IR, 27, 10, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 27 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 28, 1, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 28, 1, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 28, 1, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 28 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 29, 2, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 29, 2, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 29, 2, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 29 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 30, 3, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 30, 3, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 30, 3, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 30 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 31, 4, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 31, 4, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 31, 4, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 31 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 32, 5, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 32, 5, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 32, 5, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 32 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_RELAY, 33, 6, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_TIMER, 33, 6, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Out ( ptr, HTTP_MODE_THERMOSTAT, 33, 6, "<font color=\"#000\">Stan OFF</font>", NULL, 0 );
		ptr = HTTP_TEMPLATE_2_Hidden ( ptr, 33 );
		ptr = HTTP_TEMPLATE_2_PanelEnd ( ptr );
		httpPar.web[channel][nrPartWWW] = (char *) httpBuff;
		httpPar.siz[channel][nrPartWWW++] = (unsigned int) ( ptr - httpBuff );
		httpPar.web[channel][nrPartWWW] = (char *) HttpStructMainReadPanel2;
		httpPar.siz[channel][nrPartWWW++] = mini_strlen ( HttpStructMainReadPanel2 );
		break;
	case WWW_SENSOR_SETTING:
		httpPar.web[channel][nrPartWWW] = httpSensorBuff[channel];
		httpPar.siz[channel][nrPartWWW++] = (u32) HTTP_SensorPageRender (
			httpSensorBuff[channel], sizeof ( httpSensorBuff[channel] ), httpSensorPort[channel],
			HTTP_SettingsPortMode ( httpSensorPort[channel] ) == HTTP_MODE_TEMP_RHU,
			httpSensorNotice[channel] );
		if ( httpPar.siz[channel][0] == 0 )
		{
			httpResponseStatus[channel] = 500;
			httpPar.web[channel][0] = "<html><body>Nie mozna utworzyc strony.</body></html>";
			httpPar.siz[channel][0] = mini_strlen ( httpPar.web[channel][0] );
		}
		break;
	case WWW_OTHER_SETTING:
		httpPar.web[channel][nrPartWWW] = httpSensorBuff[channel];
		httpPar.siz[channel][nrPartWWW++] = (u32) HTTP_SettingsPageRender (
			httpSensorBuff[channel], sizeof ( httpSensorBuff[channel] ), httpSettingsPage[channel],
			httpSettingsInstance[channel], httpSensorNotice[channel] );
		if ( httpPar.siz[channel][0] == 0 )
		{
			httpResponseStatus[channel] = 500;
			httpPar.web[channel][0] = "<html><body>Nie mozna utworzyc strony ustawien.</body></html>";
			httpPar.siz[channel][0] = mini_strlen ( httpPar.web[channel][0] );
		}
		break;
	case WWW_HTTP_ERROR:
		httpPar.web[channel][nrPartWWW] = "<!doctype html><html><body><p>Niepoprawny lub nieobslugiwany adres.</p><a href='/set'>Ustawienia</a></body></html>";
		httpPar.siz[channel][nrPartWWW++] = mini_strlen ( httpPar.web[channel][0] );
		break;
	case WWW_FAVICON:
	default:
		break;
	}
	/* A CGI response already contains the freshly rendered form and its save
	 * result. Replace its URL without re-submitting the write or losing errors. */
	if ( httpSensorNotice[channel] && httpResponseStatus[channel] != 500 )
	{
		char path[64] = { 0 };
		if ( nrWWW == WWW_MAIN_SETTING )
			strcpy ( path, "/set" );
		else if ( nrWWW == WWW_SENSOR_SETTING )
			snprintf ( path, sizeof path, "/temp%03u", httpSensorPort[channel] );
		else if ( nrWWW == WWW_OTHER_SETTING )
			HTTP_SettingsCatalogPath ( httpSettingsPage[channel], httpSettingsInstance[channel], path, sizeof path );
		if ( path[0] )
		{
			int length = snprintf ( httpCleanAddress[channel], sizeof httpCleanAddress[channel],
									"<script>if(window.history&&window.history.replaceState){"
									"window.history.replaceState(null,'','%s');}"
									"else{window.location.replace('%s');}</script>",
									path, path );
			httpPar.web[channel][nrPartWWW] = httpCleanAddress[channel];
			httpPar.siz[channel][nrPartWWW++] = (u32) length;
		}
	}
	/* HTTP/1.x clients require a status line, content type and a body boundary. */
	{
		u32 bodySize = 0;
		for ( int i = 0; i < nrPartWWW; i++ )
			bodySize += httpPar.siz[channel][i];
		const char *reason = httpResponseStatus[channel] == 200 ? "OK" : httpResponseStatus[channel] == 400 ? "Bad Request"
																	 : httpResponseStatus[channel] == 404	? "Not Found"
																	 : httpResponseStatus[channel] == 503	? "Service Unavailable"
																	 : httpResponseStatus[channel] == 501	? "Not Implemented"
																											: "Internal Server Error";
		const char *contentType = nrWWW == WWW_LPC_ASSET ? httpLpcAsset[channel]->content_type : ( nrWWW == WWW_LPC_DATA || nrWWW == WWW_LPC_CONFIG || nrWWW == WWW_LPC_SAVE || nrWWW == WWW_LPC_LIVE ) ? "application/json; charset=UTF-8"
																							 : ( nrWWW == WWW_DATA_REFRESH || ( nrWWW == WWW_OUTPUT_CONTROL && httpSensorNotice[channel] ) )			? "text/plain; charset=UTF-8"
																							 : nrWWW == WWW_FAVICON																						? "image/x-icon"
																																																		: "text/html; charset=UTF-8";
		int headerSize = snprintf ( httpResponseHeader[channel], sizeof ( httpResponseHeader[channel] ),
									"HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n",
									httpResponseStatus[channel], reason, contentType, (unsigned long) bodySize );
		for ( int i = nrPartWWW; i > 0; i-- )
		{
			httpPar.web[channel][i] = httpPar.web[channel][i - 1];
			httpPar.siz[channel][i] = httpPar.siz[channel][i - 1];
		}
		httpPar.web[channel][0] = httpResponseHeader[channel];
		httpPar.siz[channel][0] = (u32) headerSize;
		nrPartWWW++;
	}
	httpPar.web[channel][nrPartWWW] = NULL;
	httpPar.siz[channel][nrPartWWW] = 0;
	httpPar.nrWeb[channel] = 0;
	return httpPar.web[channel][0];
}

static int HTTP_QueueResponse ( unsigned channel, int page, unsigned port,
								const char *notice, int status,
								const HTTP_SettingPage *settingsPage, uint16_t settingsInstance,
								const HTTP_LpcAsset *asset )
{
	for ( int i = 0; i < ESP_MAX_HTTP_CONN; i++ )
		if ( httpPar.que[i] == channel )
			return -1;
	for ( int i = 0; i < ESP_MAX_HTTP_CONN; i++ )
		if ( httpPar.que[i] == 0xFF )
		{
			httpPar.que[i] = (u8) channel;
			httpSensorPort[i] = port;
			httpSensorNotice[i] = notice;
			httpResponseStatus[i] = status;
			httpSettingsPage[i] = settingsPage;
			httpSettingsInstance[i] = settingsInstance;
			httpLpcAsset[i] = asset;
			httpTxPhase[i] = 0;
			httpTxOffset[i] = 0;
			httpPar.ptr[i] = HTTP_SetWeb ( i, page );
			httpPar.nr[i] = 0;
			httpPar.len[i] = 0;
			return i;
		}
	return -1;
}

static int HTTP_TargetEquals ( const char *target, size_t length, const char *expected )
{
	return strlen ( expected ) == length && memcmp ( target, expected, length ) == 0;
}

static void HTTP_HandleTarget ( unsigned channel, const char *target, size_t length, void *context )
{
	int page = WWW_HTTP_ERROR, status = 404;
	unsigned port = 0;
	const HTTP_SettingPage *settingsPage = NULL;
	uint16_t settingsInstance = 0;
	const char *notice = NULL;
	const HTTP_LpcAsset *asset = HTTP_LpcFindAsset ( target, length );
	unsigned lpcPage = LPC_ResolvePage ( target, length ), lpcApi = LPC_ResolveSettingsApi ( target, length );
	(void) context;
	if ( channel >= ESP_MAX_HTTP_CONN )
		return;
	/* Do not execute a write if its response cannot be queued. */
	int freeSlot = 0;
	for ( int i = 0; i < ESP_MAX_HTTP_CONN; i++ )
	{
		if ( httpPar.que[i] == channel )
			return;
		if ( httpPar.que[i] == 0xFF )
			freeSlot = 1;
	}
	if ( !freeSlot )
		return;
	HTTP_OutputCommand command;
	int outputCommand = HTTP_OutputControlParse ( target, length, &command );
	if ( outputCommand )
	{
		status = 400;
		if ( outputCommand > 0 )
		{
			unsigned mode = HTTP_SettingsPortMode ( command.port );
			/* Toggle against the same source displayed by /nowa and /TME.txt,
             * instead of treating an unreported relay as initially OFF. */
			if ( command.kind != HTTP_OUTPUT_PWM && ( mode == HTTP_MODE_RELAY || mode == HTTP_MODE_IR ) )
			{
				int state = HTTP_LpcOutputStateFromRefresh ( HttpRefr_sdram, strlen ( HttpRefr_sdram ), command.port );
				if ( state >= 0 )
					RELAY_UpdateState ( command.port, (unsigned) state );
			}
			int accepted = command.kind == HTTP_OUTPUT_PWM ? ( mode == HTTP_MODE_PWM && RELAY_RequestPwm ( command.port, command.value ) ) : ( ( mode == HTTP_MODE_RELAY || mode == HTTP_MODE_IR ) && RELAY_RequestToggle ( command.port ) );
			if ( accepted )
			{
				page = WWW_OUTPUT_CONTROL;
				status = 200;
				port = command.port;
				notice = command.kind == HTTP_OUTPUT_PWM ? "pwm" : NULL;
			}
			if ( command.kind == HTTP_OUTPUT_PWM )
			{
				DbgVarDma ( 1, 160, "\r\nHTTP GET PWM: TCP=%u port=%u PWM=%u%% -> %s\r\n",
							channel + 1u, command.port, command.value, accepted ? "OK" : "ODRZUCONO (tryb portu)" );
			}
			else
			{
				const RelayState *state = RELAY_GetState ( command.port );
				DbgVarDma ( 1, 160, "\r\nHTTP GET PK: TCP=%u port=%u zadano=%s -> %s\r\n",
							channel + 1u, command.port, accepted && state ? ( state->requested_on ? "ON" : "OFF" ) : "-",
							accepted ? "OK" : "ODRZUCONO (tryb portu)" );
			}
		}
		else
		{
			DbgVarDma ( 1, 128, "\r\nHTTP GET PWM/PK: TCP=%u ODRZUCONO (bledne parametry)\r\n", channel + 1u );
		}
	}
	else if ( !target )
	{
		status = 400;
	}
	else if ( asset )
	{
		page = WWW_LPC_ASSET;
		status = 200;
	}
	else if ( lpcPage )
	{
		page = WWW_LPC_SD;
		port = lpcPage;
		status = 200;
	}
	else if ( lpcApi )
	{
		page = WWW_LPC_CONFIG;
		port = lpcApi;
		status = 200;
	}
	else if ( HTTP_TargetEquals ( target, length, "/lpc" ) )
	{
		page = WWW_LPC_CATALOG;
		status = 200;
	}
	else if ( HTTP_TargetEquals ( target, length, "/api/lpcdata" ) )
	{
		page = WWW_LPC_LIVE;
		status = 200;
	}
	else if ( HTTP_TargetEquals ( target, length, "/api/nowa" ) )
	{
		page = WWW_LPC_DATA;
		status = 200;
	}
	else if ( HTTP_TargetEquals ( target, length, "/" ) || HTTP_TargetEquals ( target, length, "/main" ) )
	{
		page = WWW_MAIN_READ;
		status = 200;
	}
	else if ( HTTP_TargetEquals ( target, length, "/set" ) || HTTP_TargetEquals ( target, length, "/Set" ) || HTTP_TargetEquals ( target, length, "/3main" ) )
	{
		page = WWW_MAIN_SETTING;
		status = 200;
	}
	else if ( length == 8 && memcmp ( target, "/temp", 5 ) == 0 &&
			  target[5] >= '0' && target[5] <= '9' && target[6] >= '0' && target[6] <= '9' && target[7] >= '0' && target[7] <= '9' )
	{
		port = (unsigned) ( target[5] - '0' ) * 100 + (unsigned) ( target[6] - '0' ) * 10 + (unsigned) ( target[7] - '0' );
		if ( port >= 1 && port <= HTTP_SENSOR_PORT_COUNT && port != 15 && port != 16 )
		{
			page = WWW_SENSOR_SETTING;
			status = 200;
		}
	}
	else if ( length >= 14 && memcmp ( target, "/lpc.cgi/A117?", 14 ) == 0 )
	{
		int lpcStatus = LPC_ApplyTarget ( target, length, &lpcPage );
		if ( lpcStatus )
		{
			page = WWW_LPC_SAVE;
			status = lpcStatus;
			port = lpcPage;
		}
		else
		{
			int result = HTTP_SettingsApplyTarget ( target, length, &settingsPage, &settingsInstance );
			/* A failed boot-time SD read previously left the save callback unset.
		 * Load existing settings before retrying, so other saved fields survive. */
			if ( result == HTTP_SETTINGS_STORAGE_ERROR && HTTP_SettingsStorageRetryLoad () )
				result = HTTP_SettingsApplyTarget ( target, length, &settingsPage, &settingsInstance );
			if ( result != HTTP_SETTINGS_NOT_HANDLED )
			{
				if ( settingsPage )
				{
					page = strcmp ( settingsPage->route, "/set" ) == 0 ? WWW_MAIN_SETTING : WWW_OTHER_SETTING;
					status = result == HTTP_SETTINGS_SAVED ? 200 : result == HTTP_SETTINGS_STORAGE_ERROR ? 503
																										 : 400;
					notice = result == HTTP_SETTINGS_SAVED ? "<p style='color:#eee'>Zapisano ustawienia.</p>" : result == HTTP_SETTINGS_STORAGE_ERROR ? "<p style='color:#eee'>Nie zapisano ustawien. Sprawdz karte SD.</p>"
																																					  : "<p style='color:#eee'>Nie zapisano: sprawdz wartosci formularza.</p>";
					if ( page == WWW_OTHER_SETTING )
						notice = result == HTTP_SETTINGS_SAVED ? "Zapisano ustawienia." : result == HTTP_SETTINGS_STORAGE_ERROR ? "Nie zapisano ustawien. Sprawdz karte SD."
																																: "Nie zapisano: sprawdz wartosci formularza.";
					if ( result == HTTP_SETTINGS_STORAGE_ERROR )
					{
						const char *message = HTTP_SettingsStorageGetErrorMessage ();
						if ( page == WWW_MAIN_SETTING )
						{
							snprintf ( httpStorageNotice[channel], sizeof httpStorageNotice[channel], "<p style='color:#eee'>%s</p>", message );
							notice = httpStorageNotice[channel];
						}
						else
							notice = message;
					}
					if ( result == HTTP_SETTINGS_SAVED )
						HTTP_SettingsBindingsCommitted ( settingsPage, settingsInstance );
				}
				else
					status = 400;
			}
			else
			{
				result = HTTP_SensorSettingsApplyTarget ( target, length, &port );
				if ( port >= 1 && port <= HTTP_SENSOR_PORT_COUNT )
				{
					page = WWW_SENSOR_SETTING;
					status = result == HTTP_SENSOR_SETTINGS_SAVED ? 200 : result == HTTP_SENSOR_SETTINGS_STORAGE_ERROR ? 503
																													   : 400;
					notice = result == HTTP_SENSOR_SETTINGS_SAVED ? "Zapisano ustawienia." : result == HTTP_SENSOR_SETTINGS_STORAGE_ERROR ? "Nie zapisano ustawien. Sprawdz karte SD."
																																		  : "Nie zapisano: sprawdz progi, histereze i czas zwloki.";
				}
				else
					status = 400;
			}
		}
	}
	else if ( HTTP_TargetEquals ( target, length, "/TME.txt" ) )
	{
		page = WWW_DATA_REFRESH;
		status = 200;
	}
	else if ( HTTP_TargetEquals ( target, length, "/favicon.ico" ) || ( length >= 8 && memcmp ( target, "/mobile/", 8 ) == 0 ) )
	{
		page = WWW_FAVICON;
		status = 200;
	}
	if ( page == WWW_HTTP_ERROR && target && status == 404 && length < 64 && !memchr ( target, '?', length ) )
	{
		char path[64];
		memcpy ( path, target, length );
		path[length] = 0;
		settingsPage = HTTP_SettingsCatalogFind ( path, &settingsInstance );
		if ( settingsPage )
		{
			page = WWW_OTHER_SETTING;
			status = settingsPage->field_count ? 200 : 501;
		}
	}
	if ( HTTP_QueueResponse ( channel, page, port, notice, status, settingsPage, settingsInstance, asset ) >= 0 )
		InitReloadHttpTime ();
}

static void HTTP_FeedReceiveRing ( void )
{
	/* Feed the producer snapshot once. Incomplete +IPD and GET lines survive
	 * subsequent DMA notifications and may wrap at the end of RecvBuffer. */
	int position = read_pos;
	const int end = recvByteFromEsp_copy;
	while ( position != end )
	{
		HTTP_RequestStreamFeed ( &httpRequestStream, (const uint8_t *) &RecvBuffer[position], 1 );
		IncPos ( &position );
	}
}

int CheckReadyToSendChnl ( int chnlPrev )
{
	if(httpPar.chnl==0xFF)		/* gdy nie ma w tle oczekiwania na SEND OK */
	{
		if(chnlPrev==-1)
		{
			LOOP_FOR(nrChnl,ESP_MAX_HTTP_CONN){
				if(httpPar.que[nrChnl]!=0xFF){
					httpPar.chnl = httpPar.que[nrChnl];
					return nrChnl;
			}}
		}
		else
		{
			int chnlNmbr=0; LOOP_FOR(nrChnl,ESP_MAX_HTTP_CONN){	  if(httpPar.que[nrChnl]!=0xFF && httpPar.ptr[nrChnl]!=NULL) chnlNmbr++;   }
			if(chnlNmbr>1)
			{
				LOOP_FOR(nrChnl,ESP_MAX_HTTP_CONN){
					if(httpPar.que[nrChnl]!=0xFF && httpPar.ptr[nrChnl]!= NULL && httpPar.que[nrChnl]!=chnlPrev){
						httpPar.chnl = httpPar.que[nrChnl];
						return nrChnl;
				}}
			}
			else
			{
				LOOP_FOR(nrChnl,ESP_MAX_HTTP_CONN){
					if(httpPar.que[nrChnl]!=0xFF){
						httpPar.chnl = httpPar.que[nrChnl];
						return nrChnl;
				}}
			}
		}

	}
	return -1;
}

static void HTTP_SendCloseChnl ( ARCHIVING_TYPE archType, int prevChnl )
{
	int slot = CheckReadyToSendChnl ( prevChnl );
	if ( slot < 0 )
		return;
	/* Skip empty fragments and validate the index before every array access. */
	while ( httpPar.nrWeb[slot] < MAX_HTML_WEBs )
	{
		unsigned part = httpPar.nrWeb[slot];
		if ( !httpPar.web[slot][part] )
			break;
		if ( httpTxOffset[slot] > httpPar.siz[slot][part] )
			break;
		if ( httpTxOffset[slot] < httpPar.siz[slot][part] )
			break;
		++httpPar.nrWeb[slot];
		httpTxOffset[slot] = 0;
		httpPar.nr[slot] = 0;
	}
	unsigned part = httpPar.nrWeb[slot];
	if ( part >= MAX_HTML_WEBs || !httpPar.web[slot][part] ||
		 httpTxOffset[slot] > httpPar.siz[slot][part] )
	{
		httpPar.ptr[slot] = NULL;
		httpPar.len[slot] = 0;
		httpTxPhase[slot] = 3;
		if ( httpPar.web[slot][1] == httpLpcSdBuff[slot] )
			DbgVarDma ( 1, 120, "\r\nHTTP LPC: TCP=%u cala odpowiedz wyslana, zamykam polaczenie\r\n", (unsigned) httpPar.chnl + 1u );
		SendToEsp32_http ( mini_snprintf ( sendBuff, sizeof ( sendBuff ), "AT+CIPCLOSE=%d\r\n", httpPar.chnl ), NULL, archType );
		return;
	}
	uint32_t remaining = httpPar.siz[slot][part] - httpTxOffset[slot];
	uint32_t length = remaining > ETH_PACKET_LEN ? ETH_PACKET_LEN : remaining;
	if ( !length || length >= sizeof ( sendBuff ) )
	{
		httpPar.chnl = 0xFF;
		return;
	}
	httpPar.ptr[slot] = httpPar.web[slot][part] + httpTxOffset[slot];
	httpPar.len[slot] = length;
	httpTxPhase[slot] = 1;
	/* Console uses channels 1..5; ESP AT commands retain IDs 0..4. */
	if ( archType == noArch )
		DbgVarDma ( 1, 5, "%u", (unsigned) httpPar.chnl + 1u );
	int status = SendToEsp32_http ( mini_snprintf ( sendBuff, sizeof ( sendBuff ), "AT+CIPSEND=%d,%u\r\n", httpPar.chnl, (unsigned) length ), NULL, archType );
	if ( status != HAL_OK )
	{
		httpTxPhase[slot] = 0;
		httpPar.len[slot] = 0;
		httpPar.chnl = 0xFF;
	}
}

static int SearchChnl(u8 channel){
	LOOP_FOR(nrChnl,ESP_MAX_HTTP_CONN){
		if(httpPar.que[nrChnl]!=0xFF){   if(channel==httpPar.que[nrChnl]) return nrChnl;  }
	}
	return -1;
}

/* A prompt may be duplicated/stale after a close or a timeout. Copy only a
 * packet that was explicitly planned for this live slot and fragment. */
static int HTTP_SendPayload ( ARCHIVING_TYPE archType )
{
	int slot = SearchChnl ( httpPar.chnl );
	if ( slot < 0 || httpTxPhase[slot] != 1 )
		return 0;
	unsigned part = httpPar.nrWeb[slot];
	uint32_t length = httpPar.len[slot], offset = httpTxOffset[slot];
	if ( part >= MAX_HTML_WEBs || !httpPar.web[slot][part] || !httpPar.ptr[slot] ||
		 !length || length > ETH_PACKET_LEN || length >= sizeof ( sendBuff ) ||
		 offset > httpPar.siz[slot][part] || length > httpPar.siz[slot][part] - offset ||
		 httpPar.ptr[slot] != httpPar.web[slot][part] + offset )
	{
		httpPar.ptr[slot] = NULL;
		httpPar.len[slot] = 0;
		httpPar.nrWeb[slot] = MAX_HTML_WEBs;
		httpTxPhase[slot] = 0;
		httpPar.chnl = 0xFF;
		return 0;
	}
	memcpy ( sendBuff, httpPar.ptr[slot], length );
	if ( SendToEsp32 ( (int) length, NULL, archType ) != HAL_OK )
	{
		httpPar.len[slot] = 0;
		httpTxPhase[slot] = 0;
		httpPar.chnl = 0xFF;
		return 0;
	}
	httpTxOffset[slot] += length;
	httpPar.ptr[slot] = httpPar.web[slot][part] + httpTxOffset[slot];
	httpPar.len[slot] = 0;
	httpTxPhase[slot] = 2;
	++httpPar.nr[slot];
	return 1;
}

static int HTTP_SendAcknowledged ( ARCHIVING_TYPE archType )
{
	int slot = SearchChnl ( httpPar.chnl );
	if ( slot < 0 || httpTxPhase[slot] != 2 )
		return 0;
	int previous = httpPar.chnl;
	httpTxPhase[slot] = 0;
	httpPar.chnl = 0xFF;
	HTTP_SendCloseChnl ( archType, previous );
	return 1;
}

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
		if(httpPar.chnl==channel) DbgVarDma(DBG,100, _S_" \r\n--- CLOSED,%d --- "_E_,channel);
		else					  DbgVarDma(DBG,100, _SE_"\r\n --- CLOSED,%d!=%d asyn --- "_E_,httpPar.chnl,channel);
	}
}

static void HTTP_ShowInitChannel(int channel,int size, ARCHIVING_TYPE archType){
	if(archType!=noArch)  DbgVarDma(DBG,100,_S_"\r\nRecv HTTP data: channel %d  size %d "_E_,channel,size);
}

/* Cloud transport shares INIT, UART DMA and CASE_Service with HTTP/SMTP. */
static const char *CLOUD_Host ( void )
{
	return cloudRedirected ? "script.googleusercontent.com" : "script.google.com";
}

__attribute__ ( ( weak ) ) int GoogleCloud_ReadTemperature10 ( void )
{
	const size_t offset = 73u + ( GOOGLE_CLOUD_TEMPERATURE_PORT - 1u ) * 12u;
	char field[6], *end;
	int sign = 1, value = 0;
	if ( GOOGLE_CLOUD_TEMPERATURE_PORT < 1 || GOOGLE_CLOUD_TEMPERATURE_PORT > 20 || strlen ( HttpRefr_sdram ) < offset + 5 )
		return -32768;
	memcpy ( field, HttpRefr_sdram + offset, 5 );
	field[5] = 0;
	end = field;
	while ( *end == ' ' )
		end++;
	if ( *end == '-' )
	{
		sign = -1;
		end++;
	}
	else if ( *end == '+' )
		end++;
	if ( *end < '0' || *end > '9' )
		return -32768;
	while ( *end >= '0' && *end <= '9' )
	{
		value = value * 10 + ( *end++ - '0' );
	}
	if ( *end++ != '.' || *end < '0' || *end > '9' )
		return -32768;
	value = value * 10 + ( *end++ - '0' );
	while ( *end == ' ' )
		end++;
	return *end ? -32768 : sign * value;
}

static int CLOUD_Nonce ( void )
{
	static int initialized;
	if ( !initialized )
	{
		RCC_OscInitTypeDef osc = { 0 };
		RCC_PeriphCLKInitTypeDef clock = { 0 };
		osc.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
		osc.HSI48State = RCC_HSI48_ON;
		if ( HAL_RCC_OscConfig ( &osc ) != HAL_OK )
			return 0;
		clock.PeriphClockSelection = RCC_PERIPHCLK_RNG;
		clock.RngClockSelection = RCC_RNGCLKSOURCE_HSI48;
		if ( HAL_RCCEx_PeriphCLKConfig ( &clock ) != HAL_OK )
			return 0;
		__HAL_RCC_RNG_CLK_ENABLE ();
		RNG->CR = RNG_CR_RNGEN;
		initialized = 1;
	}
	for ( unsigned i = 0; i < 4; i++ )
	{
		uint32_t start = HAL_GetTick ();
		while ( !( RNG->SR & RNG_SR_DRDY ) )
		{
			if ( ( RNG->SR & ( RNG_SR_CECS | RNG_SR_SECS ) ) || HAL_GetTick () - start > 10u )
			{
				RNG->CR = 0;
				initialized = 0;
				return 0;
			}
		}
		if ( RNG->SR & ( RNG_SR_CECS | RNG_SR_SECS ) )
		{
			RNG->CR = 0;
			initialized = 0;
			return 0;
		}
		snprintf ( cloudNonce + i * 8u, 9, "%08x", (unsigned) RNG->DR );
	}
	return 1;
}

static void CLOUD_Close ( int error )
{
	DbgVarDma ( DBG, 200, "Google CLOSE: wynik=%d, etap=%d (0=OK, -4=timeout, -5=AT, -6=HTTP, -7=potwierdzenie)\r\n", error, _GET_ACTUAL_CASE_ );
	cloudResult = error;
	cloudAt.events = 0;
	cloudCleanupTick = xTaskGetTickCount ();
	_SET_NEW_CASE_ ( 900 );
	SendToEsp32 ( 0, "AT+CIPCLOSE=3\r\n", noArch );
}

static void CLOUD_RestoreHttp ( void )
{
	DbgVarDma ( DBG, 100, "Google HTTP: przywracam serwer, port=%d\r\n", GetHttpPort () );
	cloudAt.events = 0;
	cloudCleanupTick = xTaskGetTickCount ();
	InitStructRqstToSendChnl ();
	HTTP_RequestStreamInit ( &httpRequestStream, HTTP_HandleTarget, NULL );
	_SET_NEW_CASE_ ( 901 );
	SendToEsp32(mini_snprintf(sendBuff,sizeof sendBuff,"AT+CIPSERVER=1,%d\r\n",GetHttpPort()),NULL,noArch);
    //SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSERVER=1,443,\"SSL\"\r\n"),NULL,noArch );
}

static void CLOUD_FeedReceiveRing ( void )
{
	int position = read_pos;
	while ( position != recvByteFromEsp_copy )
	{
		CloudAt_Feed ( &cloudAt, (const uint8_t *) &RecvBuffer[position], 1 );
		if ( _GET_ACTUAL_CASE_ == 901 )
			HTTP_RequestStreamFeed ( &httpRequestStream, (const uint8_t *) &RecvBuffer[position], 1 );
		IncPos ( &position );
	}
	UpdateReadPos ();
}

/* UART/debug notifications always wake us immediately. Only active cloud
 * transfers need short polls; idle HTTP sleeps until the next upload is due. */
static TickType_t CLOUD_WaitTicks ( void )
{
	if ( connectionType == GOOGLE_CLOUD_CONNECTION )
		return pdMS_TO_TICKS ( 100 );
	if ( !googleCloudStatus.configured )
		return portMAX_DELAY;
	if ( connectionType == INIT_CONNECTION )
	{
		int step = _GET_ACTUAL_CASE_;
		return step >= 22 && step <= 27 ? pdMS_TO_TICKS ( 100 ) : portMAX_DELAY;
	}
	if ( connectionType == HTTP_CONNECTION && googleCloudStatus.ready &&
		 ( Const.wifiGeneral.mode == WIFI_MODE_STA || Const.wifiGeneral.mode == WIFI_MODE_AP_STA ) )
	{
		TickType_t elapsed = xTaskGetTickCount () - (TickType_t) cloudLastAttempt;
		TickType_t period = pdMS_TO_TICKS ( GOOGLE_CLOUD_PERIOD_MS );
		/* When HTTP is busy, defer the attempt without spinning at zero ticks. */
		return elapsed < period ? period - elapsed : pdMS_TO_TICKS ( 250 );
	}
	return portMAX_DELAY;
}

void HTTP_SensorLCDWake ( void )
{
	if ( vtaskWifiHandle )
		xTaskNotify ( vtaskWifiHandle, BIT_ESP_SRV, eSetBits );
}

void vtaskWifi(void *argument)
{
	char *pHttp,*pHttp2,  *ptr;
	int channel=0, size=0, code=0, len, result;   int nrHTTPpacket=0;  int nrSMTP=0;


	uint32_t ulNotifiedValue;


	int typeSendArch = arch;
	int typeRecvArch = arch;

	InitReloadHttpTime();
	StartDMA();
	ESP_ON;

	EmailSendParam.start=0;
	DefaultSettingsWIFI();
	DefaultSettingsEmail();
	DefaultSettingsDNS();
	DefaultSettingsSNTP();
	HTTP_SensorStorageInit ();
	HTTP_SettingsStorageInit ();
	HTTP_SettingsBindingsApply ();
	LPC_StorageInit ();
	HTTP_RequestStreamInit ( &httpRequestStream, HTTP_HandleTarget, NULL );
	InitStructRqstToSendChnl();
	RcvTimeoutTimerInit();
	memset ( (void *) &googleCloudStatus, 0, sizeof googleCloudStatus );
	googleCloudStatus.configured = GOOGLE_CLOUD_ENABLED && CloudConfig_Valid ( GOOGLE_CLOUD_TOKEN, GOOGLE_CLOUD_DEVICE, GOOGLE_CLOUD_PATH );
	googleCloudStatus.last_error = GOOGLE_CLOUD_ENABLED && !googleCloudStatus.configured ? -1 : 0;
	DbgVarDma ( DBG, 160, "Google CONFIG: enabled=%d, poprawna=%d, okres=%u ms, port temperatury=%u\r\n", GOOGLE_CLOUD_ENABLED, googleCloudStatus.configured, (unsigned) GOOGLE_CLOUD_PERIOD_MS, (unsigned) GOOGLE_CLOUD_TEMPERATURE_PORT );
	cloudLastAttempt = xTaskGetTickCount ();
	CloudAt_Reset ( &cloudAt );

	Dbg(DBG,"\r\nStart vtaskWifi\r\n");   //StartUp aktivity dla tego watki jezeli nie ma odp na AT to innty watek restartuje ten watek


//WYMUS CallbackTimeou np poprzez zamkniecie n agle przgladarki !!!!


	strcpy(HttpRefr_sdram,HttpRefr);  //!!!!!!!!!!!!!!!



	//Cykliczne odpytatywanie czy nie zawieszony np AT

	while(1)
	{
		HTTP_SensorLCDService ();
		/* Apply a timeout reset in the WiFi task before copying more data. */
		if ( httpResetRequested )
		{
			taskENTER_CRITICAL ();
			httpResetRequested = 0;
			taskEXIT_CRITICAL ();
			InitStructRqstToSendChnl ();
			UpdateReadPos ();
		}
		/* FreeRTOS copies and clears pending bits atomically before returning.
       * Notifications arriving during processing remain pending for next time. */
		if ( xTaskNotifyWait ( 0, UINT32_MAX, &ulNotifiedValue, CLOUD_WaitTicks () ) == pdPASS )
		{
			/*	if(ulTaskNotifyTake(pdTRUE,portMAX_DELAY)) */ /* Czekaj na powiadomienie.  Dzięki pdTRUE w pierwszym argumencie, po wyjściu z funkcji wartość powiadomienia zostanie zresetowana do 0 */
			if ( ulNotifiedValue & BIT_ESP_SRV )
			{
				GetNewReadPos ();															  //Zastanowic sie co w przypadku przepelnienia bufora cyklicznego i nie zdarzenie oblugi go
				SCB_InvalidateDCache_by_Addr ( (uint32_t *) RecvBuffer, ESP_RECV_BUFF_SIZE ); /* Jesli w MPU ustawimy adres bufora w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_InvalidateDCache_by_Addr() nie jest potrzebny */

				switch ( connectionType )
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
						SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSERVERMAXCONN=%d\r\n",ESP_MAX_HTTP_CONN), NULL, typeSendArch );
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

								if ( googleCloudStatus.configured )
								{
									cloudCleanupTick = xTaskGetTickCount ();
									DbgVarDma ( DBG, 120, "Google INIT: czas SNTP OK, certyfikat CA=%u, instalacja=%d\r\n", (unsigned) GOOGLE_CLOUD_CA_INDEX, GOOGLE_CLOUD_INSTALL_CA );
									if ( GOOGLE_CLOUD_INSTALL_CA )
									{
										_SET_NEW_CASE_ ( 22 );
										cloudCertOffset = 0;
										cloudCleanupTick = xTaskGetTickCount ();
										SendToEsp32 ( mini_snprintf ( sendBuff, sizeof sendBuff, "AT+SYSMFG=2,\"client_ca\",\"client_ca.%u\",8,%u\r\n",
																	  (unsigned) GOOGLE_CLOUD_CA_INDEX, (unsigned) sizeof google_cloud_roots ),
													  NULL, noArch );
									}
									else
									{
										_SET_NEW_CASE_ ( 25 );
										SendToEsp32 ( mini_snprintf ( sendBuff, sizeof sendBuff, "AT+CIPSSLCCONF=3,2,0,%u\r\n", (unsigned) GOOGLE_CLOUD_CA_INDEX ), NULL, noArch );
									}
									COMMAND_Service ( _SET, sendBuff );
								}
								else
								{
									connectionType = HTTP_CONNECTION;
									_SET_NEW_CASE_ ( 0 );
									UpdateReadPos ();
								}
								/* Time comes from the existing SNTP INIT, no second Wi-Fi setup. */ /* Poniewż nie używasz w tym case funkcji SendToEsp32(), w której jest UpdateReadPos() musisz sam wywołać UpdateReadPos() */
							}
							else
							{
								vTaskDelay(2000);    //jezeli nie ma odpowiedzi sntp to trzeba cyklicznie co godzine np pytac
								if(_GET_REP_CASE_ == SNTP_NMBR_QUERY)
								{
									if ( googleCloudStatus.configured )
									{
										googleCloudStatus.last_error = -2;
										DbgVarDma ( DBG, 100, "Google INIT BLAD: brak czasu SNTP (-2), wysylanie zablokowane\r\n" );
									}
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
					else if ( CASE_Service ( 22, ">", txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							googleCloudStatus.last_error = -3;
							_SET_NEW_CASE_ ( 26 );
							SendToEsp32 ( 0, "AT\r\n", noArch );
						}
						else
						{
							_SET_NEW_CASE_ ( 23 ); /* Certificate chunks continue on DMA-ready ticks. */
						}
					}
					else if ( CASE_Service ( 24, txt_OK, txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							googleCloudStatus.last_error = -3;
							_SET_NEW_CASE_ ( 26 );
							SendToEsp32 ( 0, "AT\r\n", noArch );
						}
						else
						{
							SendToEsp32 ( mini_snprintf ( sendBuff, sizeof sendBuff, "AT+CIPSSLCCONF=3,2,0,%u\r\n", (unsigned) GOOGLE_CLOUD_CA_INDEX ), NULL, noArch );
						}
					}
					else if ( CASE_Service ( 25, txt_OK, txt_ERR, noArch ) )
					{
						DbgVarDma ( DBG, 100, "Google INIT: konfiguracja weryfikacji TLS=%s\r\n", _GET_ANSW_CASE_ == _OK ? "OK" : "BLAD" );
						googleCloudStatus.ready = _GET_ANSW_CASE_ == _OK;
						if ( !googleCloudStatus.ready )
							googleCloudStatus.last_error = -3;
						SendToEsp32 ( 0, "AT+CIPRECVMODE=0\r\n", noArch );
						COMMAND_Service ( _SET, sendBuff );
					}
					else if ( CASE_Service ( 26, txt_OK, txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							googleCloudStatus.ready = 0;
							googleCloudStatus.last_error = -3;
						}
						SendToEsp32 ( 0, "AT+CIPDINFO=0\r\n", noArch );
					}
					else if ( CASE_Service ( 27, txt_OK, txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							googleCloudStatus.ready = 0;
							googleCloudStatus.last_error = -3;
						}
						connectionType = HTTP_CONNECTION;
						_SET_NEW_CASE_ ( 0 );
						UpdateReadPos ();
						DbgVarDma ( DBG, 100, "Google INIT: %s\r\n", googleCloudStatus.ready ? "TLS ready" : "TLS configuration error" );
					}
					else
					{
						ESP32_FreeAnswers(1/*2*/);			/* Jezeli NIE wchodzimy w żaden case to rownież wywolujemy ESP32_FreeAnswers() */
					}
					break;

				case GOOGLE_CLOUD_CONNECTION:
					CLOUD_FeedReceiveRing ();
					if ( CASE_Service ( 0, txt_OK, txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							CLOUD_Close ( -5 );
							break;
						}
						SendToEsp32 ( mini_snprintf ( sendBuff, sizeof sendBuff, "AT+CIPSSLCCN=3,\"%s\"\r\n", CLOUD_Host () ), NULL, noArch );
					}
					else if ( CASE_Service ( 1, txt_OK, txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							CLOUD_Close ( -5 );
							break;
						}
						SendToEsp32 ( mini_snprintf ( sendBuff, sizeof sendBuff, "AT+CIPSSLCSNI=3,\"%s\"\r\n", CLOUD_Host () ), NULL, noArch );
					}
					else if ( CASE_Service ( 2, txt_OK, txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							CLOUD_Close ( -5 );
							break;
						}
						DbgVarDma ( DBG, 120, "Google TLS: lacze z %s:443, TCP=3\r\n", CLOUD_Host () );
						CloudAt_Reset ( &cloudAt );
						SendToEsp32 ( mini_snprintf ( sendBuff, sizeof sendBuff, "AT+CIPSTART=3,\"SSL\",\"%s\",443\r\n", CLOUD_Host () ), NULL, noArch );
					}
					else if ( CASE_Service ( 3, txt_OK, txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							CLOUD_Close ( -5 );
							break;
						}
						DbgVarDma ( DBG, 120, "Google TLS: OK; wysylam %s, %u bajtow\r\n", cloudRedirected ? "GET potwierdzenia" : "POST pomiaru", (unsigned) cloudRequestLength );
						cloudOffset = 0;
						cloudPacketLength = cloudRequestLength;
						if ( cloudPacketLength > sizeof sendBuff - 1 )
							cloudPacketLength = sizeof sendBuff - 1;
						SendToEsp32 ( mini_snprintf ( sendBuff, sizeof sendBuff, "AT+CIPSEND=3,%u\r\n", (unsigned) cloudPacketLength ), NULL, noArch );
					}
					else if ( CASE_Service ( 4, ">", txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							CLOUD_Close ( -5 );
							break;
						}
						SendToEsp32 ( (int) cloudPacketLength, cloudRequest + cloudOffset, noArch );
					}
					else if ( CASE_Service ( 5, "\r\nSEND OK", txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							CLOUD_Close ( -5 );
							break;
						}
						cloudOffset += cloudPacketLength;
						DbgVarDma ( DBG, 120, "Google SEND OK: TCP=3, %u/%u bajtow\r\n", (unsigned) cloudOffset, (unsigned) cloudRequestLength );
						if ( cloudOffset < cloudRequestLength )
						{
							cloudPacketLength = cloudRequestLength - cloudOffset;
							if ( cloudPacketLength > sizeof sendBuff - 1 )
								cloudPacketLength = sizeof sendBuff - 1;
							_SET_NEW_CASE_ ( 4 );
							SendToEsp32 ( mini_snprintf ( sendBuff, sizeof sendBuff, "AT+CIPSEND=3,%u\r\n", (unsigned) cloudPacketLength ), NULL, noArch );
						}
					}
					else if ( CASE_Service ( 10, txt_OK, txt_ERR, noArch ) )
					{
						CloudAt_Reset ( &cloudAt );
						_SET_NEW_CASE_ ( 1 );
						SendToEsp32 ( mini_snprintf ( sendBuff, sizeof sendBuff, "AT+CIPSSLCCN=3,\"%s\"\r\n", CLOUD_Host () ), NULL, noArch );
					}
					else if ( CASE_Service ( 900, txt_OK, txt_ERR, noArch ) )
					{
						CLOUD_RestoreHttp ();
					}
					else if ( CASE_Service ( 901, txt_OK, txt_ERR, noArch ) )
					{
						if ( _GET_ANSW_CASE_ != _OK )
						{
							cloudResult = -9;
							googleCloudStatus.ready = 0;
						}
						googleCloudStatus.last_error = cloudResult;
						if ( cloudResult )
							googleCloudStatus.failures++;
						else
							googleCloudStatus.successes++;
						cloudLastAttempt = xTaskGetTickCount ();
						DbgVarDma ( DBG, 200, "Google KONIEC: wynik=%d, HTTP=%s, OK=%u, bledy=%u\r\n", cloudResult, cloudResult == -9 ? "BLAD - restart ESP" : "przywrocony", (unsigned) googleCloudStatus.successes, (unsigned) googleCloudStatus.failures );
						RcvTimeoutTimerStop ();
						BackToHttpService ( &nrHTTPpacket );
						xTaskNotify ( vtaskWifiHandle, BIT_ESP_SRV, eSetBits );
						if ( cloudResult == -9 )
						{
							connectionType = INIT_CONNECTION;
							_SET_NEW_CASE_ ( 0 );
							ESP_OFF;
							vTaskDelay ( pdMS_TO_TICKS ( 20 ) );
							ESP_ON;
						}
					}
					/* SEND OK and the complete HTTP response may arrive together. */
					if ( connectionType == GOOGLE_CLOUD_CONNECTION && _GET_ACTUAL_CASE_ == 6 )
					{
						int parsed = cloudAt.overflow ? -1 : CloudHttp_Parse ( cloudAt.wire, cloudAt.used, cloudAt.eof, &cloudResponse );
						if ( parsed < 0 )
						{
							CLOUD_Close ( -6 );
						}
						else if ( !parsed )
						{
							DbgVarDma ( DBG, 120, "Google RESPONSE: HTTP=%d, przekierowanie=%d\r\n", cloudResponse.status, cloudRedirected );
							if ( ( cloudResponse.status == 302 || cloudResponse.status == 303 ) && !cloudRedirected &&
								 !CloudRedirect_Path ( cloudResponse.location, cloudRedirect, sizeof cloudRedirect ) )
							{
								DbgVarDma ( DBG, 120, "Google REDIRECT: HTTPS script.googleusercontent.com (bez tokenu)\r\n" );
								cloudRedirected = 1;
								cloudRequestLength = CloudRequest_Build ( cloudRequest, sizeof cloudRequest, CLOUD_Host (), cloudRedirect, NULL, NULL, 0, 0, NULL );
								if ( !cloudRequestLength )
								{
									CLOUD_Close ( -6 );
									break;
								}
								cloudAt.events = 0;
								_SET_NEW_CASE_ ( 10 );
								SendToEsp32 ( 0, "AT+CIPCLOSE=3\r\n", noArch );
							}
							else
							{
								int relay;
								if ( cloudResponse.status != 200 || CloudReply_Parse ( cloudResponse.body, cloudNonce, &relay ) )
								{
									DbgVarDma ( DBG, 120, "Google ACK BLAD: HTTP=%d lub niezgodne potwierdzenie/nonce\r\n", cloudResponse.status );
									CLOUD_Close ( -7 );
								}
								else
								{
									googleCloudStatus.demo_relay = relay;
									DbgVarDma ( DBG, 200, CoB_ "Google ACK OK: HTTP 200, poprawne nonce; przekaznik DEMO odebrany z Google=%s (%d)\r\n" _X_, relay ? "ON" : "OFF", relay );
									CLOUD_Close ( 0 );
								}
							}
						}
					}
					break;

				case HTTP_CONNECTION_2:		/* Only for:  AT+CIPSERVERMAXCONN=1 */
					RcvTimeoutTimerStop();
					DispRecvBuff(++nrHTTPpacket,typeSendArch);  ESP32_FreeAnswers(0);

					if ((pHttp=strstr_(NULL,"0,CONNECT\r\n")))						/* RecvFromEsp("0,CONNECT\r\n")   0-channel */			/* Równoczesne właczenie różnych przegladarek pod ten sam adres IP powoduje że jedna czeka na zakończenie drugiego, w trakcie połączenia 0,CONNECT nie pojawia sie np 1,CONNECT tylko po zakończeniu 0,CONNECT pojawia sie z drugiej przegladarki rownież 0,CONNECT */
					{
						INIT_BUFF(answer, "+IPD,"ESP_HTTP_CHANNEL);
						if ((pHttp=strstr_(pHttp,answer)))						/* RecvFromEsp("+IPD,0,698:GET /")   0-channel, 698-received bytes */
						{
							if ((pHttp2=strstr_(pHttp,":GET / ")))
							{
								GetHTTPpacketParam(pHttp,&channel,&size);					/* char temp[20]={0};  strcpy2_(temp,pHttp,0,pHttp2-pHttp);   temp="+IPD,0,698" */
								if(typeSendArch!=noArch)  DbgVarDma(DBG,100,_S_"\r\nRecv HTTP data: channel %d  size %d "_E_,channel,size);
								SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,"AT+CIPSEND=%d,%d\r\n",channel,mini_strlen(HTML_TXT_CODE)), NULL, typeSendArch );
							}
							else if ((pHttp2=strstr_(pHttp,":GET /favicon.ico")))				/* Każde nowe połączenie generuje 0,CONNECT tj. czeka na zakończenie jednego by 'weszlo' drugie, nie ma przychodzących rownocześnie połączeń */
							{
								GetHTTPpacketParam(pHttp,&channel,&size);
								if(typeSendArch!=noArch)  DbgVarDma(DBG,100,_S_"\r\nRecv HTTP data: channel %d  size %d "_E_,channel,size);
								SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPCLOSE=%d\r\n",channel), NULL, typeSendArch );
							}
							else UpdateReadPos();
						}
					}
					else if (RecvFromEsp("\r\nOK\r\n\r\n>"))
					{
					/*	SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,HTML_TXT_CODE), NULL, typeSendArch ); */
						strcpy(sendBuff,HTML_TXT_CODE);  SendToEsp32(2039,NULL,typeSendArch /*noArch*/ );
					}
					else if (RecvFromEsp(",CLOSED\r\n"))
					{
						if (RecvFromEsp("\r\nOK\r\n"))
						{
							if(typeSendArch!=noArch) DbgDma(DBG, _S_" --- CLOSED --- "_E_);
							UpdateReadPos();
						}
					}
					else if (RecvFromEsp("ERROR"))
					{
						if(typeSendArch!=noArch) DbgDma(DBG, _SE_" --- ERROR --- "_E_);
						UpdateReadPos();
					}
					else if ((pHttp=RecvFromEsp("\r\nRecv ")))						/* RecvFromEsp("\r\nRecv 88 bytes")   88-received bytes by ESP */
					{
						if (strstr_(pHttp," bytes\r\n")){
							if (strstr_(pHttp,"\r\nSEND OK"))
							{
								char temp[50]={0};  strcpy_(temp,pHttp,1,'\r');		/* strcpy2_(temp,pHttp,0,30); */
								int val = STRING_GetInt(temp,' ');					/* val = atoi_(pHttp,mini_strlen("\r\nRecv ")); */
								if(typeSendArch!=noArch){
									DbgVarDma(DBG,200,_S_"\r\n%d received bytes by ESP32 "_E_,val);
									DbgDma(DBG, _S_" --- SEND OK --- "_E_);
								}

								if(httpPar.nr[0] > 100){  httpPar.nr[0]=0;
									SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPCLOSE=%d\r\n",channel), NULL, typeSendArch);		/* Czas wykonania SendToEsp32() to 28us */
								}
								else{  httpPar.nr[0]++; DbgDma(1,".");
									SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,"AT+CIPSEND=%d,%d\r\n",channel,mini_strlen(HTML_TXT_CODE)), NULL, typeSendArch );
								}
							}
						}
					}
					else
					{
						UpdateReadPos();
					}
					break;


				case HTTP_CONNECTION:
					RcvTimeoutTimerStop();
					typeSendArch=noArch;
					/*DispRecvBuff(++nrHTTPpacket,typeSendArch);*/  ESP32_FreeAnswers(0);  //KASUJ STRUCT PAR HTTP jesli zawisnie na kakis czas !!!!!!! w timer calback !!!!

					HTTP_FeedReceiveRing ();
					HTTP_SendCloseChnl ( typeSendArch, -1 );

					if ((pHttp=RecvFromEsp("\r\nOK\r\n\r\n>")))
					{
						HTTP_SendPayload ( typeSendArch );
					}

					if ((pHttp=RecvFromEsp(",CLOSED\r\n")))
					{
						u8 chnlClose=HTTP_GetClosedChannel(pHttp);
						HTTP_ShowClosedChannel(chnlClose,typeSendArch);

						if(httpPar.chnl == chnlClose)    /* jesli ten Case nastapil po wysylce at+cipsend lub at+cipclose - SYNCHRONICZNY */
						{
							int nr = SearchChnl(httpPar.chnl);
							if(nr>=0){
								httpTxPhase[nr] = 0;
								httpTxOffset[nr] = 0;
								httpPar.que[nr] = 0xFF; // daj jako funkcja zerujaca to szwszystko
								httpPar.nr[nr]=0;
								httpPar.ptr[nr]=NULL;
								httpPar.siz[nr][0]=0;
								httpPar.len[nr]=0;
								httpPar.web[nr][0]=NULL;
								httpPar.nrWeb[nr]=0;
							}
							httpPar.chnl=0xFF;		/* zezwol na nastepna wysylke */
							HTTP_SendCloseChnl(typeSendArch,-1);
						}
						else /* if(httpPar.chnl != chnlClose) */       /* jesli ten Case nastapil NIE po wysylce at+cipsend lub at+cipclose - ASYNCHRONICZNY */
						{
							int nr = SearchChnl(chnlClose);
							if(nr>=0){
								httpTxPhase[nr] = 0;
								httpTxOffset[nr] = 0;
								httpPar.que[nr] = 0xFF;
								httpPar.nr[nr]=0;
								httpPar.ptr[nr]=NULL;
								httpPar.siz[nr][0]=0;
								httpPar.len[nr]=0;
								httpPar.web[nr][0]=NULL;
								httpPar.nrWeb[nr]=0;
							}
						}

					}
					if ((pHttp=RecvFromEsp("ERROR")))
					{
						if(typeSendArch!=noArch) DbgDma(DBG, _SE_" --- ERROR --- "_E_);

					}
					if ((pHttp=RecvFromEsp("\r\nSEND OK")))
					{
						HTTP_SendAcknowledged ( typeSendArch );
					}
					UpdateReadPos();
					break;

//ZROB tablice allokacji !!!!!!!!!!!!!! wyswieltlanie na zadanie
//Gdy ida razem HTTP i SMTP to moze nie otrzymac odpowiedzi od  AT+CIPSERVER=0 i WISI   (daj jesli w SMTP dlugo czeka na odpowiedz to po np 30 sekund wracaj do HTTP) !!!!!!!!!!!!!!


				case TEST_CONNECTION:
					if (CASE_Service(0,txt_OK,txt_ERR,typeRecvArch))
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

//Ugupuj zmienne !!!!!
//SMTP duzeilosci danych przygotuj kod !!!!!
//SMTP jesli po uplywie czasu nie otrzymama odpowiedzi to WRACAM do HTTP (teraz wraca do http jesli otrzym ERROR) !!!!!


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
								len=mini_snprintf(sendBuff,sizeof(sendBuff),"TO: ");
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
						//SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSERVER=1,443,\"SSL\"\r\n"),NULL,typeRecvArch );
						SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSERVER=1,%d\r\n",GetHttpPort()),NULL,typeSendArch );
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


		}


		if (ulNotifiedValue & BIT_DBG_SRV)
		{
			DEBUG_InvalidateDCache();

			if ( DEBUG_IsTxtReceive ( "cloud" ) )
			{
				DbgVarDma ( 1, 220, "Google: configured=%d ready=%d error=%d OK=%u FAIL=%u temp10=%d DEMO=%d\r\n",
							googleCloudStatus.configured, googleCloudStatus.ready, googleCloudStatus.last_error,
							(unsigned) googleCloudStatus.successes, (unsigned) googleCloudStatus.failures,
							googleCloudStatus.temperature10, googleCloudStatus.demo_relay );
			}
			else if ( connectionType == GOOGLE_CLOUD_CONNECTION ||
					  ( connectionType == INIT_CONNECTION && _GET_ACTUAL_CASE_ >= 22 && _GET_ACTUAL_CASE_ <= 27 ) )
			{
				DbgDma ( DBG, "\r\nESP zajety przez Google; ponow polecenie po zakonczeniu.\r\n" );
			}
			else if ( DEBUG_IsTxtReceive ( "a" ) )
			{
				DbgVarDma2(1,50,"\r\nHTTP PAR: chnl:%d\r\n",httpPar.chnl);
				HTTP_ShowHttpStruct();

			}
			else if(DEBUG_IsTxtReceive("b"))
			{
				DbgDma(DBG,_S_"\r\n"_E_); HTTP_ShowHttpStruct(); DbgDma(DBG,_S_"\r\nRESET http chnl struct "_E_); InitStructRqstToSendChnl();

			}
			else if(DEBUG_IsTxtReceive("s"))
			{
				if (connectionType==HTTP_CONNECTION  &&  xTimerIsTimerActive(xWaitOnSendTimeoutTimer)==pdFALSE  &&  IsTimeoutForReloadHttp()  &&  !IsActiveAnyRqstToSendChnl())
				{
					SendEmail(2, 1<<1, EMAIL_MEASURE);			/* z  'interia.pl'  i  'wp.pl'  musi byc ustawione: Główne_Ustawienia -> Parametry -> Korzystam z programu pocztowego aby mogl wysylac */

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
			else if(DEBUG_IsTxtReceive("o"))
			{
				connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
				DbgDma(DBG, _S_"\r\nSend Test AT "_E_);
				SendToEsp32(0,"AT+CIPSTATUS\r\n",arch);						/* STATUS:3		+CIPSTATUS:4,"SSL","213.180.147.145",465,53751,0	  OK */
			}
			else if(DEBUG_IsTxtReceive("p"))
			{
				connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
				DbgDma(DBG, _S_"\r\nSend Test AT "_E_);
				SendToEsp32(0,"AT+CIPSTATE?\r\n",arch);						/* 				+CIPSTATE:4,"SSL","213.180.147.145",465,53751,0		  OK */
			}
			else if(DEBUG_IsTxtReceive("c"))
			{
				DbgDma(DBG, Clr_"\r\n-------------------- Start -------------------- ");
			}
			else if(DEBUG_IsTxtReceive("m"))
			{
				DbgDma(DBG, _S_"\r\nTest "_E_);
				strcpy(HttpRefr_sdram,"11:34:12 &nbsp;26/07/25280000000000000 111111111111111111 55555555555555 -79.11 65%0 -78.32 98%1 -56.01 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0       6.0       5.7      ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0 V     V              1.005   KWh            2.006  0          3.007  0          4.008  0          5.009  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0");
				DbgDma(DBG, HttpRefr_sdram);
			}
			else if(DEBUG_IsTxtReceive("n"))
			{
				DbgDma(DBG, _S_"\r\nTest "_E_);
				strcpy(HttpRefr_sdram,"11:34:12 &nbsp;26/07/25280000000000000 111111111111111111 55555555555555  99.11 65%0 -78.32 98%1 -56.01 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0  --.-0 --%0       6.0       5.7      ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0   ---   0 V     V              1.005   KWh            2.006  0          3.007  0          4.008  0          5.009  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0          0.000   KWh            0.000  0          0.000  0          0.000  0          0.000  0");
				DbgDma(DBG, HttpRefr_sdram);
			}
		/*
			else if(DEBUG_IsTxtReceive("0")){ GoToTest(0); SendToEsp32(0,"AT+RFPOWER?\r\n",arch); }
			else if(DEBUG_IsTxtReceive("1")){ GoToTest(0); SendToEsp32(0,"AT+RFCAL\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("2")){ GoToTest(0); SendToEsp32(0,"AT+CWSTATE?\r\n",arch); }
			else if(DEBUG_IsTxtReceive("3")){ GoToTest(0); SendToEsp32(0,"AT+CWLAP\r\n",arch); }
			else if(DEBUG_IsTxtReceive("4")){ GoToTest(0); SendToEsp32(0,"AT+CWSAP?\r\n",arch); }			//wyswietla parametry swojej sieci AP
			else if(DEBUG_IsTxtReceive("5")){ GoToTest(0); SendToEsp32(0,"AT+CWQAP\r\n",arch); }
			else if(DEBUG_IsTxtReceive("6")){ GoToTest(0); SendToEsp32(0,"AT+CWLIF\r\n",arch); }			//Obtain IP Address of the Station That Connects to an ESP SoftAP
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
			else if(DEBUG_IsTxtReceive("k")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=2,\"client_ca\",\"client_ca.0\",8,807\r\n",arch);    }  	//zapisz cert
			else if(DEBUG_IsTxtReceive("j")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=1,\"client_ca\",\"client_ca.0\"\r\n",arch);    }  	//odpytaj sprawdz czy zapisano     //client_ca.1,2,3.. to klejne certy
			else if(DEBUG_IsTxtReceive("h")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=1,\"client_ca\",\"client_ca.1\"\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("g")){ GoToTest(0); SendToEsp32(0,"AT+CIPSSLCSNI=4,\"poczta.interia.pl\"\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("f")){ GoToTest(0); SendToEsp32(0,"AT+CIPSSLCSNI?\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("m")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=1,\"client_ca\",\"client_ca\"\r\n",arch);    }
			else if(DEBUG_IsTxtReceive("n")){ GoToTest(0); SendToEsp32(0,"AT+SYSMFG=2,\"client_ca\",\"client_ca.1\",8,807\r\n",arch);    }

			else if(DEBUG_IsTxtReceive("?"))
			{
				connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
				DbgDma(DBG, _S_"\r\nWysylam DATA cert "_E_);
				SendToEsp32(0,"-----BEGIN CERTIFICATE-----MIIDrzCCApegAwIBAgIQCDvgVpBCwQdBmJ8V4vhx7DANBgkqhkiG9w0BAQsFADBhMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBDQTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5jb20xIDAeBgNVBAMMF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1EQmYN5QLuEZDY6ntPEGwihkagMrYueAtvxhgreZdkgy5y5CsBBe3N5nJJyXghQIGMC4gKs8Omc79w4Z89yr3DH4mTJUYisH6g6NnIKVWhm5O1LKQXwpAsBq6t8UR695QnNMc9asj6L4gqq8GAe4uM9N178gKv1Cyg9SxZg6Es747teA9Wh2BKRpoY32SWhF9b8SjK4769PkVqAhDJ1U0Y7FVThB4S960uG6P440dgE2B3S4Y6T6qv7vU76RAfB9CTh9K6GfFED7P1B2b4vP5lBp4mMc36X9U8v5eB6H8K5gJ2779EX0G2uLgWp4f9Jtw1F65A7GwIDAQABo2MwYTAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QnvStFj8Q92bJv8ONM0OD8KgwHwYDVR0jBBgwFoAUA95QnvStFj8Q92bJv8ONM0OD8KgwDQYJKoZIhvcNAQELBQADggEBAF3m3EehIiKNtAsST6VKVDTjCejY373bTIi7P35XmKAVffv2yQth564v96f7y9S41U9fDxU8yX5VfJv+D0P/X7H7gqH5fU1FfZ/pX76V/0j8+y6b0g177p+p7gK4S9P6VfXf9C8=-----END CERTIFICATE-----",arch);  //1108
				SendToEsp32(0,"-----BEGIN CERTIFICATE-----\nMIIDjjCCAnagAwIBAgIQAzrx5W63S0b6AE5BHSTHPTANBgkqhkiG9w0BAQsFADBh\nMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\nd3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\nMjAeFw0zMDA4MDExMjAwMDBaFw01MDA4MDExMjAwMDBaMGExCzAJBgNVBAYTAlVT\nMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWcipWVydC5j\nb20xIDAeBgNVBAMMF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1EQmYqrrXSVzXgS7OwtWc9L\nFBBg6snA5bVvOr4H7PDg3VU8qgifgJuogGxOn4UOFIOMpUMG7up6X8XSpYtH8Wwi\nhy67vvwbFJuINkwXgX6V1E24gwEsPDyDxa6e4T6kKvTOUeNANJMch80wFH57S875\nfA3lyWMa2L6+fThvAt04VIO4u30R73MD4b46p6UoMQk96Sbt8v60pYgCE6EAsvIK\nA67X9M1N9L04wREOcIEX3gS6g7LwK7Bf8XpBfXbSB3o9A7S3XfR7O2EDMA40OpFL\nwX10r8A7E+V2N8H4NURbY6K7J4wNURM9OhSgI==-----END CERTIFICATE-----",arch);   //807
			}
			else if(DEBUG_IsTxtReceive("6"))
			{
				connectionType=TEST_CONNECTION;   _SET_NEW_CASE_(0);
				DbgDma(DBG, _S_"\r\nSend Test AT3 "_E_);
				SendToEsp32(0,"AT+SYSFLASH?\r\n",arch);
			}
		*/

		}
	  }
	  if ( connectionType == INIT_CONNECTION && googleCloudStatus.configured &&
		   _GET_ACTUAL_CASE_ >= 22 && _GET_ACTUAL_CASE_ <= 27 &&
		   xTaskGetTickCount () - cloudCleanupTick >= pdMS_TO_TICKS ( GOOGLE_CLOUD_TIMEOUT_MS ) )
	  {
		  DbgVarDma ( DBG, 120, "Google INIT BLAD: timeout certyfikatu/TLS, etap=%d; restart ESP\r\n", _GET_ACTUAL_CASE_ );
		  /* An ESP stuck in raw certificate upload must be reset, not fed AT text. */
		  googleCloudStatus.configured = 0;
		  googleCloudStatus.ready = 0;
		  googleCloudStatus.last_error = -3;
		  RcvTimeoutTimerStop ();
		  _SET_NEW_CASE_ ( 0 );
		  UpdateReadPos ();
		  ESP_OFF;
		  vTaskDelay ( pdMS_TO_TICKS ( 20 ) );
		  ESP_ON;
	  }
	  else if ( connectionType == INIT_CONNECTION && _GET_ACTUAL_CASE_ == 23 )
	  {
		  /* SYSMFG takes raw bytes, not CIPSEND; never overwrite active UART DMA. */
		  if ( ESP_UART_HANDLE.gState == HAL_UART_STATE_READY )
		  {
			  size_t bytes = sizeof google_cloud_roots - cloudCertOffset;
			  if ( bytes > sizeof sendBuff - 1 )
				  bytes = sizeof sendBuff - 1;
			  if ( cloudCertOffset + bytes == sizeof google_cloud_roots )
			  {
				  _SET_NEW_CASE_ ( 24 );
			  }
			  if ( SendToEsp32 ( (int) bytes, (char *) google_cloud_roots + cloudCertOffset, noArch ) == HAL_OK )
				  cloudCertOffset += bytes;
			  else
			  {
				  googleCloudStatus.configured = 0;
				  googleCloudStatus.last_error = -3;
				  RcvTimeoutTimerStop ();
				  _SET_NEW_CASE_ ( 0 );
				  UpdateReadPos ();
				  ESP_OFF;
				  vTaskDelay ( pdMS_TO_TICKS ( 20 ) );
				  ESP_ON;
			  }
		  }
	  }
	  if ( connectionType == GOOGLE_CLOUD_CONNECTION )
	  {
		  uint32_t now = xTaskGetTickCount ();
		  int step = _GET_ACTUAL_CASE_;
		  if ( step < 900 && now - cloudStarted >= pdMS_TO_TICKS ( GOOGLE_CLOUD_TIMEOUT_MS ) )
			  CLOUD_Close ( -4 );
		  else if ( step == 900 && now - cloudCleanupTick >= pdMS_TO_TICKS ( 5000 ) )
			  CLOUD_RestoreHttp ();
		  else if ( step == 901 && now - cloudCleanupTick >= pdMS_TO_TICKS ( 5000 ) )
		  {
			  DbgVarDma ( DBG, 120, "Google HTTP BLAD: timeout przywracania serwera (-9); restart ESP\r\n" );
			  googleCloudStatus.last_error = -9;
			  googleCloudStatus.failures++;
			  googleCloudStatus.ready = 0;
			  cloudLastAttempt = now;
			  RcvTimeoutTimerStop ();
			  InitStructRqstToSendChnl ();
			  connectionType = INIT_CONNECTION;
			  _SET_NEW_CASE_ ( 0 );
			  UpdateReadPos ();
			  ESP_OFF;
			  vTaskDelay ( pdMS_TO_TICKS ( 20 ) );
			  ESP_ON;
		  }
	  }
	  else if ( connectionType == HTTP_CONNECTION && googleCloudStatus.ready &&
				( Const.wifiGeneral.mode == WIFI_MODE_STA || Const.wifiGeneral.mode == WIFI_MODE_AP_STA ) &&
				!IsActiveAnyRqstToSendChnl () && xTaskGetTickCount () - tickCnt >= pdMS_TO_TICKS ( 250 ) &&
				xTimerIsTimerActive ( xWaitOnSendTimeoutTimer ) == pdFALSE &&
				xTaskGetTickCount () - cloudLastAttempt >= pdMS_TO_TICKS ( GOOGLE_CLOUD_PERIOD_MS ) )
	  {
		  cloudLastAttempt = xTaskGetTickCount ();
		  int temp = GoogleCloud_ReadTemperature10 ();
		  if ( temp < -800 || temp > 1500 )
		  {
			  googleCloudStatus.last_error = -18;
			  googleCloudStatus.failures++;
			  DbgVarDma ( DBG, 120, "Google POMINIETO: brak temperatury lub poza zakresem (-18), odczyt=%d\r\n", temp );
		  }
		  else if ( !CLOUD_Nonce () )
		  {
			  googleCloudStatus.last_error = -8;
			  googleCloudStatus.failures++;
			  DbgVarDma ( DBG, 100, "Google BLAD: generator RNG/nonce (-8)\r\n" );
		  }
		  else
		  {
			  googleCloudStatus.temperature10 = temp;
			  cloudRedirected = 0;
			  cloudResult = 0;
			  cloudRequestLength = CloudRequest_Build ( cloudRequest, sizeof cloudRequest, CLOUD_Host (), GOOGLE_CLOUD_PATH,
														GOOGLE_CLOUD_TOKEN, GOOGLE_CLOUD_DEVICE, temp, googleCloudStatus.demo_relay, cloudNonce );
			  if ( !cloudRequestLength )
			  {
				  googleCloudStatus.last_error = -1;
				  googleCloudStatus.failures++;
				  DbgVarDma ( DBG, 100, "Google BLAD: nie mozna zbudowac zadania (-1)\r\n" );
			  }
			  else
			  {
				  CloudAt_Reset ( &cloudAt );
				  cloudStarted = xTaskGetTickCount ();
				  DbgVarDma ( DBG, 200, CoB_ "\r\nGoogle START: wysylana temperatura=%s%d.%d C; DEMO ACK=%d; wstrzymuje HTTP\r\n" _X_, temp < 0 ? "-" : "", ( temp < 0 ? -temp : temp ) / 10, ( temp < 0 ? -temp : temp ) % 10, googleCloudStatus.demo_relay );
				  connectionType = GOOGLE_CLOUD_CONNECTION;
				  _SET_NEW_CASE_ ( 0 );
				  SendToEsp32 ( 0, "AT+CIPSERVER=0\r\n", noArch );
			  }
		  }
	  }
	}
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


void WIFI_RxCallbackService(void)
{
	Dbg(DBG, "\r\n -----  USART6  HAL_UART_RxCpltCallback -------  ");   //Przerob aby w przerwaniach byly tylko xTaskNotify do vLog Task !!!!!!!!!!!!!!!!!!
}


/* ------------- ATTENTIONS ------------------------------ */
/*
AKTUALIZUJ firmware ESP przez strone ESp Home bo przez esp download tool nie dziala */   /* !!! BOOT pin (IO0) low,  EN high !!!		RXD0,TXDO -> do progr.  IO17,IO18 -> at commands
https://web.esphome.io/
https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Binary_Lists/esp_at_binaries.html#firmware-esp32-wroom-32-series
Aby sprawdzic firmawer : at+gmr [enter]+[ctrl+j]

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

	AT+CWAUTOCONN=1: Włącza automatyczne łączenie z AP przy starcie (standardowo jest włączone).
	AT+SYSSTORE=1:   Upewnia się, że zmiany w konfiguracji Wi-Fi (jak SSID i hasło) są zapisywane w pamięci flash, aby przetrwały restart.
	AT+SYSMSG:       Pozwala na konfigurację dodatkowych komunikatów systemowych (np. o rozłączeniu), co jest dostępne w nowszych wersjach oprogramowania (powyżej v2.1.0.0).
*/

