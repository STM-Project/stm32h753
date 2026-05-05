/*
 * sntp_dns.c
 *
 *  Created on: 17.01.2021
 *      Author: Elektronika RM
 */

#include "stm32h7xx_hal.h"
#include <string.h>
#include <time.h>
#include "sntp_dns.h"
#include "variables.h"
//#include "def.h"

struct tm *sntpTime;

void DefaultSettingsDNS(void)
{
	VAR_SetTabVal(Const_dns_enable, NO_TAB, 1);
	VAR_SetTabVal(Const_dns_IP1, NO_TAB, LWIP_MAKEU32(8,8,8,8));
	VAR_SetTabVal(Const_dns_IP2, NO_TAB, LWIP_MAKEU32(8,8,8,8));
	VAR_SetTabVal(Const_dns_IP3, NO_TAB, LWIP_MAKEU32(8,8,8,8));
}

void DefaultSettingsSNTP(void)
{
	VAR_SetStr(Const_sntp_nameServer1, NO_TAB, "vega.cbk.poznan.pl");
	VAR_SetStr(Const_sntp_nameServer2, NO_TAB, "ntp1.tp.pl");
	VAR_SetTabVal(Const_sntp_timezone, NO_TAB, 1);

	sntpTime->tm_year=20+100;
	sntpTime->tm_mon=6-1;
	sntpTime->tm_mday=15;
	sntpTime->tm_hour=23;
	sntpTime->tm_min=59;
	sntpTime->tm_sec=34;
	VAR_SetTabVal(Const_sntp_time, NO_TAB, mktime(sntpTime));
}
