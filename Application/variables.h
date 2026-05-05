/*
 * variables.h
 *
 *  Created on: 09.09.2020
 *      Author: Elektronika RM
 */

#ifndef VARIABLES_H_
#define VARIABLES_H_

#include "stm32h7xx_hal.h"
#include "esp32wroom.h"
#include "sntp_dns.h"
#include "_smtp.h"

#define SIZE_TXT   	160
#define NO_TAB			0

#define _Size__s_Wifi     			1
#define MAX_EMAIL_SENDERS			3
#define MAX_EMAIL_RECIPIENTS		8
#define SMTP_BUFFER_SIZE 			300000

enum{
	Var_slider0,
	Var_slider1,
	Var_slider2,
	Var_gsmStrenght,
	Const_STA_ssid,
	Const_STA_pass,
	Const_AP_ssid,
	Const_AP_pass,
	Const_Wifi_mac,
	Const_Wifi_ip,
	Const_Wifi_mask,
	Const_Wifi_gate,
	Const_Wifi_port,
	Const_Wifi_dhcp,
	Const_Wifi_sta,
	Const_Wifi_ap,
	Const_emailSend_server,
	Const_emailSend_IP,
	Const_emailSend_port,
	Const_emailSend_name,
	Const_emailSend_login,
	Const_emailSend_password,
	Const_emailSend_useSSL,
	Const_emailRecv_email,
	Const_dns_enable,
	Const_dns_IP1,
	Const_dns_IP2,
	Const_dns_IP3,
	Const_sntp_nameServer1,
	Const_sntp_nameServer2,
	Const_sntp_timezone,
	Const_sntp_time,
	Const_wifiAP_mac,
	Const_wifiAP_ip,
	Const_wifiAP_mask,
	Const_wifiAP_gate,
	Const_wifiAP_port,
	Const_wifiAP_dhcp,
	Const_wifiAP_name,
	Const_wifiAP_pass,
	Const_wifiSTA_mac,
	Const_wifiSTA_ip,
	Const_wifiSTA_mask,
	Const_wifiSTA_gate,
	Const_wifiSTA_port,
	Const_wifiSTA_dhcp,
	Const_wifiSTA_name,
	Const_wifiSTA_pass,
	Const_wifiGeneral_nrAP,
	Const_wifiGeneral_nrSTA,
	Const_wifiGeneral_mode,
	Const_Checkbox_test_1,
	Const_Checkbox_test_2,
	Const_Checkbox_test_3,
	Const_Checkbox_test_4,
	Const_Checkbox_test_5
};

//-------------- Zmienne NIEzapisywalne -------------------------
typedef struct
{
   int value[50000];
   int GsmStrenght;
}  s_wartosci;

//-------------- Zmienne zapisywalne -------------------------
typedef struct  __attribute__ ((packed))
{

	 s_net_wifi		s_Wifi[_Size__s_Wifi];
	 s_smtp_sender	emailSend[MAX_EMAIL_SENDERS];
	 s_smtp_recipient	emailRecv[MAX_EMAIL_RECIPIENTS];
	 s_dns dns;
	 s_sntp sntp;
	 s_wifi_AP		wifiAP[WIFI_AP_MAX];
	 s_wifi_STA		wifiSTA[WIFI_STA_MAX];
	 s_wifi_select wifiGeneral;
	 int selectAP;
	 int selectSTA;
	 int checkboxVal[10]; //do usuniecia!!!

}Zmienne_zapisywalne;

extern Zmienne_zapisywalne  Const;

Zmienne_zapisywalne* VAR_GetMainPtr(void);
Zmienne_zapisywalne  VAR_GetMain	  (void);
void VAR_SetVal(int nameVar, int val);
void VAR_IncrVal(int nameVar, int incrSize);
void VAR_DecrVal(int nameVar, int decrSize);
int VAR_GetVal(int nameVar);
void VAR_SetTabVal(int nameVar, int nrTab, int val);
int VAR_GetTabVal(int nameVar,int nrTab);
void VAR_SetStr(int nameVar, int nrTab, char *str);
char* VAR_GetStr(int nameVar, int nrTab);
void VAR_SetVal64(int nameVar, int nrTab, uint64_t val);
uint64_t VAR_GetVal64(int nameVar, int nrTab);

#endif /* VARIABLES_H_ */
