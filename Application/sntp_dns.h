/*
 * sntp_dns.h
 *
 *  Created on: 17.01.2021
 *      Author: Elektronika RM
 */

#ifndef SNTP_DNS_SNTP_DNS_H_
#define SNTP_DNS_SNTP_DNS_H_

typedef struct __attribute__ ((packed))
{
	uint8_t enable;
	uint32_t IP1;
	uint32_t IP2;
	uint32_t IP3;

}s_dns;

typedef struct __attribute__ ((packed))
{
	char nameServer1[64];
	char nameServer2[64];
	int timezone;
	int time;

}s_sntp;

extern struct tm *sntpTime;

void DefaultSettingsDNS(void);
void DefaultSettingsSNTP(void);

#endif /* SNTP_DNS_SNTP_DNS_H_ */
