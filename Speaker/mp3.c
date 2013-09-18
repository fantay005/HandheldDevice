#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"

#define SPI_CS_SET       GPIO_SetBits(GPIOA, GPIO_Pin_4)
#define SPI_CS_RESET     GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define MP3_SYNC_SET	 GPIO_SetBits(GPIOB, GPIO_Pin_8)
#define MP3_SYNC_RESET	 GPIO_ResetBits(GPIOB, GPIO_Pin_8)
#define MP3_RST_SET	     GPIO_SetBits(GPIOC, GPIO_Pin_5)
#define MP3_RST_RESET	 GPIO_ResetBits(GPIOC, GPIO_Pin_5)
#define MP3_IRQ			 GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1)

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

int SPI_Readbyte(int data) {
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI1, data);
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(SPI1);
}

void MP3Transmit(int data) {
	MP3_SYNC_SET;
	SPI_CS_RESET;
	vTaskDelay(configTICK_RATE_HZ / 1000);
	SPI_Readbyte(data);
	vTaskDelay(configTICK_RATE_HZ / 1000);
	SPI_CS_SET;
	vTaskDelay(configTICK_RATE_HZ / 1000);
}

void MP3_Init(void) {
	MP3_RST_RESET;     //Ӳ����λ

	vTaskDelay(configTICK_RATE_HZ / 200);
	MP3Transmit(0xff);                   //��һ����Ч�ֽڼ���spi

	MP3_RST_SET;
	SPI_CS_SET;        //cs=0
	MP3_SYNC_SET;
	while (MP3_IRQ == 0);//�ȴ�DREQΪ��
	vTaskDelay(configTICK_RATE_HZ / 10);

	MP3_SYNC_SET;           //�����λ
	SPI_CS_RESET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x08);
	SPI_Readbyte(0x04);
	SPI_CS_SET;
	while (MP3_IRQ == 0);

	MP3_SYNC_SET;
	SPI_CS_RESET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x03);
	SPI_Readbyte(0x98);
	SPI_Readbyte(0x00);
	SPI_CS_SET;
	while (MP3_IRQ == 0);

	//********���ò�����  ������48k
	MP3_SYNC_SET;
	SPI_CS_RESET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x05);
	SPI_Readbyte(0xBB);
	SPI_Readbyte(0x81);
	SPI_CS_SET;
	while (MP3_IRQ == 0);

	//*********��������
	MP3_SYNC_SET;
	SPI_CS_RESET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x55);
	SPI_CS_SET;
	while (MP3_IRQ == 0);

	//*********��������
	MP3_SYNC_SET;
	SPI_CS_RESET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x0B);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	SPI_CS_SET;
	while (MP3_IRQ == 0);
	vTaskDelay(configTICK_RATE_HZ / 20);

	//*********����spi  ����4����Ч����
	SPI_CS_SET;
	MP3_SYNC_RESET;
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	MP3_SYNC_SET;

}

void MP3_Test(void) {
	MP3_RST_RESET;
	vTaskDelay(configTICK_RATE_HZ / 1000);
	MP3_RST_SET;
	vTaskDelay(configTICK_RATE_HZ / 20);
	SPI_Readbyte(0xff);
	vTaskDelay(configTICK_RATE_HZ / 10);

	//*********��������
	SPI_CS_RESET;
	MP3_SYNC_SET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x0B);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	SPI_CS_SET;
	while (MP3_IRQ == 0);
	vTaskDelay(configTICK_RATE_HZ / 20);

	SPI_CS_RESET;
	MP3_SYNC_SET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x0B);
	SPI_Readbyte(0xFE);
	SPI_Readbyte(0xFE);
	SPI_CS_SET;
	while (MP3_IRQ == 0);
	vTaskDelay(configTICK_RATE_HZ / 2);

}

void MP3_SCITest(int data) {           //SCI����

	MP3_RST_RESET;
	vTaskDelay(configTICK_RATE_HZ / 20);
	MP3Transmit(0xff);                   //��һ����Ч�ֽڼ���spi
	MP3_RST_SET;

	MP3_SYNC_SET;
	SPI_CS_RESET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x0B);
	SPI_Readbyte(data);
	SPI_Readbyte(data);
	SPI_CS_SET;
	while (MP3_IRQ == 0);
	vTaskDelay(configTICK_RATE_HZ / 2);

}

void MP3_SinusoidTest(void) {		 //���Ҳ�����

	MP3_RST_RESET;     //Ӳ����λ
	vTaskDelay(configTICK_RATE_HZ / 20);
	SPI_Readbyte(0xff);
	vTaskDelay(configTICK_RATE_HZ / 20);
	SPI_CS_SET;
	MP3_SYNC_SET;
	MP3_RST_SET;
	while (MP3_IRQ == 0);

	MP3_SYNC_SET;
	SPI_CS_RESET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x0B);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	SPI_CS_SET;
	while (MP3_IRQ == 0);

	MP3_SYNC_SET;
	SPI_CS_RESET;
	SPI_Readbyte(0x02);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x08);
	SPI_Readbyte(0x20);
	SPI_CS_SET;
	while (MP3_IRQ == 0);
	vTaskDelay(configTICK_RATE_HZ / 2);

	SPI_CS_SET;
	MP3_SYNC_RESET;
	SPI_Readbyte(0x53);
	SPI_Readbyte(0xEF);
	SPI_Readbyte(0x6E);
	SPI_Readbyte(0x30);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	MP3_SYNC_SET;
	vTaskDelay(configTICK_RATE_HZ * 3);

	SPI_CS_SET;
	MP3_SYNC_RESET;
	SPI_Readbyte(0x45);
	SPI_Readbyte(0x78);
	SPI_Readbyte(0x69);
	SPI_Readbyte(0x74);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	SPI_Readbyte(0x00);
	MP3_SYNC_SET;
	vTaskDelay(configTICK_RATE_HZ / 2);

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

