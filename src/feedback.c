#include <stdio.h>
#include "FreeRTOS.h" 
#include "queue.h" 
#include "task.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "commu.h"

/////////////////////////////////////////////////////////////////////
//MF522命令字
/////////////////////////////////////////////////////////////////////
#define PCD_IDLE              0x00               //取消当前命令
#define PCD_AUTHENT           0x0E               //验证密钥
#define PCD_RECEIVE           0x08               //接收数据
#define PCD_TRANSMIT          0x04               //发送数据
#define PCD_TRANSCEIVE        0x0C               //发送并接收数据
#define PCD_RESETPHASE        0x0F               //复位
#define PCD_CALCCRC           0x03               //CRC计算

/////////////////////////////////////////////////////////////////////
//Mifare_One卡片命令字
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL           0x26               //寻天线区内未进入休眠状态
#define PICC_REQALL           0x52               //寻天线区内全部卡
#define PICC_ANTICOLL1        0x93               //防冲撞
#define PICC_ANTICOLL2        0x95               //防冲撞
#define PICC_AUTHENT1A        0x60               //验证A密钥
#define PICC_AUTHENT1B        0x61               //验证B密钥
#define PICC_READ             0x30               //读块
#define PICC_WRITE            0xA0               //写块
#define PICC_DECREMENT        0xC0               //扣款
#define PICC_INCREMENT        0xC1               //充值
#define PICC_RESTORE          0xC2               //调块数据到缓冲区
#define PICC_TRANSFER         0xB0               //保存缓冲区中数据
#define PICC_HALT             0x50               //休眠

/////////////////////////////////////////////////////////////////////
//MF522 FIFO长度定义
/////////////////////////////////////////////////////////////////////
#define DEF_FIFO_LENGTH       64                 //FIFO size=64byte

/////////////////////////////////////////////////////////////////////
//MF522寄存器定义
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
//和MF522通讯时返回的错误代码
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
	
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //全双工模式
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master; //SPI主模式
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; //8bit数据

	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; //CLK空闲时为低电平
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge; //CLK上升沿采样，因为上升沿是第一个边沿动作，所以也可以理解为第一个边沿采样
	
  //SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; //片选用软件控制
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; //SPI频率
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; //高位在前
	SPI_InitStructure.SPI_CRCPolynomial = 7; //crc7，stm32spi带硬件ecc
	SPI_Init(SPI1, &SPI_InitStructure);
	
	SPI_Cmd(SPI1, ENABLE);
}

uint8_t SPIWriteByte(uint8_t TxData) {
	while((SPI1->SR&1<<1)==0);//等待发送区空
	SPI1->DR=TxData;	 	  //发送一个byte
	while((SPI1->SR&1<<0)==0);//等待接收完一个byte
	return SPI1->DR;          //返回收到的数据
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
	
	WriteRC522Reg(ModeReg,0x3D); //定义CRC协处理器CalCRC命令的预置值，缺省值为0x3F
	
	WriteRC522Reg(RxSelReg,0x86); //数据发送后，接收器会延迟6个位时钟，缺省值为0x84
	
	WriteRC522Reg(RFCfgReg,0x7F); //定义接收器增益为48DB，缺省值为0x4F(33DB)
	
	WriteRC522Reg(TModeReg,0x8D); //定时器受通讯协议影响，在发送结束后自动启动，在接收到第一个数据位后自动停止
	
	WriteRC522Reg(TPrescalerReg,0x3E); //定义定时器的频率为6.78MHZ/0xD3E = 2KHZ
	
	WriteRC522Reg(TReloadRegL,30); /*当一个启动时间出现时，TReload的值装入定时器。
	                                 只有下次启动事件出现时，寄存器的内容才会改变，进而影响定时器*/
	
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
    
    WriteRC522Reg(ModeReg, 0x3D);            //和Mifare卡通讯，CRC初始值0x6363
    WriteRC522Reg(TReloadRegL, 30);           
    WriteRC522Reg(TReloadRegH, 0);
    WriteRC522Reg(TModeReg, 0x8D);
    WriteRC522Reg(TPrescalerReg, 0x3E);
    WriteRC522Reg(TxAutoReg, 0x40);
	
    return MI_OK;
}

char RC522ComM1( unsigned char Command, //RC522命令字
                 unsigned char *Send_Data, //通过RC522发送到卡片的数据
                 unsigned char Send_LenByte, //发送数据的字节长度
                 unsigned char *Rec_Data, //接收到的卡片返回数据
                 unsigned int  *Rec_LenBit) {  //返回数据的位长度
									 
	  char status = MI_ERR;
    unsigned char irqEn   = 0x00; //使能中断请求发送到IRQ管脚
    unsigned char waitFor = 0x00;
    unsigned char lastBits;
    unsigned char tmp;
    unsigned int count;
		int i;
    switch (Command)
    {
       case PCD_AUTHENT:
          irqEn   = 0x12; //使能接收机中断请求和错误中断请求发送到IRQ脚，控制位
          waitFor = 0x10; //当一个命令自身终止，该位置位，控制位
          break;
       case PCD_TRANSCEIVE:
          irqEn   = 0x77; //使能除高报警中断外所有中断请求发送到IRQ管脚，控制位
          waitFor = 0x30;  //当一个命令自身终止，该位置位和接收器检测到一个有效数据流结束后该位置位，控制位
          break;
       default:
         break;
    }
   
    WriteRC522Reg(ComIEnReg,irqEn|0x80); //使能中断请求发送到IRQ管脚
    ClearRegBit(ComIrqReg,0x80); //定义ComIrqReg的寄存器中的屏蔽位清零
    WriteRC522Reg(CommandReg,PCD_IDLE); //取消当前命令，打开模拟电路部分
    SetRegBit(FIFOLevelReg,0x80); //内部FIFO缓冲区的读写指针和寄存器ErrReg的BufferOvfl标志立即被清除
    
    for (i=0; i<Send_LenByte; i++) {   
			WriteRC522Reg(FIFODataReg, Send_Data[i]); //向内部FIFO缓冲区写入数据
		}
    WriteRC522Reg(CommandReg, Command); //根据命令字来激活执行命令
      
    if (Command == PCD_TRANSCEIVE) {  
				SetRegBit(BitFramingReg,0x80);  //启动数据发送
		}
    
//    i = 600;//根据时钟频率调整，操作M1卡最大等待时间25ms
    count = 2000;
    do 
    {
         tmp = ReadRC522Reg(ComIrqReg); //读中断请求标志位
         count--;
    }
    while ((count!=0) && !(tmp&0x01) && !(tmp&waitFor));  //当计数到0，或者定时器TimerValue寄存器的值递减为0，或者和waitFor位与为0时，跳出
    ClearRegBit(BitFramingReg,0x80); //停止数据传送
	      
    if (count!=0) {    
			 if(!(ReadRC522Reg(ErrorReg)&0x1B)) {  //检测是否有FIFO缓冲区溢出，检测到位冲突,RxModeReg寄存器的RxCRCEn置位且CRC计算出错
					 status = MI_OK;
					 if (tmp & irqEn & 0x01) {   
							status = MI_NOTAGERR;  
					 }
					 if (Command == PCD_TRANSCEIVE) {
							tmp = ReadRC522Reg(FIFOLevelReg); //查看FIFO中保存的字节数
							lastBits = ReadRC522Reg(ControlReg) & 0x07; //检测接收到最后一个字节的有效位数
						 
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
   
   SetRegBit(ControlReg,0x80);           // 停止定时器运行
   WriteRC522Reg(CommandReg,PCD_IDLE);  //取消当前命令
   return status;	
}

char RC522Request(unsigned char Seek_Card_Mode,unsigned char *Card_Type) { //寻卡函数
	 char status;  
   unsigned int  Len;
   unsigned char ComMRC522Buf[MAXRLEN]; 

   ClearRegBit(Status2Reg,0x08);
   WriteRC522Reg(BitFramingReg,0x07); //定义发送的最后一个字节的位数为7，缺省值为0x00(最后一个字节的所有位都发送)
	
	 SetRegBit(TxControlReg,0x03); //天线开启
 
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

char RC522Anticollision(unsigned char *cdkey) { //参数：卡片序列号，4个字节，防碰撞函数
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

char RC522Select(unsigned char *cdkey) {  //参数：卡片序列号，4个字节，         卡选择函数
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

char RC522AuthState(unsigned char auth_mode, //密码验证模式, 0x60 = 验证A密钥, 0x61 = 验证B密钥
									unsigned char addr, //块地址
									unsigned char *pKey, //密码
									unsigned char *cdkey) { //卡片序列号，4字节,                    验证卡密码函数
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

char RC522Read_data(unsigned char addr,unsigned char *pData) //读取M1卡一块的数据，ADDR为块地址
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

char RC522Write(unsigned char addr,unsigned char *pData) { //写数据到M1卡一块, 块地址为addr
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

