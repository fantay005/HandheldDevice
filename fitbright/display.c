#include <string.h>
#include <stdio.h>
#include "display.h"
#include "ili9320_api.h"
#include "ili9320.h"
#include "key.h"


void Boot_Interface(void){
	
	Lcd_DisplayChinese32(60, 116, (const unsigned char *)"������");
	Lcd_DisplayChinese32(100, 116, (const unsigned char *)"׷��׿Խ");
	
	Lcd_DisplayChinese16(192, 80, (const unsigned char *)"�Ϸʴ������ܿƼ��ɷ����޹�˾");
	Lcd_DisplayChinese16(216, 80, (const unsigned char *)"www.fitbright.cn");
}

void Main_Interface(void){
	Lcd_DisplayChinese32(4, 112, "��ѡ��");
	GUI_Line(40,16,319,16,BLACK);
	
}
