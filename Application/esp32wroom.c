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

#define ESP_RECV_BUFF_SIZE		2048
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

#define HTML_TXT_CODE		"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>ESP32 SSL 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789</h1></body></html>"
#define ESP32_DOMAIN_ERROR 	"\r\nNOT updated email Server IP "
#define TXT_OK				"\r\nOK\r\n"
#define TXT_ERR				"ERROR"
#define EMAIL_ERROR			" --- email ERROR --- "
#define RecvFromEsp(txt)   strstr(RecvBuffer,txt) 		/* alternatywnie memcmp() strnstr()  */

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

typedef enum
{
	_OK=1, _ERROR
} ANSWER_TYPE;

typedef enum
{
	INIT_CONNECTION, HTTP_CONNECTION, SMTP_CONNECTION
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

static int DBG = 1;
static uint8_t connectionType = INIT_CONNECTION;
static int recvByteFromEsp = 0;

extern UART_HandleTypeDef ESP_UART_HANDLE;
extern DMA_HandleTypeDef ESP_UART_DMA_RX;
static xTaskHandle vtaskWifiHandle=NULL;
static int resetDMA=0;

RAM_D2_ALIGN32 static char RecvBuffer[ESP_RECV_BUFF_SIZE];
RAM_D2_ALIGN32 static char sendBuff[PACKET_SEND_LEN];	//BYC moze zwiekszenie predkosci uart dopiero gdy wlacze FIFO_UART enable !!!!!!!!!!!!!!!!!!!!!

static int qqq=0;
void ESP32_Notify2EspThread(int interruptSrc, uint16_t size, long *pxWoken)					/* size: ile zostalo wolnego miejsca w buforze DMA,  size=0 to bufor DMA calkowice zapelniony */
{
	recvByteFromEsp = ESP_RECV_BUFF_SIZE - size;
/*  vTaskNotifyGiveFromISR(vtaskWifiHandle, pxWoken);	*/					/* Wyślij powiadomienie bezpośrednio do wątku */
    if(interruptSrc==0){
    	if(vtaskWifiHandle!=NULL) xTaskNotifyFromISR(vtaskWifiHandle,BIT_ESP_SRV,eSetBits,pxWoken);  }
    else{
    	if(vtaskWifiHandle!=NULL) xTaskNotifyFromISR(vtaskWifiHandle,BIT_DBG_SRV,eSetBits,pxWoken);  }
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
		VAR_SetVal64(Const_wifiSTA_mac, i, 0x1122334455);
		VAR_SetTabVal(Const_wifiSTA_ip, i, LWIP_MAKEU32(192,168,1,99));
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
}

static void StartDMA(void)																	/* Jesli w tym momencie przyjdzie jakis komunikat asynchroniczny z ESP32 to NIGDY go nie odczytam bo wyczyszcze go. Rozwiazaniem jest Circle DMA */
{
	memset(RecvBuffer, 0, ESP_RECV_BUFF_SIZE);												/* memset() takes 18us */
	SCB_CleanDCache_by_Addr((uint32_t*)RecvBuffer, ESP_RECV_BUFF_SIZE);						/* Wypchnij bufor RecvBuffer z casha do RAMu by wyczyscic pamiec DMA */
	UART_ClearFlags(&ESP_UART_HANDLE);
	HAL_UARTEx_ReceiveToIdle_DMA(&ESP_UART_HANDLE, (uint8_t*) RecvBuffer, ESP_RECV_BUFF_SIZE);
	UART_ClearFlags2(&ESP_UART_HANDLE);
}

static void RestartDMA(void)
{
	HAL_UART_DMAStop(&ESP_UART_HANDLE);
	StartDMA();
}

static int SendToEsp32(int len, char *data, ARCHIVING_TYPE archType)								/* if data=NULL we use buffer 'sendBuff' as default. 		if len=0 we calculate length text. */
{
	int len_ = CONDITION( 0==len, mini_strlen(CONDITION(NULL==data,sendBuff,data)), len );
	if(len_ > PACKET_SEND_LEN-1)  len_=PACKET_SEND_LEN-1;
	if(data != sendBuff && data != NULL){ strncpy(sendBuff,data,len_); }	sendBuff[len_]=0;		/* memcpy(sendBuff, data, len_)  jest szybsze niż strncpy */

		 if(arch ==archType){ DbgMultiDma(DBG,CoR2_"\r\nSEND_START: "_X_,sendBuff,CoR2_" SEND_STOP\r\n"_X_); }
	else if(arch2==archType){ DbgMultiDma(DBG,"\r\n",sendBuff,"\r\n"); }

	//RestartDMA();
	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, PACKET_SEND_LEN);								/* SCB_CleanDCache_by_Addr() takes only 4us */		/* Jesli w MPU ustawimy adres bufora 'sendBuff' w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_CleanDCache_by_Addr() nie jest potrzebny */

/*	uint32_t clean_size = (len_ + (CACHE_LINE_BYTES - 1)) & ~(CACHE_LINE_BYTES - 1); 	 		  Czyszczenie Cache z rozmiarem zaokrąglonym do pełnych linii 32-bajtowych, czyszczenie tylko tego fragmentu, który faktycznie wysyłamy 	CACHE_LINE_BYTES = 32
	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, clean_size);	*/

	int result = HAL_UART_Transmit_DMA(&ESP_UART_HANDLE, (uint8_t*) sendBuff, len_);
	if(result != HAL_OK)  DbgVarDma(DBG,100,_SE_"\r\nHAL_ERROR: %d "_E_,result);
	return result;
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
	char *ptr=NULL;
	char rcv1[]="+CIFSR:APIP,\"";
	char rcv2[]="+CIFSR:APMAC,\"";
	char rcv3[]="+CIFSR:STAIP,\"";
	char rcv4[]="+CIFSR:STAMAC,\"";

	if ((ptr=RecvFromEsp(rcv1)))  Const.wifiAP[ Const.wifiGeneral.nrAP ].ip    = IPStr2Int	  (ptr+mini_strlen(rcv1));  //Powinno zapisywac do zmiennych niezapisywalnych a nie di zapisywalnych !!!!
	if ((ptr=RecvFromEsp(rcv2)))  Const.wifiAP[ Const.wifiGeneral.nrAP ].mac   = MACStr2Int64 (ptr+mini_strlen(rcv2));
	if ((ptr=RecvFromEsp(rcv3)))  Const.wifiAP[ Const.wifiGeneral.nrSTA ].ip   = IPStr2Int	  (ptr+mini_strlen(rcv3));
	if ((ptr=RecvFromEsp(rcv4)))  Const.wifiSTA[ Const.wifiGeneral.nrSTA ].mac = MACStr2Int64 (ptr+mini_strlen(rcv4));
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

void ESP32_FreeAnswers(void)
{
	char *ptr=NULL;
	int flag=0,len=0;

	LOOP_FOR(i,PTR_TAB_SIZE(freeAnswerTypes)){
		if((ptr=RecvFromEsp(freeAnswerTypes[i]))){
			if(RecvFromEsp("+STA_CONNECTED:")||RecvFromEsp("+DIST_STA_IP:")){
				char buf[80]={0}; int i=0;
				while(*(ptr+i)>0x20){ if(i==sizeof(buf)-1){buf[i]=0; break;}  buf[i]=*(ptr+i);  i++; }
				DbgVarDma(DBG,100,_SE_"\r\n%s -%d- "_E_,buf,recvByteFromEsp);
				len=i;
			}
			else{ DbgVarDma(DBG,100,_SE_"\r\n%s -%d- "_E_,freeAnswerTypes[i],recvByteFromEsp);
				  len=mini_strlen(freeAnswerTypes[i]);
			}
			if(*(ptr+len)=='\r'&&*(ptr+len+1)=='\n') len+=2;
			memset(ptr,' ',len);
			flag=1;
		}
	}

	if(flag)
	{
	/*	SCB_CleanDCache_by_Addr((uint32_t*)txt, CACHE_ALLIGN_LEN(size)); */
		uint32_t clean_size = (recvByteFromEsp + (CACHE_LINE_BYTES - 1)) & ~(CACHE_LINE_BYTES - 1); 	 /* Czyszczenie Cache z rozmiarem zaokrąglonym do pełnych linii 32-bajtowych, czyszczenie tylko tego fragmentu, który faktycznie wysyłamy 	CACHE_LINE_BYTES = 32 */
		SCB_CleanDCache_by_Addr((uint32_t*)RecvBuffer, clean_size);
	}
}

static int WaitForRcvEsp(const char* recv1, const char* recv2)
{
	SCB_InvalidateDCache_by_Addr((uint32_t*)RecvBuffer, ESP_RECV_BUFF_SIZE);
	 	 if( NULL!=RecvFromEsp(recv1) && NULL==RecvFromEsp(recv2) ) return 1;
	else if( NULL==RecvFromEsp(recv1) && NULL!=RecvFromEsp(recv2) ) return 2;
	else if( NULL!=RecvFromEsp(recv1) && NULL!=RecvFromEsp(recv2) ) return 3;
	else 															return 0;		/* if( NULL==RecvFromEsp(recv1) && NULL==RecvFromEsp(recv2) ) */
}

static void DispRecvBuff(int nrCase, ARCHIVING_TYPE archType)
{//RECV_START_001_ilosc bajtow do odczytu:
		 if(arch ==archType){ DbgVarDma(DBG,1024,CoG3_"\r\nRECV_START_%03d:"_X_,nrCase); DbgDma(DBG,RecvBuffer); DbgDma(DBG,CoG3_" RECV_STOP\r\n"_X_); }
	else if(arch2==archType){ DbgMultiDma(DBG,"\r\n",RecvBuffer,"\r\n");	}
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
		int hasRecv1 = (recv1 != NULL) && (strstr(RecvBuffer, recv1) != NULL);
		int hasRecv2 = (recv2 != NULL) && (strstr(RecvBuffer, recv2) != NULL);
	    if (hasRecv1 && hasRecv2){ actualCase++; flag=3; flagCase=3; }
	    if (hasRecv2)  			 { actualCase++; flag=2; flagCase=2; }
	    if (hasRecv1) 			 { actualCase++; flag=1; flagCase=1; }
	    if(flag){ DispRecvBuff(nrCase,archType); ESP32_FreeAnswers(); }
	}
	return flag;
}

static int ErrorAnswerService(void)
{
	if(_GET_ANSW_CASE_==_ERROR){  DbgVarDma(DBG,200,_SE_"\r\nCMD_ERROR: %s "_E_,COMMAND_Service(_GET,NULL));  return 1; }
	return 0;
}

void BackFromEmail(int nrInfo)
{
	if(_GET_ANSW_CASE_==_ERROR){
		char *ptr=NULL;
		if((ptr=RecvFromEsp("ERROR"))){
			memset(ptr,' ',5);
			uint32_t clean_size = (recvByteFromEsp + (CACHE_LINE_BYTES - 1)) & ~(CACHE_LINE_BYTES - 1); 	 /* Czyszczenie Cache z rozmiarem zaokrąglonym do pełnych linii 32-bajtowych, czyszczenie tylko tego fragmentu, który faktycznie wysyłamy 	CACHE_LINE_BYTES = 32 */
			SCB_CleanDCache_by_Addr((uint32_t*)RecvBuffer, clean_size);
		}
	}
	if(nrInfo) DbgDma(DBG, _S_ EMAIL_ERROR _E_);
	_CLR_ACTUAL_CASE_;
	connectionType=HTTP_CONNECTION;

	//"AT+CIPCLOSE=4r\n"
}

void vtaskWifi(void *argument)
{
	static char* pMem=NULL;

	char *pHttp,*pHttp2,  *ptr;   int lenHTTP=0;
	int channel=0, size=0, code=0, len, result, result2;   int nrHTTP=0;   int nrPages=0;
	int j;

	uint32_t ulNotifiedValue;

	int typeSendArch = arch;
	int typeRecvArch = arch;

	StartDMA();
	ESP_ON;

	EmailSendParam.start=0;
	DefaultSettingsWIFI();
	DefaultSettingsEmail();
	DefaultSettingsDNS();
	DefaultSettingsSNTP();

	ResetTestTab(); //Do USUNIECIA !!!

	Dbg(DBG,"\r\nStart vtaskWifi\r\n");   //StartUp aktivity dla tego watki jezeli nie ma odp na AT to innty watek restartuje ten watek


/*
	AT+CWAUTOCONN=1: Włącza automatyczne łączenie z AP przy starcie (standardowo jest włączone).
	AT+SYSSTORE=1:   Upewnia się, że zmiany w konfiguracji Wi-Fi (jak SSID i hasło) są zapisywane w pamięci flash, aby przetrwały restart.
	AT+SYSMSG:       Pozwala na konfigurację dodatkowych komunikatów systemowych (np. o rozłączeniu), co jest dostępne w nowszych wersjach oprogramowania (powyżej v2.1.0.0).


*/


//	StartMeasureTime_us();
//	StopMeasureTime_us("\r\nTEST: ");


//	while(1)
//	{
//		vTaskDelay(1000);
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789a");
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789ab");
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abc");
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcd");
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcde");
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcdef");
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcdefgh");
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcdefghi");
//		DbgDmaQue(1,"\r\n0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789abcdefghij");
//	}


	while(1)
	{

	  if(xTaskNotifyWait(0x00, 0x00, &ulNotifiedValue, portMAX_DELAY) == pdPASS)			/* xTaskNotifyWait(bitmask_na_wejsciu, bitmask_na_wyjsciu, &zmienna, czas)		bitmask: 0x00: Nie czyść nic , 0xFFFFFFFF (Wszystkie bity): Czyści całą wartość powiadomienia po wyjściu.   Problem: Jeśli w tym samym momencie (ułamek sekundy po wybudzeniu zadania, ale przed zakończeniem tej funkcji) inne zadanie lub przerwanie (ISR) przyśle nowy bit, zostanie on bezpowrotnie skasowany i utracony. Dlatego najbezpieczniejsze jest uzycie pod koniec obslugi tego zdarzenia reczne czyszczenie aktualnego bitu funkcją ulTaskNotifyValueClear(NULL, ulNotifiedValue) */
	  {
	 /*	if(ulTaskNotifyTake(pdTRUE,portMAX_DELAY)) */										/* Czekaj na powiadomienie.  Dzięki pdTRUE w pierwszym argumencie, po wyjściu z funkcji wartość powiadomienia zostanie zresetowana do 0 */
		if (ulNotifiedValue & BIT_ESP_SRV)
		{
			SCB_InvalidateDCache_by_Addr((uint32_t*)RecvBuffer, ESP_RECV_BUFF_SIZE);		/* Jesli w MPU ustawimy adres bufora w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_InvalidateDCache_by_Addr() nie jest potrzebny */

			switch (connectionType)
			{
				case INIT_CONNECTION:

					if (CASE_Service(0,"ready",NULL,typeRecvArch))
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
						vTaskDelay(10);
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
									switch(atoi(ptr+mini_strlen(answer))){
										case 1:  result=ESP_CONNECTION_TIMEOUT; 		break;
										case 2:  result=ESP_WRONG_PASSWORD;				break;
										case 3:  result=ESP_CANNOT_FIND_THE_TARGET_AP;	break;
										case 4:  result=ESP_CONNECTION_FAILED;			break;
										default: result=ESP_UNKNOW_ERROR_OCCURRED;		break;
								}}
								else result=ESP_UNKNOW_ERROR_OCCURRED;
							}
							DbgVarDma2(DBG,100,_S_"\r\nSTA_CONNECTION status: %d\r\n"_E_,result);		//dac max buffer jako SEND_BUFF_SIZE !!!
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
						SendToEsp32(0,"AT+CIPSERVERMAXCONN=1\r\n",typeSendArch);
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
						SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSERVER=1,443,\"SSL\"\r\n"),NULL,typeSendArch );
						//SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSERVER=1,%d\r\n",GetHttpPort()),NULL,typeSendArch );
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(18,txt_OK,txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()) break;
						SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSTO=%d\r\n",TCP_SERVER_TIMEOUT_S), NULL, typeSendArch );
						COMMAND_Service(_SET,sendBuff);

					}
//					else if (CASE_Service(19,txt_OK,txt_ERR,typeRecvArch))
//					{
//						_THE_SAME_CASE_;								/* przewidujemy w tym case cykliczne powtarzanie */
//
//						/* Obsługa odpowiedzi */
//						{
//							if(_GET_REP_CASE_ == 0)
//							{
//								if(ErrorAnswerService()) break;
//							}
//							else if(IS_RANGE(_GET_REP_CASE_,1,MAX_EMAIL_SENDERS-1))
//							{
//								if(ErrorAnswerService()){ ; }   		/* z tym błędem nic nie rob */
//								else
//								{
//									INIT_BUFF(answer,"+CIPDOMAIN:");
//									if ((ptr=RecvFromEsp(answer))){  Const.emailSend[_GET_REP_CASE_-1].IP = IPStr2Int(ptr+mini_strlen(answer)+1);  }
//									else  						  {  DbgDma(DBG,_S_ ESP32_DOMAIN_ERROR);  }
//								}
//							}
//						}
//
//						/* Obsługa wysylania i iteracji */
//						{
//							if( (WIFI_MODE_STA 	  == Const.wifiGeneral.mode ||
//								 WIFI_MODE_AP_STA == Const.wifiGeneral.mode))
//							{
//								SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPDOMAIN=\"%s\"\r\n",Const.emailSend[_GET_REP_CASE_].server), NULL, typeSendArch );
//								if(_GET_REP_CASE_ == MAX_EMAIL_SENDERS-1)
//									_SET_NEXT_CASE_;
//							}
//							else
//							{
//								SendToEsp32(0,"AT\r\n",typeSendArch);
//								_SET_NEXT_CASE_;
//							}
//							COMMAND_Service(_SET,sendBuff);
//						}
//
//					}
//					else if (CASE_Service(20,txt_OK,txt_ERR,typeRecvArch))
//					{
//						if( (WIFI_MODE_STA 	  == Const.wifiGeneral.mode ||
//							 WIFI_MODE_AP_STA == Const.wifiGeneral.mode))
//						{
//							if(ErrorAnswerService()){ ; }   	 		/* z tym błędem nic nie rob */
//							else
//							{
//								INIT_BUFF(answer,"+CIPDOMAIN:");
//								if ((ptr=RecvFromEsp(answer))){  Const.emailSend[MAX_EMAIL_SENDERS-1].IP = IPStr2Int(ptr+mini_strlen(answer)+1);  }
//								else  						  {  DbgDma(DBG,_S_ ESP32_DOMAIN_ERROR);  }
//							}
//						}
//						else
//						{
//							if(ErrorAnswerService()) break;
//						}
//						SendToEsp32(0,"AT+SYSTIMESTAMP?\r\n",typeSendArch);
//						COMMAND_Service(_SET,sendBuff);
//
//					}
//					else if (CASE_Service(21,txt_OK,txt_ERR,typeRecvArch))
//					{
//						if(ErrorAnswerService()) break;
//						_THE_SAME_CASE_;								/* przewidujemy w tym case cykliczne powtarzanie */
//						time_t getTime;
//						INIT_BUFF(answer,"+SYSTIMESTAMP:");
//						if ((ptr=RecvFromEsp(answer)))
//						{
//							getTime=(time_t)atoi(ptr+mini_strlen(answer));
//							if(getTime>1565853509)
//							{
//								gmtime_r(&getTime,sntpTime);			/* lepsze dla wielowatkowosci niz  sntpTime=gmtime(&getTime) */
//								Const.sntp.time = getTime;
//								DbgVarDma(DBG,500,_S_"\r\nESP32 TIME LOADED %d: %02d-%02d-%02d  %02d:%02d:%02d"_E_,
//										Const.sntp.time,
//										sntpTime->tm_year-100,
//										sntpTime->tm_mon+1,
//										sntpTime->tm_mday,
//										sntpTime->tm_hour,
//										sntpTime->tm_min,
//										sntpTime->tm_sec);
//
//								connectionType = HTTP_CONNECTION;   _SET_NEW_CASE_(0);
//								RestartDMA();
//							}
//							else
//							{
//								vTaskDelay(2000);
//								if(_GET_REP_CASE_ == SNTP_NMBR_QUERY-1)
//								{
//									SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+SYSTIMESTAMP=%d\r\n",Const.sntp.time), NULL,typeSendArch );
//									while(WaitForRcvEsp(txt_OK,txt_ERR)==0) vTaskDelay(10);			/* Wyjątek: czekanie na odpowiedz w pętli bez udzialu przerwania */
//									DispRecvBuff(_GET_ACTUAL_CASE_,typeSendArch);
//									connectionType = HTTP_CONNECTION;   _SET_NEW_CASE_(0);
//									RestartDMA();
//								}
//								else
//								{
//									SendToEsp32(0,"AT+SYSTIMESTAMP?\r\n",typeSendArch);
//									COMMAND_Service(_SET,sendBuff);
//								}
//							}
//						}
//
//					}
					else
					{
						ESP32_FreeAnswers();
					}
					break;



				case HTTP_CONNECTION:

					DispRecvBuff(++nrHTTP,typeSendArch);  ESP32_FreeAnswers();

					if ((pHttp=strstr(RecvBuffer,"0,CONNECT\r\n")))					/* RecvFromEsp("0,CONNECT\r\n")   0-channel */
					{
						if ((pHttp=strstr(pHttp,"+IPD,0")))							/* RecvFromEsp("+IPD,0,698:GET /")   0-channel, 698-received bytes */
						{
							if ((pHttp2=strstr(pHttp,":GET / ")))
							{
								GetSizeAndChannel(pHttp2, &channel, &size);
								SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,"AT+CIPSEND=%d,%d\r\n",channel,mini_strlen(HTML_TXT_CODE)), NULL, typeSendArch );
							}
							else if ((pHttp2=strstr(pHttp,":GET /favicon.ico")))
							{
								GetSizeAndChannel(pHttp2, &channel, &size);
								SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPCLOSE=%d\r\n",channel), NULL, typeSendArch );
							}
							else RestartDMA();
						}
					}
					else if (RecvFromEsp("\r\nOK\r\n\r\n>"))
					{
						SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,HTML_TXT_CODE), NULL, typeSendArch /*noArch*/ );
					}
					else if (RecvFromEsp(",CLOSED\r\n"))
					{
						if (RecvFromEsp("\r\nOK\r\n"))
						{
							if(typeSendArch!=noArch) DbgDma(DBG, _S_" --- CLOSED --- "_E_);
							RestartDMA();
						}
					}
					else if (RecvFromEsp("ERROR"))
					{
						if(typeSendArch!=noArch) DbgDma(DBG, _S_" --- ERROR --- "_E_);
						RestartDMA();
					}
					else if ((pHttp=RecvFromEsp("\r\nRecv ")))						/* RecvFromEsp("\r\nRecv 88 bytes")   88-received bytes by ESP */
					{
						if (strstr(pHttp," bytes\r\n")){
							if (strstr(pHttp,"\r\nSEND OK"))
							{
								int val = STRING_GetInt(pHttp,' ');
								if(typeSendArch!=noArch){
									DbgVarDma(DBG,200,_S_"\r\n%d received bytes by ESP32 "_E_,val);
									DbgDma(DBG, _S_" --- SEND OK --- "_E_);
								}

								if(nrPages > 200){  nrPages=0;
									SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPCLOSE=%d\r\n",channel), NULL, typeSendArch);		/* Czas wykonania SendToEsp32() to 28us */
								}
								else{  nrPages++; DbgDma(1,".");
									SendToEsp32( mini_snprintf(sendBuff,sizeof(sendBuff)-1,"AT+CIPSEND=%d,%d\r\n",channel,mini_strlen(HTML_TXT_CODE)), NULL, typeSendArch );
								}
							}
						}
					}
					else
					{
						RestartDMA();
					}
					break;

//i jesli nie bedzie odpowiedzi po np 30 sekund to timercallbak timeout !!!!!!
//ZROB tablice allokacji !!!!!!!!!!!!!! wyswieltlanie na zadanie
					//SPRAWDZ czy czasem razem nie moze isc HTTP i SMTP !!!!!!!

				case SMTP_CONNECTION:
					if (CASE_Service(0,txt_OK,txt_ERR,typeRecvArch))		/* CASE_Service() osluguje ESP32_FreeAnswers() */
					{
						if(ErrorAnswerService()){  BackFromEmail(0); nrHTTP=0; break;  }
						len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSTART="ESP_EMAIL_CHANNEL",\"%s\",\"%s\",%d\r\n",CONDITION(Const.emailSend[EmailSendParam.whichSender].useSSL,"SSL","TCP"), IP2Str(Const.emailSend[EmailSendParam.whichSender].IP), Const.emailSend[EmailSendParam.whichSender].port);
						SendToEsp32(len,NULL,typeSendArch);
						COMMAND_Service(_SET,sendBuff);

					}
					else if (CASE_Service(1, ESP_EMAIL_CHANNEL",CONNECT\r\n"TXT_OK, txt_ERR, typeRecvArch))				/* "...,CONNECT\r\n\r\nOK\r\n"  a  "\r\n+IPD,..."  jest szczelina czasowa, mozna wydluzyc parametr timeout dla UART6 aby nie generowalo przerwania po 1 czesci ale jednak robimy inaczej: czekamy na calosc 1 czesc i 2 czesc w CASE_Service() */
					{
						if(ErrorAnswerService()){  BackFromEmail(0); nrHTTP=0;  break;  }								/* Details:"4,CONNECT\r\n\r\nOK\r\n\r\n+IPD,4,31:220 smtp.poczta.onet.pl ESMTP\r\n\r\n", '\0' <repeats 1985 times> */
						INIT_BUFF(answer, "\r\n+IPD,"ESP_EMAIL_CHANNEL);	//RecvBuffer																	/* Details:"4,CONNECT\r\n\r\nOK\r\n\r\n+IPD,4,78:421 4.7.0 smtp.poczta.onet.pl Error: too many connections from 46.205.198.71\r\n\r\n", '\0' <repeats 1938 times> */
						if ((ptr=RecvFromEsp(answer)))																	/* Free answer					   	   +IPD,4,55:421 4.4.2 smtp.poczta.onet.pl Error: timeout exceeded */
						{
							channel = STRING_GetInt(ptr,',');	 ptr += mini_strlen(answer);
							size 	= STRING_GetInt(ptr,',');
							code 	= STRING_GetInt(ptr,':');
							if(typeSendArch!=noArch)  DbgVarDma(DBG,100,_S_"\r\nRecv email data: channel %d  size %d "_E_,channel,size);
							if(code==220)
							{
								DbgDma(DBG, _S_" --- Email 220 --- "_E_);
								len = mini_snprintf(sendBuff, sizeof(sendBuff), "EHLO %s\r\n", Const.emailSend[EmailSendParam.whichSender].name);
								if((pMem = (char*)pvPortMalloc((len+1)*sizeof(char)))){												  /* Uruchom vApplicationMallocFailedHook() dla  #define configUSE_MALLOC_FAILED_HOOK 1 */
									strncpy(pMem,sendBuff,len);  *(pMem+len)=0;
									len = mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSEND="ESP_EMAIL_CHANNEL",%d\r\n",len);
									SendToEsp32(len,NULL,typeSendArch);
									COMMAND_Service(_SET,sendBuff);
								}
								else{  BackFromEmail(0); nrHTTP=0;  break; }
							}
							else{  BackFromEmail(1); nrHTTP=0;  break; }
						}
						else _THE_SAME_CASE_;

					}
					else if (CASE_Service(2,TXT_OK"\r\n>",txt_ERR,typeRecvArch))
					{
						if(ErrorAnswerService()){  if(pMem) vPortFree(pMem); BackFromEmail(0); nrHTTP=0;  break;  }
						SendToEsp32(0,pMem,typeSendArch);
						COMMAND_Service(_SET,sendBuff);
						if(pMem) vPortFree(pMem);

					}
					else if (CASE_Service(3,"\r\nSEND OK\r\n\r\n+IPD,4",txt_ERR,typeRecvArch)) 					/* Details:"\r\nRecv 20 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,4,169:250-smtp.poczta.onet.pl\r\n250-PIPELINING\r\n250-SIZE 90000000\r\n250-ETRN\r\n250-AUTH PLAIN LOGIN XOAUTH2\r\n250-AUTH=PLAIN LOGIN XOAUTH2\r\n250-ENHANCEDSTATUSCODES\r\n250... */
					{
						if(ErrorAnswerService()){  BackFromEmail(0); nrHTTP=0;  break;  }
						if((ptr=RecvFromEsp("\r\nRecv "))){
							size = STRING_GetInt(ptr,' ');
							if(typeSendArch!=noArch){
								DbgVarDma(DBG,200,_S_"\r\n%d received bytes by ESP32 "_E_,size);
								DbgDma(DBG, _S_" --- SEND OK --- "_E_);
							}
						}
						INIT_BUFF(answer,"+IPD,");
						if ((ptr=RecvFromEsp(answer))){
							channel = STRING_GetInt(ptr,',');	 ptr += mini_strlen(answer);
							size 	= STRING_GetInt(ptr,',');
							code 	= STRING_GetInt(ptr,':');
							if(typeSendArch!=noArch)  DbgVarDma(DBG,100,_S_"\r\nRecv email data: channel %d  size %d "_E_,channel,size);
							if(code==250){
								DbgDma(DBG, _S_" --- Email 250 --- "_E_);
								BackFromEmail(0); nrHTTP=0;
							}
							else{  BackFromEmail(1); nrHTTP=0;  break; }
						}
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
				DbgDmaQue(DBG, _S_"Rafal MarkielowskiRafal MarkielowskiRafal MarkielowskiRafal MarkielowskiRafal MarkielowskiRafal Markielowski "_E_);
			}
			else if(DEBUG_IsTxtReceive("s"))
			{
				SendEmail(0, 1<<1, EMAIL_MEASURE);

				if( (WIFI_MODE_STA 	  == Const.wifiGeneral.mode   ||
					 WIFI_MODE_AP_STA == Const.wifiGeneral.mode)  &&  Const.emailSend[ EmailSendParam.whichSender ].IP )
				{
					DbgDma(DBG, _S_"\r\nWysylam email... "_E_);
					_CLR_ACTUAL_CASE_;
					connectionType=SMTP_CONNECTION;
					SendToEsp32(0,"AT\r\n"/*"AT+CIPSERVER=0\r\n"*/,typeSendArch);
					COMMAND_Service(_SET,sendBuff);
				}




			}
			else if(DEBUG_IsTxtReceive("x"))
			{
				DbgDma(DBG, _S_"x"_E_);
			}
			else if(DEBUG_IsTxtReceive("z"))
			{
				DbgDma(DBG, _S_"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"_E_);
			}
			else if(DEBUG_IsTxtReceive("q"))
			{
				Dbg(DBG, _S_"\r\n uint32_t ulPoprzedniaWartosc = ulTaskNotifyValueClear(NULL, ulNotifiedValue) "_E_);
			}
			else if(DEBUG_IsTxtReceive("c"))
			{
				DbgDma(DBG, Clr_"\r\n-------------------- Start -------------------- ");
			}




			uint32_t ulPoprzedniaWartosc = ulTaskNotifyValueClear(NULL, ulNotifiedValue);
			if ((ulPoprzedniaWartosc & BIT_DBG_SRV) != 0) {												/* Sprawdzenie, czy zgłoszenie w ogóle występowało przed wyczyszczeniem: */

				asm("nop");
			}
		}
	  }
	}
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
	RestartDMA();
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
