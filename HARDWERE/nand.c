#include "nand.h"

// NAND属性结构体
typedef struct
{
    uint16_t page_totalsize;             //每页总大小，main区和spare区总和
    uint16_t page_mainsize;              //每页的main区大小
    uint16_t page_sparesize;             //每页的spare区大小
    uint8_t block_pagenum;               //每个块包含的页数量
    uint16_t plane_blocknum;             //每个plane包含的块数量
    uint16_t block_totalnum;             //总的块数量
    uint32_t id;                         // NAND FLASH ID

} nand_attriute;

enum nand_wr_error{
	WRITESPARESIZEERROR = 2,
	READSPARESIZEERROR,
	BLOCKNUMERROR,
	NANDADDRERROR,
	WRITEADDRHAVEVAL
};

#define NAND_ADDRESS 0X80000000 // nand flash的访问地址,接NCE3,地址为:0X8000 0000
#define NAND_CMD 1 << 16        //发送命令
#define NAND_ADDR 1 << 17       //发送地址

// NAND FLASH命令
#define NAND_READID 0X90  //读ID指令
#define NAND_FEATURE 0XEF //设置特性指令
#define NAND_RESET 0XFF   //复位NAND
#define NAND_READSTA 0X70 //读状态
#define NAND_AREA_A 0X00
#define NAND_AREA_TRUE1 0X30
#define NAND_WRITE0 0X80
#define NAND_WRITE_TURE1 0X10


// NAND FLASH状态
#define NSTA_READY 0X40    
#define NSTA_ERROR 0X01      
#define NSTA_TIMEOUT 0X02    


// NAND FLASH型号和对应的ID号
#define MT29F4G08ABADA 	0XDC909556  
#define MT29F16G08ABABA 0X48002689 

// NAND FLASH操作相关延时参数
#define NAND_TADL_DELAY 35      	// tADL等待延迟,最少70ns
#define NAND_TWHR_DELAY 30      	// tWHR等待延迟,最少60ns
#define NAND_TRHW_DELAY 50      	// tRHW等待延迟,最少100ns
#define NAND_TRST_FIRST_DELAY 3 	// tRST复位后的第一次等待时间，最大为3ms


static nand_attriute nand_dev;

NAND_HandleTypeDef hnand1;

static uint32_t FMC_Initialized = 0;

static void HAL_FMC_MspInit(void)
{

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if (FMC_Initialized) {
		return ;
	}
	FMC_Initialized = 1;
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FMC;
	PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_D1HCLK;
	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

	/* Peripheral clock enable */
	__HAL_RCC_FMC_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;

	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	/* GPIO_InitStruct */
	GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_14|GPIO_PIN_15
						  |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;

	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* GPIO_InitStruct */
	GPIO_InitStruct.Pin = GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF9_FMC;

	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* GPIO_InitStruct */
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;

	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}

static void fmc_gpio_init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
}

void HAL_NAND_MspInit(NAND_HandleTypeDef* nandHandle){

	HAL_FMC_MspInit();
}

// NAND延时
static void nand_delay(volatile uint32_t i)
{
	while (i > 0) {
		i--;
	}	
}

static uint8_t nand_readstatus(void)
{
	volatile uint8_t data = 0;
	*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_READSTA; 
	nand_delay(NAND_TWHR_DELAY);					 
	data = *(volatile uint8_t *)NAND_ADDRESS;					 
	return data;
}

static uint8_t nand_wait_for_ready(void)
{
	uint8_t status = 0;
	volatile uint32_t time = 0;
	while (1) {	
		status = nand_readstatus(); 
		if (status & NSTA_READY) {
			break;
		}
		time++;
		if (time >= 0X1FFFF) {
			return NSTA_TIMEOUT; 
		}	
	}
	
	return NSTA_READY;
}


static uint8_t nand_modeset(uint8_t mode)
{
	*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_FEATURE; 
	*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0X01;		  

	nand_delay(NAND_TADL_DELAY); //等待tADL

	*(volatile uint8_t *)NAND_ADDRESS = mode; 
	*(volatile uint8_t *)NAND_ADDRESS = 0;
	*(volatile uint8_t *)NAND_ADDRESS = 0;
	*(volatile uint8_t *)NAND_ADDRESS = 0;
	if (nand_wait_for_ready() == NSTA_READY) {
		return 0; 
	}	
	else {
		return 1; 
	}	
}

static uint32_t nand_read_id(void)
{
	uint8_t deviceid[5];
	uint32_t id;
	*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_READID; //发送读取ID命令
	*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0X00;

	nand_delay(NAND_TWHR_DELAY);

	// ID一共有5个字节
	deviceid[0] = *(volatile uint8_t *)NAND_ADDRESS;
	deviceid[1] = *(volatile uint8_t *)NAND_ADDRESS;
	deviceid[2] = *(volatile uint8_t *)NAND_ADDRESS;
	deviceid[3] = *(volatile uint8_t *)NAND_ADDRESS;
	deviceid[4] = *(volatile uint8_t *)NAND_ADDRESS;

	id = ((uint32_t)deviceid[1]) << 24 | ((uint32_t)deviceid[2]) << 16 | ((uint32_t)deviceid[3]) << 8 | deviceid[4];
	
	if (nand_wait_for_ready() == NSTA_READY) {
		return id; 
	}
	else {
		return 0xFFFFFFFF;
	}	
}


static uint8_t nand_reset(void)
{
	*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_RESET; //发送复位命令
													
	HAL_Delay(NAND_TRST_FIRST_DELAY);
	
	if (nand_wait_for_ready() == NSTA_READY) {
		return 0; 
	}	
	else {
		return 1; 
	}
}

static void nand_mpu_config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct = {0};

	/* Disables the MPU */
	HAL_MPU_Disable();

	/** Initializes and configures the Region and the memory to be protected
	*/
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress = 0x80000000;
	MPU_InitStruct.Number = MPU_REGION_NUMBER4;
	MPU_InitStruct.Size = MPU_REGION_SIZE_512MB;
	MPU_InitStruct.SubRegionDisable = 0x0;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	/* Enables the MPU */
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

uint8_t fmc_nand_init(void)
{
	
	nand_mpu_config();
	
	fmc_gpio_init();
	
	FMC_NAND_PCC_TimingTypeDef ComSpaceTiming = {0};
	FMC_NAND_PCC_TimingTypeDef AttSpaceTiming = {0};

	hnand1.Instance = FMC_NAND_DEVICE;
	/* hnand1.Init */
	hnand1.Init.NandBank = FMC_NAND_BANK3;
	hnand1.Init.Waitfeature = FMC_NAND_WAIT_FEATURE_ENABLE;
	hnand1.Init.MemoryDataWidth = FMC_NAND_MEM_BUS_WIDTH_8;
	hnand1.Init.EccComputation = FMC_NAND_ECC_DISABLE;
	hnand1.Init.ECCPageSize = FMC_NAND_ECC_PAGE_SIZE_2048BYTE;
	hnand1.Init.TCLRSetupTime = 0;
	hnand1.Init.TARSetupTime = 0;
	
	/* hnand1.Config */
	hnand1.Config.PageSize = 2048;
	hnand1.Config.SpareAreaSize = 64;
	hnand1.Config.BlockSize = 64;
	hnand1.Config.BlockNbr = 4096;
	hnand1.Config.PlaneNbr = 2;
	hnand1.Config.PlaneSize = 2048;
	hnand1.Config.ExtraCommandEnable = DISABLE;
	
	/* ComSpaceTiming */
	ComSpaceTiming.SetupTime = 4;
	ComSpaceTiming.WaitSetupTime = 4;
	ComSpaceTiming.HoldSetupTime = 5;
	ComSpaceTiming.HiZSetupTime = 4;
	
	/* AttSpaceTiming */
	AttSpaceTiming.SetupTime = 4;
	AttSpaceTiming.WaitSetupTime = 4;
	AttSpaceTiming.HoldSetupTime = 5;
	AttSpaceTiming.HiZSetupTime = 4;
	
	HAL_NAND_Init(&hnand1, &ComSpaceTiming, &AttSpaceTiming);
  
	nand_reset(); 

	nand_dev.id = nand_read_id(); 
	
	nand_modeset(4);					//设置为MODE4,高速模式
	
	if (nand_dev.id == MT29F16G08ABABA) {	
		nand_dev.page_totalsize = 4320;	
		nand_dev.page_mainsize = 4096;
		nand_dev.page_sparesize = 224;
		nand_dev.block_pagenum = 128;
		nand_dev.plane_blocknum = 2048;
		nand_dev.block_totalnum = 4096;
	}
	else if (nand_dev.id == MT29F4G08ABADA) {	 
		nand_dev.page_totalsize = 2112;
		nand_dev.page_mainsize = 2048;
		nand_dev.page_sparesize = 64;
		nand_dev.block_pagenum = 64;
		nand_dev.plane_blocknum = 2048;
		nand_dev.block_totalnum = 4096;
	}
	else {
		return 1; 
	}
	return 0;
}

uint8_t nand_read_everywhere(uint32_t offset, uint8_t *pbuffer, uint32_t numbytetoread, nand_area_t area)
{
	
	if(offset > nand_dev.page_totalsize * nand_dev.block_pagenum * nand_dev.block_totalnum) {
		return NANDADDRERROR;
	}
	
	uint32_t pagenum = 0;
	uint16_t colnum = 0;
	uint16_t numpage_toread = 0;
	uint32_t file_offset = 0;
		
    if (area == NAND_SPARE_AREA) {
		if(numbytetoread > nand_dev.page_sparesize) {
			return READSPARESIZEERROR;
		}
		 /* 计算页号和列地址 */
		pagenum = offset / nand_dev.page_totalsize;    
        colnum = nand_dev.page_mainsize;  // 列地址指向spare区域
		
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_AREA_A;
		//发送地址
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)colnum;
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(colnum >> 8);
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)pagenum;
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 8);
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 16);
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_AREA_TRUE1;
		
		nand_delay(NAND_TADL_DELAY);  // 等待tADL

		for (uint16_t i = 0; i < numbytetoread; i++) {
			*pbuffer++ = *(volatile uint8_t *)NAND_ADDRESS;
		}
    }
	else {
		
		file_offset = offset;			/*当读的是主区时，实际上的偏移地址是文件的偏移地址*/
		
		pagenum = file_offset / nand_dev.page_mainsize ;
		colnum = file_offset % nand_dev.page_mainsize;
		
		uint16_t readsize = nand_dev.page_mainsize - colnum;		/*第一次最多读该页剩余的字节*/
	
		if(readsize > numbytetoread) {
			readsize = numbytetoread;
		}
			
		if(numbytetoread > nand_dev.page_mainsize) {
			numpage_toread = numbytetoread / nand_dev.page_mainsize;
			/*读下一页剩余的内容*/
			if(numbytetoread % nand_dev.page_mainsize) {
				numpage_toread += 1;
			}
		}
		else {
			numpage_toread = 1;
		}
		
		while(numpage_toread > 0) {
			
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_AREA_A;
			//发送地址
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)colnum;
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(colnum >> 8);
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)pagenum;
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 8);
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 16);
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_AREA_TRUE1;

			nand_delay(NAND_TADL_DELAY);  // 等待tADL

			for (uint16_t i = 0; i < readsize; i++) {
				*pbuffer++ = *(volatile uint8_t *)NAND_ADDRESS;	
			}
			
			if (nand_wait_for_ready() != NSTA_READY) {
				return NSTA_ERROR;
			}
			
			pagenum += 1;
			colnum = 0;
			numbytetoread -= readsize;
			numpage_toread--;
			readsize = (numbytetoread > 1 ) ? nand_dev.page_mainsize : numbytetoread;	
		}
    }
	
    if (nand_wait_for_ready() != NSTA_READY) {
        return NSTA_ERROR;
    }

    return 0;  
}

uint8_t nand_writ_everywhere(uint32_t offset, uint8_t *pbuffer, uint32_t numbytetowrite, nand_area_t area)
{
	
	if(offset > nand_dev.page_totalsize * nand_dev.block_pagenum * nand_dev.block_totalnum) {
		return NANDADDRERROR;
	}
	
	uint8_t readbuf[256] = {0};
	
	uint16_t numpage_towrite = 0;

	/* 计算页号和列地址 */
	uint32_t pagenum = offset / nand_dev.page_mainsize;    
	uint16_t colnum = offset % nand_dev.page_mainsize;
	uint16_t writesize = nand_dev.page_mainsize - colnum;	/*第一次最多写该页剩余的字节*/
	
	if(writesize > numbytetowrite) {
		writesize = numbytetowrite;
	}
	
	if (area == NAND_SPARE_AREA) {
		if(numbytetowrite > nand_dev.page_sparesize) {
			return WRITESPARESIZEERROR;
		}
		colnum = nand_dev.page_mainsize;  // 列地址指向spare区域

		//发送命令
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_WRITE0;
		//发送地址
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)colnum;
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(colnum >> 8);
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)pagenum;
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 8);
		*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 16);
		__DSB();

		for (uint16_t i = 0; i < numbytetowrite; i++) {
			*(volatile uint8_t *)NAND_ADDRESS = *(volatile uint8_t *)pbuffer++;
		}

		*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_WRITE_TURE1;
		__DSB();
	}
	else {
		
		nand_read_everywhere(offset,readbuf,256,area);
		for (uint16_t i = 0; i < 256 ; i++) {
			if(readbuf[i] != 0xff) {
				return WRITEADDRHAVEVAL;
			}
		}
				
		if(numbytetowrite > nand_dev.page_mainsize) {
			numpage_towrite = numbytetowrite / nand_dev.page_mainsize;
			/*写下一页剩余的内容*/
			if(numbytetowrite % nand_dev.page_mainsize) {
				numpage_towrite += 1;
			}
		}
		else {
			numpage_towrite = 1;
		}
		
		while(numpage_towrite > 0) {
					
			//发送命令
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_WRITE0;
			//发送地址
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)colnum;
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(colnum >> 8);
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)pagenum;
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 8);
			*(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 16);
			__DSB();

			for (uint16_t i = 0; i < writesize; i++) {
				*(volatile uint8_t *)NAND_ADDRESS = *(volatile uint8_t *)pbuffer++;
			}

			*(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_WRITE_TURE1;
			__DSB();	
			
			pagenum += 1;
			colnum = 0;
			numbytetowrite -= writesize;
			numpage_towrite--;
			writesize = (numpage_towrite > 1) ? nand_dev.page_mainsize : numbytetowrite;

		}		
	}

	if (nand_wait_for_ready() != NSTA_READY) {
		return NSTA_ERROR; 
	}
	
	return 0;
}

uint8_t nand_block_erase(uint16_t blocknum)
{
	if(blocknum >= nand_dev.block_totalnum) {
		return BLOCKNUMERROR;
	}
	NAND_AddressTypeDef address;
	address.Block = blocknum;
	address.Page = 0;
	address.Plane = 0;

	HAL_NAND_Erase_Block(&hnand1, &address);

	return 0;
}
