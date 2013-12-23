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
#include "stm32f10x_usart.h"


#define WOMANSOUND  0x33
#define MANSOUND	0X32
#define MIXSOUND	0X34

extern int GsmTaskSendTcpData(const char *p, int len);
extern int GsmTaskResetSystemAfter(int seconds);

//USERParam __userParam1;
//
//static inline void __storeUSERParam1(void) {
//	NorFlashWrite(USER_PARAM_STORE_ADDR, (const short *)&__userParam1, sizeof(__userParam1));
//}
//
//static void __restorUSERParam1(void) {
//	NorFlashRead(USER_PARAM_STORE_ADDR, (short *)&__userParam1, sizeof(__userParam1));
//}

typedef enum {
	TermActive  = 0x31,
	ParaSetup = 0x32,
	QueryPara = 0x33,
	Mp3File = 0x34,
	TypeChooseReply = 0x39,
} TypeChoose;

typedef enum {
	Login = 0x31,
	Heart =	0x32,
	Polling = 0x34,

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

	Record	= 0x31,
	SMSPrompt = 0x32,
	RecordPrompt = 0x33,
	Mp3Music = 0x34,
	LongSMS = 0x35,

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
//
//void SoftReset(void) {
//	__set_FAULTMASK(1);  //关闭所有终端
//	NVIC_SystemReset();	 //复位
//}

char *ProtoclCreatLogin(char *imei, int *size) {
	return ProtocolMessage(TermActive, Login, imei, size);
}

char *ProtoclCreateHeartBeat(int *size) {
	return ProtocolMessage(TermActive, Heart, NULL, size);
}

char *ProtoclQueryMeteTim(int *size) {
	return ProtocolMessage(QueryPara, Polling, "58210", size);	  //颍上58210    
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

typedef void (*ProtocolHandleFunction)(ProtocolHeader *header, char *p);
typedef struct {
	unsigned char type;
	unsigned char class;
	ProtocolHandleFunction func;
} ProtocolHandleMap;


void HandleLogin(ProtocolHeader *header, char *p) {
	ProtocolDestroyMessage(p);
}

void HandleHeartBeat(ProtocolHeader *header, char *p) {
	ProtocolDestroyMessage(p);
}

void HandleSettingUser(ProtocolHeader *header, char *p) {
	int len;
	int j, i = p[0] - '0';
	p[12] = 0;
	SMSCmdSetUser(i, (char *)&p[1]);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleRemoveUser(ProtocolHeader *header, char *p) {
	int len;
	int index = p[0] - '0';
	SMSCmdRemoveUser(index);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleDeadTime(ProtocolHeader *header, char *p) {
	int len;
//	int choose;
//	choose = (p[1] - '0') * 10 + (p[0] - '0');
//	XfsTaskSetSpeakPause(choose);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleVoiceType(ProtocolHeader *header, char *p) {
	int len;
//	choose = *p;
//	if (choose == 0x34) {
//		choose = MANSOUND;
//	} else if (choose == 0x35) {
//		choose = MIXSOUND;
//	} else if (choose == 0x33) {
//		choose = WOMANSOUND;
//	}
//	XfsTaskSetSpeakType(choose);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleVolumeSetting(ProtocolHeader *header, char *p) {
	int len;
//	choose = *p;
//	XfsTaskSetSpeakVolume(choose);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleBroadcastTimes(ProtocolHeader *header, char *p) {
	int len;
//	times = (p[1] - '0') * 10 + (p[0] - '0');
//	XfsTaskSetSpeakTimes(times);
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleSendSMS(ProtocolHeader *header, char *p) {
	int len;
	uint8_t *gbk;
	len = (header->lenH << 8) + header->lenL;
#if defined(__SPEAKER__)
	XfsTaskSpeakUCS2(p, len);
#elif defined(__LED__)
	gbk = Unicode2GBK(p, len);
//	SMS_Prompt();
	DisplayClear();
	MessDisplay(gbk);
	__storeSMS1(gbk);
	Unicode2GBKDestroy(gbk);
	LedDisplayToScan(0, 0, LED_PHY_DOT_WIDTH - 1 , LED_PHY_DOT_HEIGHT - 1);
#endif
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
	return;
}

void HandleRestart(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
	GsmTaskResetSystemAfter(10);
}


void HandleRecoverFactory(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleBasicParameter(ProtocolHeader *header, char *p) {

	ProtocolDestroyMessage(p);
}

void HandleCoordinate(ProtocolHeader *header, char *p) {
	ProtocolDestroyMessage(p);
}


void USART3_Send_Byte(unsigned char byte){
    USART_SendData(USART3, byte); 
    while( USART_GetFlagStatus(USART3,USART_FLAG_TC)!= SET);          
}

void UART3_Send_Str(unsigned char *s, int size){
    unsigned char i=0; 
    for(; i < size; ++i) 
    {
       USART3_Send_Byte(s[i]); 
    }
}

static char para[210];

void HandleWeatherStation(ProtocolHeader *header, char *p) {
	int i, n;
	uint8_t *gbk;
	int len;
	len = (header->lenH << 8) + header->lenL;
	memset(para, ' ', sizeof(para));
	gbk = Unicode2GBK(&p[0], (len - 4));
	if((*gbk++ == 'T') && (*gbk++ == 'E')){

		n = 0;
		para[n++] = '#';
		para[n++] =	'H';
		para[n++] =	'O';
		para[n++] =	'T';

	    //显示温度
		n = 4;
	    para[n++] = 0xCE; //温
		para[n++] = 0xC2;
		para[n++] = 0xB6; //度
		para[n++] = 0xC8;
		para[n++] = 0x3A; //:
	   	for(i=0; i<10; i++){
			para[n++] = *gbk++;
		    while((*gbk == 'D')&& (*(gbk+1) == 'I')){
			      i = 20;
				  gbk = gbk + 2;
				  break;
		    }
	    }
		if(i==10){
		   Unicode2GBKDestroy(gbk);
		   ProtocolDestroyMessage(p);
		   return;
		}
		para[++n] = 0xA1; //℃
		para[++n] = 0xE6;

		//显示风向
		n = 22;
		para[n++] = 0xB7; //风
		para[n++] = 0xE7;
		para[n++] = 0xCF; //向
		para[n++] = 0xF2;
		para[n++] = 0x3A; //:
	    for(i=0; i<14; i++){
	        para[n++] = *gbk++;
		    while((*gbk == 'S') && (*(gbk+1) == 'P')){
			      i = 20;
				  gbk = gbk + 2;
				  break;
		    }
	    }
		if(i==14){
		   Unicode2GBKDestroy(gbk);
		   ProtocolDestroyMessage(p);
		   return;
		}

		//显示风速
		n = 40;
		para[n++] = 0xB7; //风
		para[n++] = 0xE7;
		para[n++] = 0xCB; //速
		para[n++] = 0xD9;
		para[n++] = 0x3A; //:
	    for(i=0; i<10; i++){
            para[n++] = *gbk++;
		    while((*gbk == 'P') && (*(gbk+1) == 'R')){
			      i = 20;
				  gbk = gbk + 2;
				  break;
		    }
	    }
		if(i==10){
		   Unicode2GBKDestroy(gbk);
		   ProtocolDestroyMessage(p);
		   return;
		}
		para[n++] =	0x6D; // m
		para[n++] =	0x2F; // /
		para[n++] =	0x73; // s

		//显示气压
		n = 58;
		para[n++] = 0xC6; //气
		para[n++] = 0xF8;
		para[n++] = 0xD1; //压
		para[n++] = 0xB9;
		para[n++] = 0x3A; //:
	    for(i=0; i<14; i++){
            para[n++] = *gbk++;
		    while((*gbk == 'H') && (*(gbk+1) == 'U')){
			      i = 20;
				  gbk = gbk + 2;
				  break;
		    }
	    }
		if(i == 14){ 
		   Unicode2GBKDestroy(gbk);
		   ProtocolDestroyMessage(p);
		   return;
		}
		para[n++] =	0x68; // h
		para[n++] =	0x70; // p
		para[n++] =	0x61; // a

		//显示相对湿度
		n = 76;
		para[n++] = 0xCF; //相
		para[n++] = 0xE0;
		para[n++] = 0xB6; //对
		para[n++] = 0xD4;
		para[n++] = 0xCA; //湿
		para[n++] = 0xAA;
		para[n++] = 0xB6; //度
		para[n++] = 0xC8;
		para[n++] = 0x3A; //:
	    for(i=0; i<10; i++){
            para[n++] = *gbk++;
		    while((*gbk == 'R') && (*(gbk+1) == 'A')){
			      i = 20;
				  gbk = gbk + 2;
				  break;
		    }
	    }
		if(i == 10){
		   Unicode2GBKDestroy(gbk);
		   ProtocolDestroyMessage(p);
		   return;
		}
		para[n++] =	0x25; // %

		//显示降雨
		n = 94;
		para[n++] = 0xBD; //降
		para[n++] = 0xB5;
		para[n++] = 0xCB; //水
		para[n++] = 0xAE;
		para[n++] = 0x3A; //:
	    for(i=0; i<10; i++){
            para[n++] = *gbk++;
		    while((*gbk == 'T') && (*(gbk+1) == 'O')){
			      i = 20;
				  gbk = gbk + 2;
				  break;
		    }
	    }
		if(i == 10){
		   Unicode2GBKDestroy(gbk);
		   ProtocolDestroyMessage(p);
		   return;
		}
		para[n++] =	0x6D; //m
		para[n++] =	0x6D; //m

		//显示24小时天气预报数据
		n = 112;
		para[n++] = 0x32; //24
		para[n++] = 0x34;
		para[n++] = 0x68; //h
		para[n++] = 0x3A; //:
	    for(i=0; i<20; i++){
            para[n++] = *gbk++;
		    while((*gbk == 'A') && (*(gbk+1) == 'T')){
			      i = 30;
				  gbk = gbk + 2;
				  break;
		    }
	    }
		if(i == 20){
		   Unicode2GBKDestroy(gbk);
		   ProtocolDestroyMessage(p);
		   return;
		}

		//显示48小时天气预报数据
		n = 130;
		para[n++] = 0x34; //48
		para[n++] = 0x38;
		para[n++] = 0x68; //h
		para[n++] = 0x3A; //:
	    for(i=0; i<20; i++){
            para[n++] = *gbk++;
		    while((*gbk == 'T') && (*(gbk+1) == 'I')){
			      i = 30;
				  gbk = gbk + 2;
				  break;
		    }
	    }
		if(i == 20){
		   Unicode2GBKDestroy(gbk);
		   ProtocolDestroyMessage(p);
		   return;
		}

		//采集数据时间
		n = 148;
	    for(i=0; i<20; i++){
            para[n++] = *gbk++;
		    while((*gbk == 'T')&& (*(gbk+1) == 'F')){
			      i = 40;
				  gbk = gbk + 2;
				  break;
		    }
	    }
		if(i == 20){
		   Unicode2GBKDestroy(gbk);
		   ProtocolDestroyMessage(p);
		   return;
		}

		//信息来源
	    n = 184;
		para[n++] =	0xF2;//颍
		para[n++] =	0xA3;
		para[n++] =	0xC9;//上
		para[n++] =	0xCF;
		para[n++] =	0xCF;//县
		para[n++] =	0xD8;
		para[n++] =	0xC6;//气
		para[n++] =	0xF8;
		para[n++] =	0xCF;//象
		para[n++] =	0xF3;
		para[n++] =	0xBE;//局
		para[n++] =	0xD6;
		para[n++] =	0xB7;//发
		para[n++] =	0xA2;
		para[n++] =	0xB2;//布
		para[n++] =	0xBC;
//		para[n++] = '\r';
//		para[n] = 0x00;
		para[sizeof(para) - 1] = '\r';

	}
    UART3_Send_Str(&para[0], sizeof(para));
	Unicode2GBKDestroy(gbk);
	ProtocolDestroyMessage(p);
}

void HandleRecordMP3(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleSMSPromptSound(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleRecordPromptSound(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleMP3Music(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}

void HandleLongSMS(ProtocolHeader *header, char *p) {
	int len;
	len = (header->lenH << 8) + header->lenL;
	p = TerminalCreateFeedback((char *) & (header->type), &len);
	GsmTaskSendTcpData(p, len);
	ProtocolDestroyMessage(p);
}


void ProtocolHandler(char *p) {
//	if (strncmp(p, "#H", 2) != 0) return;
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
		{'3', '4', HandleWeatherStation},
		{'4', '1', HandleRecordMP3},
		{'4', '2', HandleSMSPromptSound},
		{'4', '3', HandleRecordPromptSound},
		{'4', '4', HandleMP3Music},
		{'4', '5', HandleLongSMS},
	};
	ProtocolHeader *header = (ProtocolHeader *)p;

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if ((map[i].type == header->type) && (map[i].class == header->class)) {
			map[i].func(header, p + sizeof(ProtocolHeader));
			break;
		}
	}
}


