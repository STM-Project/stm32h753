/*
 * smtp.c
 *
 *  Created on: 04.01.2021
 *      Author: Elektronika RM
 */

#include "stm32h7xx_hal.h"
#include <string.h>
#include "_smtp.h"
#include "variables.h"
#include "mini_printf.h"
//#include "def.h"
#include "SD_Card.h"
#include "common.h"
#include "FreeRTOS.h"

#define MAX_SIZE_TEMPBUFF	100

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

static email_status emailStatus;
SDRAM char dataBufferSMTP[SMTP_BUFFER_SIZE];
static char tempBuff[MAX_SIZE_TEMPBUFF];
Email_Send_Param EmailSendParam;

uint8_t GetEmailState(void)
{
	return emailStatus.state;
}
char *GetStrEmailState(void)
{
	switch (emailStatus.state)
	{
	case SMTP_START:
		strcpy(tempBuff, "SMTP_START");
		break;
	case SMTP_INPROGRESS:
		strcpy(tempBuff, "SMTP_INPROGRESS");
		break;
	case SMTP_SUCCESS:
		strcpy(tempBuff, "SMTP_SUCCESS");
		break;
	case SMTP_FAIL:
		strcpy(tempBuff, "SMTP_FAIL");
		break;
	default:
		strcpy(tempBuff, "--------");
		break;
	}
	return tempBuff;
}

int GetEmailCode(void)
{
	return emailStatus.code;
}
void SetEmailState(uint8_t state)
{
	emailStatus.state=state;
}
void SetEmailCode(int code)
{
	emailStatus.code=code;
}

static void encodeblock(unsigned char in[3], unsigned char out[4], int len)
{
	out[0]=cb64[in[0]>>2];
	out[1]=cb64[((in[0]&0x03)<<4)|((in[1]&0xf0)>>4)];
	out[2]=(unsigned char) (len>1 ? cb64[((in[1]&0x0f)<<2)|((in[2]&0xc0)>>6)] : '=');
	out[3]=(unsigned char) (len>2 ? cb64[in[2]&0x3f] : '=');
}

void base64_enc(char *in_buf, char *out_buf, int in_len)
{
	unsigned char in[3], out[4];
	int i, bb;

	while (in_len)
	{
		bb=0;
		for (i=0; i<3; i++)
		{
			if (in_len>0)
			{
				in[i]=(unsigned char) *in_buf++;
				bb++;
				in_len--;
			}
			else
				in[i]=0;
		}
		if (bb)
		{
			encodeblock(in, out, bb);
			for (i=0; i<4; i++)
				*out_buf++=out[i];
		}
	}
	*out_buf++=0;
}

char *base64_enc2(char *in_buf)
{
	unsigned char in[3], out[4];
	char *ptr=&tempBuff[0];
	int i, bb;
	int in_len=mini_strlen(in_buf);
	if (in_len>(3*MAX_SIZE_TEMPBUFF)/4)
		return NULL;

	while (in_len)
	{
		bb=0;
		for (i=0; i<3; i++)
		{
			if (in_len>0)
			{
				in[i]=(unsigned char) *in_buf++;
				bb++;
				in_len--;
			}
			else
				in[i]=0;
		}

		if (bb)
		{
			encodeblock(in, out, bb);
			for (i=0; i<4; i++)
				*ptr++=out[i];
		}
	}
	*ptr++=0;
	return tempBuff;
}

static void decodeblock(unsigned char in[4], unsigned char out[3])
{
	out[0]=(unsigned char) (in[0]<<2|in[1]>>4);
	out[1]=(unsigned char) (in[1]<<4|in[2]>>2);
	out[2]=(unsigned char) (((in[2]<<6)&0xc0)|in[3]);
}

void base64_dec(char *in_buf, char *out_buf, int in_len, int *out_len)
{
	unsigned char in[4], out[3], v;
	int i, len, in_len_p, aa;
	in_len_p=in_len;

	if ((in_len%4)!=0)
		return;

	aa=0;
	if (in_buf[in_len-1]=='=')
		aa++;
	if (in_buf[in_len-2]=='=')
		aa++;

	while (in_len)
	{
		for (len=0, i=0; i<4&&in_len; i++)
		{
			v=0;
			while (in_len&&v==0)
			{
				v=*in_buf++;
				in_len--;
				v=(unsigned char) ((v<43||v>122) ? 0 : cd64[v-43]);
				if (v)
				{
					v=(unsigned char) ((v=='$') ? 0 : v-61);
				}
			}
			if (in_len>=0)
			{
				len++;
				if (v)
				{
					in[i]=(unsigned char) (v-1);
				}
			}
			else
			{
				in[i]=0;
			}
		}
		if (len)
		{
			decodeblock(in, out);

			for (i=0; i<len-1; i++)
			{
				*out_buf++=out[i];
			}
		}
	}
	*out_buf++=0;
	*out_len=(in_len_p/4)*3;
	*out_len=*out_len-aa;
}

void SendEmail(uint8_t whichSender, uint32_t recepientsMask, uint16_t id)
{
	if(EmailSendParam.start==0)
	{
		EmailSendParam.start=1;
		EmailSendParam.whichSender= whichSender>=MAX_EMAIL_SENDERS ? 0 : whichSender;
		EmailSendParam.recepientsMask=recepientsMask;
		EmailSendParam.id=id;
	}
}

char *GetTitleEmail(void)
{
	switch (EmailSendParam.id)
	{
	case EMAIL_TEST:
		strcpy(tempBuff, "EMAIL_TEST");
		break;
	case EMAIL_MEASURE:
		strcpy(tempBuff, "EMAIL_MEASURE");
		break;
	default:
		strcpy(tempBuff, "--------");
		break;
	}
	return tempBuff;
}

DATA_TO_SEND* GetDataEmail(int id)
{
	DATA_TO_SEND *temp = (DATA_TO_SEND*)pvPortMalloc(sizeof(DATA_TO_SEND));

	temp->pData=dataBufferSMTP;

	switch (id)
	{
	case EMAIL_TEST:
		temp->len = mini_snprintf(temp->pData, SMTP_BUFFER_SIZE, "<html><style>table{width: 20%%;} td{ border-bottom:1px solid #ddd; padding:7px; font-family:Arial; text-align:center; white-space:nowrap;}</style>");
		temp->len += mini_snprintf(temp->pData + temp->len, SMTP_BUFFER_SIZE, "<table><tr><td>MODEL</td><td>NAME</td></tr>");

	while(temp->len<250000)
	{
		temp->len += mini_snprintf(dataBufferSMTP + temp->len, SMTP_BUFFER_SIZE, "<tr><td>FIRMWARE</td><td>VERSION</td></tr>");
		temp->len += mini_snprintf(dataBufferSMTP + temp->len, SMTP_BUFFER_SIZE, "<tr><td>SN</td><td>123456789</td></tr>");
		temp->len += mini_snprintf(dataBufferSMTP + temp->len, SMTP_BUFFER_SIZE, "<tr><td>ID</td><td>Devices ID</td></tr>");
		temp->len += mini_snprintf(dataBufferSMTP + temp->len, SMTP_BUFFER_SIZE, "<tr><td>DESC</td><td>Opis plytki</td></tr>");

	}

	temp->len += mini_snprintf(dataBufferSMTP + temp->len, SMTP_BUFFER_SIZE, "<tr><td>FIRMWARE</td><td>VERSION</td></tr>");
	temp->len += mini_snprintf(dataBufferSMTP + temp->len, SMTP_BUFFER_SIZE, "<tr><td>SN</td><td>XXXXXXX</td></tr>");
	temp->len += mini_snprintf(dataBufferSMTP + temp->len, SMTP_BUFFER_SIZE, "<tr><td>ID</td><td>Devices XXXXX</td></tr>");
	temp->len += mini_snprintf(dataBufferSMTP + temp->len, SMTP_BUFFER_SIZE, "<tr><td>DESC</td><td>Rafal Markielowski</td></tr>");

		temp->len += mini_snprintf(temp->pData + temp->len, SMTP_BUFFER_SIZE, "</table></html>\r\n");
		temp->len += mini_snprintf(temp->pData + temp->len, SMTP_BUFFER_SIZE, "\r\n.\r\n");

		break;

	case EMAIL_MEASURE:
		break;
	}

	return temp;
}

void DefaultSettingsEmail(void)
{
	int i;
	for (i=0; i<MAX_EMAIL_SENDERS; ++i)
	{
		switch (i)  // poczta.interia i gmail nie dziala dla prostych urzadzen trzeba wykupic w google !!!!
		{
		case 0:
			VAR_SetTabVal(Const_emailSend_IP, i, LWIP_MAKEU32(173, 194, 222, 109)); //to nadpisujke z CIPDOMAIN !!!
			VAR_SetStr(Const_emailSend_server, i, "smtp.poczta.onet.pl");
			VAR_SetTabVal(Const_emailSend_port, i, 465);
			VAR_SetStr(Const_emailSend_name, i, "ElektronikaRM");
			VAR_SetStr(Const_emailSend_login, i, "marafal@op.pl");       //metronic@onet.eu     metronic123456789@gmail.com
			VAR_SetStr(Const_emailSend_password, i, "Krasnoludki1979");  //Metro1234            metronicAKP123456789!
			VAR_SetTabVal(Const_emailSend_useSSL, i, 1);
			break;
		case 1:
			VAR_SetTabVal(Const_emailSend_IP, i, LWIP_MAKEU32(173, 194, 222, 109));
			VAR_SetStr(Const_emailSend_server, i, "smtp.interia.pl");
			VAR_SetTabVal(Const_emailSend_port, i, 465);
			VAR_SetStr(Const_emailSend_name, i, "ElektronikaRM");
			VAR_SetStr(Const_emailSend_login, i, "marafal@interia.pl");
			VAR_SetStr(Const_emailSend_password, i, "niunia6666");
			VAR_SetTabVal(Const_emailSend_useSSL, i, 1);
			break;
		case 2:
			VAR_SetTabVal(Const_emailSend_IP, i, LWIP_MAKEU32(173, 194, 222, 109));
			VAR_SetStr(Const_emailSend_server, i, "smtp.wp.pl");   //smtp.gmail.com
			VAR_SetTabVal(Const_emailSend_port, i, 465);
			VAR_SetStr(Const_emailSend_name, i, "marafal6666");
			VAR_SetStr(Const_emailSend_login, i, "marafal6666@wp.pl");  //marafal6666@gmail.com
			VAR_SetStr(Const_emailSend_password, i, "Markielowski1979!");    //Markielowski1979
			VAR_SetTabVal(Const_emailSend_useSSL, i, 1);
			break;
		}
	}
	for(i=0;i<MAX_EMAIL_RECIPIENTS;++i)
	{
//		switch(i)
//		{
//		case 0:
		VAR_SetStr(Const_emailRecv_email, i, "marafal@interia.pl");
		//VAR_SetStr(Const_emailRecv_email, i, "marafal@interia.pl");
//			break;
//		}
	}

	VAR_SetStr(Const_emailRecv_email, 1, "marafal@interia.pl");
	VAR_SetStr(Const_emailRecv_email, 5, "marafal@op.pl");   //"rafal.markielowski@gmail.com"

}
