#ifdef __LED__
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32f10x_fsmc.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "ledconfig.h"
#include "led_lowlevel.h"
#include "zklib.h"
#include "display.h"
#include "norflash.h"
#include "xfs.h"
#include "unicode2gbk.h"
#include "font_dot_array.h"

#define DISPLAY_TASK_STACK_SIZE		( configMINIMAL_STACK_SIZE + 64 )

static xSemaphoreHandle __scrollSemaphore;

#if 0
void __scrollDisplayTask(void *helloString) {
	portBASE_TYPE rc;
	int xContentEnd = LED_VIR_DOT_WIDTH-1;
	int xScan = LED_PHY_DOT_WIDTH-1;
	int xDisp = 0;
	printf("ScrollDisplayTask: start->\n");
	while(1) {
		if (xSemaphoreTake(__scrollSemaphore, portMAX_DELAY) == pdTRUE) {
			LedScrollDisplayToScan(xDisp, 0, xScan, 0);
			if (xScan > 0) {
				--xScan;
			} else {
				++xDisp;
				if (xDisp >= xContentEnd) {
					xScan = LED_PHY_DOT_WIDTH-1;
					xDisp = 0;
				}
			}
		}
	}
}
#else

extern char SMScome(void);
extern void FlagChange(void);

void __scrollDisplayTask(void *helloString) {
	portBASE_TYPE rc;
	int yDisp = 0, i;
	printf("ScrollDisplayTask, start\n");
	while(1) {
		if (xSemaphoreTake(__scrollSemaphore, portMAX_DELAY) == pdTRUE) {
			if (SMScome() != 0) {
				yDisp = 0;
				FlagChange();
			}
			LedScrollDisplayToScan(0, yDisp, 0, 0);
			if (yDisp % 16 == 0) {
				int yPre = yDisp - 16;
				if (yPre < 0) {
					yPre +=LED_VIR_DOT_HEIGHT;
				}
				DisplayScrollNotify(yPre);
			}
			++yDisp;

			if (yDisp >= LED_VIR_DOT_HEIGHT) {
				yDisp = 0;
			}
			
			if ((yDisp == 0) || (yDisp == LED_PHY_DOT_HEIGHT) || (yDisp == LED_PHY_DOT_HEIGHT * 2)){
				TIM_Cmd(TIM3, DISABLE);
				for (i = 0; i < 300; i++) {
					if (SMScome() == 1) {
						break;
					}
				  vTaskDelay(configTICK_RATE_HZ / 100);
				}
				TIM_Cmd(TIM3, ENABLE);
			}
		}
	}
}

#endif

void TIM3_IRQHandler() {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	TIM3->SR = ~(0x00FF);
	xSemaphoreGiveFromISR(__scrollSemaphore, &xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}
}


static void __init(void) {
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_TimeBaseStructure.TIM_Period = 36000;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_PrescalerConfig(TIM3, (180 - 1), TIM_PSCReloadMode_Immediate);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 1800;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE);

	TIM_Cmd(TIM3, ENABLE);

}


void ScrollDisplayInit(void) {
	__init();
	vSemaphoreCreateBinary(__scrollSemaphore);
	xTaskCreate(__scrollDisplayTask, (signed portCHAR *) "SCROLL", DISPLAY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 100, NULL);
}

#endif