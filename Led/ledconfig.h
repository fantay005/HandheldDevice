#ifndef __LEDCONFIG_H__
#define __LEDCONFIG_H__

/// LEDɨ��ģʽ, ������16,8,4,2; �ֱ���1/16,1/8,1/4,1/2ɨ��;
/// #define LED_SCAN_MUX 4

/// ɨ�����ĳ���
/// #define LED_SCAN_LENGTH (128)

/// #define LED_DOT_HEIGHT 16
/// #define LED_DOT_WIDTH (32)


/// #define LED_DRIVER_LEVEL  1    // ������ƽ��0��1
/// #define LED_STROBE_PAUSE  0    // LED_LT�����ƽ��0��1
/// #define LED_OE_LEVEL 0

// #define USE_QIANGLI_P10_1R1G 1
// #define QIANGLI_UNIT_X_NUM 12
// #define QIANGLI_UNIT_Y_NUM 2

#if defined(USE_QIANGLI_P10_1R1G) && (USE_QIANGLI_P10_1R1G!=0)
#  define LED_SCAN_MUX 4
#  define LED_SCAN_LENGTH (32*4*QIANGLI_UNIT_X_NUM)
#  define LED_PHY_DOT_WIDTH (32*QIANGLI_UNIT_X_NUM)
#  define LED_PHY_DOT_HEIGHT (16*QIANGLI_UNIT_Y_NUM)
#  define LED_DRIVER_LEVEL  1    // ������ƽ��0��1	ǿ��˫ɫΪ1
#  define LED_STROBE_PAUSE  1    // LED_LT�����ƽ��0��1
#  define LED_OE_LEVEL 0
#endif

#if defined(USE_QIANGLI_P10_1R) && (USE_QIANGLI_P10_1R!=0)
#  define LED_SCAN_MUX 4
#  define LED_SCAN_LENGTH       (32*4*QIANGLI_UNIT_X_NUM)
#  define LED_PHY_DOT_WIDTH     (32*QIANGLI_UNIT_X_NUM)
#  define LED_PHY_DOT_HEIGHT     (16*QIANGLI_UNIT_Y_NUM)
#  define LED_DRIVER_LEVEL  0    // ������ƽ��0��1  ǿ����ɫΪ0
#  define LED_STROBE_PAUSE  1    // LED_LT�����ƽ��0��1
#  define LED_OE_LEVEL      0
#endif

#ifdef __LED_LIXIN__
#define LED_VIR_DOT_WIDTH    (LED_PHY_DOT_WIDTH)
#define LED_VIR_DOT_HEIGHT   (LED_PHY_DOT_HEIGHT)
#else
#define LED_VIR_DOT_WIDTH    LED_PHY_DOT_WIDTH
#define LED_VIR_DOT_HEIGHT    LED_PHY_DOT_HEIGHT
#endif

//#define LED_DOT_XEND (LED_DOT_WIDTH-1)
//#define LED_DOT_YEND (LED_DOT_HEIGHT-1)

#endif // __LEDCONFIG_H__
