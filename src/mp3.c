#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"



static void initHardware() {
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	            //CS

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);				//SCK,MOSI

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);				 //MISO

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //˫��ģʽ
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master; //SPI��ģʽ
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; //8bit����
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; //CLK����ʱΪ�͵�ƽ
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; //CLK�����ز�������Ϊ�������ǵ�һ�����ض���������Ҳ�������Ϊ��һ�����ز���
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; //Ƭѡ���������
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; //SPIƵ��
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; //��λ��ǰ
	SPI_InitStructure.SPI_CRCPolynomial = 7; //crc7��stm32spi��Ӳ��ecc
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);		  //MP3_IRQ

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);		   //MP3_SYNC

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);		   //MP3_RST

}




void vMP3(void *parameter) {
	initHardware();

	printf("Gsm start\n");
	for (;;) {
		// wait uart ?
		printf("Gsm: loop again\n");
		vTaskDelay(configTICK_RATE_HZ * 3);
	}
}

