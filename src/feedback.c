#include <stdio.h>
#include "FreeRTOS.h" 
#include "queue.h" 
#include "task.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "commu.h"

/////////////////////////////////////////////////////////////////////
//MF522������
/////////////////////////////////////////////////////////////////////
#define PCD_IDLE              0x00               //ȡ����ǰ����
#define PCD_AUTHENT           0x0E               //��֤��Կ
#define PCD_RECEIVE           0x08               //��������
#define PCD_TRANSMIT          0x04               //��������
#define PCD_TRANSCEIVE        0x0C               //���Ͳ���������
#define PCD_RESETPHASE        0x0F               //��λ
#define PCD_CALCCRC           0x03               //CRC����

/////////////////////////////////////////////////////////////////////
//Mifare_One��Ƭ������
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL           0x26               //Ѱ��������δ��������״̬
#define PICC_REQALL           0x52               //Ѱ��������ȫ����
#define PICC_ANTICOLL1        0x93               //����ײ
#define PICC_ANTICOLL2        0x95               //����ײ
#define PICC_AUTHENT1A        0x60               //��֤A��Կ
#define PICC_AUTHENT1B        0x61               //��֤B��Կ
#define PICC_READ             0x30               //����
#define PICC_WRITE            0xA0               //д��
#define PICC_DECREMENT        0xC0               //�ۿ�
#define PICC_INCREMENT        0xC1               //��ֵ
#define PICC_RESTORE          0xC2               //�������ݵ�������
#define PICC_TRANSFER         0xB0               //���滺����������
#define PICC_HALT             0x50               //����

/////////////////////////////////////////////////////////////////////
//MF522 FIFO���ȶ���
/////////////////////////////////////////////////////////////////////
#define DEF_FIFO_LENGTH       64                 //FIFO size=64byte

/////////////////////////////////////////////////////////////////////
//MF522�Ĵ�������
/////////////////////////////////////////////////////////////////////
// PAGE 0
#define     RFU00                 0x00    
#define     CommandReg            0x01    
#define     ComIEnReg             0x02    
#define     DivlEnReg             0x03    
#define     ComIrqReg             0x04    
#define     DivIrqReg             0x05
#define     ErrorReg              0x06    
#define     Status1Reg            0x07    
#define     Status2Reg            0x08    
#define     FIFODataReg           0x09
#define     FIFOLevelReg          0x0A
#define     WaterLevelReg         0x0B
#define     ControlReg            0x0C
#define     BitFramingReg         0x0D
#define     CollReg               0x0E
#define     RFU0F                 0x0F
// PAGE 1     
#define     RFU10                 0x10
#define     ModeReg               0x11
#define     TxModeReg             0x12
#define     RxModeReg             0x13
#define     TxControlReg          0x14
#define     TxAutoReg             0x15
#define     TxSelReg              0x16
#define     RxSelReg              0x17
#define     RxThresholdReg        0x18
#define     DemodReg              0x19
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F
// PAGE 2    
#define     RFU20                 0x20  
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E
#define     TCounterValueRegL     0x2F
// PAGE 3      
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39  
#define     TestDAC2Reg           0x3A   
#define     TestADCReg            0x3B   
#define     RFU3C                 0x3C   
#define     RFU3D                 0x3D   
#define     RFU3E                 0x3E   
#define     RFU3F		  0x3F

/////////////////////////////////////////////////////////////////////
//��MF522ͨѶʱ���صĴ������
/////////////////////////////////////////////////////////////////////
#define MI_OK                          0
#define MI_NOTAGERR                    (-1)
#define MI_ERR                         (-2)

static xQueueHandle __queue;

#define  FEEDBACK_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 64)

#define GPIO_SPI   GPIOA
#define SPI_NSS    GPIO_Pin_4 
#define SPI_SCK    GPIO_Pin_5
#define SPI_MISO   GPIO_Pin_6
#define SPI_MOSI   GPIO_Pin_7

#define GPIO_LOCK  GPIOB
#define PIN_LOCK   GPIO_Pin_10

#define GPIO_RST   GPIO
#define PIN_RST    GPIO_Pin_

#define CLR_CS     (GPIO_SetBits(GPIO_SPI, SPI_NSS))
#define SET_CS     (GPIO_ResetBits(GPIO_SPI, SPI_NSS))

void SPI_init(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin =  SPI_NSS;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_SPI, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin =  SPI_SCK;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_SPI, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin =  SPI_MISO;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIO_SPI, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin =  SPI_NSS;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_SPI, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_LOCK;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_LOCK, &GPIO_InitStructure);
	
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //ȫ˫��ģʽ
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master; //SPI��ģʽ
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; //8bit����

	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; //CLK����ʱΪ�͵�ƽ
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge; //CLK�����ز�������Ϊ�������ǵ�һ�����ض���������Ҳ�������Ϊ��һ�����ز���
	
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; //Ƭѡ���������
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; //SPIƵ��
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; //��λ��ǰ
	SPI_InitStructure.SPI_CRCPolynomial = 7; //crc7��stm32spi��Ӳ��ecc
	SPI_Init(SPI1, &SPI_InitStructure);
	
	SPI_Cmd(SPI1, ENABLE);
}

uint8_t SPIWriteByte(uint8_t TxData) {
	while((SPI1->SR&1<<1)==0);//�ȴ���������
	SPI1->DR=TxData;	 	  //����һ��byte
	while((SPI1->SR&1<<0)==0);//�ȴ�������һ��byte
	return SPI1->DR;          //�����յ�������
}

uint8_t inline SPIReadByte(void) {
	SPIWriteByte(0);
}

uint8_t ReadRC522Reg(uint8_t Address) {
	uint8_t chAddr;
	uint8_t Result;
	
	SET_CS;
	chAddr = ((Address << 1) & 0x7f) | 0x80;
	SPIWriteByte(chAddr);
	Result = SPIReadByte();
	CLR_CS;
	
	return Result;
}

void WriteRC522Reg(uint8_t Address, uint8_t Value) {	
	uint8_t chAddr;
	uint8_t Result;
	
	SET_CS;
	chAddr = (Address << 1) & 0x7f;
	SPIWriteByte(chAddr);
	CLR_CS;
}

void ClearRegBit(uint8_t reg, uint8_t mask) {
	char tmp;
	
	tmp = ReadRC522Reg(reg);
	WriteRC522Reg(reg, tmp & ~mask);
}

void SetRegBit(uint8_t reg, uint8_t mask) {
	char tmp;
	
	tmp = ReadRC522Reg(reg);
	WriteRC522Reg(reg, tmp | mask);

}

void AntennaOff(void){
	ClearRegBit(TxControlReg, 0x03);
}

void AntennaOn(void){
	char tmp;
	
	tmp = ReadRC522Reg(reg);
	if(!(tmp & 0x03)){
		SetRegBit(TxControlReg, 0x03);
	}
}

void RC522Config(void){
	
	ClearRegBit(Status2Reg, 0x08);
	
	WriteRC522Reg(ModeReg,0x3D); //����CRCЭ������CalCRC�����Ԥ��ֵ��ȱʡֵΪ0x3F
	
	WriteRC522Reg(RxSelReg,0x86); //���ݷ��ͺ󣬽��������ӳ�6��λʱ�ӣ�ȱʡֵΪ0x84
	
	WriteRC522Reg(RFCfgReg,0x7F); //�������������Ϊ48DB��ȱʡֵΪ0x4F(33DB)
	
	WriteRC522Reg(TModeReg,0x8D); //��ʱ����ͨѶЭ��Ӱ�죬�ڷ��ͽ������Զ��������ڽ��յ���һ������λ���Զ�ֹͣ
	
	WriteRC522Reg(TPrescalerReg,0x3E); //���嶨ʱ����Ƶ��Ϊ6.78MHZ/0xD3E = 2KHZ
	
	WriteRC522Reg(TReloadRegL,30); /*��һ������ʱ�����ʱ��TReload��ֵװ�붨ʱ����
	                                 ֻ���´������¼�����ʱ���Ĵ��������ݲŻ�ı䣬����Ӱ�춨ʱ��*/
	
	
}
