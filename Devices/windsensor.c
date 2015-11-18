/*
 * �紫����
 */
#include "windsensor.h"
#include "comm.h"

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "adc_dma.h"


#define GPIOx			GPIOA
#define PINx			GPIO_Pin_1
#define TIMx			TIM2
#define TIMx_IRQHandler	TIM2_IRQHandler
#define TIMx_IRQn		TIM2_IRQn
#define FOSC			72000000UL
#define CYCLE		 	100000

void WindSensor_Config(void)
{
	/*GPIO*/{
	GPIO_InitTypeDef GPIO_InitStructure;  
	GPIO_InitStructure.GPIO_Pin = PINx;		
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	   
	GPIO_Init(GPIOx, &GPIO_InitStructure);		
	}
	/*���ö�ʱ��*/{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
    TIM_DeInit(TIMx);
    TIM_TimeBaseStructure.TIM_Period=65535;		 								/* �Զ���װ�ؼĴ������ڵ�ֵ(����ֵ) */
    TIM_TimeBaseStructure.TIM_Prescaler= (FOSC/CYCLE - 1);				                /* ʱ��Ԥ��Ƶ�� 72M/360 */
    TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 		                /* ������Ƶ */
    TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;                   /* ���ϼ���ģʽ */
    TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure);
    TIM_ClearFlag(TIMx, TIM_FLAG_Update);	
	}
	/*����ͨ��2Ϊ����ģʽ*/{
	TIM_ICInitTypeDef  TIM_ICInitStructure;
	
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_PWMIConfig(TIMx, &TIM_ICInitStructure);
	
	TIM_SelectInputTrigger(TIMx, TIM_TS_TI2FP2);
	TIM_SelectSlaveMode(TIMx, TIM_SlaveMode_Reset);
	TIM_SelectMasterSlaveMode(TIMx, TIM_MasterSlaveMode_Enable);
	TIM_ITConfig(TIMx, TIM_IT_CC2|TIM_IT_Update, ENABLE);
	}
	/*���ö�ʱ���ж�*/{
    NVIC_InitTypeDef NVIC_InitStructure; 
	
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  													
    NVIC_InitStructure.NVIC_IRQChannel = TIMx_IRQn;	  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;	
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	}
	TIM_Cmd(TIMx, ENABLE);
}

static uint32_t speed;

/*
 * ���ټ���: 10000/retVal m/s
 * ��ע: retVal == 0 ʱ��ʾû�з�
 */
uint32_t WindSpeed_Read(void)
{//������(10us): 
	uint32_t tmp = speed;
	speed = 0;
	return tmp;
}

/*
 * ���������: ��ѹֵ(mV)
 * ��ע:ʹ��100ŷķ��������������ѹ��ת��
 * ����:400mV	����~:500mV		����:600mV	��~��:700mV
 * ����:800mV	��~��:900mV		����:1000mV	����~:1100mV
 * ����:1200mV	����~:1300mV	����:1400mV	��~��:1500mV
 * ����:1600mV	��~��:1700mV	����:1800mV	����~:1900mV
 */
uint32_t WindDir_Read(void)
{//������(mV) = retVal *3300/4096
	return ADCx_Read(ADC_ADDR_WINDDIR);
}

void TIMx_IRQHandler(void)
{
	if ( TIM_GetITStatus(TIMx , TIM_IT_Update) != RESET ){	
		TIM_ClearITPendingBit(TIMx , TIM_IT_Update); 
	}else if( TIM_GetITStatus(TIMx, TIM_IT_CC2) != RESET ){
		TIM_ClearITPendingBit(TIMx, TIM_IT_CC2);
		speed = TIM_GetCapture2(TIMx);
	}	 	
}


