/**
  ******************************************************************************
  * @file    sdram_driver.h
  * @author  LUYIHAO
  * @brief   This file contains all the functions prototypes for the SDRAM
  *          module driver.
  ******************************************************************************
  * @attention
  *		STM32H723ZGT6����SDRAM  �ͺţ�W9825G6KH-6
  *		��SDRAM��ַ��0xC000 0000 �� 0xC200 0000	��32MB
  *		
  *		���δ����cubemx FMC io�ڳ�ʼ����ֱ��ʹ�ô���������������в���
  *		1��ʹ�ñ�����֮ǰ����stm32h7xx_hal_sdram.c�Լ�stm32h7xx_ll_fmc.c
  *		2����stm32h7xx_hal_conf.h�У����HAL_SDRAM_MODULE_ENABLED�ú�
  *  	
  *		����stm32 FMC��sdram���������Զ�ˢ������
  *		SDRAM refresh rate = 64ms / 8192(rows) = 7.81us
  *		count = 7.81us * 100Mhz  - 20 = 761
  *		�������õ�ʱ�����޸��·������ĵڶ�������
  *		HAL_SDRAM_ProgramRefreshRate(hsdram,761);
  *		
  *		ʹ��֮ǰ�����ⲿ�����ֵ��X�����޸�HAL_FMC_MspInit���������ڲ�������ʹ�侭��
  *				��X\PLL2M*PLL2N/PLL2R���ֵΪ200��
  *
  *		������FMC��Ӧ��io�ڣ�������Ҫ�����޸�
  *		PF0   ------> FMC_A0
  *		PF1   ------> FMC_A1
  *		PF2   ------> FMC_A2
  *		PF3   ------> FMC_A3
  *		PF4   ------> FMC_A4
  *		PF5   ------> FMC_A5
  *		PC0   ------> FMC_SDNWE
  *		PC2_C   ------> FMC_SDNE0
  *		PC3_C   ------> FMC_SDCKE0
  *		PF11   ------> FMC_SDNRAS
  *		PF12   ------> FMC_A6
  *		PF13   ------> FMC_A7
  *		PF14   ------> FMC_A8
  *		PF15   ------> FMC_A9
  *		PG0   ------> FMC_A10
  *		PG1   ------> FMC_A11
  *		PE7   ------> FMC_D4
  *		PE8   ------> FMC_D5
  *		PE9   ------> FMC_D6
  *		PE10   ------> FMC_D7
  *		PE11   ------> FMC_D8
  *		PE12   ------> FMC_D9
  *		PE13   ------> FMC_D10
  *		PE14   ------> FMC_D11
  *		PE15   ------> FMC_D12
  *		PD8   ------> FMC_D13
  *		PD9   ------> FMC_D14
  *		PD10   ------> FMC_D15
  *		PD14   ------> FMC_D0
  *		PD15   ------> FMC_D1
  *		PG2   ------> FMC_A12
  *		PG4   ------> FMC_BA0
  *		PG5   ------> FMC_BA1
  *		PG8   ------> FMC_SDCLK
  *		PD0   ------> FMC_D2
  *		PD1   ------> FMC_D3
  *		PG15   ------> FMC_SDNCAS
  *		PE0   ------> FMC_NBL0
  *		PE1   ------> FMC_NBL1  
  ******************************************************************************
*/

#ifndef SDRAM_DRIVER_H
#define SDRAM_DRIVER_H

#include "stm32h7xx_hal.h"

#define SDRAM_BANK1_ADDR     ((uint32_t)0xC0000000)

/**
  ************************************************************************************************************************
  *name��fmc_sdram_init
  *func��FMC����SDRAM��ʼ������
  *arg:NULL
  ************************************************************************************************************************
*/
void fmc_sdram_init(void);

#endif	/*SDRAM_DRIVER_H*/
