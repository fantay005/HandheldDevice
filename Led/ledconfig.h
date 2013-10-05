#ifndef __LEDCONFIG_H__
#define __LEDCONFIG_H__

/// LEDɨ��ģʽ, ������16,8,4,2; �ֱ���1/16,1/8,1/4,1/2ɨ��;
#define LED_SCAN_MUX 4

/// ɨ�����ĳ���
#define LED_SCAN_LENGTH (32*12*4)
#define LED_SCAN_BITS_MASK 0x0F
#define LED_SCAN_BITS 4

#define LED_DOT_HEIGHT 16
#define LED_DOT_WIDTH (32*12)

#define LED_DOT_XEND (LED_DOT_WIDTH-1)
#define LED_DOT_YEND (LED_DOT_HEIGHT-1)

#define LED_DRIVER_LEVEL  1    // ������ƽ��0��1
#define LED_STROBE_PAUSE  0    // LED_LT�����ƽ��0��1
#define LED_OE_LEVEL 0

#endif // __LEDCONFIG_H__
