/*
 * wifi.c
 *
 *  Created on: 11.12.2020
 *      Author: RafalMar
 */
#include "wwwPages.h"
#include <string.h> /* memset */
#include <stdlib.h> /* atoi */
#include "esp32wroom.h"
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

#define ESP_RECV_BUFF_SIZE		2048
#define PACKET_SEND_LEN 		2048

#define HTTP_ANSWER_DELAY_MS		500
#define SMTP_CONNECTION_DELAY_MS		15000
#define SMTP_ANSWER_DELAY_MS		10000
#define CONNECTION_TIMEOUT_MS		30000
#define TCP_SERVER_TIMEOUT_S		10
#define SNTP_SERVER_TIMEOUT_MS		5000
#define DNS_SERVER_TIMEOUT_MS		15000

#define DBG		1

#define ESP_ON 		HAL_GPIO_WritePin(ESP_EN_GPIO_TYPE, ESP_EN_GPIO_PIN, GPIO_PIN_SET)
#define ESP_OFF		HAL_GPIO_WritePin(ESP_EN_GPIO_TYPE, ESP_EN_GPIO_PIN, GPIO_PIN_RESET)

#define RecvFromEsp(txt)   strstr(RecvBuffer,txt)

typedef enum
{
	INIT_CONNECTION, HTTP_CONNECTION, SMTP_CONNECTION
} CONNECTION_TYPE;

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

static uint8_t connectionType;

extern UART_HandleTypeDef ESP_UART_HANDLE;
extern DMA_HandleTypeDef ESP_UART_DMA_RX;
static xTaskHandle vtaskWifiHandle;
static int resetDMA=0;

RAM_D2_ALIGN32 char RecvBuffer[ESP_RECV_BUFF_SIZE];
RAM_D2_ALIGN32 char sendBuff[PACKET_SEND_LEN];


void ESP32_Notify2EspThread(uint16_t size, long *pxWoken)		/* size: ile zostalo wolnego miejsca w buforze DMA,  size=0 to bufor DMA calkowice zapelniony */
{
    vTaskNotifyGiveFromISR(vtaskWifiHandle, pxWoken);			/* Wyślij powiadomienie bezpośrednio do wątku */
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
		VAR_SetTabVal(Const_wifiAP_dhcp, i, 0);
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
		VAR_SetTabVal(Const_wifiSTA_dhcp, i, 0);
		VAR_SetStr(Const_wifiSTA_name, i, "T-Mobile_Swiatlowod_8638");
		VAR_SetStr(Const_wifiSTA_pass, i, "03109069984530029251");
//		VAR_SetStr(Const_wifiSTA_name, i, "MetronicAKP");
//		VAR_SetStr(Const_wifiSTA_pass, i, "1qaZ@MetronicZ3");
	}
	VAR_SetTabVal(Const_wifiGeneral_nrAP,NO_TAB,0);
	VAR_SetTabVal(Const_wifiGeneral_nrSTA,NO_TAB,0);
	VAR_SetTabVal(Const_wifiGeneral_mode,NO_TAB,WIFI_MODE_STA);
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

static void StartDMA(void)
{
	memset(RecvBuffer, 0, ESP_RECV_BUFF_SIZE);
	SCB_CleanDCache_by_Addr((uint32_t*)RecvBuffer, ESP_RECV_BUFF_SIZE);						/* Wypchnij bufor RecvBuffer z casha do RAMu by wyczyscic pamiec DMA */
	UART_ClearFlags(&ESP_UART_HANDLE);
	HAL_UART_Receive_DMA(&ESP_UART_HANDLE, (uint8_t*) RecvBuffer, ESP_RECV_BUFF_SIZE);
	UART_ClearFlags2(&ESP_UART_HANDLE);
}

static void RestartDMA(void)
{
	HAL_UART_DMAStop(&ESP_UART_HANDLE);
	StartDMA();
}

static int SendToEsp(char *txt)
{
	int len = mini_strlen(txt);  // to trzeba zmienic koniecznie
	if(txt != sendBuff){
		strncpy(sendBuff,txt,len);
	}
	RestartDMA();
	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, PACKET_SEND_LEN);  //POPRAW TO !!!!!!! n ie wiem czy szkoda czasu na caly bufor spradz pomiarem uS
	int result= HAL_UART_Transmit_DMA(&ESP_UART_HANDLE, (uint8_t*) sendBuff, len);
	return result;
}

static int SendToEsp2(char *pData, int lenData)  // to daj tuu z on lub OFF -> DbgMulti(DBG,"\r\n*** SEND: ",sendBuff," END SEND ***\r\n");
{
	if(pData != sendBuff){
		strncpy(sendBuff,pData,lenData);
	}
	RestartDMA();
	SCB_CleanDCache_by_Addr((uint32_t*)sendBuff, PACKET_SEND_LEN);							/* Jesli w MPU ustawimy adres bufora w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_CleanDCache_by_Addr() nie jest potrzebny */
	return HAL_UART_Transmit_DMA(&ESP_UART_HANDLE, (uint8_t*) sendBuff, lenData);
}

static bool isAnythingRecv(void)
{
    uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);		/* Czekaj na powiadomienie.  Dzięki pdTRUE w pierwszym argumencie, po wyjściu z funkcji wartość powiadomienia zostanie zresetowana do 0 */

    if(ulNotificationValue > 0)
    {
    	return true;
    }

    return false;


//	if (RecvBuffer[0]>0)
//		return true;
//	else
//		return false;
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
	if ((result=vSendData(temp->pData, temp->len, ESP_EMAIL_CHANNEL)))
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
	if ((result=vSendDataPacket(pCommand, commandLen, ESP_EMAIL_CHANNEL)))
	{
		SetEmailState(SMTP_FAIL);
		SetEmailCode(100+result);
	}
	return result;
}

static int GetRecvCodeEmail(char *pBuf)
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

	} while (*channel!=ESP_EMAIL_CHANNEL);

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
						ESP_EMAIL_CHANNEL,
						buftemp,
						IP2Str(VAR_GetTabVal(Const_emailSend_IP, EmailSendParam.whichSender)),
						VAR_GetTabVal(Const_emailSend_port, EmailSendParam.whichSender));
//				len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSTART=%d,\"%s\",\"213.180.147.145\",%d\r\n",
//						ESP_EMAIL_CHANNEL,
//						buftemp,
//						VAR_GetTabVal(Const_emailSend_port, EmailSendParam.whichSender));
				SendToEsp2(sendBuff, len);
				Dbg(DBG,"\r\n"); Dbg(DBG,sendBuff);
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
	char *ptr;
	int nrAP=VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB);
	int nrSTA=VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB);

	if ((ptr=RecvFromEsp("+CIFSR:APIP,\"")))
		VAR_SetTabVal(Const_wifiAP_ip, nrAP, IPStr2Int(ptr+13));

	if ((ptr=RecvFromEsp("+CIFSR:APMAC,\"")))
		VAR_SetVal64(Const_wifiAP_mac, nrAP, MACStr2Int64(ptr+14));

	if ((ptr=RecvFromEsp("+CIFSR:STAIP,\"")))
		VAR_SetTabVal(Const_wifiSTA_ip, nrSTA, IPStr2Int(ptr+14));

	if ((ptr=RecvFromEsp("+CIFSR:STAMAC,\"")))
		VAR_SetVal64(Const_wifiSTA_mac, nrSTA, MACStr2Int64(ptr+15));
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

static void vLoadTime(time_t timeSet)
{
	int len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+SYSTIMESTAMP=%d\r\n",timeSet);
	SendToEsp2(sendBuff,len);
	while (RecvFromEsp("\r\nOK")==0)
		vTaskDelay(10);
}

int nnnnr=0;
void vtaskWifi(void *argument)
{
	char *pHttpGet;   int lenHTTP=0;
	int channel=0, size=0, len, result, result2;
	int j;

	StartDMA();
	ESP_ON;

	connectionType=INIT_CONNECTION;
	EmailSendParam.start=0;
	DefaultSettingsWIFI();
	DefaultSettingsEmail();
	DefaultSettingsDNS();
	DefaultSettingsSNTP();

	ResetTestTab(); //Do USUNIECIA !!!

	Dbg(DBG,"\r\nStart vtaskWifi\r\n");   //StartUp aktivity dla tego watki jezeli nie ma odp na AT to innty watek restartuje ten watek
	nnnnr=0;


/*
	AT+CWAUTOCONN=1: Włącza automatyczne łączenie z AP przy starcie (standardowo jest włączone).
	AT+SYSSTORE=1:   Upewnia się, że zmiany w konfiguracji Wi-Fi (jak SSID i hasło) są zapisywane w pamięci flash, aby przetrwały restart.
	AT+SYSMSG:       Pozwala na konfigurację dodatkowych komunikatów systemowych (np. o rozłączeniu), co jest dostępne w nowszych wersjach oprogramowania (powyżej v2.1.0.0).


*/


	while(1)
	{
		if(ulTaskNotifyTake(pdTRUE,portMAX_DELAY))
		{
			SCB_InvalidateDCache_by_Addr((uint32_t*)RecvBuffer, ESP_RECV_BUFF_SIZE);			/* Jesli w MPU ustawimy adres bufora w kawalku pamieci jako MPU_ACCESS_NOT_CACHEABLE to SCB_InvalidateDCache_by_Addr() nie jest potrzebny */

			switch (connectionType)
			{
				case INIT_CONNECTION:

					if (nnnnr==0 && RecvFromEsp("ready"))
					{
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_111_");
						SendToEsp("ATE0\r\n");
						nnnnr++;
					}
					else if (nnnnr==1 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_222_");
						mini_snprintf(sendBuff, sizeof(sendBuff), "AT+UART_CUR=%d,8,1,0,0\r\n", ESP_UART_BUADRATE);
						SendToEsp(sendBuff);
						nnnnr++;
					}
					else if (nnnnr==2 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_333_");
						vTaskDelay(10);
						ChangeUartBuadRate(ESP_UART_BUADRATE);
						SendToEsp("AT+GMR\r\n");
						nnnnr++;
					}
					else if (nnnnr==3 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_444_");
						len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CWMODE=%d\r\n",VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB));
						SendToEsp2(sendBuff,len);
						//DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
						nnnnr++;
					}
					else if (nnnnr==4 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_555_");
						if(WIFI_MODE_DISABLED==VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB)){
							Dbg(DBG, "\r\nWifi DISABLED ");
							break;
						}
						SendToEsp("AT+CWLAPOPT=1,23\r\n");
						nnnnr++;
					}
					else if (nnnnr==5 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_666_");
						SendToEsp("AT+CIPMUX=1\r\n");
						nnnnr++;
					}
					else if (nnnnr==6 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_777_");
						SendToEsp("AT+CWDHCP=0,3\r\n");
						nnnnr++;
					}
					else if (nnnnr==7 && RecvFromEsp("\r\nOK"))
					{
						int flag=1;
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_888_");
						if(0==VAR_GetTabVal(Const_wifiAP_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB))&&
							1==VAR_GetTabVal(Const_wifiSTA_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)) )
						{
							SendToEsp("AT+CWDHCP=1,1\r\n");
							flag=0;
						}
						else if(1==VAR_GetTabVal(Const_wifiAP_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB))&&
								  0==VAR_GetTabVal(Const_wifiSTA_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)) )
						{
							SendToEsp("AT+CWDHCP=1,2\r\n");
							flag=0;
						}
						else if(1==VAR_GetTabVal(Const_wifiAP_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB))&&
								  1==VAR_GetTabVal(Const_wifiSTA_dhcp,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)) )
						{
							SendToEsp("AT+CWDHCP=1,3\r\n");
							flag=0;
						}
						if(flag) SendToEsp("AT\r\n");
						nnnnr++;
					}
					else if (nnnnr==8 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_999_");
						SendToEsp("AT+CWHOSTNAME=\"Elektronika_STM\"\r\n");
						nnnnr++;
					}
					else if (nnnnr==9 && (RecvFromEsp("\r\nOK")||RecvFromEsp("ERROR")))
					{
						Dbg(DBG,RecvBuffer);   Dbg(DBG,"_AAA_");
						if(RecvFromEsp("ERROR")) break;

						int flag=1;
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
								//DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
								flag=0;
							}
							break;
						}
						if(flag) SendToEsp("AT\r\n");
						nnnnr++;
					}
					else if (nnnnr==10 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);
						int flag=1;   Dbg(DBG,"_BBB_");
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
								//DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
								flag=0;
							}
							break;
						}
						if(flag) SendToEsp("AT\r\n");
						nnnnr++;
					}
					else if (nnnnr==11 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);
						int flag=1;   Dbg(DBG,"_CCC_");
						switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
						{
						case WIFI_MODE_AP:
						case WIFI_MODE_AP_STA:
							len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CWSAP=\"%s\",\"%s\",5,3\r\n",
									VAR_GetStr(Const_wifiAP_name,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB)),
									VAR_GetStr(Const_wifiAP_pass,VAR_GetTabVal(Const_wifiGeneral_nrAP,NO_TAB)));
							SendToEsp2(sendBuff,len);
							//DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
							flag=0;
							break;
						}
						if(flag) SendToEsp("AT\r\n");
						nnnnr++;
					}
					else if (nnnnr==12 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);
						int flag=1;   Dbg(DBG,"_DDD_");
						switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
						{
						case WIFI_MODE_STA:
						case WIFI_MODE_AP_STA:
							len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CWJAP=\"%s\",\"%s\"\r\n",
									VAR_GetStr(Const_wifiSTA_name,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)),
									VAR_GetStr(Const_wifiSTA_pass,VAR_GetTabVal(Const_wifiGeneral_nrSTA,NO_TAB)));  // Timer do logowania !!!!!  i poprawic GetPort !! zamiast port na indeks
							SendToEsp2(sendBuff,len);
							//DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
							flag=0;
							break;
						}
						if(flag) SendToEsp("AT\r\n");
						nnnnr++;
					}
					else if (nnnnr==13 && (RecvFromEsp("\r\nOK")||RecvFromEsp("ERROR")))
					{
						if(RecvFromEsp("WIFI CONNECTED") && RecvFromEsp("WIFI GOT IP"))
						{
							Dbg(DBG, "----- MAM IP :) -----");
						}
						Dbg(DBG,RecvBuffer);
						result=vGetConnectionResultToSTA();   Dbg(DBG,"_EEE_");   //vGetConnectionResultToSTA() w tej funkcji trzeba zmienic pozostalosc po poprzednim !!!!!!!!!!!!!!!
						if(ESP_CONNECTION_OK!=result)
							DbgVar(DBG,30,"\r\nERROR_ESP_CONNECTION: %d\r\n",result);
						SendToEsp("AT+CIFSR\r\n");
						nnnnr++;
					}
					else if (nnnnr==14 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer);  Dbg(DBG,"_FFF_");
						GetAddressesForConnection();
						SendToEsp("AT+CIPSERVERMAXCONN=1\r\n");
						nnnnr++;
					}
					else if (nnnnr==15 && RecvFromEsp("\r\nOK"))
					{
						Dbg(DBG,RecvBuffer); Dbg(DBG,"_GGG_");
						len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPDNS=1,\"%s\",\"%s\",\"%s\"\r\n",
								IP2Str(VAR_GetTabVal(Const_dns_IP1,NO_TAB)),
								IP2Str(VAR_GetTabVal(Const_dns_IP2,NO_TAB)),
								IP2Str(VAR_GetTabVal(Const_dns_IP3,NO_TAB)));
						//DbgMulti(DBG,"\r\n",sendBuff,"\r\n");
						SendToEsp2(sendBuff,len);   //!!!!!!!!!!!!!! Czekaj na flage z HAL_UART_TxCpltCallback() !!!!
						nnnnr++;
					}
					else if (nnnnr==16 && (RecvFromEsp("\r\nOK")||RecvFromEsp("ERROR")))
					{
						Dbg(DBG, RecvBuffer);   Dbg(DBG,"_HHH_");
						len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSNTPCFG=1,%d,\"%s\",\"%s\"\r\n",
								VAR_GetTabVal(Const_sntp_timezone,NO_TAB),
								VAR_GetStr(Const_sntp_nameServer1,NO_TAB),
								VAR_GetStr(Const_sntp_nameServer2,NO_TAB));
						SendToEsp2(sendBuff,len);
						nnnnr++;
					}
					else if (nnnnr==17 && (RecvFromEsp("\r\nOK")||RecvFromEsp("ERROR")))
					{
						Dbg(DBG, RecvBuffer);   Dbg(DBG,"_III_");
						//len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSERVER=1,%d\r\n", GetHttpPort());
						len=mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSERVER=1,443,\"SSL\"\r\n");
						SendToEsp2(sendBuff,len);
						nnnnr++;
					}
					else if (nnnnr==18 && (RecvFromEsp("\r\nOK")||RecvFromEsp("ERROR")))
					{
						Dbg(DBG, RecvBuffer);   Dbg(DBG,"_JJJ_");
						len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPSTO=%d\r\n",TCP_SERVER_TIMEOUT_S);
						SendToEsp2(sendBuff,len);
						nnnnr++;
					}
					else if (nnnnr==19 && (RecvFromEsp("\r\nOK")||RecvFromEsp("ERROR")))
					{
						Dbg(DBG, RecvBuffer);   Dbg(DBG,"_KKK _");
						int flag=1;
						switch(VAR_GetTabVal(Const_wifiGeneral_mode,NO_TAB))
						{
							case WIFI_MODE_STA:
							case WIFI_MODE_AP_STA:
								len=mini_snprintf(sendBuff,sizeof(sendBuff),"AT+CIPDOMAIN=\"%s\"\r\n",VAR_GetStr(Const_emailSend_server,0/*i*/));			//!!!!!! for(i=0;i<MAX_EMAIL_SENDERS;++i)  !!!!!!!!
								SendToEsp2(sendBuff,len);  DbgMulti(DBG,"\r\n",sendBuff," ");
								flag=0;
								break;
						}
						if(flag) SendToEsp("AT\r\n");
						nnnnr++;
					}
					else if (nnnnr==20 && (RecvFromEsp("\r\nOK")||RecvFromEsp("ERROR")))
					{
						if(RecvFromEsp("+TIME_UPDATED"))  ///UTWORZ TABLICE WOLNYCH ODPOWIEDZI Z ESP32 np DISCONNECTED itd !!!!!!!!!!!!!
						{
							Dbg(DBG," ---- Czas Zaladowany :) ---- ");
						}
						char *ptr;
						Dbg(DBG, RecvBuffer);   Dbg(DBG,"_LLL_");
						if (RecvFromEsp("\r\nOK"))
						{
							if ((ptr=RecvFromEsp("+CIPDOMAIN:")))  //ZROB LISTE MOZLIWYCH ODPOWIEDZI JESLI NIE MA TAKIEJ TO WYSWIETL JA !!!!!!!
							{																	//for(i=0;i<MAX_EMAIL_SENDERS;++i) !!!!!!!!
								VAR_SetTabVal(Const_emailSend_IP,0/*i*/,IPStr2Int(ptr+12)); //POPRAWIC to '12' !!!!!! dac jako przeszukuje do znaki ":"   +CIPDOMAIN:"213.180.147.145"
								DbgMulti(DBG,"\r\n",ptr,"  ");
								SendToEsp("AT+SYSTIMESTAMP?\r\n");
							}
							else
								break;
						}
						else if (RecvFromEsp("ERROR"))
							return;

						nnnnr++;
					}
					else if (nnnnr==21 && (RecvFromEsp("\r\nOK")||RecvFromEsp("ERROR")))
					{
						if(RecvFromEsp("+TIME_UPDATED"))
						{
							Dbg(DBG," ---- Czas Zaladowany22222 :) ---- ");
						}
						int itx=0;
						char *ptr;
						time_t getTime;
						Dbg(DBG, RecvBuffer);   Dbg(DBG,"_MMM_");
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

								connectionType=HTTP_CONNECTION;  nnnnr=0;
								RestartDMA();
							}
							else{
								vTaskDelay(2000);
								SendToEsp("AT+SYSTIMESTAMP?\r\n");  //zrobic cykliczne odpytywanie az bedzie czas SNTP_SERVER_TIMEOUT_MS jak nie za jakis czas to zero wpisac
							}
						}

						//Dbg(DBG,"\r\nKKKKKKKKKKKOOOOOOOOOOOOOOOOOONNNNNNNNNNNNNIEEEEEEEEEECCCCCCC !!!!!");
					}
					else
					{

							//Dbg(DBG, "\r\nSTART:\r\n");  Dbg(DBG, RecvBuffer); Dbg(DBG, " KONIEC\r\n\r\n");

					}

					break;

				case HTTP_CONNECTION:  //dac jesli HAL error!!!   if (HAL_OK!=SendToEsp2(tempBuff, commandLen)) return 1

					DbgMulti(DBG,"\r\nRECV_START: ",RecvBuffer," RECV_STOP\r\n");

					if ((pHttpGet=RecvFromEsp(",CONNECT\r\n")))		/* RecvFromEsp("0,CONNECT\r\n")   0-channel */
					{
						if ((pHttpGet=RecvFromEsp("+IPD,")))				/* RecvFromEsp("+IPD,0,698:GET /")   0-channel, 698-received bytes */
						{
							if ((pHttpGet=RecvFromEsp(":GET / ")))
							{
								GetSizeAndChannel(pHttpGet, &channel, &size);
								lenHTTP = mini_strlen("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>ESP32 SSL</h1></body></html>");
								int len = mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSEND=%d,%d\r\n", channel, lenHTTP);
								DbgMulti(DBG,"\r\nSEND_START: ",sendBuff," SEND STOP\r\n");
								SendToEsp2(sendBuff, len );
							}
							else if ((pHttpGet=RecvFromEsp(":GET /favicon.ico")))
							{
								GetSizeAndChannel(pHttpGet, &channel, &size);
								int len = mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPCLOSE=%d\r\n", channel);
								DbgMulti(DBG,"\r\nSEND_START: ",sendBuff," SEND STOP\r\n");
								SendToEsp2(sendBuff, len);
							}
						}
						else  //Tu srpadzaj cala tablice roznych dozwolonych odpowiedzi i podejmuj akcje
						{
							if ((pHttpGet=RecvFromEsp(",CLOSED\r\n")))
							{
								RestartDMA();
							}
						}


					}
//					else if ((pHttpGet=RecvFromEsp(":GET /")))
//					{
//						GetSizeAndChannel(pHttpGet, &channel, &size);
//						lenHTTP = mini_strlen("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>ESP32 SSL</h1></body></html>");
//						int len = mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPSEND=%d,%d\r\n", channel, lenHTTP);
//						DbgMulti(DBG,"\r\nSEND_START: ",sendBuff," SEND STOP\r\n");
//						SendToEsp2(sendBuff, len );
//					}
					else if ((pHttpGet=RecvFromEsp("\r\nOK\r\n\r\n>")))
					{
						//SendToEsp2("0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789", 100);

						int len = mini_snprintf(sendBuff, sizeof(sendBuff), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>ESP32 SSL</h1></body></html>");
						DbgMulti(DBG,"\r\nSEND_START: ",sendBuff," SEND STOP\r\n");
						SendToEsp2(sendBuff, len);
					}
					else if ((pHttpGet=RecvFromEsp(",CLOSED\r\n")))
					{
						if ((pHttpGet=RecvFromEsp("\r\nOK\r\n")))
						{
							Dbg(DBG, " ---- ,CLOSED ---- ");
							RestartDMA();
						}
					}
					else if ((pHttpGet=RecvFromEsp("ERROR")))
					{
						Dbg(DBG, " ---- ERROR ---- ");
						RestartDMA();
					}
					else if ((pHttpGet=RecvFromEsp("\r\nRecv ")))		/* RecvFromEsp("\r\nRecv 88 bytes")   88-received bytes for ESP */
					{
						if ((pHttpGet=RecvFromEsp(" bytes\r\n")))
						{
							if ((pHttpGet=RecvFromEsp("\r\nSEND OK")))
							{
								Dbg(DBG, " ---- SEND OK ---- ");
								int len = mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPCLOSE=%d\r\n", channel);
								DbgMulti(DBG,"\r\nSEND_START: ",sendBuff," SEND STOP\r\n");
								SendToEsp2(sendBuff, len);
							}
						}
					}
//					else if ((pHttpGet=RecvFromEsp("\r\nSEND OK")))
//					{
//						Dbg(DBG, " ---- SEND OK ---- ");
//						int len = mini_snprintf(sendBuff, sizeof(sendBuff), "AT+CIPCLOSE=%d\r\n", channel);
//						DbgMulti(DBG,"\r\nSEND_START: ",sendBuff," SEND STOP\r\n");
//						SendToEsp2(sendBuff, len);
//					}
					else
					{
						RestartDMA();
					}
					break;

				case SMTP_CONNECTION:
					break;


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
				if ((pHttpGet=RecvFromEsp(":GET /")))
				{
					GetSizeAndChannel(pHttpGet, &channel, &size);

					while (GetDMACountByte()<size)
						vTaskDelay(1);

					if (RecvFromEsp(":GET /TME.txt")==0)
						DisplayRequestGET(pHttpGet, 2000);

					result=vSendDataHTTP(pHttpGet, channel);
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
	xTaskCreate(vtaskWifi, "vtaskWifi", 1000, NULL, (unsigned portBASE_TYPE ) 4, &vtaskWifiHandle);
}

void CloseWifiTask(void)
{
	vTaskDelete(vtaskWifiHandle);
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
	resetDMA=1;
}

void WIFI_RxCallbackService(void)
{
	RestartDMA();
	Dbg(DBG, "\r\n -----  USART6  HAL_UART_RxCpltCallback -------  ");
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
