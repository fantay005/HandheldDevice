#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "rtc.h"
#include "gsm.h"
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
#include "soundcontrol.h"
#include "second_datetime.h"
#include "sms_cmd.h"

#define GSM_TASK_STACK_SIZE			     (configMINIMAL_STACK_SIZE + 512)
#define GSM_GPRS_HEART_BEAT_TIME     (configTICK_RATE_HZ * 60 * 5)
#define GSM_IMEI_LENGTH              15

#define  RING_PIN  GPIO_Pin_15

#if defined(__SPEAKER_V1__)
#  define RESET_GPIO_GROUP           GPIOA
#  define RESET_GPIO                 GPIO_Pin_11
#elif defined(__SPEAKER_V2__)|| defined (__SPEAKER_V3__)
#  define RESET_GPIO_GROUP           GPIOG
#  define RESET_GPIO                 GPIO_Pin_10
#elif defined(__LED__)
#  define RESET_GPIO_GROUP           GPIOB
#  define RESET_GPIO                 GPIO_Pin_1
#endif

#define __gsmAssertResetPin()        GPIO_SetBits(RESET_GPIO_GROUP, RESET_GPIO)
#define __gsmDeassertResetPin()      GPIO_ResetBits(RESET_GPIO_GROUP, RESET_GPIO)

#if defined (__SPEAKER_V3__)
#define __gsmPowerSupplyOn()         GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define __gsmPowerSupplyOff()        GPIO_SetBits(GPIOB, GPIO_Pin_0)

#else
#define __gsmPowerSupplyOn()         GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define __gsmPowerSupplyOff()        GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#endif

#define __gsmPortMalloc(size)        pvPortMalloc(size)
#define __gsmPortFree(p)             vPortFree(p)


XFSspeakParam  __speakParam;

void __gsmSMSEncodeConvertToGBK(SMSInfo *info ) {
	uint8_t *gbk;

	if (info->encodeType == ENCODE_TYPE_GBK) {
		return;
	}
	gbk = Unicode2GBK((const uint8_t *)info->content, info->contentLen);
	strcpy((char *)info->content, (const char *)gbk);
	Unicode2GBKDestroy(gbk);
	info->encodeType = ENCODE_TYPE_GBK;
	info->contentLen = strlen((const char *)info->content);
}


/// GSM task message queue.
static xQueueHandle __queue;

/// Save the imei of GSM modem, filled when GSM modem start.
static char __imei[GSM_IMEI_LENGTH + 1];

const char *GsmGetIMEI(void) {
	return __imei;
}

/// Save runtime parameters for GSM task;
static GMSParameter __gsmRuntimeParameter = {"61.190.61.78", 5555, "221.130.129.72", 5555, 1, 1, "0620"};	   // 老平台服务器及端口："221.130.129.72",5555                                                                                    // 新平台服务器及端口: "61.190.61.78",5555
/// Basic function for sending AT Command, need by atcmd.c.
/// \param  c    Char data to send to modem.
void ATCmdSendChar(char c) {
	USART_SendData(USART2, c);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

//Store __gsmRuntimeParameter to flash.
static inline void __storeGsmRuntimeParameter(void) {
	NorFlashWrite(GSM_PARAM_STORE_ADDR, (const short *)&__gsmRuntimeParameter, sizeof(__gsmRuntimeParameter));
}

// Restore __gsmRuntimeParameter from flash.
//static inline void __restorGsmRuntimeParameter(void) {
//	NorFlashRead(GSM_PARAM_STORE_ADDR, (short *)&__gsmRuntimeParameter, sizeof(__gsmRuntimeParameter));
//}


/// Low level set TCP server IP and port.
//static void __setGSMserverIPLowLevel(char *ip, int port) {
//	strcpy(__gsmRuntimeParameter.serverIP, ip);
//	__gsmRuntimeParameter.serverPORT = port;
//	__storeGsmRuntimeParameter();
//}

typedef enum {
	TYPE_NONE = 0,
	TYPE_SMS_DATA,
	TYPE_RING,
	TYPE_GPRS_DATA,
	TYPE_RTC_DATA,
	TYPE_TUDE_DATA,
	TYPE_SEND_TCP_DATA,
	TYPE_RESET,
	TYPE_NO_CARRIER,
	TYPE_SEND_AT,
	TYPE_SEND_SMS,
	TYPE_SET_GPRS_CONNECTION,
	TYPE_SETIP,
	TYPE_HTTP_DOWNLOAD,
	TYPE_SET_NIGHT_QUIET,
	TYPE_QUIET_TIME,
} GsmTaskMessageType;

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
    GsmTaskMessage *message;
    if(strncasecmp((const char *)pdu, "<SETIP>", 7) == 0){
	   message = __gsmCreateMessage(TYPE_SETIP, &pdu[7], len-7);
	} else if (strncasecmp((const char *)pdu, "<QUIET>", 7) == 0){
	   message = __gsmCreateMessage(TYPE_QUIET_TIME, &pdu[9], len-9);
	} else {
	   message = __gsmCreateMessage(TYPE_SEND_SMS, pdu, len);
	}
//	char *dat = __gsmGetMessageData(message);
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 15)) {
		__gsmDestroyMessage(message);
		return false;
	}
	return true;
}


bool GsmTaskSetParameter(const char *dat, int len) {
	GsmTaskMessage *message;
	  if(strncasecmp((const char *)dat, "<CTCP>", 6) == 0){
	   message = __gsmCreateMessage(TYPE_SET_GPRS_CONNECTION, &dat[6], 1);
	} else if(strncasecmp((const char *)dat, "<QUIET>", 7) == 0){
	   message = __gsmCreateMessage(TYPE_SET_NIGHT_QUIET, &dat[7], 1);
	}
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
	GsmTaskMessage *message;
	if(strncasecmp((const char *)dat, "<http>", 6) == 0){
	    message = __gsmCreateMessage(TYPE_HTTP_DOWNLOAD, &dat[6], (len - 6));
	} else {	
	    message = __gsmCreateMessage(TYPE_SEND_TCP_DATA, dat, len);
	}
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 15)) {
		__gsmDestroyMessage(message);
		return true;
	}
	return false;
}

static void __gsmInitUsart(int baud) {
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
	GPIO_Init(GPIOA, &GPIO_InitStructure);				   //GSM模块的串口

	__gsmDeassertResetPin();
	GPIO_InitStructure.GPIO_Pin =  RESET_GPIO;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(RESET_GPIO_GROUP, &GPIO_InitStructure);				    //GSM模块的RTS和RESET

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOG, &GPIO_InitStructure);				    //GSM模块的RTS和RESET

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);				   //GSM模块的STATAS

	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);				   //__gsmPowerSupplyOn,29302

#if defined(__SPEAKER_V3__)
	GPIO_ResetBits(GPIOG, GPIO_Pin_14);
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOG, &GPIO_InitStructure);	               //SMS到来标志位

    GPIO_ResetBits(GPIOG, RING_PIN);
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOG, &GPIO_InitStructure);				   //RING到来标志位

	GPIO_SetBits(GPIOD, GPIO_Pin_3);
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);				   //判断TCP是否打开
#endif

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
//	{ "RING", TYPE_RING },
	{ "NO CARRIER", TYPE_NO_CARRIER },
	{ NULL, TYPE_NONE },
};



static char buffer[4096];
static int bufferIndex = 0;
static char isIPD = 0;
static char isSMS = 0;
static char isRTC = 0;
static char isTUDE = 0;
static char Which = 0;
static char isRING = 0;
static char CardisEXIST = 1;
static int lenIPD;

static inline void __gmsReceiveIPDData(unsigned char data) {
	if (isIPD == 1) {
		lenIPD = data << 8;
		isIPD = 2;
	} else if (isIPD == 2) {
		lenIPD += data;
		isIPD = 3;
	}
	buffer[bufferIndex++] = data;
	if ((isIPD == 3) && (bufferIndex >= lenIPD + 14)) {
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		GsmTaskMessage *message;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_GPRS_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isIPD = 0;
		bufferIndex = 0;
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

static char tick = 0;
static inline void __gmsReceiveRINGData(unsigned char data) {
	if (data == 0x0A) {
		tick++;
		buffer[bufferIndex++] = 0;
		if(tick >= 2){
		    GsmTaskMessage *message;
				portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
				message = __gsmCreateMessage(TYPE_RING, buffer, bufferIndex);
				if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
					if (xHigherPriorityTaskWoken) {
						taskYIELD();
					}
				}
				isRING = 0;				
				tick = 0;
		}
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	  if (strncmp(buffer, "+CLIP: \"", 8) == 0) {
			bufferIndex = 0;
		}
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
	char buf[64];
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
	
	if (isRING) {
		__gmsReceiveRINGData(data);
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
			
			if (strncmp(buffer, "RING", 4) == 0) {
				bufferIndex = 0;
				isRING = 1;
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
		sprintf(buf, "RECV FROM:%s:%d",__gsmRuntimeParameter.serverIP, __gsmRuntimeParameter.serverPORT);
		if (strncmp(buffer, (const char *)&buf, (12 + strlen(__gsmRuntimeParameter.serverIP))) == 0) {
			bufferIndex = 0;
			Which = 1;
		}
		
		sprintf(buf, "RECV FROM:%s:%d",__gsmRuntimeParameter.detectionIP, __gsmRuntimeParameter.detectionPORT);
		if (strncmp(buffer, (const char *)&buf, (12 + strlen(__gsmRuntimeParameter.detectionIP))) == 0) {
			bufferIndex = 0;
			Which = 2;
		}
		
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
		
		if (strncmp(buffer, "+CME ERROR: 3517", 16) == 0) {
			bufferIndex = 0;
			CardisEXIST = 0;
		}
		
		if (strncmp(buffer, "CALL READY", 10) == 0) {
			bufferIndex = 0;
			CardisEXIST = 1;
		}
	}
}

/// Start GSM modem.
void __gsmModemStart() {
	__gsmPowerSupplyOff();
	vTaskDelay(configTICK_RATE_HZ / 2);

	__gsmPowerSupplyOn();
	vTaskDelay(configTICK_RATE_HZ / 2);

	__gsmAssertResetPin();
	vTaskDelay(configTICK_RATE_HZ * 2);

	__gsmDeassertResetPin();
	vTaskDelay(configTICK_RATE_HZ * 5);
}


static  char stamp = 0;
static  char sign = 0;				 

bool sound_Prompt(void) {
	int i;
	char prompt[58] = {0xFD, 0x00, 0x37, 0x01, 0x01, '[', 'm', '5', '3', ']', 's', 'o', 'u', 'n', 'd', '2', '2', '5',//sound225
                     0xBB, 0xB6,  0xD3, 0xAD,  0xC4, 0xFA,  0xCA, 0xB9,  0xD3, 0xC3,  ',', 0xB0, 0xB2,  0xBB, 0xD5, 
                     0xD6, 0xD0,  0xBF, 0xC6,  0xBD, 0xF0,  0xB3, 0xCF,  ',', 0xD6, 0xC7,  0xC4, 0xDC,  0xCE, 0xDE, 
		                 0xCF, 0xDF,  0xB9, 0xE3,  0xB2, 0xA5,  0xB2, 0xFA,  0xC6, 0xB7 }; //欢迎您使用,安徽中科金诚,智能无线广播产品
  if(stamp != 1) return false;
  SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
	for (i = 0; i < 58; i++) {
		USART_SendData(USART3, prompt[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 10);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
	recover();
	return true;
}
/// Check if has the GSM modem connect to a TCP server.
/// \return true   When the GSM modem has connect to a TCP server.
/// \return false  When the GSM modem dose not connect to a TCP server.
static char Judge = 0;
static char decideI = 0;
static char decideII = 0;

bool __gsmIsTcpConnected() {
	char *reply;
	char buf[64];

	while (1) {
		reply = ATCommand("AT+QISTATE\r", "OK", configTICK_RATE_HZ * 2);
		if (reply == NULL) {
			return false;
		}
		if (!ATCommandAndCheckReply(NULL, "STATE: IP PROCESSING", configTICK_RATE_HZ * 20)) {
			return false;
		}
		if(Judge == 1){
				sprintf(buf, "+QISTATE: 0, \"TCP\", \"%s\", %d,\"CONNECTED\"", __gsmRuntimeParameter.serverIP, __gsmRuntimeParameter.serverPORT);
				if (ATCommandAndCheckReply(NULL, (const char*)&buf, configTICK_RATE_HZ * 10)){
					stamp++;
					sign = 0;			
					if(stamp >= 5) stamp = 5;
					AtCommandDropReplyLine(reply);
					sound_Prompt();
					Judge = 0;
					decideI = 1;
					return true;
				}
				
				sprintf(buf, "+QISTATE: 1, \"TCP\", \"%s\", %d,\"CONNECTED\"", __gsmRuntimeParameter.detectionIP, __gsmRuntimeParameter.detectionPORT);
				if (ATCommandAndCheckReply(NULL, (const char*)&buf, configTICK_RATE_HZ * 10)){
					AtCommandDropReplyLine(reply);
					Judge = 0;
					decideII = 1;
					return true;
		    }
		} else if(Judge == 2){
				sprintf(buf, "+QISTATE: 0, \"TCP\", \"%s\", %d,\"CONNECTED\"", __gsmRuntimeParameter.serverIP, __gsmRuntimeParameter.serverPORT);
				if(ATCommandAndCheckReply(NULL, (const char*)&buf, configTICK_RATE_HZ * 20)){
					decideI = 1;
				} else {
					decideI = 0;
				}
				sprintf(buf, "+QISTATE: 1, \"TCP\", \"%s\", %d,\"CONNECTED\"", __gsmRuntimeParameter.detectionIP, __gsmRuntimeParameter.detectionPORT);
				if (ATCommandAndCheckReply(NULL, (const char*)&buf, configTICK_RATE_HZ * 10)){
					AtCommandDropReplyLine(reply);
					Judge = 0;
					decideII = 1;
					return true;
		    } else {
					decideII = 0;
				}
	  }
		AtCommandDropReplyLine(reply);
		break;
  }
	return false;

}

bool __gsmSendTcpDataLowLevel(const char *p, int len) {
	int i;
	char buf[16];
	char *reply;
	if (Which == 1) { 
		sprintf(buf, "AT+QISEND=0,%d\r", len);		  //len多大1460
		ATCommand(buf, NULL, configTICK_RATE_HZ / 2);
		for (i = 0; i < len; i++) {
			ATCmdSendChar(*p++);
		}
	} else if(Which == 2){
		sprintf(buf, "AT+QISEND=1,%d\r", len);		  //len多大1460
		ATCommand(buf, NULL, configTICK_RATE_HZ / 2);
		for (i = 0; i < len; i++) {
			ATCmdSendChar(*p++);
		}
	}	
  Which = 0;
	
	while (1) {
		reply = ATCommand(NULL, "SEND", configTICK_RATE_HZ * 3);
		if (reply == NULL) {
			return false;
		}
		if (0 == strncmp(reply, "SEND OK", 7)) {
			AtCommandDropReplyLine(reply);
			return true;
		} else if (0 == strncmp(reply, "SEND FAIL", 9)) {
			AtCommandDropReplyLine(reply);
			return false;
		} else if (0 == strncmp(reply, "ERROR", 5)) {
			AtCommandDropReplyLine(reply);
			return false;
		} else {
			AtCommandDropReplyLine(reply);
		}
	}
}

bool __gsmCheckTcpAndConnect(const char *ip, unsigned short port) {
	char buf[44];
	char buff[20];
	char *reply;
	if (__gsmIsTcpConnected()) {
		return true;
	}
	
	if((decideI == 0) && (decideII == 0) && (Judge == 1)){
		ATCommand("AT+QIDEACT\r", NULL, configTICK_RATE_HZ * 2);
		sprintf(buf, "AT+QIOPEN=0,\"TCP\",\"%s\",%d\r", ip, port);
		reply = ATCommand(buf, "OK", configTICK_RATE_HZ * 40);
		sprintf(buff, "0, CONNECT OK");
		Which = 1;
	} else if ((decideI == 0) && (decideII == 0) && (Judge == 2)){
    ATCommand("AT+QIDEACT\r", NULL, configTICK_RATE_HZ * 2);
		sprintf(buf, "AT+QIOPEN=1,\"TCP\",\"%s\",%d\r", ip, port);
		reply = ATCommand(buf, "OK", configTICK_RATE_HZ * 40);
		sprintf(buff, "1, CONNECT OK");
		Which = 2;
	}else if ((decideI == 1) && (decideII == 0)){
		sprintf(buf, "AT+QIOPEN=1,\"TCP\",\"%s\",%d\r", ip, port);
		reply = ATCommand(buf, "OK", configTICK_RATE_HZ * 40);
		sprintf(buff, "1, CONNECT OK");
		Which = 2;
	} else if ((decideI == 0) && (decideII == 1)){
		sprintf(buf, "AT+QIOPEN=0,\"TCP\",\"%s\",%d\r", ip, port);
		reply = ATCommand(buf, "OK", configTICK_RATE_HZ * 40);
		sprintf(buff, "0, CONNECT OK");
		Which = 1;
	}
	if (reply == NULL) {
		return false;
	}
	
  if (ATCommandAndCheckReply(NULL, (const char *)&buff, configTICK_RATE_HZ * 30)) {
	 	int size;
		const char *data;
		AtCommandDropReplyLine(reply);
		data = ProtoclCreatLogin(__imei, &size);
		__gsmSendTcpDataLowLevel(data, size);
		ProtocolDestroyMessage(data);
		return true;
	}
	AtCommandDropReplyLine(reply);
	return false;
}

bool sound1_Prompt(void) {
	int i;
	char prompt[30] = {0xFD, 0x00, 0x1B, 0x01, 0x01, '[', 'm', '5', '4', ']', 's', 'o', 'u', 'n', 'd', '3', '1', '2', //sound312
		                 0xCA,0xD6,  0xBB,0xFA,  0xBF,0xA8,  0xCE,0xB4,  0xB2,0xE5,  0xC8,0xEB}; //手机卡未插入
	if(CardisEXIST == 1) return false;
  SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
	for (i = 0; i < 30; i++) {
		USART_SendData(USART3, prompt[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 5);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
	return true;
}

bool sound2_Prompt(void) {
	int i;
	char prompt[36] = {0xFD, 0x00, 0x21, 0x01, 0x01, '[', 'm', '5', '4', ']', 's', 'o', 'u', 'n', 'd', '3', '0', '8',//sound308 
		0xCE,0xB4,  0xC4,0xDC,  0xBD,0xD3,  0xC8,0xEB,  0xB7,0xFE,  0xCE,0xF1,  0xC6,0xF7,  0xC6,0xBD,  0xCC,0xA8}; //未能接入服务器平台
	if(sign != 10) return false;
  SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
	for (i = 0; i < 36; i++) {
		USART_SendData(USART3, prompt[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 5);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
	return true;
}

bool sound3_Prompt(void) {
	int i;
	char prompt[36] = {0xFD, 0x00, 0x21, 0x01, 0x01, '[', 'm', '5', '3', ']', 's', 'o', 'u', 'n', 'd', '2', '1', '8',//sound218
		       0xCA,0xD6,  0xBB,0xFA,  0xC4,0xA3,  0xBF,0xE9,  0xB3,0xF5,  0xCA,0xBC,  0xBB,0xAF,  0xB3,0xC9,  0xB9,0xA6}; //手机模块初始化成功
  SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
	for (i = 0; i < 36; i++) {
		USART_SendData(USART3, prompt[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 5);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
	return true;
}

bool sound4_Prompt(void) {
	int i;
	char prompt[36] = {0xFD, 0x00, 0x21, 0x01, 0x01, '[', 'm', '5', '4', ']', 's', 'o', 'u', 'n', 'd', '3', '0', '4',//sound304
		0xCE,0xB4,  0xC4,0xDC,  0xD3,0xEB,  0xCA,0xD6,  0xBB,0xFA,  0xC4,0xA3,  0xBF,0xE9,  0xCD,0xA8, 0xD1,0xB6}; //不能与手机模块通讯
  SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
	for (i = 0; i < 36; i++) {
		USART_SendData(USART3, prompt[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 5);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
	return true;
}

bool __initGsmRuntime() {
	int i;
	static const int bauds[] = {19200, 9600, 115200, 38400, 57600, 4800};
  sound1_Prompt();
	for (i = 0; i < ARRAY_MEMBER_NUMBER(bauds); ++i) {
		// 设置波特率
		printf("Init gsm baud: %d\n", bauds[i]);
		__gsmInitUsart(bauds[i]);
		ATCommandAndCheckReply("AT\r", "OK", configTICK_RATE_HZ );
		ATCommandAndCheckReply("AT\r", "OK", configTICK_RATE_HZ );

		if (ATCommandAndCheckReply("ATE0\r", "OK", configTICK_RATE_HZ * 2)) {
			break;
		}
	}
	if (i >= ARRAY_MEMBER_NUMBER(bauds)) {
		printf("All baud error\n");
		sound4_Prompt();
		return false;
	}

	if (!ATCommandAndCheckReply("AT+IPR=19200\r", "OK", configTICK_RATE_HZ * 2)) {
		printf("AT+IPR=19200 error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CFUN=1\r", "OK", configTICK_RATE_HZ * 2)) {
		printf("AT+CFUN error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+QNITZ=1\r", "OK", configTICK_RATE_HZ)) {
		printf("AT+IFC error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+IFC=2,2\r", "OK", configTICK_RATE_HZ)) {
		printf("AT+QNITZ error\r");
		return false;
	}
	
	if (!ATCommandAndCheckReply("AT+CMEE=2\r", "OK", configTICK_RATE_HZ * 10)) {
		printf("AT+CMEE error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT&W\r", "OK", configTICK_RATE_HZ * 2)) {
		printf("AT&W error\r");
		return false;
	}

	if (!ATCommandAndCheckReply(NULL, "Call Ready", configTICK_RATE_HZ * 30)) {
		printf("Wait Call Realy timeout\n");
	}
	
	if (!ATCommandAndCheckReply("AT+QIMUX=1\r", "OK", configTICK_RATE_HZ * 10)) {
		printf("AT+QIMUX error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+QCLIP=1\r", "OK", configTICK_RATE_HZ * 2)) {
		printf("AT+QCLIP error\r");
		return false;
	}
	
	if (!ATCommandAndCheckReply("AT+CLIP=1\r", "OK", configTICK_RATE_HZ * 2)) {
		printf("AT+CLIP error\r");
		return false;
	}

// 	if (!ATCommandAndCheckReply("AT+CMGF=1\r", "OK", configTICK_RATE_HZ * 2)) {
// 		printf("AT+CMGF=0 error\r");
// 		return false;
// 	}
// 	
// 	if (!ATCommandAndCheckReply("AT+CSCS=\"GSM\"\r", "OK", configTICK_RATE_HZ * 2)) {
// 		printf("AT+CNMI error\r");
// 		return false;
// 	}
	
	if (!ATCommandAndCheckReply("AT+CMGF=0\r", "OK", configTICK_RATE_HZ * 2)) {
		printf("AT+CMGF=0 error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CNMI=2,2,0,0,0\r", "OK", configTICK_RATE_HZ * 2)) {
		printf("AT+CNMI error\r");
		return false;
	}

	if (!ATCommandAndCheckReplyUntilOK("AT+CPMS=\"SM\"\r", "+CPMS", configTICK_RATE_HZ * 10, 3)) {
		printf("AT+CPMS error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CMGD=1,4\r", "OK", configTICK_RATE_HZ * 5)) {
		printf("AT+CMGD error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CSQ\r", "+CSQ:", configTICK_RATE_HZ / 5)) {
		printf("AT+CSQ error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+QIDEACT\r", "DEACT", configTICK_RATE_HZ *10)) {
		printf("AT+QIDEACT error\r");
		return false;
	}		   //关闭GPRS场景

	if (!ATCommandAndCheckReply("AT+QIHEAD=0\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+QIHEAD error\r");
		return false;
	}		   //配置接受数据时是否显示IP头

	if (!ATCommandAndCheckReply("AT+QISHOWRA=1\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+QISHOWRA error\r");
		return false;
	}		  //配置接受数据时是否显示发送方的IP地址和端口号

	if (!ATCommandAndCheckReply("AT+QISHOWPT=1\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+QISHOWPT error\r");
		return false;
	}		   //配置接受数据IP头是否显示传输协议

	if (!ATCommandAndCheckReply("AT+QIFGCNT=0\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+QIFGCNT error\r");
		return false;
	}			//配置前置场为GPRS

	if (!ATCommandAndCheckReply("AT+QICSGP=1,\"CMNET\"\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+QICSGP error\r");
  		return false;
	}			//打开GPRS连接
	
	if (!ATCommandAndCheckReply("AT+QGSMLOC=1\r", "OK", configTICK_RATE_HZ * 20)) {
		printf("AT+QGSMLOC error\r");
//		return false;
	}
	
	sound3_Prompt();
	return true;
}

static 	char FlagII = 0;
static 	char FlagIII = 0;

void __handleSMS(GsmTaskMessage *p) {
	SMSInfo *sms;
	uint32_t second;
	DateTime dateTime;
	const char *dat = __gsmGetMessageData(p);
	if(__gsmRuntimeParameter.isonQUIET){
	   second = RtcGetTime();
	   SecondToDateTime(&dateTime, second);
	   if((dateTime.hour < ((__gsmRuntimeParameter.time[0] - '0') * 10 + (__gsmRuntimeParameter.time[1] - '0'))) ||
	      (dateTime.hour > ((__gsmRuntimeParameter.time[2] - '0') * 10 + (__gsmRuntimeParameter.time[3] - '0')))){
			if(FlagII == 0){	
	   	  XfsTaskSetSpeakVolume('0');
				FlagII = 1;
				FlagIII = 0;
      }	
	   } else {
			 if(FlagIII == 0){
	   	   XfsTaskSetSpeakVolume('9');
				 FlagII = 0;
				 FlagIII = 1;
			 }
	   }
	}
	sms = __gsmPortMalloc(sizeof(SMSInfo));
	printf("Gsm: got sms => %s\n", dat);
	SMSDecodePdu(dat, sms);
	if(sms->contentLen == 0) {
		__gsmPortFree(sms);
		return;
	}
#if defined(__LED__)
	__gsmSMSEncodeConvertToGBK(sms);
#endif
	printf("Gsm: sms_content=> %s\n", sms->content);
	ProtocolHandlerSMS(sms);
	__gsmPortFree(sms);
}

bool __gsmIsValidImei(const char *p) {
	int i;
	if (strlen(p) != GSM_IMEI_LENGTH) {
		return false;
	}

	for (i = 0; i < GSM_IMEI_LENGTH; i++) {
		if (!isdigit(p[i])) {
			return false;
		}
	}

	return true;
}

int __gsmGetImeiFromModem() {
	char *reply;	
	reply = ATCommand("AT+GSN\r", ATCMD_ANY_REPLY_PREFIX, configTICK_RATE_HZ / 2);
	if (reply == NULL) {
		return 0;
	}
	if (!__gsmIsValidImei(reply)) {
		return 0;
	}
	strcpy(__imei, reply);
	AtCommandDropReplyLine(reply);
	return 1;
}

void __handleProtocol(GsmTaskMessage *msg) {
	ProtocolHandler(__gsmGetMessageData(msg));
}

void __handleSendTcpDataLowLevel(GsmTaskMessage *msg) {
	__gsmSendTcpDataLowLevel(__gsmGetMessageData(msg), msg->length);
}

void __handleM35RTC(GsmTaskMessage *msg) {
	DateTime dateTime;
	char *p = __gsmGetMessageData(msg);	 
	p++;
	dateTime.year = (p[0] - '0') * 10 + (p[1] - '0');
	dateTime.month = (p[3] - '0') * 10 + (p[4] - '0');
	dateTime.date = (p[6] - '0') * 10 + (p[7] - '0');
	dateTime.hour = (p[9] - '0' + 8) * 10 + (p[10] - '0');
	dateTime.minute = (p[12] - '0') * 10 + (p[13] - '0');
	dateTime.second = (p[15] - '0') * 10 + (p[16] - '0');
	RtcSetTime(DateTimeToSecond(&dateTime));
}

static char longitude[12] = {0}, latitude[12] = {0};

void __handleTUDE(GsmTaskMessage *msg) {
	int i, j = 0, count = 0;
	char date[12], time[10];
	DateTime __dateTime;
  char *p = __gsmGetMessageData(msg);
	*p++;
	*p++;
	for(i = 0; i < 4; i++){
		  count++;
			while(*p != ','){
				if (count == 1){
					longitude[j++] = *p++;
				}	else if (count == 2){ 
				  latitude[j++] = *p++;
        }	else if (count == 3){ 
				  date[j++] = *p++;
        }	else if (count == 4){					
				    time[j++] = *p++;
					if(*p == 0){
					  break;
          }
        }	
			}
			*p++;
			j = 0;
	}
  __dateTime.year = (date[2] - '0') * 10 + (date[3] - '0');
	__dateTime.month = (date[5] - '0') * 10 + (date[6] - '0');
	__dateTime.date = (date[8] - '0') * 10 + (date[9] - '0');
	__dateTime.hour = (time[0] - '0') * 10 + (time[1] - '0') + 8;
	__dateTime.minute = (time[3] - '0') * 10 + (time[4] - '0');
	__dateTime.second = (time[6] - '0') * 10 + (time[7] - '0');
	RtcSetTime(DateTimeToSecond(&__dateTime));	
}

const char *__gsmGetTUDE(char *p) {
	p = pvPortMalloc(30);
	memset(p, 0, 30);
	strcpy(p, longitude);
  strcat(p, ",");
	strcat(p, latitude);
	return p;
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
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_GSM, 0);
	GPIO_SetBits(GPIOG, RING_PIN);
}

void __handleRING(GsmTaskMessage *msg) {
	char *p = __gsmGetMessageData(msg);
	if(Autho(p)){
		ATCommandAndCheckReply("ATA\r", "OK", configTICK_RATE_HZ);
		SoundControlSetChannel(SOUND_CONTROL_CHANNEL_GSM, 1);
		GPIO_ResetBits(GPIOG, RING_PIN);
	} else{
		ATCommandAndCheckReply("ATH\r", "OK", configTICK_RATE_HZ * 30);
	}
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
	char *dat = __gsmGetMessageData(msg);
	__gsmRuntimeParameter.isonTCP = (*dat != 0x30);
    __storeGsmRuntimeParameter();
	if(!__gsmRuntimeParameter.isonTCP){
	   	if (!ATCommandAndCheckReply("AT+CGATT=0\r", "OK", configTICK_RATE_HZ )) {
		     printf("AT+CGATT error\r");
	    }
	}
}

//<SETIP>"221.130.129.72"5555
void __handleSetIP(GsmTaskMessage *msg) {
     int j = 0;
     char *dat = __gsmGetMessageData(msg);
	 memset(__gsmRuntimeParameter.serverIP, 0, 16);
	 if(*dat++ == 0x22){
		while(*dat != 0x22){
			 __gsmRuntimeParameter.serverIP[j++] = *dat++;
		}
		*dat++;
	 }
	 __gsmRuntimeParameter.serverPORT = atoi(dat);
	 __storeGsmRuntimeParameter();
}

void __handleNightQuiet(GsmTaskMessage *msg) {
    char *dat = __gsmGetMessageData(msg);
	__gsmRuntimeParameter.isonQUIET = (*dat != 0x30);
	__storeGsmRuntimeParameter();
}

void __handleQuietTime(GsmTaskMessage *msg) {
     int i;
	 char *dat = __gsmGetMessageData(msg);
	 for(i = 0; i < 4; i++){
	 	 __gsmRuntimeParameter.time[i] = *dat++;
	 }
	 //__gsmRuntimeParameter.time = *dat;
	 __storeGsmRuntimeParameter();
}

void __handleHttpDownload(GsmTaskMessage *msg) {
    char buf[44];
    char *dat = __gsmGetMessageData(msg);
	char *pref = pvPortMalloc(100);
	strcpy(pref, "http://");
	strcat(pref, dat);
	sprintf(buf, "AT+QHTTPURL=%d,35\r", strlen(pref));

	if (!ATCommandAndCheckReply("AT+QIFGCNT=1\r", "OK", configTICK_RATE_HZ / 2)) {
		printf("AT+QIFGCNT error\r");
		return;
	}	

	if (!ATCommandAndCheckReply(buf, "CONNECT", configTICK_RATE_HZ * 6)) {
		printf("AT+QHTTPURL error\r");
		vPortFree(pref);
  		return;
	}

	if (!ATCommandAndCheckReply(pref, "OK", configTICK_RATE_HZ)) {
		printf("URL error\r");
		vPortFree(pref);
  		return;
	}

	if(!ATCommandAndCheckReply("AT+QHTTPGET=60\r", "OK", configTICK_RATE_HZ * 5 )) {
		printf("AT+QHTTPGET error\r");
		vPortFree(pref);
  		return;
	}					// 发送HTTP获得资源请求

	if (!ATCommandAndCheckReply("AT+QHTTPREAD=30\r", "OK", configTICK_RATE_HZ * 10)) {
		printf("AT+QHTTREAD error\r");
		vPortFree(pref);
  		return;
	}

	vPortFree(pref);
	
	if (!ATCommandAndCheckReply("AT+QIFGCNT=0\r", "OK", configTICK_RATE_HZ / 2)) {
		printf("AT+QIFGCNT error\r");
		return;
	}			//配置前置场为GPRS	
}

typedef struct {
	GsmTaskMessageType type;
	void (*handlerFunc)(GsmTaskMessage *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ TYPE_SMS_DATA, __handleSMS },
	{ TYPE_RING, __handleRING },
	{ TYPE_GPRS_DATA, __handleProtocol },
	{ TYPE_SEND_TCP_DATA, __handleSendTcpDataLowLevel },
	{ TYPE_RTC_DATA, __handleM35RTC},
	{ TYPE_RESET, __handleReset },
	{ TYPE_NO_CARRIER, __handleResetNoCarrier },
	{ TYPE_SEND_AT, __handleSendAtCommand },
	{ TYPE_SEND_SMS, __handleSendSMS },
  { TYPE_TUDE_DATA, __handleTUDE},
	{ TYPE_SET_GPRS_CONNECTION, __handleGprsConnection },
	{ TYPE_SETIP, __handleSetIP },
	{ TYPE_HTTP_DOWNLOAD, __handleHttpDownload },
	{ TYPE_SET_NIGHT_QUIET, __handleNightQuiet },
	{ TYPE_QUIET_TIME, __handleQuietTime},
	{ TYPE_NONE, NULL },
};

static void __gsmTask(void *parameter) {
	portBASE_TYPE rc;
	GsmTaskMessage *message;
	portTickType lastT = 0, lastTime = 0;
	__storeGsmRuntimeParameter();
	while (1) {
		printf("Gsm start\n");
		__gsmModemStart();
		if (__initGsmRuntime()) {
			break;
		}
			   
	}
	while (!__gsmGetImeiFromModem()) {
		vTaskDelay(configTICK_RATE_HZ);
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
			continue;
			if(__gsmRuntimeParameter.isonTCP == 0){
				 sound2_Prompt();
			   continue;
			}
//			SMS_Prompt();
			curT = xTaskGetTickCount();
			Judge = 1;
			if (0 == __gsmCheckTcpAndConnect(__gsmRuntimeParameter.serverIP, __gsmRuntimeParameter.serverPORT)) {
				sign++;
				if(sign > 10){
					sign = 0;
				}
				sound2_Prompt();
				printf("Gsm: Connect server error\n");
			} else if ((curT - lastT) >= GSM_GPRS_HEART_BEAT_TIME) {
				int size;
				const char *dat = ProtoclCreateHeartBeat(&size);
				Which = 1;
				__gsmSendTcpDataLowLevel(dat, size);
				ProtocolDestroyMessage(dat);
				lastT = curT;
			}
			
			Judge = 2;
			if (0 == __gsmCheckTcpAndConnect(__gsmRuntimeParameter.detectionIP, __gsmRuntimeParameter.detectionPORT)) {
				printf("Gsm: Connect detection error\n");
			} else if ((curT - lastTime) >= GSM_GPRS_HEART_BEAT_TIME) {
				int sizeI;
				const char *date = ProtoclCreateHeartBeat(&sizeI);
				Which = 2;
				__gsmSendTcpDataLowLevel(date, sizeI);
				ProtocolDestroyMessage(date);
				lastTime = curT;
			}
		}
	}
}


void GSMInit(void) {
	ATCommandRuntimeInit();
	__gsmInitHardware();
	__queue = xQueueCreate(5, sizeof( GsmTaskMessage *));
	xTaskCreate(__gsmTask, (signed portCHAR *) "GSM", GSM_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}
