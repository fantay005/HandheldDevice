/*
 * ����
 */
#include "rainfall.h"
#include "comm.h"
#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "misc.h"

#define GPIOx	GPIOB
#define PINx	GPIO_Pin_8
#define TIGPIOx	GPIO_PortSourceGPIOB
#define TIPINx	GPIO_PinSource8
#define LINEx	EXTI_Line8
#define IRQx	EXTI9_5_IRQn
#define EXTIx_IRQHandler	EXTI9_5_IRQHandler

static
void __Exti_Config(void)
{
	/*GPIO*/{
	GPIO_InitTypeDef GPIO_InitStructure; 
	
	GPIO_InitStructure.GPIO_Pin = PINx;		
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;  // ��������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx, &GPIO_InitStructure);
	}
	/*EXTI*/{
	EXTI_InitTypeDef EXTI_InitStructure;
	
	GPIO_EXTILineConfig(TIGPIOx, TIPINx); 
	EXTI_InitStructure.EXTI_Line = LINEx;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure); 
	}
	/*NVIC*/{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = IRQx;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	}
}

static uint32_t count;

void Rainfall_Config(void)
{
	__Exti_Config();
}

/*
 * �������㹫ʽ: retVal * 0.2 mm
 */
uint32_t Rainfall_Read(void)
{//������:
	return count;
}

void EXTIx_IRQHandler(void)
{
	if(EXTI_GetITStatus(LINEx) != RESET){
		count++;
		EXTI_ClearITPendingBit(LINEx);
	}
}

