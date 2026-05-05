/*
 * smtp.h
 *
 *  Created on: 04.01.2021
 *      Author: Elektronika RM
 */

#ifndef SMTP_SMTP_H_
#define SMTP_SMTP_H_

#include "stm32h7xx_hal.h"
//#include "sdram.h"
#include "common.h"

typedef enum
{
	SMTP_START=1,
	SMTP_INPROGRESS,
	SMTP_SUCCESS,
	SMTP_FAIL
}SMTP_STATE;

typedef enum
{
	EMAIL_TEST,
	EMAIL_MEASURE
}ID_EMAIL;

typedef struct
{
	uint8_t state;
	int code;
}email_status;

typedef struct
{
	uint8_t start;
	uint8_t whichSender;
	uint32_t recepientsMask;
	uint16_t id;
}Email_Send_Param;

typedef struct __attribute__ ((packed))
{
	char server[96];
	uint32_t IP;
	uint16_t port;
	char name[32];
	char login[96];
	char password[96];
	uint8_t useSSL;
}s_smtp_sender;

typedef struct __attribute__ ((packed))
{
	char email[96];
}s_smtp_recipient;

extern Email_Send_Param EmailSendParam;

DATA_TO_SEND* GetDataEmail(int id);
void DefaultSettingsEmail(void);
uint8_t GetEmailState(void);
char *GetStrEmailState(void);
int GetEmailCode(void);
void SetEmailState(uint8_t state);
void SetEmailCode(int code);
void base64_enc( char *in_buf, char *out_buf, int in_len);
char *base64_enc2( char *in_buf);
void base64_dec( char *in_buf, char *out_buf, int in_len, int *out_len);
void SendEmail(uint8_t whichSender, uint32_t recepientsMask, uint16_t id);
char *GetTitleEmail(void);

#endif /* SMTP_SMTP_H_ */
