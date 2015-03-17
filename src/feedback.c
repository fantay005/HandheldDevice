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

#define GPIO_RST   GPIOB
#define PIN_RST    GPIO_Pin_12

#define CLR_CS     (GPIO_SetBits(GPIO_SPI, SPI_NSS))
#define SET_CS     (GPIO_ResetBits(GPIO_SPI, SPI_NSS))

#define RST_H      (GPIO_SetBits(GPIO_RST, PIN_RST))
#define RST_L      (GPIO_ResetBits(GPIO_RST, PIN_RST))

#define MAXRLEN    18

void SPI_init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin =  PIN_RST;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_RST, &GPIO_InitStructure);
	
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
	
  //SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; //Ƭѡ���������
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
	
	tmp = ReadRC522Reg(TxControlReg);
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

void CalulateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData) {
    unsigned char i,n;
    ClearRegBit(DivIrqReg, 0x04);
    WriteRC522Reg(CommandReg, PCD_IDLE);
    SetRegBit(FIFOLevelReg, 0x80);
	
    for (i=0; i<len; i++) { 
			 WriteRC522Reg(FIFODataReg, *(pIndata+i));  
		}
		
    WriteRC522Reg(CommandReg, PCD_CALCCRC);
    i = 0xFF;
		
    do {
        n = ReadRC522Reg(DivIrqReg);
        i--;
    }while ((i!=0) && !(n&0x04));
		
    pOutData[0] = ReadRC522Reg(CRCResultRegL);
    pOutData[1] = ReadRC522Reg(CRCResultRegM);
}

char PcdReset(void) {
    RST_H;
    vTaskDelay(configTICK_RATE_HZ / 100);
    RST_L;
    vTaskDelay(configTICK_RATE_HZ / 100);
    RST_H;
	  vTaskDelay(configTICK_RATE_HZ / 10);

    WriteRC522Reg(CommandReg, PCD_RESETPHASE);
    
    WriteRC522Reg(ModeReg, 0x3D);            //��Mifare��ͨѶ��CRC��ʼֵ0x6363
    WriteRC522Reg(TReloadRegL, 30);           
    WriteRC522Reg(TReloadRegH, 0);
    WriteRC522Reg(TModeReg, 0x8D);
    WriteRC522Reg(TPrescalerReg, 0x3E);
    WriteRC522Reg(TxAutoReg, 0x40);
	
    return MI_OK;
}

char RC522ComM1( unsigned char Command, //RC522������
                 unsigned char *Send_Data, //ͨ��RC522���͵���Ƭ������
                 unsigned char Send_LenByte, //�������ݵ��ֽڳ���
                 unsigned char *Rec_Data, //���յ��Ŀ�Ƭ��������
                 unsigned int  *Rec_LenBit) {  //�������ݵ�λ����
									 
	  char status = MI_ERR;
    unsigned char irqEn   = 0x00; //ʹ���ж������͵�IRQ�ܽ�
    unsigned char waitFor = 0x00;
    unsigned char lastBits;
    unsigned char tmp;
    unsigned int count;
		int i;
    switch (Command)
    {
       case PCD_AUTHENT:
          irqEn   = 0x12; //ʹ�ܽ��ջ��ж�����ʹ����ж������͵�IRQ�ţ�����λ
          waitFor = 0x10; //��һ������������ֹ����λ��λ������λ
          break;
       case PCD_TRANSCEIVE:
          irqEn   = 0x77; //ʹ�ܳ��߱����ж��������ж������͵�IRQ�ܽţ�����λ
          waitFor = 0x30;  //��һ������������ֹ����λ��λ�ͽ�������⵽һ����Ч�������������λ��λ������λ
          break;
       default:
         break;
    }
   
    WriteRC522Reg(ComIEnReg,irqEn|0x80); //ʹ���ж������͵�IRQ�ܽ�
    ClearRegBit(ComIrqReg,0x80); //����ComIrqReg�ļĴ����е�����λ����
    WriteRC522Reg(CommandReg,PCD_IDLE); //ȡ����ǰ�����ģ���·����
    SetRegBit(FIFOLevelReg,0x80); //�ڲ�FIFO�������Ķ�дָ��ͼĴ���ErrReg��BufferOvfl��־���������
    
    for (i=0; i<Send_LenByte; i++) {   
			WriteRC522Reg(FIFODataReg, Send_Data[i]); //���ڲ�FIFO������д������
		}
    WriteRC522Reg(CommandReg, Command); //����������������ִ������
      
    if (Command == PCD_TRANSCEIVE) {  
				SetRegBit(BitFramingReg,0x80);  //�������ݷ���
		}
    
//    i = 600;//����ʱ��Ƶ�ʵ���������M1�����ȴ�ʱ��25ms
    count = 2000;
    do 
    {
         tmp = ReadRC522Reg(ComIrqReg); //���ж������־λ
         count--;
    }
    while ((count!=0) && !(tmp&0x01) && !(tmp&waitFor));  //��������0�����߶�ʱ��TimerValue�Ĵ�����ֵ�ݼ�Ϊ0�����ߺ�waitForλ��Ϊ0ʱ������
    ClearRegBit(BitFramingReg,0x80); //ֹͣ���ݴ���
	      
    if (count!=0) {    
			 if(!(ReadRC522Reg(ErrorReg)&0x1B)) {  //����Ƿ���FIFO�������������⵽λ��ͻ,RxModeReg�Ĵ�����RxCRCEn��λ��CRC�������
					 status = MI_OK;
					 if (tmp & irqEn & 0x01) {   
							status = MI_NOTAGERR;  
					 }
					 if (Command == PCD_TRANSCEIVE) {
							tmp = ReadRC522Reg(FIFOLevelReg); //�鿴FIFO�б�����ֽ���
							lastBits = ReadRC522Reg(ControlReg) & 0x07; //�����յ����һ���ֽڵ���Чλ��
						 
							if (lastBits){   
									*Rec_LenBit = (tmp-1)*8 + lastBits;  
							}
							else {   
									*Rec_LenBit = tmp*8; 
							}
							
							if (tmp == 0) {   
									tmp = 1;    
							}
							
							if (tmp > MAXRLEN) {  
									tmp = MAXRLEN;  
							}
							
							for (i=0; i<tmp; i++){   
									Rec_Data[i] = ReadRC522Reg(FIFODataReg);   
							}
					}
			 }
			 else {   
					status = MI_ERR;  
			 }       
   }
   
   SetRegBit(ControlReg,0x80);           // ֹͣ��ʱ������
   WriteRC522Reg(CommandReg,PCD_IDLE);  //ȡ����ǰ����
   return status;	
}

char RC522Request(unsigned char Seek_Card_Mode,unsigned char *Card_Type) { //Ѱ������
	 char status;  
   unsigned int  Len;
   unsigned char ComMRC522Buf[MAXRLEN]; 

   ClearRegBit(Status2Reg,0x08);
   WriteRC522Reg(BitFramingReg,0x07); //���巢�͵����һ���ֽڵ�λ��Ϊ7��ȱʡֵΪ0x00(���һ���ֽڵ�����λ������)
	
	 SetRegBit(TxControlReg,0x03); //���߿���
 
   ComMRC522Buf[0] = Seek_Card_Mode;

   status = RC522ComM1(PCD_TRANSCEIVE, ComMRC522Buf, 1, ComMRC522Buf, &Len);
	
	 if ((status == MI_OK) && (Len == 0x10)) {    
       *Card_Type = ComMRC522Buf[0];
       *(Card_Type+1) = ComMRC522Buf[1];
   }
   else { 
			 status = MI_ERR;   
	 }
   
   return status;
}

char RC522Anticollision(unsigned char *cdkey) { //��������Ƭ���кţ�4���ֽڣ�����ײ����
    char status;
    unsigned char i,snr_check=0;
    unsigned int  Len;
    unsigned char ComMRC522Buf[MAXRLEN]; 
    
    ClearRegBit(Status2Reg,0x08);
    WriteRC522Reg(BitFramingReg,0x00);
    ClearRegBit(CollReg,0x80);
 
    ComMRC522Buf[0] = PICC_ANTICOLL1;
    ComMRC522Buf[1] = 0x20;

    status = RC522ComM1(PCD_TRANSCEIVE,ComMRC522Buf,2,ComMRC522Buf,&Len);

    if (status == MI_OK) {
    	 for (i=0; i<4; i++){   
				 *(cdkey+i)  = ComMRC522Buf[i];
				 snr_check ^= ComMRC522Buf[i];
       }
			 if (snr_check != ComMRC522Buf[i]) {  
					 status = MI_ERR;    
			 }
		}
    
    SetRegBit(CollReg,0x80);
    return status;
}

char RC522Select(unsigned char *cdkey) {  //��������Ƭ���кţ�4���ֽڣ�         ��ѡ����
    char status;
    unsigned char i;
    unsigned int  Len;
    unsigned char ComMRC522Buf[MAXRLEN]; 
    
    ComMRC522Buf[0] = PICC_ANTICOLL1;
    ComMRC522Buf[1] = 0x70;
    ComMRC522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ComMRC522Buf[i+2] = *(cdkey+i);
    	ComMRC522Buf[6]  ^= *(cdkey+i);
    }
    CalulateCRC(ComMRC522Buf,7,&ComMRC522Buf[7]);
  
    ClearRegBit(Status2Reg,0x08);

    status = RC522ComM1(PCD_TRANSCEIVE, ComMRC522Buf, 9, ComMRC522Buf, &Len);
    
    if ((status == MI_OK) && (Len == 0x18)) {   
			 status = MI_OK;  
		}
    else {  
			 status = MI_ERR;   
		}

    return status;
}

char RC522AuthState(unsigned char auth_mode, //������֤ģʽ, 0x60 = ��֤A��Կ, 0x61 = ��֤B��Կ
									unsigned char addr, //���ַ
									unsigned char *pKey, //����
									unsigned char *cdkey) { //��Ƭ���кţ�4�ֽ�,                    ��֤�����뺯��
    char status;
    unsigned int  Len;
    unsigned char i,ComMRC522Buf[MAXRLEN]; 

    ComMRC522Buf[0] = auth_mode;
    ComMRC522Buf[1] = addr;
		
    for (i=0; i<6; i++) {  
			ComMRC522Buf[i+2] = *(pKey+i); 
		}
		
    for (i=0; i<6; i++) {
			ComMRC522Buf[i+8] = *(cdkey+i); 
		}

    status = RC522ComM1(PCD_AUTHENT, ComMRC522Buf, 12, ComMRC522Buf, &Len);
		
    if ((status != MI_OK) || (!(ReadRC522Reg(Status2Reg) & 0x08))) {  
			status = MI_ERR;
		}
    
    return status;
}

char RC522Read_data(unsigned char addr,unsigned char *pData) //��ȡM1��һ������ݣ�ADDRΪ���ַ
{
    char status;
    unsigned int  Len;
    unsigned char i,ComMRC522Buf[MAXRLEN]; 

    ComMRC522Buf[0] = PICC_READ;
    ComMRC522Buf[1] = addr;
    CalulateCRC(ComMRC522Buf,2,&ComMRC522Buf[2]);
   
    status = RC522ComM1(PCD_TRANSCEIVE, ComMRC522Buf, 4, ComMRC522Buf, &Len);
    if ((status == MI_OK) && (Len == 0x90)) {
        for (i=0; i<16; i++) {   
					 *(pData+i) = ComMRC522Buf[i];   
				}
    }
    else { 
				status = MI_ERR;  
		}
    
    return status;
}

char RC522Write(unsigned char addr,unsigned char *pData) { //д���ݵ�M1��һ��, ���ַΪaddr
    char status;
    unsigned int  Len;
    unsigned char i, ComMRC522Buf[MAXRLEN]; 
    
    ComMRC522Buf[0] = PICC_WRITE;
    ComMRC522Buf[1] = addr;
    CalulateCRC(ComMRC522Buf, 2, &ComMRC522Buf[2]);
 
    status = RC522ComM1(PCD_TRANSCEIVE, ComMRC522Buf, 4, ComMRC522Buf, &Len);

    if ((status != MI_OK) || (Len != 4) || ((ComMRC522Buf[0] & 0x0F) != 0x0A)) {  
				status = MI_ERR;  
		}
        
    if (status == MI_OK) {
        for (i=0; i<16; i++) {  
					ComMRC522Buf[i] = *(pData+i);
				}
				
        CalulateCRC(ComMRC522Buf,16,&ComMRC522Buf[16]);

        status = RC522ComM1(PCD_TRANSCEIVE, ComMRC522Buf, 18, ComMRC522Buf, &Len);
				
        if ((status != MI_OK) || (Len != 4) || ((ComMRC522Buf[0] & 0x0F) != 0x0A)) {
					status = MI_ERR; 
				}
    }
    
    return status;
}

