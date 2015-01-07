#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
#include "gsm.h"
#include "misc.h"
#include "sms.h"
#include "stm32f10x_gpio.h"
#include "zklib.h"
#include "sms_cmd.h"
#include "libupdater.h"


USERParam __userParam;

static void __cmd_UPDATA_Handler(const SMSInfo *p) {              /*Éý¼¶¹Ì¼þ³ÌÐò*/
	int i;
	int j = 0;
	FirmwareUpdaterMark *mark;
	char *pcontent = (char *)p->content;
	char *host = (char *)&pcontent[8];
	char *buff[3] = {0, 0, 0};

	for (i = 10; pcontent[i] != 0; ++i) {
		if (pcontent[i] == ',') {
			pcontent[i] = 0;
			++i;
			if (j < 3) {
				buff[j++] = (char *)&pcontent[i];
			}
		}
	}

	if (j != 3) {
		return;
	}

	mark = pvPortMalloc(sizeof(*mark));
	if (mark == NULL) {
		return;
	}

	if (FirmwareUpdateSetMark(mark, host, atoi(buff[0]), buff[1], buff[2])) {	
		NVIC_SystemReset();
	}
	vPortFree(mark);
}

typedef struct {
	char *cmd;
	void (*smsCommandFunc)(const SMSInfo *p);
	uint32_t permission;
} SMSModifyMap;

const static SMSModifyMap __SMSModifyMap[] = {
	{"<UPDATA>", __cmd_UPDATA_Handler},
	{NULL, NULL},
};

void ProtocolHandlerSMS(const SMSInfo *sms) {
	const SMSModifyMap *map;

	for (map = __SMSModifyMap; map->cmd != NULL; ++map) {
		if (strncasecmp((const char *)sms->content, map->cmd, strlen(map->cmd)) == 0) {
				map->smsCommandFunc(sms);
				return;
		}
	}
}
