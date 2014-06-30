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
#include "stm32f10x_usart.h"
#include "xfs.h"
#include "soundcontrol.h"

#define SHT_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE + 64 )

#define LED_INDEX_HUMI_L 5
#define LED_INDEX_HUMI_H 6
#define LED_INDEX_TEMP_L 7
#define LED_INDEX_TEMP_H 8
#define LED_INDEX_WEEK 9
#define LED_INDEX_YEAR_H 10
#define LED_INDEX_YEAR_L 11
#define LED_INDEX_MONTH_H 12
#define LED_INDEX_MONTH_L 13
#define LED_INDEX_DATE_H 14
#define LED_INDEX_DATE_L 15
#define LED_INDEX_HOUR_H 16
#define LED_INDEX_HOUR_L 17
#define LED_INDEX_MINUTE_H 18
#define LED_INDEX_MINUTE_L 19

void BKUI_Prompt(char para) {
	int i;
	char prompt[41] = {0xFD, 0x00, 0x26, 0x01, 0x01, '[', 'm', '5', '1', ']', 's', 'o', 'u', 'n', 'd', '1', '2', '3',//sound123
		                 0xD5,0xFB,  0xB5,0xE3,  0xB1,0xA8,  0xCA,0xB1,  ',',  0xB1,0xBB,  0xBE,0xA9,  0xCA,0xB1,  0xBC,0xE4, 
                     ',',  '8',  0xB5,0xE3,  0xD5,0xFB,}; //整点报时，北京时间八点整
	if(para >= 0x0A){
		prompt[35] = 0x31;
		prompt[36] = para + 0x27;
	}	else {	
	  prompt[36] = para + 0x31;	
	}		
  SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
	for (i = 0; i < 41; i++) {
		USART_SendData(USART3, prompt[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 5);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
	recover();
}

void HALF_Prompt(void) {
	int i;
	char prompt[25] = {0xFD, 0x00, 0x16, 0x01, 0x01, '[', 'm', '3', ']', 's', 'o', 'u', 'n', 'd', '1', '2', '3',//sound123
		                 0xB0,0xEB,  0xB5,0xE3,  0xB1,0xA8,  0xCA,0xB1};  //半点报时
  SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
	for (i = 0; i < 25; i++) {
		USART_SendData(USART3, prompt[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 2);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
}

static void __ledTestTask(void *nouse) {
	DateTime dateTime;
	uint32_t second;
	
	   while (1) {
		   if (!RtcWaitForSecondInterruptOccured(portMAX_DELAY)) {
			  continue;
		   }

		   second = RtcGetTime();
	     SecondToDateTime(&dateTime, second);
			 
		   if ((dateTime.hour == 0x00) && (dateTime.minute == 0x00) && (dateTime.second >= 0x00) && (dateTime.second <= 0x05)) {
		   		printf("Reset From Default Configuration\n");
				 vTaskDelay(configTICK_RATE_HZ * 5);
	            NVIC_SystemReset();
		   }
			 if ((dateTime.hour >= 0x08) && (dateTime.hour <= 0x12) && (dateTime.minute == 0x3B) && (dateTime.second == 0x36)){
				  BKUI_Prompt(dateTime.hour);
			 }
			 
			 if ((dateTime.hour >= 0x08) && (dateTime.hour < 0x12) && (dateTime.minute == 0x1D) && (dateTime.second == 0x36)){
				  HALF_Prompt();
			 }
	}
}

void SHT10TestInit(void) {
	xTaskCreate(__ledTestTask, (signed portCHAR *) "TST", SHT_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
