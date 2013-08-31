#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_tim.h"

vu16 CCR1_Val = 8192;
vu16 CCR2_Val = 8192;
vu16 CCR3_Val = 8192;
vu16 CCR4_Val = 8192;

static void initHardware() {

	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	TIM_TimeBaseStructure.TIM_Period = 60000;//���ֵԽС���ж�ʱ��Խ��,��Լ10//����������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ������ȡֵ������ 0x0000 ��0xFFFF֮��.
	//�������Զ�װ���ֵ
	TIM_TimeBaseStructure.TIM_Prescaler = 35999;//TIM_Prescaler������������Ϊ TIMx ʱ��Ƶ�ʳ�����Ԥ��Ƶֵ������ȡֵ������ 0x0000 ��0xFFFF ֮��  ���㷽����CK_INT/(TIM_Perscaler+1)
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;//TIM_ClockDivision ������ʱ�ӷָ
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//TIM_CounterMode ѡ���˼�����ģʽ,ѡ�����ϼ���ģʽ��
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	// Prescaler configuration
	TIM_PrescalerConfig(TIM2, 35999, TIM_PSCReloadMode_Immediate);

	// Output Compare Timing Mode configuration: Channel1
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = CCR1_Val;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC1Init(TIM2, &TIM_OCInitStructure);

	//TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

	// TIM IT enable
	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
	TIM_Cmd(TIM2, DISABLE);

	//TIM3==========================================
	TIM_TimeBaseStructure.TIM_Period = 20000;
	TIM_TimeBaseStructure.TIM_Prescaler = 35999;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	// Prescaler configuration
	TIM_PrescalerConfig(TIM3, 35999, TIM_PSCReloadMode_Immediate);

	// Output Compare Timing Mode configuration: Channel1
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = CCR1_Val;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;

	TIM_OC1Init(TIM3, &TIM_OCInitStructure);

	//TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

	// TIM IT enable
	TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE);
	TIM_Cmd(TIM3, DISABLE);
}

void vTim(void *parameter) {
	initHardware();

}
