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
#include "norflash.h"

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

static void __ledTestTask(void *nouse) {
	int temp;
	int humi;
	uint32_t second;
	DateTime dateTime;
//	static const char *const weekDayStringTable[] = {
//		"一", "二", "三", "四", "五", "六", "日",
//	};

//	unsigned short unicode = 0x4E2D;
//	Unicode2GBKDestroy(Unicode2GBK((const uint8_t *)&unicode, 2));

	while (1) {
		if (!RtcWaitForSecondInterruptOccured(portMAX_DELAY)) {
			continue;
		}
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
		if ((dateTime.hour == 0x00) && (dateTime.minute == 0x00) && (dateTime.second >= 0x00) && (dateTime.second <= 0x05)) {
		   	printf("Reset From Default Configuration\n");
				vTaskDelay(configTICK_RATE_HZ * 5);
	      NVIC_SystemReset();
		}

		if ((dateTime.hour == 0x01) && (dateTime.minute == 0x00) && (dateTime.minute == 0x00) && (dateTime.second == 0x00)) {
				FSMC_NOR_EraseSector(SMS2_PARAM_STORE_ADDR);
				vTaskDelay(configTICK_RATE_HZ / 10);
				FSMC_NOR_EraseSector(SMS3_PARAM_STORE_ADDR);
				vTaskDelay(configTICK_RATE_HZ / 10);
				FSMC_NOR_EraseSector(SMS4_PARAM_STORE_ADDR);
				vTaskDelay(configTICK_RATE_HZ / 10);
				FSMC_NOR_EraseSector(SMS5_PARAM_STORE_ADDR);
				vTaskDelay(configTICK_RATE_HZ / 10);
				FSMC_NOR_EraseSector(SMS6_PARAM_STORE_ADDR);
				vTaskDelay(configTICK_RATE_HZ / 10);
				FSMC_NOR_EraseSector(SMS7_PARAM_STORE_ADDR);
				vTaskDelay(configTICK_RATE_HZ / 10);
		}

	}
}

void SHT10TestInit(void) {
	SHT10Init();
	xTaskCreate(__ledTestTask, (signed portCHAR *) "TST", SHT_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
