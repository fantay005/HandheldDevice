#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_gpio.h"
#include "watchdog.h"
#include "CommZigBee.h"

#define RECOVERY_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )

#define Pin_REC   GPIO_Pin_9   
#define GPIO_REC  GPIOB 

#define Pin_LED   GPIO_Pin_8
#define GPIO_LED  GPIOB

/// \brief  ��ʼ���ָ��������ð�����Ҫ��CPUƬ����Դ.
/// ��GPIOG_13���ó�����״̬.
void RecoveryInit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin =  Pin_REC;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIO_REC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin =  Pin_LED;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIO_LED, &GPIO_InitStructure);
}

void ConfigEnd(void){
	GPIO_ResetBits(GPIO_LED, Pin_LED);
}

void ConfigStart(void){
	GPIO_SetBits(GPIO_LED, Pin_LED);
}

/// \brief  ִ�лָ��������õ�������.
/// ����������Nor Flash�е�����, ������ϵͳ.
void __taskRecovery(void *nouse) {
	
}

/// \brief  ����������ָ���������.
/// ���ָ��������ð���PIN, �����ס����5�봴��ִ�лָ��������õ�����.
void RecoveryToFactory(void) {
	char currentState;
	static unsigned long lastTick = 0;
	if (lastTick == 0xFFFFFFFF) {
		return;
	}
	currentState = GPIO_ReadInputDataBit(GPIO_REC, Pin_REC);
	if (currentState == Bit_SET) {
		lastTick = xTaskGetTickCount();
	} else {
		if (xTaskGetTickCount() - lastTick > configTICK_RATE_HZ * 2) {
			ConfigStart();
			ComxConfigData("AAA", 4);
			//xTaskCreate(__taskRecovery, (signed portCHAR *) "REC", RECOVERY_TASK_STACK_SIZE, (void *)'2', tskIDLE_PRIORITY + 5, NULL);
			lastTick = 0xFFFFFFFF;
		}
	}
}

