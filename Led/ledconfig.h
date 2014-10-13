#ifndef __LEDCONFIG_H__
#define __LEDCONFIG_H__

#if defined(USE_QIANGLI_P10_1R1G) && (USE_QIANGLI_P10_1R1G!=0)
#  define LED_SCAN_MUX 4
#  define LED_SCAN_LENGTH (32*4*QIANGLI_UNIT_X_NUM)
#  define LED_PHY_DOT_WIDTH (32*QIANGLI_UNIT_X_NUM)
#  define LED_PHY_DOT_HEIGHT (16*QIANGLI_UNIT_Y_NUM)
#  define LED_DRIVER_LEVEL  1    // 驱动电平，0或1	强力双色为1
#  define LED_STROBE_PAUSE  1    // LED_LT脉冲电平，0或1
#  define LED_OE_LEVEL 0
#endif

#if defined(USE_QIANGLI_P10_1R) && (USE_QIANGLI_P10_1R!=0)
#  define LED_SCAN_MUX 4
#  define LED_SCAN_LENGTH       (32*4*QIANGLI_UNIT_X_NUM)
#  define LED_PHY_DOT_WIDTH     (32*QIANGLI_UNIT_X_NUM)
#  define LED_PHY_DOT_HEIGHT     (16*QIANGLI_UNIT_Y_NUM)
#  define LED_DRIVER_LEVEL  0    // 驱动电平，0或1  强力单色为0
#  define LED_STROBE_PAUSE  1    // LED_LT脉冲电平，0或1
#  define LED_OE_LEVEL      1
#endif

#ifdef __LED_LIXIN__
#define LED_VIR_DOT_WIDTH    (LED_PHY_DOT_WIDTH)
#define LED_VIR_DOT_HEIGHT   (LED_PHY_DOT_HEIGHT)
#else
#define LED_VIR_DOT_WIDTH    LED_PHY_DOT_WIDTH
#define LED_VIR_DOT_HEIGHT    LED_PHY_DOT_HEIGHT
#endif

#endif // __LEDCONFIG_H__
