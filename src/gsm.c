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
#include "norflash.h"


#define GSM_TASK_STACK_SIZE		( configMINIMAL_STACK_SIZE + 64 )
#define DELAY_TIME             (configTICK_RATE_HZ)

static xQueueHandle __gsmQueue;

const char *com_data = "#one#\r\n";
const char *stop = "#onestop#\r\n";

typedef enum {
	TYPE_NONE = 0,
	TYPE_CALL_AGAIN,
	TYPE_PAUSE_SEV,
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

static inline void __storeSpeakParam(char *p) {
	NorFlashWrite(GSM_PARAM_STORE_ADDR, (const short *)p, strlen(p));
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

void EXTI15_INTI(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);		  //MP3_IRQ


	EXTI_InitStructure.EXTI_Line = EXTI_Line15; //选择中断线路2 3 5
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; //设置为中断请求，非事件请求
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //设置中断触发方式为上下降沿触发
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;                                          //外部中断使能
  EXTI_Init(&EXTI_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource15);	  

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;     //选择中断通道1
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占式中断优先级设置为0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        //响应式中断优先级设置为0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                                   //使能中断
  NVIC_Init(&NVIC_InitStructure);
}

void EXTI14_INTI(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);		  //MP3_IRQ


	EXTI_InitStructure.EXTI_Line = EXTI_Line14; //选择中断线路2 3 5
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; //设置为中断请求，非事件请求
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //设置中断触发方式为上下降沿触发
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;                                          //外部中断使能
  EXTI_Init(&EXTI_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);	  

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;     //选择中断通道1
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //抢占式中断优先级设置为0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        //响应式中断优先级设置为0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                                   //使能中断
  NVIC_Init(&NVIC_InitStructure);
}

void EXTI7_INTI(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOG, &GPIO_InitStructure);		  //MP3_IRQ


	EXTI_InitStructure.EXTI_Line = EXTI_Line7; //选择中断线路2 3 5
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; //设置为中断请求，非事件请求
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //设置中断触发方式为上下降沿触发
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;                                          //外部中断使能
  EXTI_Init(&EXTI_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOG, GPIO_PinSource7);	  

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;     //选择中断通道1
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //抢占式中断优先级设置为0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        //响应式中断优先级设置为0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                                   //使能中断
  NVIC_Init(&NVIC_InitStructure);
}

static portTickType lastTI = 0;
static portTickType lastTII = 0;
static portTickType lastTIII = 0;

void EXTI9_5_IRQHandler(void) {
	int curT;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if ((curT - lastTIII) >= DELAY_TIME){
			GsmTaskMessage *message;
			curT = xTaskGetTickCount();
		  lastTIII = curT;
			EXTI_ClearITPendingBit(EXTI_Line7);
			message = __gsmCreateMessage(TYPE_CALL_AGAIN, "OK", 2);
			if (pdTRUE == xQueueSendFromISR(__gsmQueue, &message, &xHigherPriorityTaskWoken)) {
				 if (xHigherPriorityTaskWoken) {
						taskYIELD();
				 }
			}
	}
}

void EXTI15_10_IRQHandler(void)
{
  int curT;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if (GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_15) == 0){
			curT = xTaskGetTickCount();
		  if ((curT - lastTI) >= DELAY_TIME){
					GsmTaskMessage *message;
					lastTI = curT;
					EXTI_ClearITPendingBit(EXTI_Line15);
					message = __gsmCreateMessage(TYPE_SEND_DATA, com_data, 7);
					if (pdTRUE == xQueueSendFromISR(__gsmQueue, &message, &xHigherPriorityTaskWoken)) {
						if (xHigherPriorityTaskWoken) {
							taskYIELD();
						}
					}	
				}
   } else if (GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14) == 0) {
		 	curT = xTaskGetTickCount();
		  if ((curT - lastTII) >= DELAY_TIME){
					GsmTaskMessage *message;
					lastTII = curT;
					EXTI_ClearITPendingBit(EXTI_Line14);
					message = __gsmCreateMessage(TYPE_PAUSE_SEV, stop, 11);
					if (pdTRUE == xQueueSendFromISR(__gsmQueue, &message, &xHigherPriorityTaskWoken)) {
						if (xHigherPriorityTaskWoken) {
							taskYIELD();
						}
					}	
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
void __handleCall(GsmTaskMessage *p) {
	const char *t = (const char *)(Bank1_NOR2_ADDR + GSM_PARAM_STORE_ADDR);
	XfsTaskSpeakGBK(t, 16);	
}

void __handlePause(GsmTaskMessage *p) {
	unsigned int len = p->length;
	char *dat = __gsmGetMessageData(p);
	ATCmdSendChar(dat, len);
}

void __handleSendData(GsmTaskMessage *p) {
	unsigned int len = p->length;
	char *dat = __gsmGetMessageData(p);
	ATCmdSendChar(dat, len);
}

void __handleDisplay(GsmTaskMessage *p) {
	char *dat = __gsmGetMessageData(p);
	__storeSpeakParam(dat);
	MessDisplay(dat);
}

typedef struct {
	GsmTaskMessageType type;
	void (*handlerFunc)(GsmTaskMessage *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ TYPE_CALL_AGAIN, __handleCall},
  { TYPE_PAUSE_SEV, __handlePause},
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
	EXTI14_INTI();
	EXTI15_INTI();
	EXTI7_INTI();
	__gsmInitUsart();
	__gsmQueue = xQueueCreate(10, sizeof( char *));
	xTaskCreate(__gsmTask, (signed portCHAR *) "GSM", GSM_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}