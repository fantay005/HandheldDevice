#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "misc.h"

#define UART_TASK_STACK_SIZE		( configMINIMAL_STACK_SIZE + 256 )
#define UART_GET_DATA_TIME           (configTICK_RATE_HZ * 60 * 15)

static xQueueHandle __uart3Queue;

typedef enum {
	directionI  = 0x00,
	directionII,
	waterLevelI,
	waterLevelII,
} addressType;


typedef struct {
	unsigned char station;
	unsigned char function;
	unsigned char origin[2];
	unsigned char readbyte[2];
	uint16_t crc;
} Info_frame;

static inline void __uart3HardwareInit(void) {
	GPIO_InitTypeDef    GPIO_InitStructure;
	USART_InitTypeDef   USART_InitStructure;
	NVIC_InitTypeDef    NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);				

	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART3, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


char *Modbusdat (addressType type){
	uint16_t p;
	char i, j;
	Info_frame *h ;
	h->station = 0x01;
	h->function = 0x03;
	h->origin[0] = 0x00;
	h->origin[1] = type;
	h->readbyte[0] = 0x00;
	h->readbyte[1] = 0x02;
	
	unsigned char *dat = (unsigned char *)h;
	for(j=0; j<6; j++) {
		p = 0xffff ^ ((char *)dat++);
		for (i=0; i<8; i++) {
			if (p & 0x01) {
				p = p >> 1;
				p = p ^ 0xA001;
			} else {
				p = p >> 1;
			}
		}
	}
	h->crc = p;
	
}

void USART3_Send_Byte(unsigned char byte){
    USART_SendData(USART3, byte); 
    while( USART_GetFlagStatus(USART3,USART_FLAG_TXE)!= SET);          
}

void UART3_Send_Str(unsigned char *s, int size){
    unsigned char i=0; 
    for(; i < size; ++i) 
    {
       USART3_Send_Byte(s[i]); 
    }
}

static void __uart3Task(void *nouse) {
	portBASE_TYPE rc;
	char *msg;
	static portTickType lastTDAT = 0;

	__uart3Queue = xQueueCreate(3, sizeof(char *));
	while (1) {
		rc = xQueueReceive(__uart3Queue, &msg, configTICK_RATE_HZ * 5);
		if (rc == pdTRUE) {
		    int size;
			const char *dat = (const char *)ProtoclQueryMeteTim(msg, &size);
		    GsmTaskSendTcpData(dat, size);
			vPortFree(msg);
		} else {
		    portTickType curT; 
			curT = xTaskGetTickCount();
		  	if ((curT - lastTDAT) >= UART_GET_DATA_TIME){
				UART3_Send_Str("QT\r", 3);
				lastTDAT = curT;
			}
		}
	}
}

static uint8_t *__uart3CreateMessage(const uint8_t *dat, int len) {
	uint8_t *r = pvPortMalloc(len);
	memcpy(r, dat, len);
	return r;
}

static char Buffer[210];

void USART3_IRQHandler(void)
{
    static int Index = 0, Hot = 0, Sensor = 0;
	uint8_t dat;
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
		dat = USART_ReceiveData(USART3);
		USART_SendData(USART1, dat);
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
		if ((dat == '\r' || dat == '\n') && (Sensor == 1)) {
		  
			uint8_t *msg;
			portBASE_TYPE xHigherPriorityTaskWoken;
			Buffer[Index++] = 0;
			msg = __uart3CreateMessage(Buffer, Index);		
			if (pdTRUE == xQueueSendFromISR(__uart3Queue, &msg, &xHigherPriorityTaskWoken)) {
				if (xHigherPriorityTaskWoken) {
					portYIELD();
				}
			}
		} 
	}
}

char *sensordat(void){
	 return &Buffer[0];
}

static inline void __uart3CreateTask(void) {
	xTaskCreate(__uart3Task, (signed portCHAR *) "UART", UART_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 5, NULL);
}

void Uart3Init() {
	__uart3HardwareInit();
	__uart3CreateTask();
}
