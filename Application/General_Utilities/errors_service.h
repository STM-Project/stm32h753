/*
 * errors.h
 *
 *  Created on: 03.04.2021
 *      Author: Elektronika RM
 */

#ifndef GENERAL_UTILITIES_ERRORS_SERVICE_H_
#define GENERAL_UTILITIES_ERRORS_SERVICE_H_

void ERROR_SDcardMount(int code);
void ERROR_SDcardOpen(int code);
void ERROR_SDcardRead(int code);
void ERROR_SDcardWrite(int code);
void ERROR_SDcardClose(int code);
void ERROR_SDcardLseek(int code);
void ERROR_SDcardInfo(int code);

void ERROR_NMIHandler(void);
void ERROR_HardFaulHandler(void);
void ERROR_MemManageHandler(void);
void ERROR_BusFaultHandler(void);
void ERROR_UsageFaultHandler(void);
void ERROR_DebugMonHandler(void);

void ERROR_Sdram(void);
void ERROR_StrChangeColor(void);

#endif /* GENERAL_UTILITIES_ERRORS_SERVICE_H_ */
