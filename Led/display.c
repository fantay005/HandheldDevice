#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "ledconfig.h"
#include "led_lowlevel.h"
#include "zklib.h"
#include "display.h"
#include "norflash.h"
#include "xfs.h"
#include "unicode2gbk.h"
#include "font_dot_array.h"

#define DISPLAY_TASK_STACK_SIZE		( configMINIMAL_STACK_SIZE + 256 )

#define MSG_CMD_DISPLAY_CONTROL 0
#define MSG_CMD_DISPLAY_MESSAGE	1
#define MSG_CMD_DISPLAY_MESSAGE_RED	2
#define MSG_CMD_DISPLAY_MESSAGE_GREEN	3
#define MSG_CMD_DISPLAY_MESSAGE_YELLOW	4


#define	MSG_DATA_DISPLAY_CONTROL_OFF 0
#define	MSG_DATA_DISPLAY_CONTROL_ON 1

static const char *host = "欢迎光临中国邮政储蓄银行";

typedef struct {
	uint32_t cmd;
	union {
		void *pointData;
		uint8_t byteData;
		uint16_t halfWordData;
		uint32_t wordData;
	} data;
} DisplayTaskMessage;

static xQueueHandle __displayQueue;


static char __displayMessageColor = 1;
static const uint8_t *__displayMessage = NULL;
static const uint8_t *__displayCurrentPoint = NULL;

void MessDisplay(char *message) {
	char *p = pvPortMalloc(strlen(message) + 1);
	DisplayTaskMessage msg;
	strcpy(p, message);
	msg.cmd = MSG_CMD_DISPLAY_MESSAGE;
	msg.data.pointData = p;

	if (pdTRUE != xQueueSend(__displayQueue, &msg, configTICK_RATE_HZ)) {
		vPortFree(p);
	}
}

void DisplayOnOff(int isOn) {
	DisplayTaskMessage msg;
	msg.cmd = MSG_CMD_DISPLAY_CONTROL;
	msg.data.byteData = isOn ? MSG_DATA_DISPLAY_CONTROL_ON : MSG_DATA_DISPLAY_CONTROL_OFF;
	xQueueSend(__displayQueue, &msg, configTICK_RATE_HZ);
}

void __displayMessageLowlevel(void) {
	const uint8_t *tmp;
	if (__displayMessage == NULL) {
		return;
	}
	if (__displayCurrentPoint == NULL) {
		__displayCurrentPoint = __displayMessage;
	}
	LedDisplayClear(0, 0, LED_DOT_XEND, LED_DOT_HEIGHT / 2 - 1);
	LedDisplayClear(0, LED_DOT_HEIGHT / 2, LED_DOT_XEND, LED_DOT_HEIGHT - 1);
	if (__displayMessageColor & 1) {
		tmp = (const uint8_t *)LedDisplayGB2312String16(0, 8, __displayCurrentPoint);
	}
	__displayCurrentPoint = tmp;
	LedDisplayToScan(0, 0, LED_DOT_XEND, LED_DOT_YEND);
}



void __handlerDisplayControl(DisplayTaskMessage *msg) {
	if (msg->data.byteData == MSG_DATA_DISPLAY_CONTROL_OFF) {
		// display off
		LedScanOnOff(0);
	} else if (msg->data.byteData == MSG_DATA_DISPLAY_CONTROL_ON) {
		// display on
		LedScanOnOff(1);
	} else {
		// unknow
	}
}

void __handlerDisplayMessage(DisplayTaskMessage *msg) {
	if (__displayMessage) {
		vPortFree((void *)__displayMessage);
	}
	__displayCurrentPoint = NULL;
	__displayMessage = msg->data.pointData;
	__displayMessageLowlevel();
}

void __destroyDisplayMessage(DisplayTaskMessage *msg) {
	vPortFree(msg->data.pointData);
}

typedef void (*MessageHandlerFunc)(DisplayTaskMessage *);
typedef void (*MessageDestroyFunc)(DisplayTaskMessage *);
static const struct {
	uint32_t cmd;
	MessageHandlerFunc handlerFunc;
	MessageDestroyFunc destroyFunc;
} __messageHandlerFunctions[] = {
	{ MSG_CMD_DISPLAY_MESSAGE, __handlerDisplayMessage, NULL },
	{ MSG_CMD_DISPLAY_CONTROL, __handlerDisplayControl, NULL},
};

void DisplayClear(void) {
	char clear[144];
	int i;
	for (i = 0; i < 144; i++) {
		clear[i] = ' ';
	}
	LedDisplayGB2312String16(0, 0, (const uint8_t *)clear);
	LedDisplayToScan(0, 0, LED_DOT_XEND, LED_DOT_YEND);
}

void DisplayTask(void *helloString) {
	portBASE_TYPE rc;
	DisplayTaskMessage msg;

	printf("DisplayTask: start-> %s\n", (const char *)helloString);
	__displayQueue = xQueueCreate(5, sizeof(DisplayTaskMessage));
	MessDisplay((char *)host);
	LedScanOnOff(1);
	while (1) {
		rc = xQueueReceive(__displayQueue, &msg, configTICK_RATE_HZ * 10);
		if (rc == pdTRUE) {
			int i;
			for (i = 0; i < ARRAY_MEMBER_NUMBER(__messageHandlerFunctions); ++i) {
				if (__messageHandlerFunctions[i].cmd == msg.cmd) {
					__messageHandlerFunctions[i].handlerFunc(&msg);
					if (__messageHandlerFunctions[i].destroyFunc != NULL) {
						__messageHandlerFunctions[i].destroyFunc(&msg);
					}
					break;
				}
			}
		} else {
			__displayMessageLowlevel();
		}
	}
}



void DisplayInit(void) {
	LedScanInit();
	xTaskCreate(DisplayTask, (signed portCHAR *) "DISPLAY", DISPLAY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 10, NULL);
}

