#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "misc.h"


#define COMx         USART2
#define COMx_IRQn    USART2_IRQn

#define Pin_COMx_TX  GPIO_Pin_2
#define Pin_COMx_RX  GPIO_Pin_3
#define GPIO_COMx    GPIOA

static void __zigbeeInitUsart(int baud) {
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(COMx, &USART_InitStructure);
	USART_ITConfig(COMx, USART_IT_RXNE, ENABLE);
	USART_Cmd(COMx, ENABLE);
}

static void __gsmInitHardware(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  Pin_COMx_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_COMx, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = Pin_COMx_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIO_COMx, &GPIO_InitStructure);				   //ZigBee模块的串口

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);				    //ZigBee模块的配置脚

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = COMx_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


static unsigned char bufferIndex;
static unsigned char buffer[255];
static unsigned char LenZIGB;

void USART2_IRQHandler(void) {
	unsigned char data;
	char param1, param2;
	
	if (USART_GetITStatus(COMx, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(COMx);
//	USART_SendData(USART1, data);
	USART_ClearITPendingBit(COMx, USART_IT_RXNE);

	if(((data >= '0') && (data <= 'F')) || (data == 0x03)){
		buffer[bufferIndex++] = data;
		if(bufferIndex == 9) {
			if (buffer[7] > '9') {
				param1 = buffer[7] - '7';
			} else {
				param1 = buffer[7] - '0';
			}
			
			if (buffer[8] > '9') {
				param2 = buffer[8] - '7';
			} else {
				param2 = buffer[8] - '0';
			}
			
			LenZIGB = (param1 << 4) + param2;
		}
	} else {
		bufferIndex = 0;
		LenZIGB = 0;
	}

	
	if ((bufferIndex == (LenZIGB + 12)) && (data == 0x03)){
		ZigbTaskMsg *msg;
		portBASE_TYPE xHigherPriorityTaskWoken;
		buffer[bufferIndex++] = 0;
		msg = __ZigbCreateMessage(TYPE_IOT_RECIEVE_DATA, (const char *)buffer, bufferIndex);		
		if (pdTRUE == xQueueSendFromISR(__ZigbeeQueue, &msg, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				portYIELD();
			}
		}
		bufferIndex = 0;
		LenZIGB = 0;
	} else if(bufferIndex > (LenZIGB + 12)){
		bufferIndex = 0;
		LenZIGB = 0;
	} else if (data == 0x02){
		bufferIndex = 0;
		buffer[bufferIndex++] = data;
	}
}
