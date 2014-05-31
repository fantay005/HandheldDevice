#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
 

#define UART_TASK_STACK_SIZE		( configMINIMAL_STACK_SIZE + 256 )
#define UART_GET_DATA_TIME      (configTICK_RATE_HZ * 30)
#define TERM_UPLOAD_DATA_TIME   (configTICK_RATE_HZ * 60 * 5)
#include "misc.h" 

static xQueueHandle __uart3Queue;

typedef enum {
	direction  = 0x01,
	waterLevel = 0x03,
} addressType;


typedef struct {
	unsigned char station;
	unsigned char function;
	unsigned char origin[2];
	unsigned char readbyte[2];
	unsigned char crc[2];
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
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_ResetBits(GPIOB, GPIO_Pin_13);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


char *Modbusdat (addressType type, Info_frame *h){
	uint16_t p = 0xffff;
	char i, j;
	uint8_t tmp;
	unsigned char *dat;
	h->station = 0x01;
	h->function = 0x03;
	h->origin[0] = 0x00;
	h->origin[1] = type;
	h->readbyte[0] = 0x00;
	h->readbyte[1] = 0x02;
	
	dat = (unsigned char *)h;
	
	for(j=0; j<6; j++) {
		tmp = p & 0xff;
		tmp = tmp ^ (*dat++);
		p = (p & 0xff00) | tmp;
		
		for (i=0; i<8; i++) {
			if (p & 0x01) {
				p = p >> 1;
				p = p ^ 0xA001;
			} else {
				p = p >> 1;
			}
		}
	}
	h->crc[0] = p & 0xff;
	h->crc[1] = p >> 8;	
	return (char *)h;
}

void USART3_Send_Byte(unsigned char byte){
    USART_SendData(USART3, byte); 
    while( USART_GetFlagStatus(USART3,USART_FLAG_TXE)!= SET);          
}

void UART3_Send_Str(unsigned char *s, int size){
	
    unsigned char i=0; 

	for(; i < size; i++) 
    {
       USART3_Send_Byte(s[i]); 
    }
	USART3_Send_Byte(0X00);
}

static char Buffer[7];
static int Index = 0;
static char flag = 0;
static unsigned char KG[2];
static unsigned char KSW[2];

char *reservoir(char *dat){
  uint16_t i, j;
	dat = pvPortMalloc(20);

	i = (KG[0] << 8) + KG[1];
	j = (KSW[0] << 8) + KSW[1];
	sprintf(dat, "KG%dKSW%d", i, j);
	return dat;
}
static void __uart3Task(void *nouse) {
	portBASE_TYPE rc;
	char *msg;
	static portTickType lastTDAT = 0, lastTime;
	Info_frame frame;
	__uart3Queue = xQueueCreate(3, sizeof(char *));
	while (1) {
		rc = xQueueReceive(__uart3Queue, &msg, configTICK_RATE_HZ * 2);
		if (rc == pdTRUE) {
		
		} else {
		    portTickType curT; 
			  curT = xTaskGetTickCount();
		  	if ((curT - lastTDAT) >= UART_GET_DATA_TIME){
				unsigned char *dat = Modbusdat(direction, &frame);
				GPIO_SetBits(GPIOB, GPIO_Pin_13);
				vTaskDelay(configTICK_RATE_HZ / 100);
	      flag = 1;
				UART3_Send_Str(dat, 8);
				GPIO_ResetBits(GPIOB, GPIO_Pin_13);
				vTaskDelay(configTICK_RATE_HZ / 5);
				GPIO_SetBits(GPIOB, GPIO_Pin_13);
				vTaskDelay(configTICK_RATE_HZ / 100);
					
			  dat = Modbusdat(waterLevel, &frame);
				flag = 2;
     		UART3_Send_Str(dat, 8);
				GPIO_ResetBits(GPIOB, GPIO_Pin_13);
				lastTDAT = curT;
			}
			
			if((curT - lastTime) >= TERM_UPLOAD_DATA_TIME){
				uint16_t m, n;
				int size;
				char *buf = pvPortMalloc(20);
				const char *dat;
				m = (KG[0] << 8) + KG[1];
				n = (KSW[0] << 8) + KSW[1];
				sprintf(buf, "_KGI%d_KSWI%d", m, n);
				dat = (const char *)ProtoclQueryMeteTim(buf, &size);
				__gsmSendTcpDataLowLevel(dat, size);
				ProtocolDestroyMessage(dat);
				vPortFree((void *)buf);
				lastTime = curT;
			}
		}
	}
}

static uint8_t *__uart3CreateMessage(const uint8_t *dat, int len) {
	uint8_t *r = pvPortMalloc(len);
	memcpy(r, dat, len);
	return r;
}


void USART3_IRQHandler(void)
{ 
	uint8_t dat;
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
		dat = USART_ReceiveData(USART3);
		USART_SendData(USART1, dat);
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
		if (Index >= 6) {		  
			uint8_t *msg;
			portBASE_TYPE xHigherPriorityTaskWoken;
			Buffer[Index++] = dat;
			msg = __uart3CreateMessage(Buffer, Index);		
			if (pdTRUE == xQueueSendFromISR(__uart3Queue, &msg, &xHigherPriorityTaskWoken)) {
				if (xHigherPriorityTaskWoken) {
					portYIELD();
				}
			}
			if(flag == 1){
				KG[0] = Buffer[3];
				KG[1] = Buffer[4];
			} else if (flag == 2){
				KSW[0] = Buffer[3];
				KSW[1] = Buffer[4];
			}
			Index = 0;
		} else {
			if(Index == 0) {
				if(dat == 0x01) {
			    Buffer[Index++] = dat;
				}
			} else if (Index == 1) {
				if(dat == 0x03) {
			    Buffer[Index++] = dat;
				} else {
					Index = 0;
				}
			} else if (Index == 2) {
				if(dat == 0x02) {
			    Buffer[Index++] = dat;
				} else {
					Index = 0;
				}
			} else {
				Buffer[Index++] = dat;
			}
		}
	}
}

static inline void __uart3CreateTask(void) {
	xTaskCreate(__uart3Task, (signed portCHAR *) "UART", UART_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 5, NULL);
}

void Uart3Init() {
	__uart3HardwareInit();
	__uart3CreateTask();
}
