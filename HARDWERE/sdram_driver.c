#include "sdram_driver.h"
//突发长度BL
#define SDRAM_MODEREG_BURST_LENGTH_1 ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2 ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4 ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8 ((uint16_t)0x0004)			
// 连续还是间隔模式
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED ((uint16_t)0x0008)
// CL等待几个时钟周期
#define SDRAM_MODEREG_CAS_LATENCY_2 ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3 ((uint16_t)0x0030)
//正常模式
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD ((uint16_t)0x0000)
//设置写操作的模式，可以选择突发模式或者单次写入模式*
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE ((uint16_t)0x0200)

/* FMC initialization function */
static void SDRAM_MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

}

static void sdram_initial_timing_sequence(SDRAM_HandleTypeDef *hsdram)
{
	
    FMC_SDRAM_CommandTypeDef command;

    //时钟使能
    command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(hsdram,&command,0xFFFF);

    //延时至少200us
    HAL_Delay(1);

    //对所有的Banks预充电
    command.CommandMode = FMC_SDRAM_CMD_PALL;				
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 2;
    command.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(hsdram,&command,0xFFFF);

    /*自动刷新*/
    command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;	
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 8;							
    command.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(hsdram,&command,0xFFFF);

    //sdram的加载寄存器命令
    command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;			
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_4 |      
           SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |         
           SDRAM_MODEREG_CAS_LATENCY_3 |                 
           SDRAM_MODEREG_OPERATING_MODE_STANDARD |       
           SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;         
    HAL_SDRAM_SendCommand(hsdram,&command,0xFFFF);

    
    HAL_SDRAM_ProgramRefreshRate(hsdram,761);
}

void fmc_sdram_init(void)
{

	SDRAM_HandleTypeDef hsdram1;
	
	SDRAM_MX_GPIO_Init();  

	FMC_SDRAM_TimingTypeDef SdramTiming = {0};

	hsdram1.Instance = FMC_SDRAM_DEVICE;
	
	/* hsdram1.Init */
	hsdram1.Init.SDBank = FMC_SDRAM_BANK1;
	hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;
	hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;
	hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
	hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
	hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
	hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
	hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
	hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
	hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
	
	/* SdramTiming */
	SdramTiming.LoadToActiveDelay = 2;
	SdramTiming.ExitSelfRefreshDelay = 8;
	SdramTiming.SelfRefreshTime = 6;
	SdramTiming.RowCycleDelay = 6;
	SdramTiming.WriteRecoveryTime = 4;
	SdramTiming.RPDelay = 2;
	SdramTiming.RCDDelay = 2;

	HAL_SDRAM_Init(&hsdram1, &SdramTiming);	

	sdram_initial_timing_sequence(&hsdram1);
}

static uint32_t FMC_Initialized = 0;

static void HAL_FMC_MspInit(void)
{
 
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	if (FMC_Initialized) {
		return;
	}
	
	FMC_Initialized = 1;
	
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FMC;	
    PeriphClkInitStruct.PLL2.PLL2M = 6;
    PeriphClkInitStruct.PLL2.PLL2N = 25;
    PeriphClkInitStruct.PLL2.PLL2P = 2;
    PeriphClkInitStruct.PLL2.PLL2Q = 2;
    PeriphClkInitStruct.PLL2.PLL2R = 1;
    PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
    PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
    PeriphClkInitStruct.PLL2.PLL2FRACN = 0.0;
    PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

	__HAL_RCC_FMC_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
						  |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_11|GPIO_PIN_12
						  |GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

	/* GPIO_InitStruct */
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* GPIO_InitStruct */
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4
						  |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

	/* GPIO_InitStruct */
	GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
						  |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
						  |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	/* GPIO_InitStruct */
	GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_14
						  |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;

	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef* sdramHandle)
{
	
	HAL_FMC_MspInit();
}


