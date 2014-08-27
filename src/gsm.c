#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "protocol.h"
#include "misc.h"
#include "sms.h"
#include "xfs.h"
#include "zklib.h"
#include "atcmd.h"
#include "norflash.h"
#include "unicode2gbk.h"
#include "gsm.h"
#include "second_datetime.h"

#define GSM_TASK_STACK_SIZE			     (configMINIMAL_STACK_SIZE + 256)
#define GSM_GPRS_HEART_BEAT_TIME     (configTICK_RATE_HZ * 30)
#define WIFI_MAC_LENGTH              12

#define RESET_GPIO_GROUP           GPIOA
#define RESET_GPIO                 GPIO_Pin_11
#define MODE_GPIO_GROUP            GPIOA
#define MODE_GPIO                  GPIO_Pin_12

#define __wifiAssertResetPin()        GPIO_SetBits(RESET_GPIO_GROUP, RESET_GPIO)
#define __wifiDeassertResetPin()      GPIO_ResetBits(RESET_GPIO_GROUP, RESET_GPIO)

#define __wifiAssertModePin()        GPIO_SetBits(MODE_GPIO_GROUP, MODE_GPIO)
#define __wifiDeassertModePin()      GPIO_ResetBits(MODE_GPIO_GROUP, MODE_GPIO)


#define __gsmPortMalloc(size)        pvPortMalloc(size)
#define __gsmPortFree(p)             vPortFree(p)


/// GSM task message queue.
static xQueueHandle __queue;

/// Save the imei of GSM modem, filled when GSM modem start.
static char __mac[WIFI_MAC_LENGTH + 1];

/// Save runtime parameters for GSM task;
static GMSParameter __gsmRuntimeParameter = {"61.190.61.78", 12121, 1};

/// Basic function for sending AT Command, need by atcmd.c.
/// \param  c    Char data to send to modem.
void ATCmdSendChar(char c) {
	USART_SendData(USART2, c);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

/// Store __gsmRuntimeParameter to flash.
static inline void __storeGsmRuntimeParameter(void) {
	NorFlashWrite(GSM_PARAM_STORE_ADDR, (const short *)&__gsmRuntimeParameter, sizeof(__gsmRuntimeParameter));
}

/// Restore __gsmRuntimeParameter from flash.
static inline void __restorGsmRuntimeParameter(void) {
	NorFlashRead(GSM_PARAM_STORE_ADDR, (short *)&__gsmRuntimeParameter, sizeof(__gsmRuntimeParameter));
}

/// Low level set TCP server IP and port.
static void __setGSMserverIPLowLevel(char *ip, int port) {
	strcpy(__gsmRuntimeParameter.serverIP, ip);
	__gsmRuntimeParameter.serverPORT = port;
	__storeGsmRuntimeParameter();
}

typedef enum {
	TYPE_NONE = 0,
	TYPE_SMS_DATA,
	TYPE_RING,
	TYPE_GPRS_DATA,
	TYPE_TUDE_DATA,
	TYPE_SEND_TCP_DATA,
	TYPE_RESET,
	TYPE_NO_CARRIER,
	TYPE_SEND_AT,
	TYPE_SEND_SMS,
	TYPE_RTC_DATA,
	TYPE_SET_GPRS_CONNECTION,
} GsmTaskMessageType;

typedef enum {
	TYPE_SSID,
	TYPE_ATRM,
	TYPE_KEY,
} WifiParaType;
/// Message format send to GSM task.
typedef struct {
	/// Message type.
	GsmTaskMessageType type;
	/// Message lenght.
	unsigned int length;
} GsmTaskMessage;


/// Get the data of a message.
/// \param  message    Which message to get data from.
/// \return The associate data of the message.
static inline void *__gsmGetMessageData(GsmTaskMessage *message) {
	return &message[1];
}

/// Create a message.
/// \param  type   The type of message to create.
/// \param  data   Associate this data to the message.
/// \param  len    Then lenght(byte number) of the data.
/// \return !=NULL The message which created.
/// \return ==NULL Create message failed.
GsmTaskMessage *__gsmCreateMessage(GsmTaskMessageType type, const char *dat, int len) {
	GsmTaskMessage *message = __gsmPortMalloc(ALIGNED_SIZEOF(GsmTaskMessage) + len);
	if (message != NULL) {
		message->type = type;
		message->length = len;
		memcpy(&message[1], dat, len);
	}
	return message;
}

/// Destroy a message.
/// \param  message   The message to destory.
void __gsmDestroyMessage(GsmTaskMessage *message) {
	__gsmPortFree(message);
}

int GsmTaskResetSystemAfter(int seconds) {
	GsmTaskMessage *message = __gsmCreateMessage(TYPE_RESET, 0, 0);
	message->length = seconds;
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 5)) {
		__gsmDestroyMessage(message);
		return 0;
	}
	return 1;
}


/// Send a AT command to GSM modem.
/// \param  atcmd  AT command to send.
/// \return true   When operation append to GSM task message queue.
/// \return false  When append operation to GSM task message queue failed.
bool GsmTaskSendAtCommand(const char *atcmd) {
	int len = strlen(atcmd);
	GsmTaskMessage *message = __gsmCreateMessage(TYPE_SEND_AT, atcmd, len + 2);
	char *dat = __gsmGetMessageData(message);
	dat[len] = '\r';
	dat[len + 1] = 0;
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 5)) {
		__gsmDestroyMessage(message);
		return false;
	}
	return true;

}

/// Send a AT command to GSM modem.
/// \param  atcmd  AT command to send.
/// \return true   When operation append to GSM task message queue.
/// \return false  When append operation to GSM task message queue failed.
bool GsmTaskSendSMS(const char *pdu, int len) {
	GsmTaskMessage *message = __gsmCreateMessage(TYPE_SEND_SMS, pdu, len);
	char *dat = __gsmGetMessageData(message);
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 15)) {
		__gsmDestroyMessage(message);
		return false;
	}
	return true;
}

/// Send data to TCP server.
/// \param  dat    Data to send.
/// \param  len    Then length of the data.
/// \return true   When operation append to GSM task message queue.
/// \return false  When append operation to GSM task message queue failed.
bool GsmTaskSendTcpData(const char *dat, int len) {
	GsmTaskMessage *message = __gsmCreateMessage(TYPE_SEND_TCP_DATA, dat, len);
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 5)) {
		__gsmDestroyMessage(message);
		return true;
	}
	return false;
}

bool GsmTaskSetGprsConnect(bool isOn) {
	GsmTaskMessage *message = __gsmCreateMessage(TYPE_SET_GPRS_CONNECTION, (char *)&isOn, sizeof(isOn));
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 15)) {
		__gsmDestroyMessage(message);
		return false;
	}
	return true;
}


static void __wifiInitUsart(int baud) {
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);
}

/// Init the CPU on chip hardware for the GSM modem.
static void __gsmInitHardware(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);				   //GSMģ��Ĵ���

	__wifiAssertResetPin();
	GPIO_InitStructure.GPIO_Pin =  RESET_GPIO;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(RESET_GPIO_GROUP, &GPIO_InitStructure);				    //RESET

	__wifiDeassertModePin();
	GPIO_InitStructure.GPIO_Pin =  MODE_GPIO;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(MODE_GPIO_GROUP, &GPIO_InitStructure);				    //READY

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

typedef struct {
	const char *prefix;
	GsmTaskMessageType type;
} GSMAutoReportMap;

static const GSMAutoReportMap __gsmAutoReportMaps[] = {
//	{ "+CMT", TYPE_SMS_DATA },
	{ "RING", TYPE_RING },
	{ "NO CARRIER", TYPE_NO_CARRIER },
	{ NULL, TYPE_NONE },
};



static char buffer[1300];
static int bufferIndex = 0;
static char isIPD = 0;
static char isSMS = 0;
static char isRTC = 0;
static char isTUDE = 0;
static int lenIPD;

static inline void __gmsReceiveIPDData(unsigned char data) {
	if (data == 0x0A) {
		GsmTaskMessage *message;
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_GPRS_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isIPD = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}


static inline void __gmsReceiveSMSData(unsigned char data) {
	if (data == 0x0A) {
		GsmTaskMessage *message;
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_SMS_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isSMS = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}

static inline void __gmsReceiveRTCData(unsigned char data) {
	if (data == 0x0A) {
		GsmTaskMessage *message;
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_RTC_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isRTC = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}

static inline void __gmsReceiveTUDEData(unsigned char data) {
	if (data == 0x0A) {
		GsmTaskMessage *message;
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_TUDE_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isTUDE = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}

void USART2_IRQHandler(void) {
	unsigned char data;
	if (USART_GetITStatus(USART2, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(USART2);
	USART_SendData(USART1, data);
	USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	if (isIPD) {
		__gmsReceiveIPDData(data);
		return;
	}

	if (isSMS) {
		__gmsReceiveSMSData(data);
		return;
	}
	
	if (isRTC) {
		__gmsReceiveRTCData(data);
		return;
	}
	
	if (isTUDE) {
		__gmsReceiveTUDEData(data);
		return;
	}

	if (data == 0x0A) {
		buffer[bufferIndex++] = 0;
		if (bufferIndex >= 2) {
			portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
			const GSMAutoReportMap *p;
			if (strncmp(buffer, "+CMT:", 5) == 0) {
				bufferIndex = 0;
				isSMS = 1;
			}
			for (p = __gsmAutoReportMaps; p->prefix != NULL; ++p) {
				if (strncmp(p->prefix, buffer, strlen(p->prefix)) == 0) {
					GsmTaskMessage *message = __gsmCreateMessage(p->type, buffer, bufferIndex);
					xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken);
					break;
				}
			}
			if (p->prefix == NULL) {
				ATCommandGotLineFromIsr(buffer, bufferIndex, &xHigherPriorityTaskWoken);
			}

			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
		if ((bufferIndex == 2) && (strncmp("#H", buffer, 2) == 0)) {
			isIPD = 1;
		}
		
		if (strncmp(buffer, "+QNITZ: ", 8) == 0) {
			bufferIndex = 0;
			isRTC = 1;
		}
		
		if (strncmp(buffer, "+QGSMLOC: ", 10) == 0) {
			bufferIndex = 0;
			isTUDE = 1;
		}
	}
}


bool __gsmIsValidMac(const char *p) {
	int i;
	if (strlen(p) != WIFI_MAC_LENGTH + 4) {
		return false;
	}
	return true;
}
/// Start GSM modem.
void __wifiModemStart() {
	__wifiDeassertResetPin();
	vTaskDelay(configTICK_RATE_HZ / 10);

	__wifiAssertResetPin();
	vTaskDelay(configTICK_RATE_HZ / 2);

	__wifiAssertModePin();
	vTaskDelay(configTICK_RATE_HZ * 2);
}


void __gsmSendTcpDataLowLevel(const char *p, int len) {
	int i;
	for (i = 0; i < len; i++) {
		ATCmdSendChar(*p++);
	}
}

bool __initWifiRuntime() {
	int i;
	char *reply;
	static const int bauds[] = {115200, 9600};
	vTaskDelay(configTICK_RATE_HZ * 12);
	for (i = 0; i < ARRAY_MEMBER_NUMBER(bauds); ++i) {
		// ���ò�����
		printf("Init wifi baud: %d\n", bauds[i]);
		__wifiInitUsart(bauds[i]);
		ATCommandAndCheckReply("+++", "+OK", configTICK_RATE_HZ * 5);
		ATCommandAndCheckReply("AT+\r", "+OK", configTICK_RATE_HZ *5);
		if (ATCommandAndCheckReply("AT+\r", "+OK", configTICK_RATE_HZ * 5)){
			break;
		}
	}

	if (i >= ARRAY_MEMBER_NUMBER(bauds)) {
		printf("All baud error\n");
		return false;
	}
	
  if (!ATCommandAndCheckReply("AT+RSTF\r", "+OK", configTICK_RATE_HZ * 10)) {
		printf("AT+RSTF error\r");
		return false;
	}
	
	if (!ATCommandAndCheckReply("AT+ATM=!1\r", "+OK", configTICK_RATE_HZ * 5)) {
		printf("AT+ATM=1 error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+Z\r", "+OK", configTICK_RATE_HZ * 5)) {
		printf("AT+Z error\r");
		return false;
	}

	vTaskDelay(configTICK_RATE_HZ * 12);
	
	do {
		printf("AT+WPRT error\r");
	}while(!ATCommandAndCheckReply("AT+WPRT=!0\r", "+OK", configTICK_RATE_HZ * 10));
	

	if (!ATCommandAndCheckReply("AT+ATPT=!100\r", "+OK", configTICK_RATE_HZ * 10)) {
		printf("AT+ATPT error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+SSID=!\"ZKJC_CMD\"\r", "+OK", configTICK_RATE_HZ * 10)) {
		printf("AT+SSID error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+ATRM=!0,0,\"192.168.1.108\",60000\r", "+OK", configTICK_RATE_HZ * 10)) {
		printf("AT+ATRM error\r");
		return false;
	}

  do {
		printf("AT+ENCRY error\r");
	}	while (!ATCommandAndCheckReply("AT+ENCRY=!6\r", "+OK", configTICK_RATE_HZ * 5));

	if (!ATCommandAndCheckReply("AT+KEY=!1,0,\"5578900000\"\r", "+OK", configTICK_RATE_HZ * 10)) {
		printf("AT+KEY error\r");
		return false;
	}
	
	if (!ATCommandAndCheckReply("AT+NIP=!0\r", "+OK", configTICK_RATE_HZ * 10)) {
		printf("AT+NIP error\r");
		return false;
	}
	
	reply = ATCommand("AT+QMAC\r", ATCMD_ANY_REPLY_PREFIX, configTICK_RATE_HZ * 5);
	if (reply == NULL) {
		return false;
	}
	if (!__gsmIsValidMac(reply)) {
		return false;
	}
	strcpy(__mac, &reply[4]);
	AtCommandDropReplyLine(reply);
	
	if (!ATCommandAndCheckReply("AT+ATM=!0\r", "+OK", configTICK_RATE_HZ * 10)) {
		printf("AT+ATM=0 error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+PMTF\r", "+OK", configTICK_RATE_HZ * 10)) {
		printf("AT+PMTF error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+Z\r", "+OK", configTICK_RATE_HZ * 10)) {
		printf("AT+Z RESET error\r");
		return false;
	}

	return true;
}

bool __setWifiParameter(WifiParaType type, char *dat) {
	int i;
	char *buff;
	static const int bauds[] = {115200, 19200, 9600};
	for (i = 0; i < ARRAY_MEMBER_NUMBER(bauds); ++i) {
		// ���ò�����
		printf("Init wifi baud: %d\n", bauds[i]);
		__wifiInitUsart(bauds[i]);
		ATCommandAndCheckReply("+++", "+OK", configTICK_RATE_HZ * 5);
		ATCommandAndCheckReply("AT+\r", "+OK", configTICK_RATE_HZ * 5);
		ATCommandAndCheckReply("AT+\r", "+OK", configTICK_RATE_HZ * 5);
	}
	if (i >= ARRAY_MEMBER_NUMBER(bauds)) {
		printf("All baud error\n");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+ATM=!1\r", "+OK", configTICK_RATE_HZ * 5)) {
		printf("AT+ATM=1 error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+Z\r", "+OK", configTICK_RATE_HZ * 5)) {
		printf("AT+Z error\r");
		return false;
	}

	vTaskDelay(configTICK_RATE_HZ * 10);
	
	switch (type){
		case TYPE_SSID:
			buff = pvPortMalloc(64);
			sprintf(buff, "AT+SSID=!\"%s\"", dat);
			if (!ATCommandAndCheckReply(buff, "+OK", configTICK_RATE_HZ * 5)) {
				printf("AT+SSID error\r");
				return false;
			}
			vPortFree(buff);
	}

	switch (type){
		case TYPE_ATRM:
			buff = pvPortMalloc(64);
			sprintf(buff, "AT+ATRM=!0,0,\"%s\",%d\r", dat);
			if (!ATCommandAndCheckReply(buff, "+OK", configTICK_RATE_HZ * 5)) {
				printf("AT+ATRM error\r");
				return false;
			}
			vPortFree(buff);
	}

	switch (type){
		case TYPE_KEY:
			buff = pvPortMalloc(64);
			sprintf(buff, "AT+KEY=!1,0,\"%s\"\r", dat);
			if (!ATCommandAndCheckReply(buff, "+OK", configTICK_RATE_HZ * 5)) {
				printf("AT+KEY error\r");
				return false;
			}
			vPortFree(buff);
	}

	if (!ATCommandAndCheckReply("AT+ATM=!0\r", "+OK", configTICK_RATE_HZ * 5)) {
		printf("AT+ATM=0 error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+PMTF\r", "+OK", configTICK_RATE_HZ * 5)) {
		printf("AT+PMTF error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+Z\r", "+OK", configTICK_RATE_HZ * 2)) {
		printf("AT+Z RESET error\r");
		return false;
	}

	return true;
}

void __handleSMS(GsmTaskMessage *p) {
	SMSInfo *sms;
	const char *dat = __gsmGetMessageData(p);
	sms = __gsmPortMalloc(sizeof(SMSInfo));
	printf("Gsm: got sms => %s\n", dat);
	SMSDecodePdu(dat, sms);
	if(sms->contentLen == 0) {
		__gsmPortFree(sms);
		return;
	}
	printf("Gsm: sms_content=> %s\n", sms->content);
#if defined(__SPEAKER__)
	XfsTaskSpeakGBK(sms->content, sms->contentLen);
#elif defined(__LED__)
	ProtocolHandlerSMS(sms);
#endif
	__gsmPortFree(sms);
}

void __handleProtocol(GsmTaskMessage *msg) {
	ProtocolHandler(__gsmGetMessageData(msg));
}

void __handleSendTcpDataLowLevel(GsmTaskMessage *msg) {
	__gsmSendTcpDataLowLevel(__gsmGetMessageData(msg), msg->length);
}

void __handleReset(GsmTaskMessage *msg) {
	unsigned int len = msg->length;
	if (len > 100) {
		len = 100;
	}
	vTaskDelay(configTICK_RATE_HZ * len);
	while (1) {
		NVIC_SystemReset();
		vTaskDelay(configTICK_RATE_HZ);
	}
}

void __handleResetNoCarrier(GsmTaskMessage *msg) {
	GPIO_SetBits(GPIOD, GPIO_Pin_2);
}

void __handleRING(GsmTaskMessage *msg) {
	GPIO_ResetBits(GPIOD, GPIO_Pin_2);
}


void __handleSendAtCommand(GsmTaskMessage *msg) {
	ATCommand(__gsmGetMessageData(msg), NULL, configTICK_RATE_HZ / 10);
}

void __handleSendSMS(GsmTaskMessage *msg) {
	static const char *hexTable = "0123456789ABCDEF";
	char buf[16];
	int i;
	char *p = __gsmGetMessageData(msg);
	sprintf(buf, "AT+CMGS=%d\r", msg->length - 1);
	ATCommand(buf, NULL, configTICK_RATE_HZ / 5);
	for (i = 0; i < msg->length; ++i) {
		ATCmdSendChar(hexTable[*p >> 4]);
		ATCmdSendChar(hexTable[*p & 0x0F]);
		++p;
	}
	ATCmdSendChar(0x1A);

	p = ATCommand(NULL, "OK", configTICK_RATE_HZ * 15);
	if (p != NULL) {
		AtCommandDropReplyLine(p);
		printf("Send SMS OK.\n");
	} else {
		printf("Send SMS error.\n");
	}
}


void __handleGprsConnection(GsmTaskMessage *msg) {	
	bool *dat = __gsmGetMessageData(msg);
	__gsmRuntimeParameter.isonTCP = *dat;
    __storeGsmRuntimeParameter();
	if(!__gsmRuntimeParameter.isonTCP){
	   	if (!ATCommandAndCheckReply("AT+CGATT=0\r", "OK", configTICK_RATE_HZ )) {
		     printf("AT+CGATT error\r");
	    }
	}
}

void __handleM35RTC(GsmTaskMessage *msg) {
	DateTime dateTime;
	char *p = __gsmGetMessageData(msg);	 
	p++;
	dateTime.year = (p[0] - '0') * 10 + (p[1] - '0');
	dateTime.month = (p[3] - '0') * 10 + (p[4] - '0');
	dateTime.date = (p[6] - '0') * 10 + (p[7] - '0');
	dateTime.hour = (p[9] - '0') * 10 + (p[10] - '0') + 8;
	dateTime.minute = (p[12] - '0') * 10 + (p[13] - '0');
	dateTime.second = (p[15] - '0') * 10 + (p[16] - '0');
	RtcSetTime(DateTimeToSecond(&dateTime));
}

void __handleTUDE(GsmTaskMessage *msg) {	
}

typedef struct {
	GsmTaskMessageType type;
	void (*handlerFunc)(GsmTaskMessage *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ TYPE_SMS_DATA, __handleSMS },
	{ TYPE_RING, __handleRING },
	{ TYPE_GPRS_DATA, __handleProtocol },
  { TYPE_RTC_DATA, __handleM35RTC},
  { TYPE_TUDE_DATA, __handleTUDE},
	{ TYPE_SEND_TCP_DATA, __handleSendTcpDataLowLevel },
	{ TYPE_RESET, __handleReset },
	{ TYPE_NO_CARRIER, __handleResetNoCarrier },
	{ TYPE_SEND_AT, __handleSendAtCommand },
	{ TYPE_SEND_SMS, __handleSendSMS },
  { TYPE_SET_GPRS_CONNECTION, __handleGprsConnection },
	{ TYPE_NONE, NULL },
};

static void __gsmTask(void *parameter) {
	portBASE_TYPE rc;
	GsmTaskMessage *message;
	portTickType lastT = 0;

	while (1) {
		printf("Wifi start\n");
		__wifiModemStart();
		if (__initWifiRuntime()) {
			break;
		}
	}

	for (;;) {
		printf("Gsm: loop again\n");
		rc = xQueueReceive(__queue, &message, configTICK_RATE_HZ * 10);
		if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			for (; map->type != TYPE_NONE; ++map) {
				if (message->type == map->type) {
					map->handlerFunc(message);
					break;
				}
			}
			__gsmDestroyMessage(message);
		} else {
			int curT;
			if(__gsmRuntimeParameter.isonTCP == 0){
			   continue;
			}
			curT = xTaskGetTickCount();
      if ((curT - lastT) >= GSM_GPRS_HEART_BEAT_TIME) {
				char *dat = pvPortMalloc(20);		
				sprintf(dat, "#H%s\r\n", __mac);
				__gsmSendTcpDataLowLevel(dat, strlen(dat));
				ProtocolDestroyMessage(dat);
				lastT = curT;
			} 
		}
	}
}


void GSMInit(void) {
	ATCommandRuntimeInit();
	__gsmInitHardware();
	__queue = xQueueCreate(5, sizeof(GsmTaskMessage *));
	xTaskCreate(__gsmTask, (signed portCHAR *) "GSM", GSM_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}
