#ifdef __LED__
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

enum { MSG_CMD_DISPLAY_CONTROL = 0,
	   MSG_CMD_DISPLAY_MESSAGE,
	   MSG_CMD_DISPLAY_MESSAGE_RED,
	   MSG_CMD_DISPLAY_MESSAGE_GREEN,
	   MSG_CMD_DISPLAY_MESSAGE_YELLOW,
	   MSG_CMD_DISPLAY_SCROLL_NOTIFY,
	 };


#define	MSG_DATA_DISPLAY_CONTROL_OFF 0
#define	MSG_DATA_DISPLAY_CONTROL_ON 1

#if defined(__LED_HUAIBEI__)
static const char *host = "安徽气象欢迎您！";
static const char *assistant = "淮北气象三农服务";
#endif


#if defined(__LED_LIXIN__)
static const char *host = "   ";
#endif

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

static const char *const __message_space = "　　";
void MessDisplay(char *message) {
	char *p = pvPortMalloc(strlen(message) + 1);
	DisplayTaskMessage msg;
	strcpy(p, message);
//	strcat(p, __message_space);
	msg.cmd = MSG_CMD_DISPLAY_MESSAGE;
	msg.data.pointData = p;

	if (pdTRUE != xQueueSend(__displayQueue, &msg, configTICK_RATE_HZ)) {
		vPortFree(p);
	}
}

void DisplayMessageRed(const char *message) {
	char *p = pvPortMalloc(strlen(message) + 1 + strlen(__message_space));
	DisplayTaskMessage msg;
	strcpy(p, message);
	strcat(p, __message_space);
	msg.cmd = MSG_CMD_DISPLAY_MESSAGE_RED;
	msg.data.pointData = p;

	if (pdTRUE != xQueueSend(__displayQueue, &msg, configTICK_RATE_HZ)) {
		vPortFree(p);
	}
}

void DisplayMessageGreen(const char *message) {
	char *p = pvPortMalloc(strlen(message) + 1 + strlen(__message_space));
	DisplayTaskMessage msg;
	strcpy(p, message);
	strcat(p, __message_space);
	msg.cmd = MSG_CMD_DISPLAY_MESSAGE_GREEN;
	msg.data.pointData = p;

	if (pdTRUE != xQueueSend(__displayQueue, &msg, configTICK_RATE_HZ)) {
		vPortFree(p);
	}
}

void DisplayScrollNotify(int x) {
	DisplayTaskMessage msg;
	msg.cmd = MSG_CMD_DISPLAY_SCROLL_NOTIFY;
	msg.data.wordData = x;
	xQueueSend(__displayQueue, &msg, configTICK_RATE_HZ);
}

void DisplayMessageYELLOW(const char *message) {
	char *p = pvPortMalloc(strlen(message) + 1 + strlen(__message_space));
	DisplayTaskMessage msg;
	strcpy(p, message);
	strcat(p, __message_space);
	msg.cmd = MSG_CMD_DISPLAY_MESSAGE_YELLOW;
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

#if defined(__LED_LIXIN__)
void __displayMessageLowlevel(void) {
	const uint8_t *tmp;
	if (__displayMessage == NULL) {
		return;
	}
	if (__displayCurrentPoint == NULL) {
		__displayCurrentPoint = __displayMessage;
	}
	LedDisplayClear(0, 0, LED_VIR_DOT_WIDTH - 1, LED_VIR_DOT_HEIGHT / 2 - 1);
	LedDisplayClear(0, LED_VIR_DOT_HEIGHT / 2, LED_VIR_DOT_WIDTH - 1, LED_VIR_DOT_HEIGHT - 1);
	if (__displayMessageColor & 1) {
		tmp = LedDisplayGB2312String32(0, 0, LED_VIR_DOT_WIDTH / 8, 32, __displayCurrentPoint);
	}

	if (__displayMessageColor & 2) {
		tmp = LedDisplayGB2312String32(0, 32, LED_VIR_DOT_WIDTH / 8, 64, __displayCurrentPoint);
	}
	__displayCurrentPoint = tmp;
	LedDisplayToScan(0, 0, LED_PHY_DOT_WIDTH - 1, LED_PHY_DOT_HEIGHT - 1);
}
#endif

#if defined(__LED_HUAIBEI__)
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
		tmp = LedDisplayGB2312String16(0, 0, __displayCurrentPoint);
	}

//	if (__displayMessageColor & 2) {
//		tmp = LedDisplayGB2312String16(0, 4, __displayCurrentPoint);
//	}
	__displayCurrentPoint = tmp;
	LedDisplayToScan(0, 0, LED_DOT_XEND, LED_DOT_YEND);

}
#endif


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
	__displayMessage = msg->data.pointData;
	__displayCurrentPoint = __displayMessage;
	DisplayClear();
	//__displayMessageLowlevel();
}


void __handlerDisplayMessageRed(DisplayTaskMessage *msg) {
	if (__displayMessage) {
		vPortFree((void *)__displayMessage);
	}
	__displayMessage = msg->data.pointData;
	__displayCurrentPoint = __displayMessage;
	__displayMessageColor = 1;
	DisplayClear();
	//__displayMessageLowlevel();
}

void __handlerDisplayMessageGreen(DisplayTaskMessage *msg) {
	if (__displayMessage) {
		vPortFree((void *)__displayMessage);
	}
	__displayMessage = msg->data.pointData;
	__displayCurrentPoint = __displayMessage;
	__displayMessageColor = 2;
	DisplayClear();
	//__displayMessageLowlevel();
}

void __handlerDisplayMessageYellow(DisplayTaskMessage *msg) {
	if (__displayMessage) {
		vPortFree((void *)__displayMessage);
	}
	__displayMessage = msg->data.pointData;
	__displayCurrentPoint = __displayMessage;
	__displayMessageColor = 3;
	DisplayClear();
//	__displayMessageLowlevel();
}
const unsigned char *LedDisplayGB2312String32ScrollUp(int x, int *py, int dy, const unsigned char *gbString);

void __handlerDisplayScrollNotify(DisplayTaskMessage *msg) {
	const uint8_t *tmp;

	static int yorg = LED_PHY_DOT_HEIGHT;
	int dy;

	int y = msg->data.wordData;		          
//	printf("yorg=%d, y=%d, %s\n", yorg, y, __displayCurrentPoint);

	if (__displayMessage == NULL) {
		return;
	}

	if (*__displayCurrentPoint == 0) {
		yorg = LED_PHY_DOT_HEIGHT;
		if (y == yorg){
			__displayCurrentPoint = __displayMessage;
		}
//		if (yorg >= LED_VIR_DOT_HEIGHT) {
//			yorg -= LED_VIR_DOT_HEIGHT;
//		}		
	}
	printf("yorg=%d, y=%d, %s\n", yorg, y, __displayCurrentPoint);

	if (yorg >= LED_VIR_DOT_HEIGHT){
		yorg = yorg - LED_VIR_DOT_HEIGHT;
	} else if (yorg > y) {
		return;
	} else {
		dy = y - yorg;
	}
//
//	if (x < xorg) {
//		if ((LED_VIR_DOT_WIDTH - xorg + x) < 96) return;
//		dx = 96;
//	} else {
//		dx = x - xorg;
//	}

	tmp = LedDisplayGB2312String32ScrollUp(0, &yorg, dy, __displayCurrentPoint);
	if (tmp == __displayCurrentPoint) {
		return;
	}
	__displayCurrentPoint = tmp;
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

#if defined(__LED_LIXIN__)
	{ MSG_CMD_DISPLAY_MESSAGE_RED, __handlerDisplayMessageRed, NULL },
	{ MSG_CMD_DISPLAY_MESSAGE_GREEN, __handlerDisplayMessageGreen, NULL },
	{ MSG_CMD_DISPLAY_MESSAGE_YELLOW, __handlerDisplayMessageYellow, NULL },
	{ MSG_CMD_DISPLAY_SCROLL_NOTIFY, __handlerDisplayScrollNotify, NULL },
#endif

	{ MSG_CMD_DISPLAY_CONTROL, __handlerDisplayControl, NULL},
};

#if defined(__LED_LIXIN__)
void DisplayClear(void) {
	LedDisplayClearAll();
	LedDisplayToScan(0, 0, LED_PHY_DOT_WIDTH - 1, LED_PHY_DOT_HEIGHT - 1);
}
#endif

#if defined(__LED_HUAIBEI__)
void DisplayClear(void) {
	LedDisplayClearAll();
	LedDisplayToScan(0, 0, LED_DOT_XEND, LED_DOT_YEND);
}

void Display2Clear(void) {
	char clear[24];
	int i;
	for (i = 0; i < 24; i++) {
		clear[i] = ' ';
	}
	LedDisplayGB2312String162(0, 0, (const uint8_t *)clear);
	LedDisplayToScan2(2 * 4, 0, LED_DOT_XEND, LED_DOT_YEND);
}
#endif

#if defined(__LED_LIXIN__)
void DisplayTask(void *helloString) {
	portBASE_TYPE rc;
	DisplayTaskMessage msg;
	const char *p;

//	printf("DisplayTask: start-> %s\n", (const char *)helloString);
	__displayQueue = xQueueCreate(5, sizeof(DisplayTaskMessage));
	p = (const char *)(Bank1_NOR2_ADDR + SMS1_PARAM_STORE_ADDR);
	if (isGB2312Start(p[0]) && isGB2312Start(p[1])) {
		host = p;
	} else if (isAsciiStart(p[0])) {
		host = p;
	}
	LedScanOnOff(1);
	DisplayMessageRed((char*)host);
	ScrollDisplayInit();
	while (1) {
		rc = xQueueReceive(__displayQueue, &msg, configTICK_RATE_HZ * 5);
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

#endif

#if defined(__LED_HUAIBEI__)
void DisplayTask(void *helloString) {
	portBASE_TYPE rc;
	DisplayTaskMessage msg;

	printf("DisplayTask: start-> %s\n", (const char *)helloString);
	__displayQueue = xQueueCreate(5, sizeof(DisplayTaskMessage));
	{
		const char *p = (const char *)(Bank1_NOR2_ADDR + SMS1_PARAM_STORE_ADDR);
		if (isGB2312Start(p[0]) && isGB2312Start(p[1])) {
			host = p;
		} else if (isAsciiStart(p[0])) {
			host = p;
		}
		p = (const char *)(Bank1_NOR2_ADDR + SMS2_PARAM_STORE_ADDR);
		if (isGB2312Start(p[0]) && isGB2312Start(p[1])) {
			assistant = p;
		} else if (isAsciiStart(p[0])) {
			assistant = p;
		}
	}
	LedScanOnOff(1);
	while (1) {
		rc = xQueueReceive(__displayQueue, &msg, configTICK_RATE_HZ * 5);
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
//			__displayMessageLowlevel();
		}
	}
}

#endif


void DisplayInit(void) {
	LedScanInit();
	xTaskCreate(DisplayTask, (signed portCHAR *) "DISPLAY", DISPLAY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 10, NULL);
}

#endif

