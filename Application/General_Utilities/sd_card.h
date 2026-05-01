/*
 * sd_card.h
 *
 *  Created on: 03.04.2021
 *      Author: Elektronika RM
 */

#ifndef GENERAL_UTILITIES_SD_CARD_H_
#define GENERAL_UTILITIES_SD_CARD_H_

#include "stm32h7xx_hal.h"

void SDCard_Init(void);
int SDCardFileOpen(uint8_t id, char *fileName, uint8_t accessMode);
int SDCardFileRead(uint8_t id, char *buff, uint32_t size);
int SDCardFileWrite(uint8_t id, char *buff, uint32_t size);
int SDCardFileClose(uint8_t id);
int SDCardFilePosition(uint8_t id, uint32_t offset);
int SDCardOpenFileSize(uint8_t id);
int SDCardFileInfo(char *name, uint32_t *fileSize);
int SDCard_FileReadClose(uint8_t id, char *fileName, char *pReadBuff, int maxSize);

int BOOT_ReadCodeToNewBinFile(char *Buff);

#endif /* GENERAL_UTILITIES_SD_CARD_H_ */
