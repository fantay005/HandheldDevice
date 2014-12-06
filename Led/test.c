#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "sht1x.h"
#include "rtc.h"
#include "seven_seg_led.h"
#include "second_datetime.h"
#include "unicode2gbk.h"
#include "softpwm_led.h"

#define SHT_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE + 64 )

static void __ledTestTask(void *nouse) {
	uint32_t second;
	DateTime dateTime;

	while (1) {
		if (!RtcWaitForSecondInterruptOccured(portMAX_DELAY)) {
			continue;
		}
		second = RtcGetTime();
		SecondToDateTime(&dateTime, second);
	}
}

void SHT10TestInit(void) {
	xTaskCreate(__ledTestTask, (signed portCHAR *) "TST", SHT_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
