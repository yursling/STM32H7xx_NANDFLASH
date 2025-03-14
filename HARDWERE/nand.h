/**
  ****************************************************************************************
  * @file    nand.h
  * @author  yursling
  * @brief   Header file of NAND FLASH module driver
  ****************************************************************************************
  * @attention
  *
  * 	STM32H723ZGT6驱动NAND FLASH  型号：MT29F4G08ABADA	512MB空间 + 16MBspare区域
  *		此NAND FLASH地址从0x8000 0000开始至0xA100 0000
  * 	如果未配置cubemx FMC io口初始化，直接使用此驱动必须进行下列操作 
  * 	1、使用本驱动之前包含stm32h7xx_hal_nand.c以及stm32h7xx_ll_fmc.c 
  * 	2、在stm32h7xx_hal_conf.h中，添加#define HAL_NAND_MODULE_ENABLED该宏
  *		
  *		NAND FLASH使用的是HCK时钟。
  *	
  *		下列是对应IO口	有不同根据开发板修改引脚
  *  	PE7   ------> FMC_D4
  *  	PE8   ------> FMC_D5
  *  	PE9   ------> FMC_D6
  *  	PE10   ------> FMC_D7
  *  	PD11   ------> FMC_CLE
  *  	PD12   ------> FMC_ALE
  *  	PD14   ------> FMC_D0
  *  	PD15   ------> FMC_D1
  *     PC6   ------> FMC_NWAIT
  *     PD0   ------> FMC_D2
  *     PD1   ------> FMC_D3
  *     PD4   ------> FMC_NOE
  *     PD5   ------> FMC_NWE
  *     PG9   ------> FMC_NCE
  ****************************************************************************************
  */

#ifndef _NAND_H
#define _NAND_H

#include "stm32h7xx_hal.h"

typedef enum {
    NAND_MAIN_AREA,  
    NAND_SPARE_AREA  
} nand_area_t;

extern NAND_HandleTypeDef hnand1;

/**
  ***********************************
  *name：fmc_nand_init
  *func：FMC驱动NAND FLASH初始化函数
  *arg:NULL
  *return: 成功0，失败1
  ***********************************
*/
uint8_t fmc_nand_init(void);

/**
  *****************************************************************************************************************************************
  * 读取NAND Flash的指定页指定列的数据(main区和spare区都可以使用此函数)如果不知道spare区域的准确地址，只需要
  * 将area参数设置为NAND_SPARE_AREA，自动定位到当前地址页的spare区域
  * 将area参数设置为NAND_MAIN_AREA，则offset参数实际意义为文件地址，要读当时写入文件的第几个字节，就写多少
  * offset要读取的地址,范围:0~nand_dev.page_totalsize * nand_dev.block_pagenum * nand_dev.block_totalnum
  * *pBuffer:指向数据存储区
  *  NumByteToRead:读取字节数
  *  area: 读取区域 ，当该参数为NAND_SPARE_AREA时，NumByteToWrite不得超过nand_dev.page_sparesize 
  * 返回值:0,成功
  *    失败,NSTA_ERROR 1 (状态错误，读取超时)
  *			READSPARESIZEERROR 3	(读sqare区域错误，检查NumByteToWrite参数)
  *			NANDADDRERROR    5		(读取地址错误，不在0~nand_dev.page_totalsize * nand_dev.block_pagenum * nand_dev.block_totalnum范围内)
  *****************************************************************************************************************************************
**/
uint8_t nand_read_everywhere(uint32_t offset, uint8_t *pbuffer, uint32_t numbytetoread, nand_area_t area);

/**
  ***************************************************************************************************************************************
  * 在NAND一页中写入指定个字节的数据(main区和spare区都可以使用此函数)如果不知道spare区域的准确地址，只需要
  * 将area参数设置为NAND_SPARE_AREA，自动定位到当前地址页的spare区域
  * offset要写入的开始地址,范围:0~nand_dev.page_totalsize * nand_dev.block_pagenum * nand_dev.block_totalnum
  * pBbuffer:指向数据存储区
  * NumByteToWrite:要写入的字节数
  * area: 写区域 ，当该参数为NAND_SPARE_AREA时，NumByteToWrite不得超过nand_dev.page_sparesize
  * return:0,成功
  *	   失败,NSTA_ERROR 1			(状态错误，写入超时)
  *    		WRITESPARESIZEERROR 2	(写sqare区域错误，检查NumByteToWrite参数)
  *			NANDADDRERROR    5   	(写地址错误，不在0~nand_dev.page_totalsize * nand_dev.block_pagenum * nand_dev.block_totalnum范围内)
  *			WRITEADDRHAVEVAL 6		要写入的地址处已经有值了，更换地址写入
  ***************************************************************************************************************************************
**/
uint8_t nand_writ_everywhere(uint32_t offset, uint8_t *pbuffer, uint32_t numbytetowrite, nand_area_t area);

/**
  ************************************************************************
  * 以块为单位擦除NAND FLASH
  * arg: blocknum	要擦除的块 不能超过nand_dev.block_totalnum大小
  * return : 0，成功 
  *			失败：BLOCKNUMERROR 4 ，块数量错误
  *************************************************************************
**/
uint8_t nand_block_erase(uint16_t blocknum);

#endif
