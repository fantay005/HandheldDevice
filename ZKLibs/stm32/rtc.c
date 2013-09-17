#include "stm32f10x_exti.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_gpio.h"
#include "misc.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "stdio.h"


#if defined(__SPEAKER__)
#define INDICTOR_LED_GPIO_PORT GPIOC
#define INDICTOR_LED_GPIO_PIN  GPIO_Pin_0
#elif defined(__LED__)
#define INDICTOR_LED_GPIO_PORT GPIOF
#define INDICTOR_LED_GPIO_PIN  GPIO_Pin_6
#else
#  error "Must define __SPKEAK__ or __LED__"
#endif

static xSemaphoreHandle __rtcSystemRunningSemaphore;

void RtcSystemRunningIndictor(void) {
//	static int count = 0;
	if (xSemaphoreTake(__rtcSystemRunningSemaphore, 0) == pdTRUE) {
		GPIO_WriteBit(INDICTOR_LED_GPIO_PORT, INDICTOR_LED_GPIO_PIN,
					  GPIO_ReadOutputDataBit(INDICTOR_LED_GPIO_PORT, INDICTOR_LED_GPIO_PIN) == Bit_RESET ? Bit_SET : Bit_RESET);
//		printf("RtcSystemRunningIndictor: %d\n", ++count);
	}
}


void RTC_IRQHandler(void) {
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET) {
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		RTC_ClearITPendingBit(RTC_IT_SEC);
		xSemaphoreGiveFromISR(__rtcSystemRunningSemaphore, &xHigherPriorityTaskWoken);
		if (xHigherPriorityTaskWoken) {
			taskYIELD();
		}
	}
}


static void rtcConfiguration(void) {

	PWR_BackupAccessCmd(ENABLE);        //����RTC�ͺ󱸼Ĵ����ķ���

	BKP_DeInit();
	RCC_LSEConfig(RCC_LSE_ON);         //�����ⲿ���پ���
	while (RESET == RCC_GetFlagStatus(RCC_FLAG_LSERDY));   //�ȴ�ʱ���ȶ�
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);      //����LSEΪRTCʱ��
	RCC_RTCCLKCmd(ENABLE);          //ʹ��RTCʱ��

	RTC_ITConfig(RTC_IT_OW, DISABLE);
	RTC_WaitForLastTask();
	RTC_ITConfig(RTC_IT_ALR, DISABLE);
	RTC_WaitForLastTask();

	RTC_WaitForSynchro();          //�ȴ�ʱ����APB1ʱ��ͬ��
	RTC_WaitForLastTask();          //�ȴ����һ�ζ�RTC�Ĵ����Ĳ������
	RTC_SetPrescaler(32767);         //����RTC��Ԥ��Ƶֵ
	RTC_WaitForLastTask();          //�ȴ����һ�ζ�RTC�Ĵ����Ĳ������
	RTC_SetAlarm(RTC_GetCounter() + 2);           //���������ֵ
	RTC_WaitForLastTask();

	RTC_ITConfig(RTC_IT_SEC, ENABLE);
	RTC_WaitForLastTask();
	RTC_ITConfig(RTC_IT_SEC, ENABLE);       //�������ж�
	RTC_WaitForLastTask();
}


static void rtcConfig(void) {
	if (0xA5A5 != BKP_ReadBackupRegister(BKP_DR1)) {
		rtcConfiguration();
		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	} else {
		if (RESET != RCC_GetFlagStatus(RCC_FLAG_LPWRRST)) { //����͹��ĸ�λ
			;
		}
		if (RESET != RCC_GetFlagStatus(RCC_FLAG_PINRST)) { //���wakeup��λ
			;
		}
		RCC_ClearFlag();
		RTC_ITConfig(RTC_IT_SEC, ENABLE);
		RTC_WaitForLastTask();
		RTC_ITConfig(RTC_IT_SEC, ENABLE);      //���������ж�
		RTC_WaitForLastTask();
	}
}

void RtcInit(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
	EXTI_InitTypeDef  EXTI_InitStructure;

	vSemaphoreCreateBinary(__rtcSystemRunningSemaphore);
	rtcConfig();

	GPIO_ResetBits(INDICTOR_LED_GPIO_PORT, INDICTOR_LED_GPIO_PIN);
	GPIO_InitStructure.GPIO_Pin =  INDICTOR_LED_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(INDICTOR_LED_GPIO_PORT, &GPIO_InitStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

