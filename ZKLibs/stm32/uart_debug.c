#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "misc.h"
#include "norflash.h"
#include "display.h"


#define DEBUG_TASK_STACK_SIZE		( configMINIMAL_STACK_SIZE + 64 )
#define UART3_TASK_STACK_SIZE		( configMINIMAL_STACK_SIZE + 256 )

static xQueueHandle __uartDebugQueue;
static xQueueHandle __uart3Queue;

static inline void __uartDebugHardwareInit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef   USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);				

	USART_InitStructure.USART_BaudRate = 19200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART3, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void __storeSMS1(const char *sms) {
	NorFlashWrite(SMS1_PARAM_STORE_ADDR, (const short *)sms, strlen(sms) + 1);
}


static void __uartDebugTask(void *nouse) {
	portBASE_TYPE rc;
	char *msg;

//	printf("UartDebugTask: start\n");
	__uartDebugQueue = xQueueCreate(3, sizeof(char *));
	while (1) {
		rc = xQueueReceive(__uartDebugQueue, &msg, portMAX_DELAY);
		if (rc == pdTRUE) {
			extern void DebugHandler(char * msg);
			DebugHandler(msg);
			vPortFree(msg);
		}
	}
}

static void __uart3Task(void *weather) {
	portBASE_TYPE rc;
	char *msg;

	__uart3Queue = xQueueCreate(3, sizeof(char *));
	while (1) {
		rc = xQueueReceive(__uart3Queue, &msg, portMAX_DELAY);
		if (rc == pdTRUE) {
			__storeSMS1(msg);
			MessDisplay(msg);
			vPortFree(msg);
		}
	}
}

static uint8_t *__uartDebugCreateMessage(const uint8_t *dat, int len) {
	uint8_t *r = pvPortMalloc(len);
	memcpy(r, dat, len);
	return r;
}

static uint8_t *__uart3CreateMessage(const uint8_t *dat, int len) {
	uint8_t *r = pvPortMalloc(len);
	memcpy(r, dat, len);
	return r;
}

static inline void __uartDebugCreateTask(void) {
	xTaskCreate(__uartDebugTask, (signed portCHAR *) "DBG", DEBUG_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
}

static inline void __uart3CreateTask(void) {
	xTaskCreate(__uart3Task, (signed portCHAR *) "WEATHER", UART3_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}

void UartDebugInit() {
	__uartDebugHardwareInit();
	__uartDebugCreateTask();
	__uart3CreateTask();
}

void USART1_IRQHandler(void) {
	static uint8_t buffer[64];
	static int index = 0;

	uint8_t dat;
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		dat = USART_ReceiveData(USART1);
		USART_SendData(USART1, dat);
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		if (dat == '\r' || dat == '\n') {
			uint8_t *msg;
			portBASE_TYPE xHigherPriorityTaskWoken;

			buffer[index++] = 0;
			msg = __uartDebugCreateMessage(buffer, index);
			if (pdTRUE == xQueueSendFromISR(__uartDebugQueue, &msg, &xHigherPriorityTaskWoken)) {
				if (xHigherPriorityTaskWoken) {
					portYIELD();
				}
			}
			index = 0;

		} else {
			buffer[index++] = dat;
		}
	}
}

static uint8_t Buffer[210];

void USART3_IRQHandler(void)
{
    static int Index = 0, Hot = 0;
	uint8_t dat;
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
		dat = USART_ReceiveData(USART3);
		USART_SendData(USART1, dat);
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
		if (dat == '\r' || dat == '\n') {
			uint8_t *msg;
			portBASE_TYPE xHigherPriorityTaskWoken;
			Buffer[Index++] = 0;
			msg = __uart3CreateMessage(Buffer, Index);
			if (pdTRUE == xQueueSendFromISR(__uart3Queue, &msg, &xHigherPriorityTaskWoken)) {
				if (xHigherPriorityTaskWoken) {
					portYIELD();
				}
			}
			Hot = 0;
			Index = 0;

		} else if (dat == '#'){
			Hot = 1;
		} else if (Hot == 1){
			if (dat == 'H'){
			   Hot = 2;
			}else{
			   Hot = 0;
			}
		} else if (Hot == 2) {
			if (dat == 'O'){
			   Hot = 3;
			}else{
			   Hot = 0;
			}
		} else if (Hot == 3) {
	    	if (dat == 'T'){
			   Hot = 4;
			}else{
			   Hot = 0;
			}
		} else if (Hot == 4) {
			Buffer[Index++] = dat;
		}
	}
}

int fputc(int c, FILE *f) {
    USART_SendData(USART1, c);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	return c;
}

int putch(int c) {
    USART_SendData(USART1, c);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	return c;
}

