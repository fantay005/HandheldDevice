#include <string.h>
#include <stdio.h>
#include "display.h"
#include "ili9320_api.h"
#include "ili9320.h"
#include "key.h"
#include "CommZigBee.h"

#define BACKCOLOR   LIGHTBLUE
#define LINECOLOR   BLACK
#define DAKENCOLOR  DARKBLUE

#define XStartFont  4
#define XStartParam 160

#define YStartLine  10

#define YFirstApart  4
#define YLineApart   18

ZigBee_Param Config_ZigBee, Display_ZigBee; 

BSN_Data Analysis_MSG;


static void BackColorSet(void){
	ili9320_Clear(BACKCOLOR);
}

static void DarkenPrepareLine(char line){
	ili9320_Darken(line, DAKENCOLOR);
}

static void Boot_Interface(void){
	BackColorSet();
	
	Lcd_DisplayChinese32(60, 116, "������");
	Lcd_DisplayChinese32(100, 116, "׷��׿Խ");
	
	Lcd_DisplayChinese16(192, 80, "�Ϸʴ������ܿƼ��ɷ����޹�˾");
	Lcd_DisplayChinese16(216, 80, "www.fitbright.cn");
}

static unsigned short YCoordValue(unsigned char line){
	return (43 + (line - 1) * 18);
}

static void Main_Interface(void){
  unsigned short i = 1;	

	BackColorSet();
	
	Lcd_DisplayChinese32(112, YFirstApart, "��ѡ��");
	GUI_Line(0,40,319,40,LINECOLOR);
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "1.����");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "2.ά��");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "3.����");
}

static void Config_Interface(void){
	unsigned short i = 1;
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "����ѡ��");
	GUI_Line(0,40,319,40,LINECOLOR);
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "1.����ѡ��");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "2.ZigBee��ַ");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "3.�߼�����");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "4.��ʾ����");
	i++;
}

static void Repair_Interface(void){
	unsigned short i;
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "ά��ѡ��");
	GUI_Line(0,40,319,40,LINECOLOR);
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "1.����ѡ��");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "2.ZigBee��ַ");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "3.������������");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "4.��/�ص�");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "5.ԭ�����");
}

static void Debug_Interface(void){
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "����ѡ��");
	GUI_Line(0,40,319,40,LINECOLOR);	
	
}

static void Gateway_Option_Interface(void){
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "����ѡ��");
	GUI_Line(0,40,319,40,LINECOLOR);	
}

static void Addr_Option_Interface(void){
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "ZigBee��ַ����");
	GUI_Line(0,40,319,40,LINECOLOR);	
	
}

static void Ballast_Data_Interface(void){
	unsigned short i = 1;
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "������������");
	GUI_Line(0,40,319,40,LINECOLOR);	
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "����״̬��");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.STATE);
	i++;
	
  Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "�� �� ֵ��");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.DIM);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "�����ѹ��");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.INPUT_VOL);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "���������");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.INPUT_CUR);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "���빦�ʣ�");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.INPUT_POW);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "�� �� ѹ��");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.LIGHT_VOL);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "PFC ��ѹ��");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.PFC_VOL);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "��    �ȣ�");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.TEMP);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "����ʱ�䣺");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.TIME);
	i++;
}

static void Reason_Analyze_Interface(void){
	unsigned short i;
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "ԭ�����");
	GUI_Line(0,40,319,40,LINECOLOR);	
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "1.������");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "2.�ƹ�");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "3.����");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "4.��Դ");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "5.ZigBeeģ��");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "6.�ӿڽӴ�����");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "7.����");
	i++;
}

static unsigned short YCoordDot(unsigned char line){
	return (YStartLine + YFirstApart + line * YLineApart);
}

static void Advanced_Config_Interface(ZigBee_Param Config_ZigBee){
	unsigned char i = 0;
	
	BackColorSet();
	
	GUI_Line(0,YStartLine,319,YStartLine,LINECOLOR);	
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 1.�ڵ��ַ��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.MAC_ADDR);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 2.�ڵ����ƣ�");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.NODE_NAME);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 3.�ڵ����ͣ�");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.NODE_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 4.�������ͣ�");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.NET_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 5.�� �� ID��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.NET_ID);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 6.����Ƶ�㣺");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.FREQUENCY);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 7.��ַ���룺");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.DATA_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 8.����ģʽ��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.TX_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 9.�� �� �ʣ�");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.BAUDRATE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "10.У    �飺");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.DATA_PARITY);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "11.�� �� λ��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.DATA_BIT);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "12.����Դַ��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.SRC_ADR);
	i++;
	
	GUI_Line(0, YCoordDot(i), 319, YCoordDot(i), LINECOLOR);
	
}

static void Display_Config_Interface(ZigBee_Param Display_ZigBee){
	unsigned char i = 0;
	
	BackColorSet();
	
	GUI_Line(0,10,319,10,LINECOLOR);	
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 1.�ڵ��ַ��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.MAC_ADDR);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 2.�ڵ����ƣ�");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.NODE_NAME);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 3.�ڵ����ͣ�");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.NODE_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 4.�������ͣ�");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.NET_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 5.�� �� ID��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.NET_ID);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 6.����Ƶ�㣺");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.FREQUENCY);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 7.��ַ���룺");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.DATA_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 8.����ģʽ��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.TX_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 9.�� �� �ʣ�");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.BAUDRATE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "10.У    �飺");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.DATA_PARITY);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "11.�� �� λ��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.DATA_BIT);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "12.����Դַ��");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.SRC_ADR);
	i++;
	
	GUI_Line(0, YCoordDot(i), 319, YCoordDot(i), LINECOLOR);
}

void ZigBeeParamInit(ZigBee_Param msg){
	sprintf(msg.BAUDRATE, "9600");
	sprintf(msg.DATA_BIT, "8+0+1");
	sprintf(msg.DATA_PARITY, "None");
	sprintf(msg.DATA_TYPE, "HEX���");
	sprintf(msg.FREQUENCY, "0F");
	sprintf(msg.MAC_ADDR, "0001");
	sprintf(msg.NET_ID, "FF");
	sprintf(msg.NET_TYPE, "������");
	sprintf(msg.NODE_NAME, "SHUNCOM");
	sprintf(msg.NODE_TYPE, "�м�·��");
	sprintf(msg.SRC_ADR, "�����");
	sprintf(msg.TX_TYPE, "�㲥");
}

void DisplayInit(void){
	ZigBeeParamInit(Config_ZigBee);
	ZigBeeParamInit(Display_ZigBee);
	
	
}