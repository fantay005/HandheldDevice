#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
#include "xfs.h"
#include "misc.h"
#include "sms.h"
#include "led_lowlevel.h"
#include "stm32f10x_gpio.h"
#include "norflash.h"
#include "zklib.h"
#include "unicode2gbk.h"
#include "led_lowlevel.h"
#include "second_datetime.h"

typedef struct {
	unsigned char type;
	unsigned char class;
} ProtocolHeader;

typedef void (*ProtocolHandleFunction)(ProtocolHeader *header, char *p);
typedef struct {
	unsigned char type;
	unsigned char class;
	ProtocolHandleFunction func;
} ProtocolHandleMap;


void ProtocolDestroyMessage(const char *p) {
	vPortFree((void *)p);
}

void HandleSendSMS(ProtocolHeader *header, char *p) {
	DisplayClear();
	MessDisplay(p);
	__storeSMS1(p);
	LedDisplayToScan(0, 0, LED_DOT_XEND, LED_DOT_YEND);
	GsmTaskSendTcpData("DISPLAY is OK\r\n", 15);
}

void HandleRestart(ProtocolHeader *header, char *p) {
  DateTime dateTime;
	p++;
	p++;
	dateTime.year = (p[0] - '0') * 10 + (p[1] - '0');
	dateTime.month = (p[3] - '0') * 10 + (p[4] - '0');
	dateTime.date = (p[6] - '0') * 10 + (p[7] - '0');
	dateTime.hour = (p[9] - '0') * 10 + (p[10] - '0') + 8;
	dateTime.minute = (p[12] - '0') * 10 + (p[13] - '0');
	dateTime.second = (p[15] - '0') * 10 + (p[16] - '0');
	RtcSetTime(DateTimeToSecond(&dateTime));

	GsmTaskSendTcpData("RTC is OK\r\n", 11);
}

void ProtocolHandler(char *p) {
	int i;
	const static ProtocolHandleMap map[] = {
		{'1', '1', HandleSendSMS},
		{'1', '2', HandleRestart},
	};
	ProtocolHeader *header = (ProtocolHeader *)p;

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if ((map[i].type == header->type) && (map[i].class == header->class)) {
			map[i].func(header, p + sizeof(ProtocolHeader));
			break;
		}
	}
}


