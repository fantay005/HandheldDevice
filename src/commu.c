#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "feedback.h"
#include "commu.h"

#define BAUD     9600
#define GPIO_COM  GPIOA
#define Pin_TX    GPIO_Pin_2
#define Pin_RX    GPIO_Pin_3

#define GPIO_MAX_EN  GPIOB
#define Pin_MAX_EN   GPIO_Pin_8

static   xQueueHandle  __commuQueue;
#define  COMMU_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE + 64)

static void inline open(GPIO_TypeDef * port, uint16_t pin) {
	GPIO_ResetBits(port, pin);
}

static void inline close(GPIO_TypeDef * port, uint16_t pin) {
	GPIO_SetBits(port, pin);
}

static void inline openthenclose(GPIO_TypeDef * port, uint16_t pin) {
	open(port, pin);
	vTaskDelay(configTICK_RATE_HZ / 5);
	close(port, pin);
}

void comm_transmit_dir() {
	GPIO_SetBits(GPIO_MAX_EN, Pin_MAX_EN);
	vTaskDelay(configTICK_RATE_HZ / 100);
}

void comm_receive_dir() {
	vTaskDelay(configTICK_RATE_HZ / 100);
	GPIO_ResetBits(GPIO_MAX_EN, Pin_MAX_EN);
}

void USART2_Send_Byte(unsigned char byte){
    USART_SendData(USART2, byte); 
    while( USART_GetFlagStatus(USART2,USART_FLAG_TXE)!= SET);          
}

unsigned short __sum(const unsigned char *p, int size) {
	unsigned short ret = 0;
	while(size-- > 0) {
		ret += *p++;
	}
	return ret;
}

bool comm_pack_check_sum(const comm_pack *pack) {
	unsigned short sum = (pack->sum[0] << 8) + pack->sum[1];
	return sum == __sum((unsigned char *)pack, 3);
}

void comm_pack_cacl_sum(comm_pack *pack) {
	unsigned short sum = __sum((unsigned char *)pack, 3);
	pack->sum[0] = sum >> 8;
	pack->sum[1] = sum;
}

void comm_pack_send(const comm_pack *pack) {
	int i;
	const unsigned char *hextable = "0123456789ABCDEF";
	unsigned const char *p = (const unsigned char *)pack;

	comm_transmit_dir();	
	USART2_Send_Byte('#');
	for (i = 0; i < sizeof(*pack); ++i, ++p) {
		USART2_Send_Byte(hextable[*p >> 4]);
		USART2_Send_Byte(hextable[*p & 0x0F]);
	}
	USART2_Send_Byte('\r');
	comm_receive_dir();
}

static unsigned char code;

void coder(void){
	unsigned int data;
	data = GPIO_ReadInputData(GPIOB);
	code = (data & 0xF800) >> 11;
}

static void uart2_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  Pin_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_COM, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = Pin_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIO_COM, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin =  Pin_MAX_EN;       //485通信接收发送转换脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_MAX_EN, &GPIO_InitStructure);
	
	GPIO_ResetBits(GPIO_MAX_EN, Pin_MAX_EN);  //MAX485处于长接收状态
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
		
	USART_InitStructure.USART_BaudRate = BAUD;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);
}

void UART2_Send_Str(unsigned char *s, int size){	
  unsigned char i=0; 
	for(; i < size; i++) 
    {
       USART2_Send_Byte(s[i]); 
    }
	USART2_Send_Byte(0x00);
}

void USART2_IRQHandler(void) {
	static char Buffer[9];
	static char Index = 0;
	static char high4bit = 1;

	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		unsigned char dat = USART_ReceiveData(USART2);
		USART_SendData(USART1, dat);
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
		if (dat == '\r') {
			if (Index  > 0) {
				portBASE_TYPE xHigherPriorityTaskWoken;
				if (pdTRUE == xQueueSendFromISR(__commuQueue, Buffer, &xHigherPriorityTaskWoken)) {
					if (xHigherPriorityTaskWoken) {
						portYIELD();
					}
				}

				high4bit = 1;
				Index = 0;
			}
		} else if (dat >= '0' && dat <= '9') {
			if (high4bit) {
				Buffer[Index] = (dat- '0') << 4;
				high4bit = 0;
			} else {
				Buffer[Index] += (dat- '0');
				high4bit = 1;
				++Index;
			}
		} else if (dat >= 'A' && dat <= 'F') {
			if (high4bit) {
				Buffer[Index] = (dat- ('A' - 10)) << 4;
				high4bit = 0;
			} else {
				Buffer[Index] += (dat- ('A' - 10));
				high4bit = 1;
				++Index;
			}
		} else if (dat == '#') {
				high4bit = 1;
				Index = 0;
		}
	}
}

static char CHOOSE = 0;
static char FLAG = 0; 

void status(machineNumber msg){
	if(msg == FLAG){
		CHOOSE = msg;
	} 
}

void max_send(machineNumber msg){
	comm_pack pack;
	pack.addr = code;
	pack.mach = msg;
	if(CHOOSE == msg){
	   pack.act = 1;
	} else {
		 pack.act = 0;
	}
	comm_pack_cacl_sum(&pack);
	comm_transmit_dir();
	comm_pack_send(&pack);
  comm_receive_dir();
	CHOOSE = 0;
}

void communicat_handle(const comm_pack *msg){
 	if (!comm_pack_check_sum(msg)) return;
	printf("start.\n");
	if (msg->mach >= Number_1  &&  msg->mach <= Number_16) {

	}
}

static void __commuTask(void *parameter) {
	portBASE_TYPE rc;
	comm_pack msg;
	
	__commuQueue = xQueueCreate(4, sizeof(comm_pack));
	printf("MOTOR start.\n");

	for (;;) {
		rc = xQueueReceive(__commuQueue, &msg, portMAX_DELAY);
		printf("+2.\n");
		if (rc == pdTRUE) {

			}
	}
}

void commuInit(void) {
	uart2_Init();
	xTaskCreate(__commuTask, (signed portCHAR *) "COMMUNICATION", COMMU_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 0, NULL);
}
