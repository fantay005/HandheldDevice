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
	char prompt[21] = {0xFD, 0x00, 0x12, 0x01, 0x03, 0x63, 0xF9, 0xAC, 0x4E, 0xF6, 0x65, 0xF4, 0x95, 
                     0x21, 0x00, 0x38, 0x00, 0xB9, 0x70, 0x74, 0x65}; //����ʱ��ʮ�˵���
	if(para >= 0x0A){
		prompt[13] = 0x31;
		prompt[15] = para + 0x26;
	}	else {	
	  prompt[15] = para + 0x30;	
	}		
  SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
	for (i = 0; i < 21; i++) {
		USART_SendData(USART3, prompt[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 3);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
}

static void __ledTestTask(void *nouse) {
	DateTime dateTime;
	uint32_t second;

#if defined(__LED__)
	int temp;
	int humi;
	enum SoftPWNLedColor color;
	uint32_t second;
//	static const char *const weekDayStringTable[] = {
//		"һ", "��", "��", "��", "��", "��", "��",
//	};

//	unsigned short unicode = 0x4E2D;
//	Unicode2GBKDestroy(Unicode2GBK((const uint8_t *)&unicode, 2));

		second = RtcGetTime();
		SecondToDateTime(&dateTime, second);
		SHT10ReadTemperatureHumidity(&temp, &humi);
		temp = (temp + 5) / 10;
		if (temp > 99) {
			temp = 99;
		}
		humi = (humi + 5) / 10;
		if (humi > 99) {
			humi = 99;
		}

		SevenSegLedSetContent(LED_INDEX_WEEK, dateTime.day);
		SevenSegLedSetContent(LED_INDEX_YEAR_H, dateTime.year / 10);
		SevenSegLedSetContent(LED_INDEX_YEAR_L, dateTime.year % 10);
		SevenSegLedSetContent(LED_INDEX_MONTH_H, dateTime.month / 10);
		SevenSegLedSetContent(LED_INDEX_MONTH_L, dateTime.month % 10);
		SevenSegLedSetContent(LED_INDEX_DATE_H, dateTime.date / 10);
		SevenSegLedSetContent(LED_INDEX_DATE_L, dateTime.date % 10);
		SevenSegLedSetContent(LED_INDEX_HOUR_H, dateTime.hour / 10);
		SevenSegLedSetContent(LED_INDEX_HOUR_L, dateTime.hour % 10);
		SevenSegLedSetContent(LED_INDEX_MINUTE_H, dateTime.minute / 10);
		SevenSegLedSetContent(LED_INDEX_MINUTE_L, dateTime.minute % 10);
		SevenSegLedSetContent(LED_INDEX_TEMP_H, temp / 10);
		SevenSegLedSetContent(LED_INDEX_TEMP_L, temp % 10);
		SevenSegLedSetContent(LED_INDEX_HUMI_H, humi / 10);
		SevenSegLedSetContent(LED_INDEX_HUMI_L, humi % 10);
		SevenSegLedDisplay();
#if defined(__LED_HUAIBEI__)
		if ((dateTime.hour == 0x00) && (dateTime.minute == 0x00) && (dateTime.minute == 0x00)) {
			color = SoftPWNLedColorNULL;
			SoftPWNLedSetColor(color);
			LedDisplayGB2312String162(2 * 4, 0, "����������ũ����");
			LedDisplayToScan2(16 * 4, 0, 16 * 12 - 1, 15);
			__storeSMS2("����������ũ����");
		}
#endif
#endif
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
			 if ((dateTime.hour >= 0x08) && (dateTime.hour <= 0x11) && (dateTime.minute == 0x00) && (dateTime.second == 0x00) && (dateTime.second == 0x00)){
				  BKUI_Prompt(dateTime.hour);
			 }
	}
}

void SHT10TestInit(void) {
#if defined(__LED__)
	SHT10Init();
#endif
	xTaskCreate(__ledTestTask, (signed portCHAR *) "TST", SHT_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
