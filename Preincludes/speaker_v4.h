#ifndef __PREINCLUDE_H__
#define __PREINCLUDE_H__

#include "preinclude_commoh.h"

//GPRS��������
#define __SPEAKER__                     1
#define __SPEAKER_V4__                  1

#define __TARGET_STRING__               "HANDHELD"
#define __VERSION__                     "V_1.0"


#else
#  error "Preinclude file can only be included once in command line with --preinclude=xxxx"
#endif
