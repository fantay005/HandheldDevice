#ifndef __LEDCONFIG_H__
#define __LEDCONFIG_H__



/// LEDɨ��ģʽ, ������16,8,4,2; �ֱ���1/16,1/8,1/4,1/2ɨ��;
#define LED_SCAN_MUX 16

/// ɨ�����ĳ���
#define LED_SCAN_LENGTH 192
#define LED_SCAN_BITS_MASK 0x0F
#define LED_SCAN_BITS 4

#define LED_DOT_HEIGHT 48
#define LED_DOT_WIDTH LED_SCAN_LENGTH

#define LED_DOT_XEND (LED_DOT_WIDTH-1)
#define LED_DOT_YEND (LED_DOT_HEIGHT-1)

#endif // __LEDCONFIG_H__