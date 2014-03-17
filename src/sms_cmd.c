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
		0X5D, 0XF2,  //已
		0x5C, 0x06,  //将
		0x00, 0x00,	 //0
		0x53, 0xF7,	 //号
		0x75, 0x28,	 //用
		0x62, 0x37,	 //户
		0x63, 0x88,	 //授
		0x67, 0x43,	 //权
		0x4E, 0x0E,	 //与
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

	if (strncasecmp(p->content, "<alock>", 7) == 0) {
		isAlock = true;
		pcontent = &p->content[7];
	} else {
		isAlock = false;
		pcontent = &p->content[6];
	}

	index = pcontent[0] - '0';
	if (index > 6) {
		return;
	}
	if (index < 1) {
		return;
	}

	if (index < 2 && !isAlock) {
		return;
	}

	if (pcontent[1] != '1') {
		return;
	}

	if (strlen(&pcontent[1]) != 11) {
		return;
	}
	__setUser(index, &pcontent[1]);
	__storeUSERParam();

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
	static const char sms_buff1[] = {0x5D, 0xF2,    //已
									 0x5C, 0x06,	 //将
									 0x00, 0x34,	 //4
									 0x53, 0xF7, 	 //号
									 0x75, 0x28,	 //用
									 0x62, 0x37,	 //户
									 0x31, 0x33, 0x38, 0x30, 0x35, 0x35, 0x31, 0x38, 0x38, 0x38, 0x38,	  //13805518888
									 0x67, 0x43,	 //权
									 0x96, 0x50,  	 //限
									 0x89, 0xE3,	 //解
									 0x96, 0x64
									};   //除
	static const char sms_buff2[] = {0x60, 0xA8, 	 //您
									 0x5D, 0xF2, 	 //已
									 0x88, 0xAB, 	 //被
									 0x89, 0xE3, 	 //解
									 0x96, 0x64, 	 //除
									 0x75, 0x28, 	 //用
									 0x62, 0x37, 	 //户
									 0x67, 0x43, 	 //权
									 0x96, 0x50
									};	 //限

	if (p->contentLen > 9) {
		return;
	}
	if ((pcontent[8] >= '7') || (pcontent[8] <= '1')) {
		return;
	}
	index = pcontent[8] - '0';
	SMSCmdRemoveUser(index);

}

static void __cmd_AHQX_Handler(const SMSInfo *p) {
}

static void __cmd_SMSC_Handler(const SMSInfo *p) {
}

static void __cmd_CLR_Handler(const SMSInfo *p) {
}

static void __cmd_DM_Handler(const SMSInfo *p) {
}

static void __cmd_DSP_Handler(const SMSInfo *p) {
}

static void __cmd_STAY_Handler(const SMSInfo *p) {
}

static void __cmd_YSP_Handler(const SMSInfo *p) {
}

static void __cmd_YM_Handler(const SMSInfo *p) {
}

static void __cmd_YD_Handler(const SMSInfo *p) {
}

static void __cmd_VOLUME_Handler(const SMSInfo *p) {
}

static void __cmd_INT_Handler(const SMSInfo *p) {
}

static void __cmd_YC_Handler(const SMSInfo *p) {
}

static void __cmd_R_Handler(const SMSInfo *p) {
}

static void __cmd_VALID_Handler(const SMSInfo *p) {
}

static void __cmd_USER_Handler(const SMSInfo *p) {
}

static void __cmd_ST_Handler(const SMSInfo *p) {
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
	int len, i;
	char *pdu;
	char imei[15];
	NorFlashRead(GSM_PARAM_STORE_ADDR, (short *)&__cmdGMSParameter, sizeof(__cmdGMSParameter));
	for (i = 0; i < 15; i++) {
		imei[i] = __cmdGMSParameter.IMEI[i];
	}
	sprintf(buf, "<IMEI>%s", imei);
	pdu = pvPortMalloc(300);
	len = SMSEncodePdu8bit(pdu, p->number, buf);
	GsmTaskSendSMS(pdu, len);
	vPortFree(pdu);
}

static void __cmd_REFAC_Handler(const SMSInfo *p) {
}

static void __cmd_RST_Handler(const SMSInfo *p) {
	NorFlashMutexLock(configTICK_RATE_HZ * 10);
	FSMC_NOR_EraseSector(XFS_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	FSMC_NOR_EraseSector(GSM_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	FSMC_NOR_EraseSector(USER_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	FSMC_NOR_EraseSector(SMS1_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	FSMC_NOR_EraseSector(SMS2_PARAM_STORE_ADDR);
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
		NVIC_SystemReset();
	}
	vPortFree(mark);
}

#if defined(__LED_HUAIBEI__) && (__LED_HUAIBEI__!=0)
static void __cmd_ALARM_Handler(const SMSInfo *p) {
	const char *pcontent = p->content;
	enum SoftPWNLedColor color;
	switch (pcontent[7]) {
	case '3':
		color = SoftPWNLedColorYellow;
		break;
	case '2':
		color = SoftPWNLedColorOrange;
		break;
	case '4':
		color = SoftPWNLedColorBlue;
		break;
	case '1':
		color = SoftPWNLedColorRed;
		break;
	default :
		color =	SoftPWNLedColorNULL;
		break;
	}
	Display2Clear();
	SoftPWNLedSetColor(color);
	LedDisplayGB2312String162(2 * 4, 0, &pcontent[8]);
	LedDisplayToScan2(2 * 4, 0, 16 * 12 - 1, 15);
	__storeSMS2((char *)&pcontent[8]);
}
#endif

#if defined(__LED_LIXIN__) && (__LED_LIXIN__!=0)

static void __cmd_RED_Display(const SMSInfo *sms) {
	const char *pcontent = sms->content;
	int plen = sms->contentLen;

	DisplayClear();
	if (sms->encodeType == ENCODE_TYPE_UCS2) {
		uint8_t *gbk = Unicode2GBK(&pcontent[6], (plen - 6));
		XfsTaskSpeakUCS2(&pcontent[6], (plen - 6));
		DisplayMessageRed(gbk);
		Unicode2GBKDestroy(gbk);
		__storeSMS1(gbk);
	} else {
		XfsTaskSpeakGBK(&pcontent[3], (plen - 3));
		DisplayMessageRed(&pcontent[3]);
		__storeSMS1(&pcontent[3]);
	}
}

static void __cmd_GREEN_Display(const SMSInfo *sms) {
	const char *pcontent = sms->content;
	int plen = sms->contentLen;
	DisplayClear();
	if (sms->encodeType == ENCODE_TYPE_UCS2) {
		uint8_t *gbk = Unicode2GBK(&pcontent[6], (plen - 6));
		XfsTaskSpeakUCS2(&pcontent[6], (plen - 6));
		DisplayMessageGreen(gbk);
		Unicode2GBKDestroy(gbk);
		__storeSMS1(gbk);
	} else {
		XfsTaskSpeakGBK(&pcontent[3], (plen - 3));
		DisplayMessageGreen(&pcontent[3]);
		__storeSMS1(&pcontent[3]);

	}
}

static void __cmd_YELLOW_Display(const SMSInfo *sms) {
	const char *pcontent = sms->content;
	int plen = sms->contentLen;
	DisplayClear();
	if (sms->encodeType == ENCODE_TYPE_UCS2) {
		uint8_t *gbk = Unicode2GBK(&pcontent[6], (plen - 6));
		XfsTaskSpeakUCS2(&pcontent[6], (plen - 6));
		DisplayMessageYELLOW(gbk);
		Unicode2GBKDestroy(gbk);
		__storeSMS1(gbk);
	} else {
		XfsTaskSpeakGBK(&pcontent[3], (plen - 3));
		DisplayMessageYELLOW(&pcontent[3]);
		__storeSMS1(&pcontent[3]);
	}
}

#endif

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
	LedDisplayToScan(0, 0, LED_PHY_DOT_WIDTH - 1, LED_PHY_DOT_HEIGHT - 1);
}

static void __cmd_VERSION_Handler(const SMSInfo *sms) {
	const char *version = Version();
	// send this string to sms->number;
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
	{"<AHQXZYTXXZX>", __cmd_AHQX_Handler, UP_ALL},
	{"<SMSC>", __cmd_SMSC_Handler, UP_ALL},
	{"<CLR>", __cmd_CLR_Handler, UP_ALL},
	{"<DM>", __cmd_DM_Handler, UP_ALL},
	{"<DSP>", __cmd_DSP_Handler, UP_ALL},
	{"<STAY>", __cmd_STAY_Handler, UP_ALL},
	{"<YSP>", __cmd_YSP_Handler, UP_ALL},
	{"<YM>", __cmd_YM_Handler, UP_ALL},
	{"<YD>", __cmd_YD_Handler, UP_ALL},
	{"<VOLUME>", __cmd_VOLUME_Handler, UP_ALL},
	{"<INT>", __cmd_INT_Handler, UP_ALL},
	{"<YC>", __cmd_YC_Handler, UP_ALL},
	{"<R>", __cmd_R_Handler, UP_ALL},
	{"<VALID>", __cmd_VALID_Handler, UP_ALL},
	{"<USER>", __cmd_USER_Handler, UP_ALL},
	{"<ST>", __cmd_ST_Handler, UP_ALL},
	{"<ERR>", __cmd_ERR_Handler, UP_ALL},
	{"<ADMIN>", __cmd_ADMIN_Handler, UP_ALL},
	{"<IMEI>", __cmd_IMEI_Handler, UP_ALL},
	{"<REFAC>", __cmd_REFAC_Handler, UP_ALL},
	{"<RST>", __cmd_RST_Handler, UP_ALL},
	{"<TEST>", __cmd_TEST_Handler, UP_ALL},
	{"<UPDATA>", __cmd_UPDATA_Handler, UP_ALL},
	{"<SETIP>", __cmd_SETIP_Handler, UP_ALL},
	{"<A>", __cmd_A_Handler, UP1 | UP2 | UP3 | UP4 | UP5 | UP6},
#if defined(__LED_HUAIBEI__) && (__LED_HUAIBEI__!=0)
	{"<ALARM>",	__cmd_ALARM_Handler, UP1 | UP2 | UP3 | UP4 | UP5 | UP6},
#endif

#if defined(__LED_LIXIN__) && (__LED_LIXIN__!=0)
	{"<1>", __cmd_RED_Display, UP_ALL},
	{"<2>", __cmd_GREEN_Display, UP_ALL},
	{"<3>", __cmd_YELLOW_Display, UP_ALL},
#endif

	{"VERSION>", __cmd_VERSION_Handler, UP_ALL},
	{NULL, NULL}
};

#if defined(__LED_HUAIBEI__)
void ProtocolHandlerSMS(const SMSInfo *sms) {
	const SMSModifyMap *map;
	DateTime dateTime;
	int index;
	const char *p = sms->time;
	const char *pnumber = sms->number;
	__restorUSERParam();
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
#if defined(__LED_HUAIBEI__)

	if (index == 0) {
		return;
	}

	DisplayClear();
	switch (index) {
	case 1:
		__storeSMS1(sms->content);
	case 2:
		__storeSMS2(sms->content);
	case 3:
		__storeSMS3(sms->content);
	case 4:
		__storeSMS4(sms->content);
	case 5:
		__storeSMS5(sms->content);
	case 6:
		__storeSMS6(sms->content);
	}
	SMS_Prompt();
	if (sms->encodeType == ENCODE_TYPE_UCS2) {
		uint8_t *gbk = Unicode2GBK((const uint8_t *)(sms->content), sms->contentLen);
		MessDisplay(gbk);
	} else {
		MessDisplay((char *)(sms->content));
	}
	LedDisplayToScan(0, 0, LED_DOT_XEND, LED_DOT_YEND);
#endif
}
#endif

#if defined(__LED_LIXIN__)
void ProtocolHandlerSMS(const SMSInfo *sms) {
	const SMSModifyMap *map;
	int i;
	__restorUSERParam();
	for (map = __SMSModifyMap; map->cmd != NULL; ++map) {
		if (strncasecmp(sms->content, map->cmd, strlen(map->cmd)) == 0) {
			map->smsCommandFunc(sms);
			return;
		}
	}
}
#endif

