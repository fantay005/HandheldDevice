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
	 
  /* ����DMA�йصļĴ�����Ϊ��ʼֵ*/ 
  DMA_DeInit(DMA1_Channel1);
	 
  /*����DMA�������ַ, �����ADC1_DR_Address ���û��Լ�����ģ���Ϊ���ת������ļĴ�����
	 �������þ��Ǹ���DMAȡ���͵�ADC1_DR_Address ������ȡ��*/  
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC3_DR_Address; 
	 
  /*�����ڴ����ַ��������DMAҪ����AD��ȡ�������ŵ�ADC_ConvertedValue�� */  
  DMA_InitStructure.DMA_MemoryBaseAddr =(u32)&ADC_ConvertedValue; 
	 
  /*�����䣬����AD������Ϊ���ݴ������Դ��������DMA�ǽ�AD�е�����ȡ���ŵ��ڴ��У����ܷ�����*/  
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; // DMA_DIR_PeripheralDST ˫����
	 
  /*ָ��DMAͨ����DMA����Ĵ�С,������DMA���ټ����ڴ�ռ䣬��������ֻȡͨ��10��AD��������ֻ�迪��һ���ڴ�ռ�*/ 
  DMA_InitStructure.DMA_BufferSize = 1;  
	 
  /*�趨�Ĵ�����ַ�̶���������DMA��ֻ�ӹ̶���һ���ط�ȡ��*/ 
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //����DMA�������ģʽ�����ʹ�õ�ͨ���ж���������ӣ�ʹ��
	
  /*�趨�ڴ��ַ�̶�����ÿ��DMA����ֻ�����ᵽ�̶����ڴ���*/ 
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //����DMA�ڴ����ģʽ��DMA���ʶ���ڴ����ʱ��ʹ��
	
  /*�趨�������ݿ�ȣ�������DMAҪȡ�����Ĵ�С*/ 
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //����DMA�ڷ�������ʱ��ÿ�β��������ݳ��ȣ�3�����ͣ�Byte, HalfWord, Word��
	
	 /*�趨�ڴ�ĵĿ��*/  
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; //
	
  /*�趨DMA������ѭ������ģʽ��������DMAҪ��ͣ�İ��ˣ�����͵��*/ 
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; //����һ�ξͲ�Ҫ���ʣ�DMA_Mode_Normal
	
  /*�趨DMAѡ����ͨ��������ȼ�*/  
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  
  /*����DMA�е�2���ڴ滥�����*/	
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 
	
	/*��DMAģ���ʼ��*/
  DMA_Init(DMA1_Channel1, &DMA_InitStructure); 
	
  /* Enable DMA channel1��CPU�кü���DMA���飬����ֻ��DMA1_Channel1�������*/  
  DMA_Cmd(DMA1_Channel1, ENABLE); 
	
	/*�����Ǵ����DMA����*/
	
  /*����ADC�����ڶ���ģʽ*/  
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; //�����ģʽ�£�˫ADC����ͬ����ÿ��ADC�ӿڶ�������
	
  /*�涨ADת�������ڵ���ģʽ������һ��ͨ������*/ 
  ADC_InitStructure.ADC_ScanConvMode = DISABLE ; //���ʹ�ö��ͨ����ҪENABLE
	
  /*�趨ADת��������ģʽ*/  
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;  //����ת��ֱ�����е�����ת����ɺ��ֹͣת����������ת����ֻת��һ�����ݾ�ֹͣ��Ҫ�ٴδ���ת���ſ���
	
  /*��ʹ���ⲿ�ٷ�ת��*/   	/*ѡ���ⲿ����ģʽ*/
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;  //ѡ�����������
	
	/*�ɼ��������ڼĴ��������Ҷ���ķ�ʽ���*/  
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; 
	
	/*�趨Ҫת����ADͨ����Ŀ*/  
	ADC_InitStructure.ADC_NbrOfChannel = 1; 
	/*��ʼ��ADC3*/
	ADC_Init(ADC3, &ADC_InitStructure);  
	
	/*����ADCʱ�ӣ�ΪPCLK2��8��Ƶ����9MHz*/ 
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);  
	
	/*����ADC3��ͨ��5Ϊ55.5���������� */ 
	ADC_RegularChannelConfig(ADC3, ADC_Channel_5, 1, ADC_SampleTime_55Cycles5); 
	
	/* Enable ADC3 DMA */ 
	ADC_DMACmd(ADC3, ENABLE); 
	
	/* Enable ADC1 */  
	ADC_Cmd(ADC3, ENABLE); 
	
	/*��λУ׼�Ĵ���*/  
	ADC_ResetCalibration(ADC3); 
	
	/*�ȴ�У׼�Ĵ�����λ���*/  
	while(ADC_GetResetCalibrationStatus(ADC3)); 
	
	/* ADCУ׼*/  
	ADC_StartCalibration(ADC3); 
	
	/* �ȴ�У׼���*/  
	while(ADC_GetCalibrationStatus(ADC3));  
	
	/* ����û�в����ⲿ����������ʹ���������ADCת��*/ 
	ADC_SoftwareStartConvCmd(ADC3, ENABLE); 	
}

static void __adcTask(void *parameter){
	unsigned int adcValue;
	unsigned int result=0; 
	unsigned char i; 

  for (;;) {
    while(!ADC_GetFlagStatus(ADC3, ADC_FLAG_EOC ));//�ȴ�ת������
    adcValue = ADC_GetConversionValue(ADC3);    //�������һ��ADC3�������ת�����
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



