#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"


#define ADC_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE + 32)

#define  ADC3_DR_Address   ((u32)0x40012400+0x4c)
__IO  uint16_t   ADC_ConvertedValue;


 void ADC3_GPIO_Config(void) {  
  GPIO_InitTypeDef  GPIO_InitStructure;  
	 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;   
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; 
  GPIO_Init(GPIOF, &GPIO_InitStructure); 
} 

 void ADC3_Mode_Config(void) {  
  DMA_InitTypeDef DMA_InitStructure; 
  ADC_InitTypeDef ADC_InitStructure;  
	 
  /* 将与DMA有关的寄存器设为初始值*/ 
  DMA_DeInit(DMA1_Channel1);
	 
  /*定义DMA外设基地址, 这里的ADC1_DR_Address 是用户自己定义的，即为存放转换结果的寄存器，
	 他的作用就是告诉DMA取数就到ADC1_DR_Address 这里来取。*/  
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC3_DR_Address; 
	 
  /*定义内存基地址，即告诉DMA要将从AD中取来的数放到ADC_ConvertedValue中 */  
  DMA_InitStructure.DMA_MemoryBaseAddr =(u32)&ADC_ConvertedValue; 
	 
  /*单向传输，定义AD外设作为数据传输的来源，即告诉DMA是将AD中的数据取出放到内存中，不能反过来*/  
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; // DMA_DIR_PeripheralDST 双向传输
	 
  /*指定DMA通道的DMA缓存的大小,即告诉DMA开辟几个内存空间，由于我们只取通道10的AD数据所以只需开辟一个内存空间*/ 
  DMA_InitStructure.DMA_BufferSize = 1;  
	 
  /*设定寄存器地址固定，即告诉DMA，只从固定的一个地方取数*/ 
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //设置DMA外设递增模式，如果使用的通道有多个外设连接，使能
	
  /*设定内存地址固定，即每次DMA，，只将数搬到固定的内存中*/ 
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //设置DMA内存递增模式，DMA访问多个内存参数时，使能
	
  /*设定外设数据宽度，即告诉DMA要取的数的大小*/ 
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //设置DMA在访问外设时，每次操作的数据长度，3种类型（Byte, HalfWord, Word）
	
	 /*设定内存的的宽度*/  
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; //
	
  /*设定DMA工作再循环缓存模式，即告诉DMA要不停的搬运，不能偷懒*/ 
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; //访问一次就不要访问，DMA_Mode_Normal
	
  /*设定DMA选定的通道软件优先级*/  
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  
  /*设置DMA中的2个内存互相访问*/	
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 
	
	/*对DMA模块初始化*/
  DMA_Init(DMA1_Channel1, &DMA_InitStructure); 
	
  /* Enable DMA channel1，CPU有好几个DMA秘书，现在只用DMA1_Channel1这个秘书*/  
  DMA_Cmd(DMA1_Channel1, ENABLE); 
	
	/*以上是搭配好DMA环境*/
	
  /*设置ADC工作在独立模式*/  
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; //在这个模式下，双ADC不能同步，每个ADC接口独立工作
	
  /*规定AD转换工作在单次模式，即对一个通道采样*/ 
  ADC_InitStructure.ADC_ScanConvMode = DISABLE ; //如果使用多个通道，要ENABLE
	
  /*设定AD转化在连续模式*/  
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;  //连续转换直到所有的数据转换完成后才停止转换，而单次转换则只转换一次数据就停止，要再次触发转换才可以
	
  /*不使用外部促发转换*/   	/*选择外部触发模式*/
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;  //选择软件触发，
	
	/*采集的数据在寄存器中以右对齐的方式存放*/  
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; 
	
	/*设定要转换的AD通道数目*/  
	ADC_InitStructure.ADC_NbrOfChannel = 1; 
	/*初始化ADC3*/
	ADC_Init(ADC3, &ADC_InitStructure);  
	
	/*配置ADC时钟，为PCLK2的8分频，即9MHz*/ 
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);  
	
	/*配置ADC3的通道5为55.5个采样周期 */ 
	ADC_RegularChannelConfig(ADC3, ADC_Channel_5, 1, ADC_SampleTime_55Cycles5); 
	
	/* Enable ADC3 DMA */ 
	ADC_DMACmd(ADC3, ENABLE); 
	
	/* Enable ADC1 */  
	ADC_Cmd(ADC3, ENABLE); 
	
	/*复位校准寄存器*/  
	ADC_ResetCalibration(ADC3); 
	
	/*等待校准寄存器复位完成*/  
	while(ADC_GetResetCalibrationStatus(ADC3)); 
	
	/* ADC校准*/  
	ADC_StartCalibration(ADC3); 
	
	/* 等待校准完成*/  
	while(ADC_GetCalibrationStatus(ADC3));  
	
	/* 由于没有采用外部触发，所以使用软件触发ADC转换*/ 
	ADC_SoftwareStartConvCmd(ADC3, ENABLE); 	
}

static void __adcTask(void *parameter){
	unsigned int adcValue;
	unsigned int result=0; 
	unsigned char i; 

  for (;;) {
    while(!ADC_GetFlagStatus(ADC3, ADC_FLAG_EOC ));//等待转换结束
    adcValue = ADC_GetConversionValue(ADC3);    //返回最近一次ADC3规则组的转换结果
		result=0; 
		for(i=16;i>0;i--) {  
			vTaskDelay(configTICK_RATE_HZ / 10);  
			result += adcValue; 
		} 
		result = (unsigned int)(((unsigned long)(result >> 4)) * 3300 >> 12); 
		printf("Now the voltage is %d\n", result);
	}
}

void ADCInit(void) {
	ADC3_GPIO_Config();
	ADC3_Mode_Config();
	xTaskCreate(__adcTask, (signed portCHAR *) "ADC", ADC_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 8, NULL);
}



