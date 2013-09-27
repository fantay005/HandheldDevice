#include "stm32f10x_iwdg.h"
#include "FreeRTOS.h"
#include "task.h"

static char __needReset = 0;
void WatchdogInit() {
	// д��0x5555,�����������Ĵ���д�빦��
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	// ����ʱ�ӷ�Ƶ,40K/256=156HZ(6.4ms)  ���Է�Ϊ 4��8��16��32��64��128��256
	IWDG_SetPrescaler(IWDG_Prescaler_256);
	//ι��ʱ�� 1s=156 @IWDG_Prescaler_256.ע�ⲻ�ܴ���4096
	IWDG_SetReload(780);     //5��
	IWDG_Enable();
}

void WatchdogStopFeed() {
	__needReset = 1;
}

void WatchdogFeed() {
	static unsigned int lastTick = 0;
	if (__needReset) {
		return;
	}
	if ((xTaskGetTickCount() - lastTick) > configTICK_RATE_HZ) {
		lastTick = xTaskGetTickCount();
		IWDG_ReloadCounter();
	}
}
