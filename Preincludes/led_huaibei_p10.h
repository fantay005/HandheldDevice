#ifndef __PREINCLUDE_H__
#define __PREINCLUDE_H__

#include "preinclude_commoh.h"

//������, ǿ��˫ɫLED, 32*9
#define __LED__                          1
#define __LED_LIXIN__                    1
#define __LED_LIXIN_288__                1
#define __TARGET_STRING__               "LED_LIXIN_288"

#define USE_QIANGLI_P10_1R               1
#define QIANGLI_UNIT_X_NUM               5
#define QIANGLI_UNIT_Y_NUM               8

#else
#  error "Preinclude file can only be included once in command line with --preinclude=xxxx"
#endif
