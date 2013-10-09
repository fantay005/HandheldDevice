#include "stm32f10x_iwdg.h"
#include "FreeRTOS.h"
#include "task.h"

static char __needResetSystem = 0;

/// \brief  ���Ź���ʼ��.
/// ��ʼ��CPUƬ�Ͽ��Ź�, 5���и�λ.
void WatchdogInit(void) {
	// д��0x5555,�����������Ĵ���д�빦��
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	// ����ʱ�ӷ�Ƶ,40K/256=156HZ(6.4ms)  ���Է�Ϊ 4��8��16��32��64��128��256
	IWDG_SetPrescaler(IWDG_Prescaler_256);
	//ι��ʱ�� 1s=156 @IWDG_Prescaler_256.ע�ⲻ�ܴ���4096
	IWDG_SetReload(780);     //5��
	IWDG_Enable();
}

/// \brief  ֹͣι���Ź�.
/// ��ִ���������֮���һ��ʱ��ϵͳ�������Ź�����.
void WatchdogResetSystem(void) {
	__needResetSystem = 1;
}

/// \brief  ι���Ź�.
/// �ú���������������ȼ�����ִ��, ÿ��һ����ι���Ź�.
void WatchdogFeed() {
	static uint32_t lastTick = 0;
	if (!__needResetSystem) {
		uint32_t currentTick = xTaskGetTickCount();
		if ((currentTick - lastTick) > configTICK_RATE_HZ) {
			lastTick = currentTick;
			IWDG_ReloadCounter();
		}
	}
}
