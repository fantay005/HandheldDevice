#include <stdint.h>
#include "norflash.h"


#define FONT_DOT_ARRAY_FLASH_OFFSET 0x10000
#define FONT_DOT_CHINESE_16X16_OFFSET (0 + FONT_DOT_ARRAY_FLASH_OFFSET)
#define FONT_DOT_CHINESE_24X24_OFFSET (0x3BE80 + FONT_DOT_ARRAY_FLASH_OFFSET)
#define FONT_DOT_CHINESE_32X32_OFFSET (0xE2000 + FONT_DOT_ARRAY_FLASH_OFFSET)

#define FONT_DOT_ASCII_16X8_OFFSET (0xCBB80 + FONT_DOT_ARRAY_FLASH_OFFSET)
#define FONT_DOT_ASCII_24X16_OFFSET (0xCC120 + FONT_DOT_ARRAY_FLASH_OFFSET)
#define FONT_DOT_ASCII_32X16_OFFSET (0xE0000 + FONT_DOT_ARRAY_FLASH_OFFSET)

#define FONT_DOT_UNICODE_16X16_OFFSET (0xD7700 + FONT_DOT_ARRAY_FLASH_OFFSET)

void FontDotArrayInit() {
	NorFlashInit();
}

int FontDotArrayFetchASCII_16(uint8_t *buf, uint8_t c) {
	uint32_t addr = (c - 0x20) * 15 + FONT_DOT_ASCII_16X8_OFFSET;
	NorFlashRead2(addr, (short *)buf, 8);
	buf[15] = 0;
	return 16;
}

int FontDotArrayFetchASCII_24(uint8_t *buf, uint8_t c) {
	uint32_t addr = (c - 0x20) * 48 + FONT_DOT_ASCII_24X16_OFFSET;
	NorFlashRead2(addr, (short *)buf, 24);
	return 48;
}

int FontDotArrayFetchASCII_32(uint8_t *buf, uint8_t c) {
	uint32_t addr = (c - 0x20) * 64 + FONT_DOT_ASCII_32X16_OFFSET;
	NorFlashRead2(addr, (short *)buf, 32);
	return 64;
}

int FontDotArrayFetchGB_16(uint8_t *buf, uint16_t code) {
	uint32_t addr = ((code >> 8) - 0xA1) * 94 + ((code & 0xff) - 0xA1);
	addr = addr * 30 + FONT_DOT_CHINESE_16X16_OFFSET;
	NorFlashRead2(addr, (short *)buf, 15);
	buf[30] = 0;
	buf[31] = 0;
	return 32;
}


int FontDotArrayFetchGB_24(uint8_t *buf, uint16_t code) {
	uint32_t addr = ((code >> 8) - 0xA1) * 94 + ((code & 0xff) - 0xA1);
	addr = addr * 72 + FONT_DOT_CHINESE_24X24_OFFSET;
	NorFlashRead2(addr, (short *)buf, 36);
	return 72;
}

int FontDotArrayFetchGB_32(uint8_t *buf, uint16_t code) {
	uint32_t addr = ((code >> 8) - 0xA1) * 94 + ((code & 0xff) - 0xA1);
	addr = addr * 128 + FONT_DOT_CHINESE_32X32_OFFSET;
	NorFlashRead2(addr, (short *)buf, 64);
	return 128;
}

//int FontDotArrayFetch16(char buf[], uint16_t code) {
//	if (isChinese(code)) {
//	}
//
//	if (isAscii(code)) {
//	}
//
//	if (isUnicode(code)) {
//		uint32_t addr = (code - 0x20) * 32 + FONT_DOT_UNICODE_16X16_OFFSET;
//		addr = addr * 30 + FONT_DOT_CHINESE_16X16_OFFSET;
//		FSMC_NOR_ReadBuffer((short *)buf, addr, 16);
//		return 32;
//	}
//
//	FSMC_NOR_ReadBuffer((short *)buf, FONT_DOT_ASCII_16X8_OFFSET, 8);
//	buf[15] = 0;
//	*width = 1;
//	*height = 16;
//	return 16;
//}


#if 0
int FontDotArrayFetch(char buf[], uint16_t code, int height) {

	//������Ҫ�����ַ��Ǻ��ֻ���ASCII��,Ȼ����������׵�ַ
	if ((code >= 0xA1A1) && (code <= 0xF7FE)) {			//������
		kk = ((kk >> 8) - 0xA1) * 94 + ((kk & 0xff) - 0xA1);
		if (height == 24) {
			font_address = kk * 72  + 0x3BE80 + 0x10000;
		} else if (height == 32) {
			font_address = kk * 128 + 0xE2000 + 0x10000;
		} else {
			font_address = kk * 30 + 0x10000;
		}
	} else if ((kk >= 0x20) && (kk <= 0x7F)) {			//ASCII�����2C B0 00
		if (CHAR_HIGH == HIGH_24) {
			font_address = (kk - 0x20) * 48 + 0xCC120 + 0x10000;    //24*16
		} else if (CHAR_HIGH == HIGH_32) {
			font_address = (kk - 0x20) * 64 + 0xE0000 + 0x10000;    //32*16
		} else {
			font_address = (kk - 0x20) * 15 + 0xCBB80 + 0x10000;    //16*8
		}
	} else if ((kk >= 0x9000) && (kk <= 0xA1A1)) { //Unicode����  D7700��ʼ   (0000- 87FF)   //ÿ���ַ�ռ32���ֽ�
		kk = kk - 0x9000;
		if (CHAR_HIGH == HIGH_24) {
			font_address = kk * 72 + 0x217050 + 0x10000;    //24*24��֧��
		} else if (CHAR_HIGH == HIGH_32) {
			font_address = kk * 128 + 0x1f5050 + 0x10000;    //32*32��֧��
		} else {
			font_address = kk * 32 + 0xd7700 + 0x10000;    //16*16��Unicode����
		}
	}
	//�Ѳ��淶�����뵱�ɿո�
	else {
		if (CHAR_HIGH == HIGH_24) {
			font_address = 0xCC120 + 0x10000;    //24*16
		} else if (CHAR_HIGH == HIGH_32) {
			font_address = 0xE0000 + 0x10000;    //32*16
		} else {
			font_address = 0xCBB80 + 0x10000;    //16*8
		}

	}
}
#endif
