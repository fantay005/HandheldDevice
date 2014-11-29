#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gsm.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
#include "misc.h"
#include "sms.h"
#include "stm32f10x_gpio.h"
#include "zklib.h"
#include "libupdater.h"

#define SIZE_BUFF  256

extern int GsmTaskSendTcpData(const char *p, int len);
extern char fillbuff(char *buffer);

typedef enum {
	TermActive  = 0x31,
  XCzigbee = 0x36,
} TypeChoose;

typedef enum {
	Login = 0x31,
	Updata =	0x33,
  Querydat = 'B',
} Classific;

typedef struct {
	unsigned char header[2];
	unsigned char lenH;
	unsigned char lenL;
	unsigned char type;
	unsigned char class;
	unsigned short radom;
	unsigned short reserve;
} ProtocolHeader;

typedef struct {
	unsigned char sum[2];
	unsigned char x0D;
	unsigned char x0A;
} ProtocolPadder;

void ProtocolDestroyMessage(const char *p) {
	vPortFree((void *)p);
}

static char HexToChar(unsigned char hex) {
	unsigned char hexTable[] = "0123456789ABCDEF";
	return hexTable[hex & 0x0F];
}

char *ProtocolMessage(TypeChoose type, Classific class, char *message, int *size) {
	int i;
	unsigned char sum = 0;
	unsigned char *p, *ret;
	int len = (message == NULL) ? 0 : strlen(message);

	*size = sizeof(ProtocolHeader) + len + sizeof(ProtocolPadder);
	ret = pvPortMalloc(*size);
	{
		ProtocolHeader *h = (ProtocolHeader *)ret;
		h->header[0] = '#';
		h->header[1] = 'H';
		h->lenH = len >> 8;
		h->lenL = len;
		h->type = type;
		h->class = class;
		h->radom = 0x3030;
		h->reserve = 0x3030;
	}

	if (message != NULL) {
		strcpy((char *)(ret + sizeof(ProtocolHeader)), message);
	}

	p = ret;
	for (i = 0; i < len + sizeof(ProtocolHeader); ++i) {
		sum += *p++;
	}

	*p++ = HexToChar(sum >> 4);
	*p++ = HexToChar(sum);
	*p++ = 0x0D;
	*p = 0x0A;
	return (char *)ret;
}

char *TerminalCreateFeedback(ProtocolHeader *header, int *size) {
	char *msg;
	char *buf = pvPortMalloc(SIZE_BUFF);
  fillbuff(buf);
	msg = ProtocolMessage(header->type, header->class, buf, size);
	vPortFree(buf);
	return msg;
}

char *ProtoclCreatLogin(char *imei, int *size) {
	return ProtocolMessage(TermActive, Login, imei, size);
}

// extern char *feedbackIMEI(void);

// void ProtoclUPloaddata(void) {
// 	int size, size1;
// 	char *tmp, *tmp1, *buf = pvPortMalloc(SIZE_BUFF);
// 	fillbuff(buf);
// 	tmp = ProtocolMessage(TermActive, Updata, buf, &size);
// 	tmp1 = ProtoclCreatLogin(feedbackIMEI(), &size1);
// 	memcpy(buf, tmp1, size1);
// 	memcpy(buf + size1, tmp, size);
// 	ProtocolDestroyMessage(tmp);
// 	ProtocolDestroyMessage(tmp1);
// 	GsmTaskSendTcpData(buf, size + size1);
// 	vPortFree(buf);
// }

void ProtoclUPloaddata(void) {
	int size;
	char *tmp, *buf = pvPortMalloc(SIZE_BUFF);
	fillbuff(buf);
	tmp = ProtocolMessage(TermActive, Updata, buf, &size);
	
	GsmTaskSendTcpData(tmp, size);
	ProtocolDestroyMessage(tmp);
	vPortFree(buf);
}

typedef void (*ProtocolHandleFunction)(ProtocolHeader *header, char *p);
typedef struct {
	unsigned char type;
	unsigned char class;
	ProtocolHandleFunction func;
} ProtocolHandleMap;

static void HandleLogin(ProtocolHeader *header, char *p) {
}

static void HandleUPdat(ProtocolHeader *header, char *p) {
}

static void HandleZigbee(ProtocolHeader *header, char *p) {
	int len;
	p = TerminalCreateFeedback(header, &len);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

void ProtocolHandler(char *p) {
	int i;
	const static ProtocolHandleMap map[] = {
		{'1', '1', HandleLogin},
		{'1', '3', HandleUPdat},
		{'6', 'B', HandleZigbee},
	};
	ProtocolHeader *header = (ProtocolHeader *)p;

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if ((map[i].type == header->type) && (map[i].class == header->class)) {
			map[i].func(header, p + sizeof(ProtocolHeader));
			break;
		}
	}
}


