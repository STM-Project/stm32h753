/*
 * variables.c
 *
 *  Created on: 09.09.2020
 *      Author: Elektronika RM
 */

#include "stm32h7xx_hal.h"
#include <string.h>
#include "variables.h"
#include "common.h"
#include "mini_printf.h"
#include "string_oper.h"

SDRAM static s_wartosci Var;
Zmienne_zapisywalne  Const;

Zmienne_zapisywalne* VAR_GetMainPtr(void){ return &Const; }
Zmienne_zapisywalne  VAR_GetMain	  (void){ return Const; }

void VAR_SetVal(int nameVar, int val)
{
	switch(nameVar)
	{
//	case Var_slider0:   	Var.value[49990]=val;     break;
//	case Var_slider1:   	Var.value[49991]=val;     break;
//	case Var_slider2:   	Var.value[49992]=val;     break;
//	case Var_gsmStrenght:  	Var.GsmStrenght=val;      break;
	case Const_Wifi_ip:		Const.s_Wifi[0].ip=val;   break;
	case Const_Wifi_mask:	Const.s_Wifi[0].mask=val;   break;
	case Const_Wifi_gate:	Const.s_Wifi[0].gate=val;   break;
	case Const_Wifi_port:	Const.s_Wifi[0].port=val;   break;
	case Const_Wifi_dhcp:	Const.s_Wifi[0].dhcp=val;   break;
	case Const_Wifi_sta:		Const.s_Wifi[0].sta=val;   break;
	case Const_Wifi_ap:		Const.s_Wifi[0].ap=val;   break;
//	case Const_Checkbox_test_1:		Const.checkboxVal[0]=val;   break;
//	case Const_Checkbox_test_2:		Const.checkboxVal[1]=val;   break;
//	case Const_Checkbox_test_3:		Const.checkboxVal[2]=val;   break;
//	case Const_Checkbox_test_4:		Const.checkboxVal[3]=val;   break;
//	case Const_Checkbox_test_5:		Const.checkboxVal[4]=val;   break;
	}
}

void VAR_DecrVal(int nameVar, int decrSize)
{
	VAR_SetVal(nameVar, VAR_GetVal(nameVar)-decrSize );
}
void VAR_IncrVal(int nameVar, int incrSize)
{
	VAR_SetVal(nameVar, VAR_GetVal(nameVar)+incrSize );
}

int VAR_GetVal(int nameVar)
{
	switch(nameVar)
	{
	case Var_slider0:      return Var.value[49990];
	case Var_slider1:      return Var.value[49991];
	case Var_slider2:      return Var.value[49992];
	case Var_gsmStrenght:  return Var.GsmStrenght;
	case Const_Wifi_ip:    return Const.s_Wifi[0].ip;
	case Const_Wifi_mask:    return Const.s_Wifi[0].mask;
	case Const_Wifi_gate:    return Const.s_Wifi[0].gate;
	case Const_Wifi_port:    return Const.s_Wifi[0].port;
	case Const_Wifi_dhcp:    return Const.s_Wifi[0].dhcp;
	case Const_Wifi_sta:     return Const.s_Wifi[0].sta;
	case Const_Wifi_ap:     return Const.s_Wifi[0].ap;
//	case Const_Checkbox_test_1:		return Const.checkboxVal[0];
//	case Const_Checkbox_test_2:		return Const.checkboxVal[1];
//	case Const_Checkbox_test_3:		return Const.checkboxVal[2];
//	case Const_Checkbox_test_4:		return Const.checkboxVal[3];
//	case Const_Checkbox_test_5:		return Const.checkboxVal[4];
	default:
		return 0;
	}
}

void VAR_SetTabVal(int nameVar, int nrTab, int val)
{
	switch(nameVar)
	{
	case Const_emailSend_IP:
		Const.emailSend[nrTab].IP=val;
		break;
	case Const_emailSend_port:
		Const.emailSend[nrTab].port=val;
		break;
	case Const_emailSend_useSSL:
		Const.emailSend[nrTab].useSSL=val;
		break;
	case Const_dns_enable:
		Const.dns.enable=val;
		break;
	case Const_dns_IP1:
		Const.dns.IP1=val;
		break;
	case Const_dns_IP2:
		Const.dns.IP2=val;
		break;
	case Const_dns_IP3:
		Const.dns.IP3=val;
		break;
	case Const_sntp_timezone:
		Const.sntp.timezone=val;
		break;
	case Const_sntp_time:
		Const.sntp.time=val;
		break;
	case Const_wifiAP_ip:
		Const.wifiAP[nrTab].ip=val;
		break;
	case Const_wifiAP_mask:
		Const.wifiAP[nrTab].mask=val;
		break;
	case Const_wifiAP_gate:
		Const.wifiAP[nrTab].gate=val;
		break;
	case Const_wifiAP_port:
		Const.wifiAP[nrTab].port=val;
		break;
	case Const_wifiAP_dhcp:
		Const.wifiAP[nrTab].dhcp=val;
		break;
	case Const_wifiSTA_ip:
		Const.wifiSTA[nrTab].ip=val;
		break;
	case Const_wifiSTA_mask:
		Const.wifiSTA[nrTab].mask=val;
		break;
	case Const_wifiSTA_gate:
		Const.wifiSTA[nrTab].gate=val;
		break;
	case Const_wifiSTA_port:
		Const.wifiSTA[nrTab].port=val;
		break;
	case Const_wifiSTA_dhcp:
		Const.wifiSTA[nrTab].dhcp=val;
		break;
	case Const_wifiGeneral_nrAP:
		Const.wifiGeneral.nrAP=val;
		break;
	case Const_wifiGeneral_nrSTA:
		Const.wifiGeneral.nrSTA=val;
		break;
	case Const_wifiGeneral_mode:
		Const.wifiGeneral.mode=val;
		break;
	}
}

int VAR_GetTabVal(int nameVar,int nrTab)
{
	switch(nameVar)
	{
	case Const_emailSend_IP:
		return Const.emailSend[nrTab].IP;
	case Const_emailSend_port:
		return Const.emailSend[nrTab].port;
	case Const_emailSend_useSSL:
		return Const.emailSend[nrTab].useSSL;
	case Const_dns_enable:
		return Const.dns.enable;
	case Const_dns_IP1:
		return Const.dns.IP1;
	case Const_dns_IP2:
		return Const.dns.IP2;
	case Const_dns_IP3:
		return Const.dns.IP3;
	case Const_sntp_timezone:
		return Const.sntp.timezone;
	case Const_sntp_time:
		return Const.sntp.time;
	case Const_wifiAP_ip:
		return Const.wifiAP[nrTab].ip;
	case Const_wifiAP_mask:
		return Const.wifiAP[nrTab].mask;
	case Const_wifiAP_gate:
		return Const.wifiAP[nrTab].gate;
	case Const_wifiAP_port:
		return Const.wifiAP[nrTab].port;
	case Const_wifiAP_dhcp:
		return Const.wifiAP[nrTab].dhcp;
	case Const_wifiSTA_ip:
		return Const.wifiSTA[nrTab].ip;
	case Const_wifiSTA_mask:
		return Const.wifiSTA[nrTab].mask;
	case Const_wifiSTA_gate:
		return Const.wifiSTA[nrTab].gate;
	case Const_wifiSTA_port:
		return Const.wifiSTA[nrTab].port;
	case Const_wifiSTA_dhcp:
		return Const.wifiSTA[nrTab].dhcp;
	case Const_wifiGeneral_nrAP:
		return Const.wifiGeneral.nrAP;
	case Const_wifiGeneral_nrSTA:
		return Const.wifiGeneral.nrSTA;
	case Const_wifiGeneral_mode:
		return Const.wifiGeneral.mode;
	default:
		return 0;
	}
}

void VAR_SetStr(int nameVar, int nrTab, char *str)
{
	int varMaxLen;
	int strLen=mini_strlen(str);
	switch(nameVar)
	{
	case Const_STA_ssid:	str[SIZE_TXT-1]=0; strcpy(Const.s_Wifi[0].STA_ssid,str);	break;
	case Const_STA_pass:	str[SIZE_TXT-1]=0; strcpy(Const.s_Wifi[0].STA_pass,str);	break;
	case Const_AP_ssid:		str[SIZE_TXT-1]=0; strcpy(Const.s_Wifi[0].AP_ssid,str);     break;
	case Const_AP_pass:		str[SIZE_TXT-1]=0; strcpy(Const.s_Wifi[0].AP_pass,str);		break;
	case Const_emailSend_server:
		varMaxLen=sizeof(Const.emailSend[nrTab].server);
		StrBuffCopylimit(Const.emailSend[nrTab].server,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_emailSend_name:
		varMaxLen=sizeof(Const.emailSend[nrTab].name);
		StrBuffCopylimit(Const.emailSend[nrTab].name,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_emailSend_login:
		varMaxLen=sizeof(Const.emailSend[nrTab].login);
		StrBuffCopylimit(Const.emailSend[nrTab].login,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_emailSend_password:
		varMaxLen=sizeof(Const.emailSend[nrTab].password);
		StrBuffCopylimit(Const.emailSend[nrTab].password,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_emailRecv_email:
		varMaxLen=sizeof(Const.emailRecv[nrTab].email);
		StrBuffCopylimit(Const.emailRecv[nrTab].email,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_sntp_nameServer1:
		varMaxLen=sizeof(Const.sntp.nameServer1);
		StrBuffCopylimit(Const.sntp.nameServer1,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_sntp_nameServer2:
		varMaxLen=sizeof(Const.sntp.nameServer2);
		StrBuffCopylimit(Const.sntp.nameServer2,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_wifiAP_name:
		varMaxLen=sizeof(Const.wifiAP[nrTab].name);
		StrBuffCopylimit(Const.wifiAP[nrTab].name,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_wifiAP_pass:
		varMaxLen=sizeof(Const.wifiAP[nrTab].pass);
		StrBuffCopylimit(Const.wifiAP[nrTab].pass,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_wifiSTA_name:
		varMaxLen=sizeof(Const.wifiSTA[nrTab].name);
		StrBuffCopylimit(Const.wifiSTA[nrTab].name,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	case Const_wifiSTA_pass:
		varMaxLen=sizeof(Const.wifiSTA[nrTab].pass);
		StrBuffCopylimit(Const.wifiSTA[nrTab].pass,str, strLen>=varMaxLen-1?varMaxLen-1:strLen);
		break;
	}
}

char* VAR_GetStr(int nameVar, int nrTab)
{
	switch(nameVar)
	{
	case Const_STA_ssid:	return Const.s_Wifi[0].STA_ssid;
	case Const_STA_pass:	return Const.s_Wifi[0].STA_pass;
	case Const_AP_ssid:		return Const.s_Wifi[0].AP_ssid;
	case Const_AP_pass:		return Const.s_Wifi[0].AP_pass;
	case Const_emailSend_server:
		return Const.emailSend[nrTab].server;
	case Const_emailSend_name:
		return Const.emailSend[nrTab].name;
	case Const_emailSend_login:
		return Const.emailSend[nrTab].login;
	case Const_emailSend_password:
		return Const.emailSend[nrTab].password;
	case Const_emailRecv_email:
		return Const.emailRecv[nrTab].email;
	case Const_sntp_nameServer1:
		return Const.sntp.nameServer1;
	case Const_sntp_nameServer2:
		return Const.sntp.nameServer2;
	case Const_wifiAP_name:
		return Const.wifiAP[nrTab].name;
	case Const_wifiAP_pass:
		return Const.wifiAP[nrTab].pass;
	case Const_wifiSTA_name:
		return Const.wifiSTA[nrTab].name;
	case Const_wifiSTA_pass:
		return Const.wifiSTA[nrTab].pass;
	default:
		return 0;
	}
}

void VAR_SetVal64(int nameVar, int nrTab, uint64_t val)
{
	switch(nameVar)
	{
	case Const_Wifi_mac:
		Const.s_Wifi[0].mac=val;
		break;
	case Const_wifiAP_mac:
		Const.wifiAP[nrTab].mac=val;
		break;
	case Const_wifiSTA_mac:
		Const.wifiSTA[nrTab].mac=val;
		break;
	}
}

uint64_t VAR_GetVal64(int nameVar, int nrTab)
{
	switch(nameVar)
	{
//	case Const_wifiAP_mac:
//		return Const.wifiAP[nrTab].mac;
//	case Const_wifiSTA_mac:
//		return Const.wifiSTA[nrTab].mac;
	default:
		return 0;
	}
}
