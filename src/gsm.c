#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "protocol.h"
#include "misc.h"
#include "xfs.h"
#include "zklib.h"
#include "unicode2gbk.h"
#include "second_datetime.h"


#define GSM_TASK_STACK_SIZE		( configMINIMAL_STACK_SIZE + 64 )

static xQueueHandle __gsmQueue;

const char *com_data = "one#\r\n";

typedef enum {
	TYPE_NONE = 0,
	TYPE_SEND_DATA,
	TYPE_DIS_DATA,
} GsmTaskMessageType;

typedef struct {
	/// Message type.
	GsmTaskMessageType type;
	/// Message lenght.
	unsigned int length;
} GsmTaskMessage;

/// Basic function for sending AT Command, need by atcmd.c.
/// \param  c    Char data to send to modem.
void ATCmdSendChar(char *msg, int len) {
	int i;
	for(i = 0; i < len; i++){
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
		USART_SendData(USART2, *msg++);
	}
}

GsmTaskMessage *__gsmCreateMessage(GsmTaskMessageType type, const char *dat, int len) {
	GsmTaskMessage *message = pvPortMalloc(ALIGNED_SIZEOF(GsmTaskMessage) + len);
	if (message != NULL) {
		message->type = type;
		message->length = len;
		memcpy(&message[1], dat, len);
	}
	return message;
}

static inline void *__gsmGetMessageData(GsmTaskMessage *message) {
	return &message[1];
}

static void __gsmInitUsart(void) {
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 19200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);
}

/// Init the CPU on chip hardware for the GSM modem.
static void __gsmInitHardware(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);				   //GSM模块的串口

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


void EXTI3_INTI(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOG, &GPIO_InitStructure);		  //MP3_IRQ


	EXTI_InitStructure.EXTI_Line = EXTI_Line13; //选择中断线路2 3 5
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; //设置为中断请求，非事件请求
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //设置中断触发方式为上下降沿触发
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;                                          //外部中断使能
  EXTI_Init(&EXTI_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOG, GPIO_PinSource13);	  

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;     //选择中断通道1
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //抢占式中断优先级设置为0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        //响应式中断优先级设置为0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                                   //使能中断
  NVIC_Init(&NVIC_InitStructure);
}

void EXTI15_10_IRQHandler(void)
{
	GsmTaskMessage *message;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	EXTI_ClearITPendingBit(EXTI_Line13);
  message = __gsmCreateMessage(TYPE_SEND_DATA, com_data, 6);
	if (pdTRUE == xQueueSendFromISR(__gsmQueue, &message, &xHigherPriorityTaskWoken)) {
		if (xHigherPriorityTaskWoken) {
			taskYIELD();
		}
	}	
}

static char buffer[800];
static int bufferIndex = 0;
static char isIPD = 0;

static inline void __gmsReceiveIPDData(unsigned char data) {
	if (data == 0x0A) {
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		GsmTaskMessage *message;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_DIS_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__gsmQueue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isIPD = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}

void USART2_IRQHandler(void) {
	unsigned char data;
	if (USART_GetITStatus(USART2, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(USART2);
	USART_SendData(USART1, data);
	USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	if (isIPD) {
		__gmsReceiveIPDData(data);
		return;
	}
	
	if (data == 0x0A) {
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    buffer[bufferIndex++] = 0;
		if(pdTRUE == xQueueSendFromISR(__gsmQueue, &buffer, &xHigherPriorityTaskWoken)){
			if(xHigherPriorityTaskWoken){
				portYIELD();
			}
		}
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
		if ((bufferIndex == 2) && (strncmp("#H", buffer, 2) == 0)) {
			isIPD = 1;
			bufferIndex = 0;
		}
	}
}

void __handleSendData(GsmTaskMessage *p) {
	unsigned int len = p->length;
	char *dat = __gsmGetMessageData(p);
	ATCmdSendChar(dat, len);
}

void __handleDisplay(GsmTaskMessage *p) {
	char *dat = __gsmGetMessageData(p);
	MessDisplay(dat);
}

typedef struct {
	GsmTaskMessageType type;
	void (*handlerFunc)(GsmTaskMessage *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ TYPE_SEND_DATA, __handleSendData },
	{ TYPE_DIS_DATA, __handleDisplay},
	{ TYPE_NONE, NULL },
};

static void __gsmTask(void *parameter) {
	portBASE_TYPE rc;
	GsmTaskMessage *message;
	for (;;) {
	  printf("Gsm: loop again\n");
	  rc = xQueueReceive(__gsmQueue, &message, configTICK_RATE_HZ * 10);
	  if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			for (; map->type != TYPE_NONE; ++map) {
				if (message->type == map->type) {
					map->handlerFunc(message);
					break;
				}
			}
			vPortFree(message);
		}
	}
}

void GSMInit(void) {
	__gsmInitHardware();
	EXTI3_INTI();
	__gsmInitUsart();
	__gsmQueue = xQueueCreate(10, sizeof( char *));
	xTaskCreate(__gsmTask, (signed portCHAR *) "GSM", GSM_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}