#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_fsmc.h"
#include "semphr.h"
#include "norflash.h"

static xSemaphoreHandle __semaphore;

void NorFlashInit(void) {
	if (__semaphore == NULL) {
		FSMC_NOR_Init();
		vSemaphoreCreateBinary(__semaphore);
	}
}

void NorFlashWrite(long flash, const short *ram, int len) {

	if (xSemaphoreTake(__semaphore, configTICK_RATE_HZ * 5) == pdTRUE) {
		FSMC_NOR_EraseSector(flash);
		FSMC_NOR_WriteBuffer(ram, flash, (len + 1) / 2);
		xSemaphoreGive(__semaphore);
	}
}

void NorFlashRead(int flash, short *ram, int len) {
	if (xSemaphoreTake(__semaphore, configTICK_RATE_HZ * 5) == pdTRUE) {
		FSMC_NOR_ReadBuffer(ram, flash, len / 2);
		xSemaphoreGive(__semaphore);
	}
}


bool NorFlashMutexLock(int time) {
	return (xSemaphoreTake(__semaphore, time) == pdTRUE);

}
void NorFlashMutexUnlock(void) {
	xSemaphoreGive(__semaphore);
}

