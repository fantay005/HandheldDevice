/**
  ******************************************************************************
  * @file    stm3210e_eval_lcd.c
  * @author  ARMJISHU Application Team
  * @version V4.5.0
  * @date    07-March-2011
  * @brief   This file includes the LCD driver for AM-240320L8TNQW00H 
  *          (LCD_ILI9320) and AM-240320LDTNQW00H (LCD_SPFD5408B) Liquid Crystal
  *          Display Module of STM3210E-EVAL board.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************  
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_fsmc.h"
#include "ili9320.h"
#include "ili9320_font.h"
#include "stm32f10x_gpio.h"
#include <stdio.h>
#include "font_dot_array.h"
#include "zklib.h"

#define Font32Color   MAGENTA
#define Font16Color   BLACK


static unsigned char arrayBuffer[128];

static char Exist32Font;

u16 DeviceCode;

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  vu16 LCD_REG;
  vu16 LCD_RAM;
} LCD_TypeDef;

/* LCD is connected to the FSMC_Bank1_NOR/SRAM4 and NE4 is used as ship select signal */
#define LCD_BASE   ((u32)(0x60000000 | 0x0C000000))
#define LCD         ((LCD_TypeDef *) LCD_BASE)

//#define SetCs  
//#define ClrCs  

#define SetCs  GPIO_SetBits(GPIOG, GPIO_Pin_12);
#define ClrCs  GPIO_ResetBits(GPIOG, GPIO_Pin_12);

//#define SetCs  GPIO_SetBits(GPIOB, GPIO_Pin_8);
//#define ClrCs  GPIO_ResetBits(GPIOB, GPIO_Pin_8);

////////////////////////////////////////////////////////////////////////////////////////
/*******************************************************************************
* Function Name  : LCD_CtrlLinesConfig
* Description    : Configures LCD Control lines (FSMC Pins) in alternate function
                   Push-Pull mode.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_CtrlLinesConfig(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable FSMC, GPIOD, GPIOE, GPIOF, GPIOG and AFIO clocks */

  /* Set PD.00(D2), PD.01(D3), PD.04(NOE), PD.05(NWE), PD.08(D13), PD.09(D14),
     PD.10(D15), PD.14(D0), PD.15(D1) as alternate 
     function push pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 |
                                GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | 
                                GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* Set PE.07(D4), PE.08(D5), PE.09(D6), PE.10(D7), PE.11(D8), PE.12(D9), PE.13(D10),
     PE.14(D11), PE.15(D12) as alternate function push pull */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | 
                                GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | 
                                GPIO_Pin_15;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  /* Set PC15(A0 (RS)) as alternate function push pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_Init(GPIOF, &GPIO_InitStructure);

  /* Set PG.12(NE4 (LCD/CS)) as alternate function push pull - CE3(LCD /CS) */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_Init(GPIOG, &GPIO_InitStructure);

  /*FSMC A21��A22���Ի������츴�����*/ 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6; 
  GPIO_Init(GPIOE, &GPIO_InitStructure); 

  /* Lcd_Light_Control */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  //Lcd_Light_OFF  
  Lcd_Light_ON

  /* LEDs pins configuration */
#if 0
  /* Set PD7 to disable NAND */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
	
  GPIO_SetBits(GPIOD, GPIO_Pin_7);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOG, &GPIO_InitStructure);
	
  GPIO_SetBits(GPIOG, GPIO_Pin_9 | GPIO_Pin_10);

  /* Set PC7 to disable NAND */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	
  GPIO_SetBits(GPIOC, GPIO_Pin_7);
#endif
}

/*******************************************************************************
* Function Name  : LCD_FSMCConfig
* Description    : Configures the Parallel interface (FSMC) for LCD(Parallel mode)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_FSMCConfig(void)
{
  FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
  FSMC_NORSRAMTimingInitTypeDef  Timing_read, Timing_write;

/*-- FSMC Configuration ------------------------------------------------------*/
/*----------------------- SRAM Bank 4 ----------------------------------------*/
  /* FSMC_Bank1_NORSRAM4 configuration */
  Timing_read.FSMC_AddressSetupTime = 6;             
  Timing_read.FSMC_AddressHoldTime = 0;  
  Timing_read.FSMC_DataSetupTime = 6; 
  Timing_read.FSMC_BusTurnAroundDuration = 0;
  Timing_read.FSMC_CLKDivision = 0;
  Timing_read.FSMC_DataLatency = 0;
  Timing_read.FSMC_AccessMode = FSMC_AccessMode_A;    

  Timing_write.FSMC_AddressSetupTime = 2;             
  Timing_write.FSMC_AddressHoldTime = 0;  
  Timing_write.FSMC_DataSetupTime = 2; 
  Timing_write.FSMC_BusTurnAroundDuration = 0;
  Timing_write.FSMC_CLKDivision = 0;
  Timing_write.FSMC_DataLatency = 0;  
  Timing_write.FSMC_AccessMode = FSMC_AccessMode_A; 
   
  /* Color LCD configuration ------------------------------------
     LCD configured as follow:
        - Data/Address MUX = Disable
        - Memory Type = SRAM
        - Data Width = 16bit
        - Write Operation = Enable
        - Extended Mode = Enable
        - Asynchronous Wait = Disable */
  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
  FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &Timing_read;
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &Timing_write;

  FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  

  /* BANK 4 (of NOR/SRAM Bank 1~4) is enabled */
  FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);
     
}


void LCD_X_Init(void)
{
 /* Configure the LCD Control pins --------------------------------------------*/
  LCD_CtrlLinesConfig();

/* Configure the FSMC Parallel interface -------------------------------------*/
  LCD_FSMCConfig();
}


/*******************************************************************************
* Function Name  : LCD_WriteReg
* Description    : Writes to the selected LCD register.
* Input          : - LCD_Reg: address of the selected register.
*                  - LCD_RegValue: value to write to the selected register.
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_WriteReg(u16 LCD_Reg,u16 LCD_RegValue)
{
  /* Write 16-bit Index, then Write Reg */
  ClrCs
  LCD->LCD_REG = LCD_Reg;
  /* Write 16-bit Reg */
  LCD->LCD_RAM = LCD_RegValue;
  SetCs
}

/*******************************************************************************
* Function Name  : LCD_ReadReg
* Description    : Reads the selected LCD Register.
* Input          : None
* Output         : None
* Return         : LCD Register Value.
*******************************************************************************/
u16 LCD_ReadReg(u8 LCD_Reg)
{
  u16 data;
  /* Write 16-bit Index (then Read Reg) */
  ClrCs
  //LCD->LCD_REG = LCD_Reg;
  data = LCD->LCD_RAM; 
    SetCs
     return    data;

  /* Read 16-bit Reg */
  //return (LCD->LCD_RAM);
}

u16 LCD_ReadSta(void)
{
  u16 data;
  /* Write 16-bit Index, then Write Reg */
  ClrCs
  data = LCD->LCD_REG;
  SetCs
  return    data;
}

void LCD_WriteCommand(u16 LCD_RegValue)
{
  /* Write 16-bit Index, then Write Reg */
  ClrCs
  LCD->LCD_REG = LCD_RegValue;
  SetCs
}

/*******************************************************************************
* Function Name  : LCD_WriteRAM_Prepare
* Description    : Prepare to write to the LCD RAM.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_WriteRAM_Prepare(void)
{
  ClrCs
  LCD->LCD_REG = R34;
  SetCs
}

/*******************************************************************************
* Function Name  : LCD_WriteRAM
* Description    : Writes to the LCD RAM.
* Input          : - RGB_Code: the pixel color in RGB mode (5-6-5).
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_WriteRAM(u16 RGB_Code)					 
{
  ClrCs
  /* Write 16-bit GRAM Reg */
  LCD->LCD_RAM = RGB_Code;
  SetCs
}

/*******************************************************************************
* Function Name  : LCD_ReadRAM
* Description    : Reads the LCD RAM.
* Input          : None
* Output         : None
* Return         : LCD RAM Value.
*******************************************************************************/
u16 LCD_ReadRAM(void)
{
  u16 dummy;
  u16 data;
  /* Write 16-bit Index (then Read Reg) */
  ClrCs
  LCD->LCD_REG = R34; /* Select GRAM Reg */
  /* Read 16-bit Reg */
  dummy = LCD->LCD_RAM; 
  dummy++;
  data = LCD->LCD_RAM; 
  SetCs
  return    data;
  //return LCD->LCD_RAM;
}

/*******************************************************************************
* Function Name  : LCD_SetCursor
* Description    : Sets the cursor position.
* Input          : - Xpos: specifies the X position.
*                  - Ypos: specifies the Y position. 
* Output         : None
* Return         : None
*******************************************************************************/


void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
  LCD_WriteReg(0x06,Ypos>>8);
  LCD_WriteReg(0x07,Ypos);
  
  LCD_WriteReg(0x02,Xpos>>8);
  LCD_WriteReg(0x03,Xpos);  
}			 

void Delay_nms(int n)
{
  
  u32 f=n,k;
  for (; f!=0; f--)
  {
    for(k=0xFFF; k!=0; k--);
  }
  
}

void Delay(u32 nCount)
{
 u32 TimingDelay; 
 while(nCount--)
   {
    for(TimingDelay=0;TimingDelay<10000;TimingDelay++);
   }
}

/**
  * @brief  Draws a chinacharacter on LCD.
  * @param  Xpos: the Line where to display the character shape.
  * @param  Ypos: start column address.
  * @param  c: pointer to the character data.
  * @retval None
  */

void LCD_DrawChinaChar24x24(u8 Xpos, u16 Ypos, const u8 *c,u16 charColor,u16 bkColor)
{
  u32 index = 0, i = 0, j = 0;
  u8 Xaddress = 0;
   
  Xaddress = Xpos;

  ili9320_SetCursor(Ypos,Xaddress);

  for(index = 0; index < 24; index++)
  {
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    for(j = 0; j < 3; j++)
    {
        for(i = 0; i < 8; i++)
        {
            if((c[3*index + j] & (0x80 >> i)) == 0x00)
                LCD_WriteRAM(bkColor);
            else
                LCD_WriteRAM(charColor);
        }   
    }
    Xaddress++;
    ili9320_SetCursor(Ypos, Xaddress);
  }
}


void LCD_DrawChinaChar16x16(u16 Xpos, u16 Ypos, const u8 *c,u16 charColor,u16 bkColor)
{
  u32 index = 0, i = 0, j = 0;
  u8 Xaddress = 0;
   
  Xaddress = Xpos;

  ili9320_SetCursor(Ypos,Xaddress);

  for(index = 0; index < 24; index++)
  {
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    for(j = 0; j < 2; j++)
    {
        for(i = 0; i < 8; i++)
        {
            if((c[3*index + j] & (0x80 >> i)) == 0x00)
                LCD_WriteRAM(bkColor);
            else
                LCD_WriteRAM(charColor);
        }   
    }
    Xaddress++;
    ili9320_SetCursor(Ypos, Xaddress);
  }
}

void LCD_DrawChinaChar32x32(u16 Xpos, u16 Ypos, const u8 *c,u16 charColor,u16 bkColor)
{
  u32 index = 0, i = 0, j = 0;
  u8 Xaddress = 0;
   
  Xaddress = Xpos;

  ili9320_SetCursor(Ypos,Xaddress);

  for(index = 0; index < 24; index++)
  {
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    for(j = 0; j < 4; j++)
    {
        for(i = 0; i < 8; i++)
        {
            if((c[3*index + j] & (0x80 >> i)) == 0x00)
                LCD_WriteRAM(bkColor);
            else
                LCD_WriteRAM(charColor);
        }   
    }
    Xaddress++;
    ili9320_SetCursor(Ypos, Xaddress);
  }
}

const unsigned char *LedDisplayGB2312String(int x, int y, const unsigned char *gbString, unsigned char  DotMatrix, int charColor, int bkColor) {
	if (!FontDotArrayFetchLock()) {
		return gbString;
	}

	while (*gbString) {
		if (isAsciiStart(*gbString)) {
			
			if(DotMatrix == 16){
				FontDotArrayFetchASCII_16(arrayBuffer, *gbString);
				LCD_DrawChinaChar16x16(x, y, (const u8 *)arrayBuffer, charColor, bkColor);
				x += BYTES_WIDTH_PER_FONT_ASCII_16X8;
			} else if(DotMatrix == 32){
				FontDotArrayFetchASCII_32(arrayBuffer, *gbString);
				LCD_DrawChinaChar32x32(x, y, (const u8 *)arrayBuffer, charColor, bkColor);
				x += BYTES_WIDTH_PER_FONT_ASCII_32X16;
			}
				++gbString;

		} else if (isGB2312Start(*gbString)) {
			int code = (*gbString++) << 8;
			if (!isGB2312Start(*gbString)) {
				goto __exit;
			}
			code += *gbString++;
			if(DotMatrix == 16){
				FontDotArrayFetchGB_16(arrayBuffer, code);
				LCD_DrawChinaChar16x16(x, y, (const u8 *)arrayBuffer, charColor, bkColor);		
				x += BYTES_WIDTH_PER_FONT_GB_16X16;
			} else if (DotMatrix == 32){
				FontDotArrayFetchGB_32(arrayBuffer, code);
				LCD_DrawChinaChar32x32(x, y, (const u8 *)arrayBuffer, charColor, bkColor);		
				x += BYTES_WIDTH_PER_FONT_GB_32X32;
			}
				
		} else if (isUnicodeStart(*gbString)) {
			int code = (*gbString++) << 8;
			code += *gbString++;
		
			if(DotMatrix == 16){
				FontDotArrayFetchUCS_16(arrayBuffer, code);
				LCD_DrawChinaChar16x16(x, y, (const u8 *)arrayBuffer, charColor, bkColor);
				x += BYTES_WIDTH_PER_FONT_GB_16X16;
			} else if (DotMatrix == 32){
				FontDotArrayFetchUCS_32(arrayBuffer, code);
				LCD_DrawChinaChar32x32(x, y, (const u8 *)arrayBuffer, charColor, bkColor);
				x += BYTES_WIDTH_PER_FONT_GB_32X32;
			}
			
		} else {
			++gbString;
		}
	}
__exit:
	FontDotArrayFetchUnlock();
	if (*gbString) {
		return gbString;
	}
	return NULL;
}

void Lcd_DisplayChinese16(int x, int y, const unsigned char *str){
	
	LedDisplayGB2312String(x, y, str, 16, Font16Color, LIGHTBLUE);
}

void Lcd_DisplayChinese32(int x, int y, const unsigned char *str){
	
	LedDisplayGB2312String(x, y, str, 32, Font32Color, LIGHTBLUE);
	
	Exist32Font = 1;
}

void LCD_DisplayWelcomeStr(u8 Line)
{
  u16 num = 0;

  /* Send the string character by character on LCD */
  for(num=0; num<13; num++)
  {
    /* Display one China character on LCD */
    LCD_DrawChinaChar24x24(Line, num*24+4, (u8 *)WelcomeStr[num],MAGENTA, LGRAY);
  }
}
/****************************************************************************
* ��    �ƣ�void ili9320_Initializtion()
* ��    �ܣ���ʼ�� ILI9320 ������
* ��ڲ�������
* ���ڲ�������
* ˵    ����
* ���÷�����ili9320_Initializtion();
****************************************************************************/
void ili9320_Initializtion(void)
{
    const u8 str[]       = " Welcome to www.fitbright.com  ";
    const u8 ID8989str[] = "";
    u8 len = sizeof(str)-1;
    u16 i, StartX;
    /*****************************
    **    Ӳ������˵��          **
    ** STM32         ili9320    **
    ** PE0~15 <----> DB0~15     **
    ** PD15   <----> nRD        **
    ** PD14   <----> RS         **
    ** PD13   <----> nWR        **
    ** PD12   <----> nCS        **
    ** PD11   <----> nReset     **
    ** PC0    <----> BK_LED     **
    ******************************/

    LCD_X_Init();
    Delay(5); /* delay 50 ms */
    LCD_WriteReg(0x0000,0x0001);  
    Delay(5); /* delay 50 ms */			//start internal osc
    DeviceCode = LCD_ReadReg(0x0000);
    Delay(5); /* delay 50 ms */
    DeviceCode = LCD_ReadReg(0x0000);  
	  LCD_WriteReg(0x0000,0x0001);           
    LCD_WriteReg(0x0010,0x0000);       
    Delay(5);  
		
		LCD_WriteReg(0x0007,0x0233);       
		LCD_WriteReg(0x0011,0x6038);       
		LCD_WriteReg(0x0002,0x0600);       
		LCD_WriteReg(0x0003,0xA8A4);//0x0804  
		LCD_WriteReg(0x000C,0x0000);//
		LCD_WriteReg(0x000D,0x080C);//       
		LCD_WriteReg(0x000E,0x2900);       
		LCD_WriteReg(0x001E,0x00B8);       
		LCD_WriteReg(0x0001,0x293F);  //����
		LCD_WriteReg(0x0010,0x0000);       
		LCD_WriteReg(0x0005,0x0000);       
		LCD_WriteReg(0x0006,0x0000);       
		LCD_WriteReg(0x0016,0xEF1C);     
		LCD_WriteReg(0x0017,0x0003);     
		LCD_WriteReg(0x0007,0x0233);		//0x0233       
		LCD_WriteReg(0x000B,0x0000|(3<<6));  //////     
		LCD_WriteReg(0x000F,0x0000);		//
		LCD_WriteReg(0x0041,0x0000);     
		LCD_WriteReg(0x0042,0x0000);     
		LCD_WriteReg(0x0048,0x0000);     
		LCD_WriteReg(0x0049,0x013F);     
		LCD_WriteReg(0x004A,0x0000);     
		LCD_WriteReg(0x004B,0x0000);     
		LCD_WriteReg(0x0044,0xEF00);     
		LCD_WriteReg(0x0045,0x0000);     
		LCD_WriteReg(0x0046,0x013F);     
		LCD_WriteReg(0x0030,0x0707);     
		LCD_WriteReg(0x0031,0x0204);     
		LCD_WriteReg(0x0032,0x0204);     
		LCD_WriteReg(0x0033,0x0502);     
		LCD_WriteReg(0x0034,0x0507);     
		LCD_WriteReg(0x0035,0x0204);     
		LCD_WriteReg(0x0036,0x0204);     
		LCD_WriteReg(0x0037,0x0502);     
		LCD_WriteReg(0x003A,0x0302);     
		LCD_WriteReg(0x003B,0x0302);     
		LCD_WriteReg(0x0023,0x0000);     
		LCD_WriteReg(0x0024,0x0000);     
		LCD_WriteReg(0x0025,0x8000);     
		LCD_WriteReg(0x004e,0);        //?(X)??0
		LCD_WriteReg(0x004f,0);        //?(Y)??0

    Lcd_Light_ON;

    ili9320_Clear(RED);
	//	ili9320_DrawPicture(u16 StartX,u16 StartY,u16 EndX,u16 EndY,u16 *pic)
    LCD_DisplayWelcomeStr(0x60);

    StartX = (320 - 8*len)/2;
    for (i=0;i<len;i++)
    {
        ili9320_PutChar((StartX+8*i),60,str[i],YELLOW, RED);
    }
    if(DeviceCode==0x8989)
		{
	    i=0;
        while(ID8989str[i]!='\0')
        {
            ili9320_PutChar((StartX+8*i),140,ID8989str[i],YELLOW, RED);
			i++;
        }
    }
    Delay(1200);  
}

/****************************************************************************
* ��    �ƣ�void ili9320_SetCursor(u16 x,u16 y)
* ��    �ܣ�������Ļ����
* ��ڲ�����x      ������
*           y      ������
* ���ڲ�������
* ˵    ����
* ���÷�����ili9320_SetCursor(10,10);
****************************************************************************/
//inline void ili9320_SetCursor(u16 x,u16 y)
void ili9320_SetCursor(u16 x,u16 y)
{
	if(DeviceCode==0x8989)
	{
	 	LCD_WriteReg(0x004e,y);        //��
    	LCD_WriteReg(0x004f,x);  //��
	}
	else if((DeviceCode==0x9919))
	{
		LCD_WriteReg(0x004e,x); // ��
  		LCD_WriteReg(0x004f,y); // ��	
	}
    /*
	else if((DeviceCode==0x9325))
	{
		LCD_WriteReg(0x0020,x); // ��
  		LCD_WriteReg(0x0021,y); // ��	
	}
	*/
	else
	{
  		LCD_WriteReg(0x0020,y); // ��
  		LCD_WriteReg(0x0021,0x13f-x); // ��
	}
}

/****************************************************************************
* ��    �ƣ�void ili9320_SetWindows(u16 StartX,u16 StartY,u16 EndX,u16 EndY)
* ��    �ܣ����ô�������
* ��ڲ�����StartX     ����ʼ����
*           StartY     ����ʼ����
*           EndX       �н�������
*           EndY       �н�������
* ���ڲ�������
* ˵    ����
* ���÷�����ili9320_SetWindows(0,0,100,100)��
****************************************************************************/
//inline void ili9320_SetWindows(u16 StartX,u16 StartY,u16 EndX,u16 EndY)
void ili9320_SetWindows(u16 StartX,u16 StartY,u16 EndX,u16 EndY)
{
  ili9320_SetCursor(StartX,StartY);
  LCD_WriteReg(0x0050, StartX);
  LCD_WriteReg(0x0052, StartY);
  LCD_WriteReg(0x0051, EndX);
  LCD_WriteReg(0x0053, EndY);
}

/****************************************************************************
* ��    �ƣ�void ili9320_Clear(u16 dat)
* ��    �ܣ�����Ļ����ָ������ɫ��������������� 0xffff
* ��ڲ�����dat      ���ֵ
* ���ڲ�������
* ˵    ����
* ���÷�����ili9320_Clear(0xffff);
****************************************************************************/
void ili9320_Clear(u16 Color)
{
  u32 index=0;
  ili9320_SetCursor(0,0); 
  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
  for(index=0;index<76800;index++)
   {
     LCD->LCD_RAM=Color;
   }
	 Exist32Font = 0;
}

/****************************************************************************





******************************************************************************/

void ili9320_Darken(u8 Line, u16 Color)
{
	u32 index=0;
	
	if(Exist32Font){
		Line = 24 + Line * 18;
	} else {
		Line = 14 + (Line - 1) * 18;
	}
	
  ili9320_SetCursor(Line*320,0); 
  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
  for(index=0; index<16*320; index++)
  {
		 if(Color != Font16Color){
			LCD->LCD_RAM=Color;
		 }
  }
}

/****************************************************************************
* ��    �ƣ�u16 ili9320_GetPoint(u16 x,u16 y)
* ��    �ܣ���ȡָ���������ɫֵ
* ��ڲ�����x      ������
*           y      ������
* ���ڲ�������ǰ������ɫֵ
* ˵    ����
* ���÷�����i=ili9320_GetPoint(10,10);
****************************************************************************/
u16 ili9320_GetPoint(u16 x,u16 y)
{
  ili9320_SetCursor(x,y);
  return (ili9320_BGR2RGB(LCD_ReadRAM()));
}
/****************************************************************************
* ��    �ƣ�void ili9320_SetPoint(u16 x,u16 y,u16 point)
* ��    �ܣ���ָ�����껭��
* ��ڲ�����x      ������
*           y      ������
*           point  �����ɫ
* ���ڲ�������
* ˵    ����
* ���÷�����ili9320_SetPoint(10,10,0x0fe0);
****************************************************************************/
void ili9320_SetPoint(u16 x,u16 y,u16 point)
{
  if ( (x>320)||(y>240) ) return;
  ili9320_SetCursor(x,y);

  LCD_WriteRAM_Prepare();
  LCD_WriteRAM(point);
}


/****************************************************************************
* ��    �ƣ�void ili9320_DrawPicture(u16 StartX,u16 StartY,u16 EndX,u16 EndY,u16 *pic)
* ��    �ܣ���ָ�����귶Χ��ʾһ��ͼƬ
* ��ڲ�����StartX     ����ʼ����
*           StartY     ����ʼ����
*           EndX       �н�������
*           EndY       �н�������
            pic        ͼƬͷָ��
* ���ڲ�������
* ˵    ����ͼƬȡģ��ʽΪˮƽɨ�裬16λ��ɫģʽ
* ���÷�����ili9320_DrawPicture(0,0,100,100,(u16*)demo);
* ��    �ߣ� www.armjishu.com
****************************************************************************/
void ili9320_DrawPicture(u16 StartX,u16 StartY,u16 EndX,u16 EndY,u16 *pic)
{
  u32  i, total;
  u16 data1,data2,data3;
  u16 *picturepointer = pic;
  u16 x,y;

  x=StartX;
  y=StartY;
  
  total = (EndX - StartX + 1)*(EndY - StartY + 1 )/2;

  for (i=0;i<total;i++)
  {
      data1 = *picturepointer++;
      data2 = *picturepointer++;
      data3 = ((data1 >>3)& 0x001f) |((data1>>5) & 0x07E0) | ((data2<<8) & 0xF800);
      ili9320_SetPoint(x,y,data3);
      y++;
      if(y > EndY)
      {
          x++;
          y=StartY;
      }


      data1 = data2;
      data2 = *picturepointer++;
      data3 = ((data1 >>11)& 0x001f) |((data2<<3) & 0x07E0) | ((data2) & 0xF800);
      ili9320_SetPoint(x,y,data3);
      y++;
      if(y > EndY)
      {
          x++;
          y=StartY;
      }
  }

}
#if 0
void ili9320_DrawPicture(u16 StartX,u16 StartY,u16 EndX,u16 EndY,u16 *pic)
{
  u32  i, total;
  u16 data1,data2,data3;
  u16 *picturepointer = pic;
  //ili9320_SetWindows(StartX,StartY,EndX,EndY);

  LCD_WriteReg(0x0003,(1<<12)|(0<<5)|(1<<4) ); 

  ili9320_SetCursor(StartX,StartY);
  
  LCD_WriteRAM_Prepare();
  total = (EndX + 1)*(EndY + 1 ) / 2;
  for (i=0;i<total;i++)
  {
      data1 = *picturepointer++;
      data2 = *picturepointer++;
      data3 = ((data1 >>3)& 0x001f) |((data1>>5) & 0x07E0) | ((data2<<8) & 0xF800);
      LCD_WriteRAM(data3);
      data1 = data2;
      data2 = *picturepointer++;
      data3 = ((data1 >>11)& 0x001f) |((data2<<3) & 0x07E0) | ((data2) & 0xF800);
      LCD_WriteRAM(data3);
  }

  LCD_WriteReg(0x0003,(1<<12)|(1<<5)|(1<<4) ); 
}
#endif
/*
void ili9320_DrawPicture(u16 StartX,u16 StartY,u16 EndX,u16 EndY,u16 *pic)
{
  u32  i, total;
  ili9320_SetWindows(StartX,StartY,EndX,EndY);
  ili9320_SetCursor(StartX,StartY);
  
  LCD_WriteRAM_Prepare();
  total = EndX*EndY;
  for (i=0;i<total;i++)
  {
      LCD_WriteRAM(*pic++);
  }
}
*/
/****************************************************************************
* ��    �ƣ�void ili9320_PutChar(u16 x,u16 y,u8 c,u16 charColor,u16 bkColor)
* ��    �ܣ���ָ��������ʾһ��8x16�����ascii�ַ�
* ��ڲ�����x          ������
*           y          ������
*           charColor  �ַ�����ɫ
*           bkColor    �ַ�������ɫ
* ���ڲ�������
* ˵    ������ʾ��Χ�޶�Ϊ����ʾ��ascii��
* ���÷�����ili9320_PutChar(10,10,'a',0x0000,0xffff);
****************************************************************************/
void ili9320_PutChar(u16 x,u16 y,u8 c,u16 charColor,u16 bkColor)  // Lihao
{
  u16 i=0;
  u16 j=0;
  
  u8 tmp_char=0;
  
  if(HyalineBackColor == bkColor)
  {
    for (i=0;i<16;i++)
    {
      tmp_char=ascii_8x16[((c-0x20)*16)+i];
      for (j=0;j<8;j++)
      {
          if ( (tmp_char >> 7-j) & 0x01 == 0x01)
          {
              ili9320_SetPoint(x+j,y+i,charColor); // �ַ���ɫ
          }
          else
          {
              // do nothing // ͸������
          }
      }
    }
  }
  else
  {
        for (i=0;i<16;i++)
    {
      tmp_char=ascii_8x16[((c-0x20)*16)+i];
      for (j=0;j<8;j++)
      {
        if ( (tmp_char >> 7-j) & 0x01 == 0x01)
          {
            ili9320_SetPoint(x+j,y+i,charColor); // �ַ���ɫ
          }
          else
          {
            ili9320_SetPoint(x+j,y+i,bkColor); // ������ɫ
          }
      }
    }
  }
}

/****************************************************************************
* ��    �ƣ�void ili9320_PutChar(u16 x,u16 y,u8 c,u16 charColor,u16 bkColor)
* ��    �ܣ���ָ��������ʾһ��8x16�����ascii�ַ�
* ��ڲ�����x          ������
*           y          ������
*           charColor  �ַ�����ɫ
*           bkColor    �ַ�������ɫ
* ���ڲ�������
* ˵    ������ʾ��Χ�޶�Ϊ����ʾ��ascii��
* ���÷�����ili9320_PutChar(10,10,'a',0x0000,0xffff);
****************************************************************************/
void ili9320_PutChar_16x24(u16 x,u16 y,u8 c,u16 charColor,u16 bkColor)
{

  u16 i=0;
  u16 j=0;
  
  u16 tmp_char=0;

  if(HyalineBackColor == bkColor)
  {
    for (i=0;i<24;i++)
    {
      tmp_char=ASCII_Table_16x24[((c-0x20)*24)+i];
      for (j=0;j<16;j++)
      {
        if ( (tmp_char >> j) & 0x01 == 0x01)
          {
            ili9320_SetPoint(x+j,y+i,charColor); // �ַ���ɫ
          }
          else
          {
              // do nothing // ͸������
          }
      }
    }
  }
  else
  {
    for (i=0;i<24;i++)
    {
      tmp_char=ASCII_Table_16x24[((c-0x20)*24)+i];
      for (j=0;j<16;j++)
      {
        if ( (tmp_char >> j) & 0x01 == 0x01)
          {
            ili9320_SetPoint(x+j,y+i,charColor); // �ַ���ɫ
          }
          else
          {
            ili9320_SetPoint(x+j,y+i,bkColor); // ������ɫ
          }
      }
    }
  }
}
/****************************************************************************
* ��    �ƣ�u16 ili9320_BGR2RGB(u16 c)
* ��    �ܣ�RRRRRGGGGGGBBBBB ��Ϊ BBBBBGGGGGGRRRRR ��ʽ
* ��ڲ�����c      BRG ��ɫֵ
* ���ڲ�����RGB ��ɫֵ
* ˵    �����ڲ���������
* ���÷�����
****************************************************************************/
u16 ili9320_BGR2RGB(u16 c)
{
  u16  r, g, b, rgb;

  b = (c>>0)  & 0x1f;
  g = (c>>5)  & 0x3f;
  r = (c>>11) & 0x1f;
  
  rgb =  (b<<11) + (g<<5) + (r<<0);

  return( rgb );
}

/****************************************************************************
* ��    �ƣ�void ili9320_BackLight(u8 status)
* ��    �ܣ�������Һ������
* ��ڲ�����status     1:���⿪  0:�����
* ���ڲ�������
* ˵    ����
* ���÷�����ili9320_BackLight(1);
****************************************************************************/
void ili9320_BackLight(u8 status)
{
  if ( status >= 1 )
  {
    Lcd_Light_ON;
  }
  else
  {
    Lcd_Light_OFF;
  }
}

/****************************************************************************
* ��    �ƣ�void ili9320_Delay(vu32 nCount)
* ��    �ܣ���ʱ
* ��ڲ�����nCount   ��ʱֵ
* ���ڲ�������
* ˵    ����
* ���÷�����ili9320_Delay(10000);
****************************************************************************/
void ili9320_Delay(vu32 nCount)
{
   Delay(nCount);
  //for(; nCount != 0; nCount--);
}
