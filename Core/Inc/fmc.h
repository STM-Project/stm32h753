/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : FMC.h
  * Description        : This file provides code for the configuration
  *                      of the FMC peripheral.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FMC_H
#define __FMC_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

	/* USER CODE END Includes */

extern SDRAM_HandleTypeDef hsdram1;

/* USER CODE BEGIN Private defines */
#define SDRAM_DEVICE_ADDR ( (uint32_t) 0xC0000000U )
#define SDRAM_MEMORY_ADDR ( (uint32_t) 0x60000000U ) /* Remaped SDRAM address */
#define SDRAM_DEVICE_SIZE ( (uint32_t) 0x01000000 )	 /* 16MB ale dla: 	#define SDRAM_DEVICE_SIZE ((uint32_t)0x03FFFFFF) mamy 64MB */
	/* USER CODE END Private defines */

void MX_FMC_Init(void);
void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef* hsdram);
void HAL_SDRAM_MspDeInit(SDRAM_HandleTypeDef* hsdram);

/* USER CODE BEGIN Prototypes */

	/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__FMC_H */

/**
  * @}
  */

/**
  * @}
  */
