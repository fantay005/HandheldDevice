#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gsm.h"
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
#include "soundcontrol.h"
#include "libupdater.h"
#include "norflash.h"
#include "fm.h"
#include "sms_cmd.h"

#define WOMANSOUND  0x33
#define MANSOUND	0X32
#define MIXSOUND	0X34

extern int GsmTaskSendTcpData(const char *p, int len);
extern int GsmTaskResetSystemAfter(int seconds);


typedef enum {
	TermActive  = 0x31,
	ParaSetup = 0x32,
	QueryPara = 0x33,
	Mp3File = 0x34,
	FmOperate = 0x35,
	TypeChooseReply = 0x39,
} TypeChoose;

typedef enum {
	Login = 0x31,
	Heart =	0x32,

	SetupUser =	0x30,
	RemoveUser = 0x31,
	StopTime = 0x32,
	VoiceType =	0x33,
	VolumeSetup = 0x34,
	BroadTimes = 0x35,
	SendSMS	= 0x36,
	Restart = 0x37,
	ReFactory =	0x38,
	ClassificReply = 0x39,

	basic =	0x31,
	Coordinate = 0x32,
	FMfreq = 0x33,
	CSQsignal = 0x34,

	Record	= 0x31,
	SMSPrompt = 0x32,
	RecordPrompt = 0x33,
	Mp3Music = 0x34,
	LongSMS = 0x35,
	
	Fm_Open = 0x31,
	Fm_Close = 0x32,
	Fm_Auto = 0x33,
	Fm_Last = 0x34,
	Fm_Next = 0x35,
	Fm_Play = 0x36,
	Fm_SNR = 0x37,
	Fm_RSSI = 0x38,
} Classific;

typedef struct {
	unsigned char header[2];
	unsigned char lenH;
	unsigned char lenL;
	TypeChoose type;
	Classific class;
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

char *ProtocolMessage(TypeChoose type, Classific class, const char *message, int *size) {
	int i;
	unsigned char sum = 0;
	unsigned char *p, *ret;
	int len = message == NULL ? 0 : strlen(message);

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

char *Protocol__Message(TypeChoose type, Classific class, const char *message, int *size) {
	int i;
	unsigned char sum = 0;
	char ram[3];
	unsigned char *p, *ret;
	int len = message == NULL ? 0 : (strlen(message) - 6);

	*size = sizeof(ProtocolHeader) + len + sizeof(ProtocolPadder);
	ret = pvPortMalloc(*size);
	{
		ProtocolHeader *h = (ProtocolHeader *)ret;
		h->header[0] = '#';
		h->header[1] = 'H';
		h->lenH = len >> 8;
		h->lenL = len;
		h->type = message[0];
		h->class = message[1];
		h->radom = 0x3030;
		h->reserve = 0x3030;
	}
	ram[0] = message[6];
	ram[1] = message[7];
	ram[2] = 0;
  strcpy((char*)(ret + sizeof(ProtocolHeader)), &ram[0]);
	if (message != NULL) {
		strcpy((char *)(ret + sizeof(ProtocolHeader) + 2), &message[8]);
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

char *ProtoclCreatLogin(char *imei, int *size) {
	return ProtocolMessage(TermActive, Login, imei, size);
}

char *ProtoclCreateHeartBeat(int *size) {
	return ProtocolMessage(TermActive, Heart, NULL, size);
}

char *TerminalCreateFeedback(const char radom[4], int *size) {
	char r[5];
	r[0] = radom[0];
	r[1] = radom[1];
	r[2] = radom[2];
	r[3] = radom[3];
	r[4] = 0;
	return ProtocolMessage(TypeChooseReply, ClassificReply, r, size);
}


USERParam  __USERNumber;
XFSspeakParam __xfsparam;
static void __restorUSERParam(void) {
	NorFlashRead(USER_PARAM_STORE_ADDR, (short *)&__USERNumber, sizeof(__USERNumber));
}

static void __restorXFSParam(void) {
	NorFlashRead(XFS_PARAM_STORE_ADDR, (short *)&__xfsparam, sizeof(__xfsparam));
}

static char flag = 0;
char *TerminalFeedback(const char radom[4], int *size) {
	char r[100];
	char *p, *dat;
	int i = 0, j = 0;
	p = pvPortMalloc(100);
	r[0] = radom[0];
	r[1] = radom[1];
	r[2] = '0';
	r[3] = '0';
	r[4] = '0';
	r[5] = '0';
	r[6] = radom[2];
	r[7] = radom[3];
	if(flag == 1){
		p = (char *)__gsmGetTUDE(dat);
		flag = 0;
	} else if (flag == 2){
		__restorUSERParam();
		__restorXFSParam();
		for(j = 0; j < 6; j++){
		   if(__USERNumber.user[j][1] == (char)0xFF){
    		  __USERNumber.user[j][0] = 0;
       } 
    }
		sprintf(p, "1,%s2,%s3,%s4,%s5,%s6,%s%02d%c%c%02d", __USERNumber.user[0], __USERNumber.user[1], __USERNumber.user[2], __USERNumber.user[3], __USERNumber.user[4], __USERNumber.user[5], __xfsparam.speakPause , __xfsparam.speakType, __xfsparam.speakVolume, __xfsparam.speakTimes);
	  flag = 0;
  }
  while(*p != 0) {
    r[8 + i++] = *p++;
  }
	r[8 + i] = 0;
  vPortFree((void *)p);	
	return Protocol__Message(TypeChooseReply, ClassificReply, r, size);
}

typedef void (*ProtocolHandleFunction)(ProtocolHeader *header, char *p);
typedef struct {
	unsigned char type;
	unsigned char class;
	ProtocolHandleFunction func;
} ProtocolHandleMap;


static void HandleLogin(ProtocolHeader *header, char *p) {
	ProtocolDestroyMessage(p);
}

static void HandleHeartBeat(ProtocolHeader *header, char *p) {
	ProtocolDestroyMessage(p);
}

static void HandleSettingUser(ProtocolHeader *header, char *p) {
	int len;
	int i = p[0] - '0';
	p[12] = 0;
	SMSCmdSetUser(i, (char *)&p[1]);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void HandleRemoveUser(ProtocolHeader *header, char *p) {
	int len;
	int index = p[0] - '0';
	SMSCmdRemoveUser(index);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void HandleDeadTime(ProtocolHeader *header, char *p) {
	int len;
	int choose;
	choose = (p[1] - '0') * 10 + (p[0] - '0');
	XfsTaskSetSpeakPause(choose);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void HandleVoiceType(ProtocolHeader *header, const char *p) {
	int len,  choose;
	choose = *p;
	if (choose == 0x34) {
		choose = MANSOUND;
	} else if (choose == 0x35) {
		choose = MIXSOUND;
	} else if (choose == 0x33) {
		choose = WOMANSOUND;
	}
	XfsTaskSetSpeakType(choose);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void HandleVolumeSetting(ProtocolHeader *header, const char *p) {
	int len,  choose;
	choose = *p;
	XfsTaskSetSpeakVolume(choose);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void HandleBroadcastTimes(ProtocolHeader *header, const char *p) {
	int len, times;
	times = (p[1] - '0');
	XfsTaskSetSpeakTimes(times);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void __GPRS_ADMIN_Handler(const char *p) {
	char buf[24];
	int len;
	char *pdu;
	if(strlen(p) != 18){
	   return;
	}
	if (NULL == __user(1)) {
		sprintf(buf, "<USER><1>%s", "EMPTY");
	} else {
		sprintf(buf, "<USER><1>%s", __user(1));
	}
	pdu = pvPortMalloc(300);
	len = SMSEncodePdu8bit(pdu, (char *)&p[7], buf);
	GsmTaskSendSMS(pdu, len);
	vPortFree(pdu);
}

static void __GPRS_IMEI_Handler(const char *p) {
	char buf[22];
	int len;
	char *pdu;
	if(strlen(p) != 17){
	   return;
	}

	sprintf(buf, "<IMEI>%s", GsmGetIMEI());
	pdu = pvPortMalloc(300);
	len = SMSEncodePdu8bit(pdu, (char *)&p[6], buf);
	GsmTaskSendSMS(pdu, len);
	vPortFree(pdu);
}

static void __GPRS_RST_Handler(const char *p) {
    NVIC_SystemReset();
}

static void __GPRS_REFAC_Handler(const char *p) {
	NorFlashMutexLock(configTICK_RATE_HZ * 4);
	FSMC_NOR_EraseSector(XFS_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	FSMC_NOR_EraseSector(GSM_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	FSMC_NOR_EraseSector(USER_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	FSMC_NOR_EraseSector(SMS1_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	FSMC_NOR_EraseSector(FLAG_PARAM_STORE_ADDR);
	vTaskDelay(configTICK_RATE_HZ / 5);
	NorFlashMutexUnlock();
	printf("Reboot From Default Configuration\n");
	NVIC_SystemReset();
}

static void __GPRS_SETIP_Handler(const char *p) {
	char *pcontent = (char *)p;
	if(pcontent[7] != 0x22){
	   return;
	}
    GsmTaskSendSMS(pcontent, strlen(pcontent));
}

static void __GPRS_UPDATA_Handler(const char *p) {
	int i;
	int j = 0;
	FirmwareUpdaterMark *mark;
	char *pcontent =(char*)p;
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
static void __GPRS_ALARM_Handler(const char *p) {
	const char *pcontent = p;
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

static void __GPRS_RED_Display(const char *p) {
	const char *pcontent = p;
	int plen = strlen(p);	
	uint8_t *gbk = Unicode2GBK(&pcontent[2], (plen - 2));
	DisplayClear();
	XfsTaskSpeakUCS2(&pcontent[2], (plen - 2));
	DisplayMessageRed(gbk);
	Unicode2GBKDestroy(gbk);
}

static void __GPRS_GREEN_Display(const char *p) {
	const char *pcontent = pt;
	int plen = strlen(p);
	uint8_t *gbk = Unicode2GBK(&pcontent[2], (plen - 2));
	DisplayClear();
	XfsTaskSpeakUCS2(&pcontent[2], (plen - 2));
	DisplayMessageGreen(gbk);
	Unicode2GBKDestroy(gbk);
}

static void __GPRS_YELLOW_Display(const char *p) {
	const char *pcontent = p;
	int plen = strlen(p);
	uint8_t *gbk = Unicode2GBK(&pcontent[2], (plen - 2));
	DisplayClear();
	XfsTaskSpeakUCS2(&pcontent[2], (plen - 2));
	DisplayMessageYELLOW(gbk);
	Unicode2GBKDestroy(gbk);
}

#endif

#if defined (__LED__)
static void __GPRS_A_Handler(const char *p) {
	const char *pcontent = p;
	int plen = strlen(p);
	uint8_t *gbk = Unicode2GBK(&pcontent[6], (plen - 6));
	XfsTaskSpeakUCS2(&pcontent[6], (plen - 6));
	Unicode2GBKDestroy(gbk);
	DisplayClear();
	SMS_Prompt();
	MessDisplay(gbk);
	__storeSMS1(gbk);
}
#endif

#if defined (__SPEAKER__)
static void __GPRS_FMC_Handler(const char *p){
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_FM, 0);
}


static void __GPRS_FMO_Handler(const char *p){
	const char *pcontent = p;
	fmopen(atoi(&pcontent[5]));
}
#endif

static void __GPRS_CTCP_Handler(const char *p){
	if((p[6] != '1') && (p[6] != '0')){
	   return;
	}

	if(strlen(p) > 8){
	   return;
	}
	GsmTaskSetParameter(p, strlen(p));
} 

static void __GPRS_QUIET_Handler(const char *p){
	if((p[7] != '1') && (p[7] != '0')){
	   return;
	}

	if(strlen(p) > 8){
	   return;
	}

	GsmTaskSetParameter(p, strlen(p));
} 

typedef struct {
	char *cmd;
	void (*gprsCommandFunc)(const char *p);
} GPRSModifyMap;

const static GPRSModifyMap __GPRSModifyMap[] = {
	{"<ADMIN>", __GPRS_ADMIN_Handler},
	{"<IMEI>", __GPRS_IMEI_Handler},
	{"<RST>", __GPRS_RST_Handler},
	{"<REFAC>", __GPRS_REFAC_Handler},
	{"<UPDATA>", __GPRS_UPDATA_Handler},
	{"<SETIP>", __GPRS_SETIP_Handler},
#if defined(__LED__)
	{"<A>", __GPRS_A_Handler},
#endif

#if defined(__LED_HUAIBEI__) && (__LED_HUAIBEI__!=0)
	{"<ALARM>",	__GPRS_ALARM_Handler},
#endif

#if defined(__LED_LIXIN__) && (__LED_LIXIN__!=0)
	{"<1>", __GPRS_RED_Display},
	{"<2>", __GPRS_GREEN_Display},
	{"<3>", __GPRS_YELLOW_Display},
#endif

#if defined (__SPEAKER__)
	{"<FMO>",  __GPRS_FMO_Handler}, 
	{"<FMC>",  __GPRS_FMC_Handler}, 
#endif
	{"<CTCP>",  __GPRS_CTCP_Handler},
	{"<QUIET>", __GPRS_QUIET_Handler},
	{NULL, NULL}
};

extern char *smsrep();

void ProtocolHandlerGPRS(char *p, int len) {
	const GPRSModifyMap *map;
	char *gbk, *ucs2;
#if defined(__LED__)
	const char *pcontent = p;
	__restorUSERParam();

#endif
	ucs2 = pvPortMalloc(len + 1);
	memcpy(ucs2, p, len);
	gbk = (char *)Unicode2GBK((const uint8_t *)ucs2, len);
	for (map = __GPRSModifyMap; map->cmd != NULL; ++map) {
		if (strncasecmp(gbk, map->cmd, strlen(map->cmd)) == 0) {
			map->gprsCommandFunc(gbk);
			return;
	    }
	}
	Unicode2GBKDestroy((uint8_t*)gbk);
	vPortFree(ucs2);
#if defined(__SPEAKER__)
	*smsrep() = 1;
	XfsTaskSpeakUCS2(p, len);
#endif

#if defined(__LED_HUAIBEI__)

	DisplayClear();
	__storeSMS1(p);
	SMS_Prompt();
	uint8_t *gbk = Unicode2GBK((const uint8_t *)(p), strlen(p));
	MessDisplay(gbk);
	LedDisplayToScan(0, 0, LED_DOT_XEND, LED_DOT_YEND);
	Unicode2GBKDestroy(gbk);
#endif
}

static void HandleSendSMS(ProtocolHeader *header, char *p) {
	int len, Plen;
	char *buf;
	buf = TerminalCreateFeedback((char *) & (header->type), &Plen);
	GsmTaskSendTcpData(buf, Plen);
	ProtocolDestroyMessage(buf);
	
	vTaskDelay(configTICK_RATE_HZ * 2);
	len = (header->lenH << 8) + header->lenL;
	ProtocolHandlerGPRS(p, len);
}

static void HandleRestart(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
	GsmTaskResetSystemAfter(10);
}


static void HandleRecoverFactory(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void HandleBasicParameter(ProtocolHeader *header, char *p) {
	int len;
	flag = 2;
	p = TerminalFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void HandleCoordinate(ProtocolHeader *header, char *p) {
	char *dat;
	int len;
	flag = 1;
	len = strlen((const char*)__gsmGetTUDE(dat)) + 2;
	p = TerminalFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
	vPortFree((void *)dat);
}

static void HandleRecordMP3(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void HandleSMSPromptSound(ProtocolHeader *header, char *p) {
	int len;
	char *pref = pvPortMalloc(100);
	strcpy(pref, "<HTTP>");
	len = (header->lenH << 8) + header->lenL;	
	strcat(pref, p);
	GsmTaskSendTcpData(pref, len + 6);
	vPortFree((void *)pref);
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	ProtocolDestroyMessage(p);
}

static void HandleRecordPromptSound(ProtocolHeader *header, char *p) {
	int len;
	char *pref = pvPortMalloc(100);
	strcpy(pref, "<HTTP>");
	len = (header->lenH << 8) + header->lenL;	
	strcat(pref, p);
	GsmTaskSendTcpData(pref, len + 6);
	vPortFree((void *)pref);
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	ProtocolDestroyMessage(p);
}

static void HandleMP3Music(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

static void HandleLongSMS(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

extern char *search_result(char * ret) ;

static void HandleQueryFMfreq(ProtocolHeader *header, char *p) {
	char *res;
	int len;
	p = ProtocolMessage(header->type, header->class, search_result(res), &len);
	vPortFree(res);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

extern const char *GsmGetCSQ(void);

static void HandleCSQsignal(ProtocolHeader *header, char *p) {
	int len;
	p = ProtocolMessage(header->type, header->class, GsmGetCSQ(), &len);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

static void HandleFMopen(ProtocolHeader *header, char *p) {
	int len;
	handlefm(header->class, NULL);
	p = ProtocolMessage(header->type, header->class, NULL, &len);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

static void HandleFMclose(ProtocolHeader *header, char *p) {
	int len;
	handlefm(header->class, NULL);
	p = ProtocolMessage(header->type, header->class, NULL, &len);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

static void HandleFMauto(ProtocolHeader *header, char *p) {
	char *res;
	int len;
	handlefm(header->class, NULL);
	p = ProtocolMessage(header->type, header->class, search_result(res), &len);
	vPortFree(res);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

static void HandleFMlast(ProtocolHeader *header, char *p) {
	int len;
	handlefm(header->class, NULL);
	p = ProtocolMessage(header->type, header->class, NULL, &len);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

static void HandleFMnext(ProtocolHeader *header, char *p) {
	int len;
	handlefm(header->class, NULL);
	p = ProtocolMessage(header->type, header->class, NULL, &len);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

static void HandleFMplay(ProtocolHeader *header, char *p) {
	int len;
	handlefm(header->class, p);
	p = ProtocolMessage(header->type, header->class, NULL, &len);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

static void HandleSNRsetting(ProtocolHeader *header, char *p) {
	int len;
	handlefm(header->class, p);
	p = ProtocolMessage(header->type, header->class, NULL, &len);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

static void HandleRSSIsetting(ProtocolHeader *header, char *p) {
	int len;
	handlefm(header->class, p);
	p = ProtocolMessage(header->type, header->class, NULL, &len);
	GsmTaskSendTcpData(p, len);	
	ProtocolDestroyMessage(p);
}

void ProtocolHandler(char *p) {
	int i;
	const static ProtocolHandleMap map[] = {
		{'1', '1', HandleLogin},
		{'1', '2', HandleHeartBeat},
		{'2', '0', HandleSettingUser},
		{'2', '1', HandleRemoveUser},
		{'2', '2', HandleDeadTime},
		{'2', '3', HandleVoiceType},
		{'2', '4', HandleVolumeSetting},
		{'2', '5', HandleBroadcastTimes},
		{'2', '6', HandleSendSMS},
		{'2', '7', HandleRestart},
		{'2', '8', HandleRecoverFactory},
		{'3', '1', HandleBasicParameter},
		{'3', '2', HandleCoordinate},
		{'3', '3', HandleQueryFMfreq},
		{'3', '4', HandleCSQsignal},
		{'4', '1', HandleRecordMP3},
		{'4', '2', HandleSMSPromptSound},
		{'4', '3', HandleRecordPromptSound},
		{'4', '4', HandleMP3Music},
		{'4', '5', HandleLongSMS},
		{'5', '1', HandleFMopen},
		{'5', '2', HandleFMclose},
		{'5', '3', HandleFMauto},
		{'5', '4', HandleFMlast},
		{'5', '5', HandleFMnext},
		{'5', '6', HandleFMplay},
		{'5', '7', HandleSNRsetting},
		{'5', '8', HandleRSSIsetting},
	};
	ProtocolHeader *header = (ProtocolHeader *)p;

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if ((map[i].type == header->type) && (map[i].class == header->class)) {
			map[i].func(header, p + sizeof(ProtocolHeader));
			break;
		}
	}
}


