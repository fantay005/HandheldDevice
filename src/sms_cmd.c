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
#include "libupdater.h"
#include "unicode2gbk.h"
#include "led_lowlevel.h"
#include "display.h"
#include "softpwm_led.h"
#include "version.h"
#include "second_datetime.h"
#include "gsm.h"


static xQueueHandle __smsQueue;

#define SMS_TASK_STACK_SIZE			(configMINIMAL_STACK_SIZE + 256)

typedef struct {
	char user[6][12];
} USERParam;

USERParam __userParam;

GMSParameter  __cmdGMSParameter;

void __storeSMS1(const char *sms) {
	NorFlashWrite(SMS1_PARAM_STORE_ADDR, (const short *)sms, strlen(sms) + 1);
}

void __storeSMS2(const char *sms) {
	NorFlashWrite(SMS2_PARAM_STORE_ADDR, (const short *)sms, strlen(sms) + 1);
}

void __storeSMS3(const char *sms) {
	NorFlashWrite(SMS3_PARAM_STORE_ADDR, (const short *)sms, strlen(sms) + 1);
}

void __storeSMS4(const char *sms) {
	NorFlashWrite(SMS4_PARAM_STORE_ADDR, (const short *)sms, strlen(sms) + 1);
}

void __storeSMS5(const char *sms) {
	NorFlashWrite(SMS5_PARAM_STORE_ADDR, (const short *)sms, strlen(sms) + 1);
}

void __storeSMS6(const char *sms) {
	NorFlashWrite(SMS6_PARAM_STORE_ADDR, (const short *)sms, strlen(sms) + 1);
}

void __storeSMS7(const char *sms) {
	NorFlashWrite(SMS7_PARAM_STORE_ADDR, (const short *)sms, strlen(sms) + 1);
}

static inline bool __isValidUser(const char *p) {
	int i;
	for (i = 0; i < 12; ++i) {
		if (p[i] == 0) {
			return true;
		}
		if (!isxdigit(p[i])) {
			return false;
		}
	}

	return false;
}

// return 1-6
// 0 no found
static int __userIndex(const char *user) {
	int i;

	if (!__isValidUser(__userParam.user[0])) {
		return 1;
	}

	for (i = 0; i < ARRAY_MEMBER_NUMBER(__userParam.user) ; ++i) {
		if (strcmp(user, __userParam.user[i]) == 0) {
			return i + 1;
		}
	}

	return 0;
}


// index  1 - 6
static const char *__user(int index) {
	if (index <= 0) {
		return NULL;
	}
	if (index >= 7) {
		return NULL;
	}

	return __isValidUser(__userParam.user[index - 1]) ? __userParam.user[index - 1] : NULL;
}

// index  1 - 6
static void __setUser(int index, const char *user) {
	strcpy(__userParam.user[index - 1], user);
}

static inline void __storeUSERParam(void) {
	NorFlashWrite(USER_PARAM_STORE_ADDR, (const short *)&__userParam, sizeof(__userParam));
}

static void __restorUSERParam(void) {
	NorFlashRead(USER_PARAM_STORE_ADDR, (short *)&__userParam, sizeof(__userParam));
}

void SMSCmdSetUser(int index, const char *user) {
	if (index <= 0 || index >= 7) {
		return;
	}
	if (strlen(user) >= 12) {
		return;
	}
	__setUser(index, user);
	__storeUSERParam();
}

void SMSCmdRemoveUser(int index) {
	const char user[12] = {0};
	if (index <= 0 || index >= 7) {
		return;
	}
	__setUser(index, user);
	__storeUSERParam();
}

// index  1 - 6
static void __sendToUser(int index, const char *content, int len) {
	char *pdu = pvPortMalloc(300);
	const char *dest = __user(index);
	if (dest == NULL) {
		return;
	}

	len = SMSEncodePduUCS2(pdu, dest, content, len);
	GsmTaskSendSMS(pdu, len);
	vPortFree(pdu);
}

static void __cmd_LOCK_Handler(const SMSInfo *p) {
	int i;
	bool isAlock;
	const char *pcontent;
	char *sms;
	int index;
	const char *num = p->number;
	// 已将2号用户授权与13800000000
	static const char __toUser1[] = {
		0X5D, 0XF2, //已
		0x5C, 0x06, //将
		0x00, 0x00,
		0x53, 0xF7,
		0x75, 0x28,
		0x62, 0x37,
		0x63, 0x88,
		0x67, 0x43,
		0x4E, 0x0E,
	};

	static const char __toUser[] = {
		0x60, 0xA8, //您
		0x5D, 0xF2, //已
		0x88, 0xAB, //被
		0x63, 0x88, //授
		0x4E, 0x88, //予
		0x00, 0x00, //k
		0x53, 0xF7, //号
		0x75, 0x28, //用
		0x62, 0x37, //户
		0x67, 0x43, //权
		0x96, 0x50, //限
	};

	if (strncasecmp((char *)p->content, "<alock>", 7) == 0) {
		isAlock = true;
		pcontent = (const char *)&p->content[7];
	} else if (strncasecmp((char *)p->content, "<lock>", 6) == 0){
		isAlock = false;
		pcontent = (const char *)&p->content[6];
	}

	index = pcontent[0] - '0';
	if (index > 6) {
		return;
	}
	if (index < 1) {
		return;
	}

	if (index < 2 && !isAlock) {              //<alock>后只能跟1，<lock>后接2,3,4,5,6
		return;
	}

	if (strlen(&pcontent[1]) != 11) {
		return;
	}
	__setUser(index, &pcontent[1]);
	__storeUSERParam();
	
	if (pcontent[1] != '1') {
		return;
	}

	sms = pvPortMalloc(60);

	memcpy(sms, __toUser1, sizeof(__toUser1));
	sms[5] = index + '0';
	for (i = 0; i < 11; i++) {
		sms[sizeof(__toUser1) + 2 * i] = 0;
		sms[sizeof(__toUser1) + 2 * i + 1] = pcontent[1 + i];
	}
	__sendToUser(1, sms, 11 * 2 + sizeof(__toUser1));

	memcpy(sms, __toUser, sizeof(__toUser));
	sms[11] = index + '0';
	__sendToUser(index, sms, sizeof(__toUser));
	vPortFree(sms);
}

static void __cmd_UNLOCK_Handler(const SMSInfo *p) {
	const char *pcontent = p->content;
	int index;
	if (p->contentLen > 9) {
		return;
	}
	if ((pcontent[8] >= '7') || (pcontent[8] <= '1')) {
		return;
	}
	index = pcontent[8] - '0';
	SMSCmdRemoveUser(index);

}

static void __cmd_AHQX_Handler(const SMSInfo *sms) {

}

static void __cmd_SMSC_Handler(const SMSInfo *sms) {
	int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
	
}

static void __cmd_CLR_Handler(const SMSInfo *sms) {
	int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if ((index != 1) || (index != 2)){
		return;
	}
}

static void __cmd_DM_Handler(const SMSInfo *sms) {
	int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_DSP_Handler(const SMSInfo *sms) {
	int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if ((index != 1) || (index != 2)){
		return;
	}
}

static void __cmd_STAY_Handler(const SMSInfo *sms) {
	int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_YSP_Handler(const SMSInfo *sms) {
		int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_YM_Handler(const SMSInfo *sms) {
		int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_YD_Handler(const SMSInfo *sms) {
		int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_VOLUME_Handler(const SMSInfo *sms) {
		int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_INT_Handler(const SMSInfo *sms) {
		int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_YC_Handler(const SMSInfo *sms) {
		int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_R_Handler(const SMSInfo *sms) {
	int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_ST_Handler(const SMSInfo *sms) {
}

static void __cmd_S_Handler(const SMSInfo *sms) {
}

static void __cmd_W_Handler(const SMSInfo *sms) {
}

static void __cmd_WR_Handler(const SMSInfo *sms) {
}

static void __cmd_T_Handler(const SMSInfo *sms) {
}

static void __cmd_TEM_Handler(const SMSInfo *sms) {
}

static void __cmd_HUM_Handler(const SMSInfo *sms) {
}

static void __cmd_WARNING_Handler(const SMSInfo *sms) {
}

static void __cmd_VALID_Handler(const SMSInfo *sms) {
	int index;
	const char *pnumber = sms->number;
	
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);	
	if (index != 1) {
		return;
	}
}

static void __cmd_USER_Handler(const SMSInfo *p) {
}

static void __cmd_TM_Handler(const SMSInfo *sms) {
	DateTime dateTime;
	const char *p = sms->time;
	dateTime.year = (p[0] - '0') * 10 + (p[1] - '0');
	dateTime.month = (p[2] - '0') * 10 + (p[3] - '0');
	dateTime.date = (p[4] - '0') * 10 + (p[5] - '0');
	dateTime.hour = (p[6] - '0') * 10 + (p[7] - '0');
	dateTime.minute = (p[8] - '0') * 10 + (p[9] - '0');
	if (p[10] != 0 && p[11] != 0) {
		dateTime.second = (p[10] - '0') * 10 + (p[11] - '0');
	} else {
		dateTime.second = 0;
	}
	RtcSetTime(DateTimeToSecond(&dateTime));
}

static void __cmd_ERR_Handler(const SMSInfo *p) {
}

static void __cmd_ADMIN_Handler(const SMSInfo *p) {
	char buf[24];
	int len;
	char *pdu;
	if (NULL == __user(1)) {
		sprintf(buf, "<USER><1>%s", "EMPTY");
	} else {
		sprintf(buf, "<USER><1>%s", __user(1));
	}
	pdu = pvPortMalloc(300);
	len = SMSEncodePdu8bit(pdu, p->number, buf);
	GsmTaskSendSMS(pdu, len);
	vPortFree(pdu);
}

static void __cmd_IMEI_Handler(const SMSInfo *p) {
	char buf[16];
	int len;
	char *pdu;

	sprintf(buf, "<IMEI>%s", GsmGetIMEI());
	pdu = pvPortMalloc(300);
	len = SMSEncodePdu8bit(pdu, (const char *)p->number, buf);
	GsmTaskSendSMS(pdu, len);
	vPortFree(pdu);
}

static void __cmd_REFAC_Handler(const SMSInfo *p) {
}

static void __cmd_RST_Handler(const SMSInfo *p) {
	NorFlashMutexLock(configTICK_RATE_HZ * 10);
	FSMC_NOR_EraseSector(GSM_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	NorFlashMutexUnlock();
	printf("Reboot From Default Configuration\n");
	WatchdogResetSystem();
}

static void __cmd_TEST_Handler(const SMSInfo *p) {
}

static void __cmd_SETIP_Handler(const SMSInfo *p) {
}

static void __cmd_UPDATA_Handler(const SMSInfo *p) {
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
		FSMC_NOR_EraseSector(XFS_PARAM_STORE_ADDR);
	  vTaskDelay(configTICK_RATE_HZ / 5);
	  FSMC_NOR_EraseSector(GSM_PARAM_STORE_ADDR);
	  vTaskDelay(configTICK_RATE_HZ / 5);
	  FSMC_NOR_EraseSector(USER_PARAM_STORE_ADDR);
	  vTaskDelay(configTICK_RATE_HZ / 5);
	  FSMC_NOR_EraseSector(SMS1_PARAM_STORE_ADDR);
	  vTaskDelay(configTICK_RATE_HZ / 5);
		NVIC_SystemReset();
	}
	vPortFree(mark);
}

static void __cmd_SMS_Handler(const SMSInfo *sms) {
	const char *pcontent = sms->content;
	int plen = sms->contentLen;
	if(pcontent[1] == 0x31){
		__storeSMS2(&pcontent[3]);
	} else if (pcontent[1] == 0x32){
		__storeSMS3(&pcontent[3]);
	} else if (pcontent[1] == 0x33){
		__storeSMS4(&pcontent[3]);
	} else if (pcontent[1] == 0x34){
		__storeSMS5(&pcontent[3]);
	} else if (pcontent[1] == 0x35){
		__storeSMS6(&pcontent[3]);
	} else if (pcontent[1] == 0x36){
		__storeSMS7(&pcontent[3]);
	}
	SMS_Prompt();
	MessDisplay((char *)&pcontent[3]);
}

#if defined (__LED__)
static void __cmd_A_Handler(const SMSInfo *sms) {
	const char *pcontent = sms->content;
	int plen = sms->contentLen;
	const char *pnumber = sms->number;
	int index;
	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);
	if (index == 0) {
		return;
	}
	if (sms->encodeType == ENCODE_TYPE_UCS2) {
		uint8_t *gbk = Unicode2GBK(&pcontent[6], (plen - 6));
		XfsTaskSpeakUCS2(&pcontent[6], (plen - 6));
		Unicode2GBKDestroy(gbk);
	} else {
		XfsTaskSpeakGBK(&pcontent[3], (plen - 3));
	}
	DisplayClear();
	SMS_Prompt();
	if (sms->encodeType == ENCODE_TYPE_UCS2) {
		uint8_t *gbk = Unicode2GBK((const uint8_t *)(&pcontent[6]), (plen - 6));
		MessDisplay(gbk);
		__storeSMS1(gbk);
	} else {
		MessDisplay((char *)&pcontent[3]);
		__storeSMS1(&pcontent[3]);
	}
//	LedDisplayToScan(0, 0, LED_DOT_XEND, LED_DOT_YEND);
}
#endif

static void __cmd_VERSION_Handler(const SMSInfo *sms) {
	const char *version = Version();
	// send this string to sms->number;
}

static void __cmd_CTCP_Handler(const SMSInfo *sms){
	const char *p = (const char *)sms->content;
	if((p[6] != '1') && (p[6] != '0')){
	   return;
	}
	GsmTaskSetGprsConnect(p[6] - '0');
} 


#define UP1 (1 << 1)
#define UP2 (1 << 2)
#define UP3 (1 << 3)
#define UP4 (1 << 4)
#define UP5 (1 << 5)
#define UP6 (1 << 6)
#define UP_ALL (1<<7)


typedef struct {
	char *cmd;
	void (*smsCommandFunc)(const SMSInfo *p);
	uint32_t permission;
} SMSModifyMap;

const static SMSModifyMap __SMSModifyMap[] = {
	{"<LOCK>", __cmd_LOCK_Handler, UP1},
	{"<ALOCK>", __cmd_LOCK_Handler, UP1},
	{"<UNLOCK>", __cmd_UNLOCK_Handler, UP1},
	{"<AHQXZYTXXZX>", __cmd_AHQX_Handler, UP1},
	{"<SMSC>", __cmd_SMSC_Handler, UP1},
	{"<CLR>", __cmd_CLR_Handler, UP1},
	{"<DM>", __cmd_DM_Handler, UP1},
	{"<DSP>", __cmd_DSP_Handler, UP1},
	{"<STAY>", __cmd_STAY_Handler, UP1},
	{"<YSP>", __cmd_YSP_Handler, UP1},
	{"<YM>", __cmd_YM_Handler, UP1},
	{"<YD>", __cmd_YD_Handler, UP1},
	{"<VOLUME>", __cmd_VOLUME_Handler, UP1},
	{"<INT>", __cmd_INT_Handler, UP1},
	{"<YC>", __cmd_YC_Handler, UP1},
	{"<R>", __cmd_R_Handler, UP1},
	{"<VALID>", __cmd_VALID_Handler, UP1},
	{"<USER>", __cmd_USER_Handler, UP1 | UP2},
	{"<TM>", __cmd_TM_Handler, UP_ALL},
  {"<ST>", __cmd_ST_Handler, UP1 | UP2},
	{"<ERR>", __cmd_ERR_Handler, UP1 | UP2},
	{"<ADMIN>", __cmd_ADMIN_Handler, UP_ALL},
	{"<IMEI>", __cmd_IMEI_Handler, UP_ALL},
	{"<REFAC>", __cmd_REFAC_Handler, UP_ALL},
	{"<RST>", __cmd_RST_Handler, UP_ALL},
	{"<TEST>", __cmd_TEST_Handler, UP_ALL},
	{"<S>", __cmd_S_Handler, UP_ALL},
	{"<W>", __cmd_W_Handler, UP_ALL},
	{"<RD>", __cmd_RST_Handler, UP_ALL},
	{"<WR>", __cmd_WR_Handler, UP_ALL},
	{"<T>", __cmd_T_Handler, UP_ALL},
	{"<TEM>", __cmd_TEM_Handler, UP_ALL},
	{"<HUM>", __cmd_HUM_Handler, UP_ALL},
	{"<#>", __cmd_WARNING_Handler, UP_ALL},
	{"<UPDATA>", __cmd_UPDATA_Handler, UP_ALL},
	{"<1>", __cmd_SMS_Handler, UP_ALL},
  {"<2>", __cmd_SMS_Handler, UP_ALL},
	{"<3>", __cmd_SMS_Handler, UP_ALL},
	{"<4>", __cmd_SMS_Handler, UP_ALL},
	{"<5>", __cmd_SMS_Handler, UP_ALL},
	{"<6>", __cmd_SMS_Handler, UP_ALL},
	{"<SETIP>", __cmd_SETIP_Handler, UP_ALL},
	{"<A>", __cmd_A_Handler, UP1 | UP2},


	{"<VERSION>", __cmd_VERSION_Handler, UP_ALL},
	{"<CTCP>",  __cmd_CTCP_Handler, UP_ALL},
	{NULL, NULL}
};


void ProtocolHandlerSMS(const SMSInfo *sms) {
	const SMSModifyMap *map;
	int index;
	const char *pnumber = sms->number;
	__restorUSERParam();

	index = __userIndex(sms->numberType == PDU_NUMBER_TYPE_INTERNATIONAL ? &pnumber[2] : &pnumber[0]);
	for (map = __SMSModifyMap; map->cmd != NULL; ++map) {
		if (strncasecmp(sms->content, map->cmd, strlen(map->cmd)) == 0) {
			if (map->permission != UP_ALL) {
				if ((map->permission & (1 << (index))) == 0) {
					return;
				}
			}

			map->smsCommandFunc(sms);
			return;
		}
	}
	
	if (index == 0) {
		return;
	}

	DisplayClear();
	__storeSMS1(sms->content);
	SMS_Prompt();
	if (sms->encodeType == ENCODE_TYPE_UCS2) {
		uint8_t *gbk = Unicode2GBK((const uint8_t *)(sms->content), sms->contentLen);
		MessDisplay(gbk);
	} else {
		MessDisplay((char *)(sms->content));
	}
	LedDisplayToScan(0, 0, LED_DOT_XEND, LED_DOT_YEND);

}

static char n = 1;

static void __smsTask(void *nouse) {
	portBASE_TYPE rc;
	char *msg;

	__smsQueue = xQueueCreate(1, sizeof(char *));
	while (1) {
		rc = xQueueReceive(__smsQueue, &msg, configTICK_RATE_HZ * 30);
		if (rc == pdTRUE) {
		} else {
			
			if(n > 7){
				n = 1;
			}
			
			if (n == 1) {
				const char *messageA = (const char *)(Bank1_NOR2_ADDR + SMS1_PARAM_STORE_ADDR);				
				if (messageA[0] == 0xff) {
					n = 2;
				} else {
				  char * p = pvPortMalloc(strlen(messageA) + 1);
				  strcpy(p, messageA);					
					MessDisplay(p);
					vPortFree(p);
				}
			} 
			
			if (n == 2) {
				const char *messageB = (const char *)(Bank1_NOR2_ADDR + SMS2_PARAM_STORE_ADDR);
				if (messageB[0] == 0xff) {
					n = 3;
				} else {
					char * v = pvPortMalloc(strlen(messageB) + 1);
				  strcpy(v, messageB);
					MessDisplay(v);
					vPortFree(v);
				}
			} 
			
			if (n == 3) {
				const char *messageC = (const char *)(Bank1_NOR2_ADDR + SMS3_PARAM_STORE_ADDR);
				if (messageC[0] == 0xff) {
					n = 4;
				} else {
					char * t = pvPortMalloc(strlen(messageC) + 1);
				  strcpy(t, messageC);
					MessDisplay(t);
					vPortFree(t);
				}
			} 
			
			if (n == 4) {
				const char *messageD = (const char *)(Bank1_NOR2_ADDR + SMS4_PARAM_STORE_ADDR);
				if (messageD[0] == 0xff) {
					n = 5;
				} else {
					char * q = pvPortMalloc(strlen(messageD) + 1);
			  	strcpy(q, messageD);
					MessDisplay(q);
					vPortFree(q);
				}
			} 

			if (n == 5) {
				const char *messageE = (const char *)(Bank1_NOR2_ADDR + SMS5_PARAM_STORE_ADDR);
				if (messageE[0] == 0xff) {
					n = 6;
				} else {
					char * h = pvPortMalloc(strlen(messageE) + 1);
				  strcpy(h, messageE);
					MessDisplay(h);
					vPortFree(h);
				}
			} 
			
			if (n == 6) {
				const char *messageF = (const char *)(Bank1_NOR2_ADDR + SMS6_PARAM_STORE_ADDR);
				if (messageF[0] == 0xff) {
					n = 7;
				} else {
					char * m = pvPortMalloc(strlen(messageF) + 1);
				  strcpy(m, messageF);
					MessDisplay(m);
					vPortFree(m);
				}
			} 

			if (n == 7) {
				const char *messageG = (const char *)(Bank1_NOR2_ADDR + SMS7_PARAM_STORE_ADDR);
				if (messageG[0] == 0xff) {
					n++;
					continue;
				} else {
					char * k = pvPortMalloc(strlen(messageG) + 1);
				  strcpy(k, messageG);
					MessDisplay(k);
					vPortFree(k);
				}
			}
			
			n++;
		}
	}
}

 void __smsCreateTask(void) {
	xTaskCreate(__smsTask, (signed portCHAR *) "SMS", SMS_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
}
