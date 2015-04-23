#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gsm.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
#include "sms.h"
#include "stm32f10x_gpio.h"
#include "norflash.h"
#include "zklib.h"
#include "libupdater.h"
#include "norflash.h"
#include "sms_cmd.h"
#include "second_datetime.h"
#include "rtc.h"
#include "version.h"
#include "elegath.h"

extern int GsmTaskSendTcpData(const char *p, int len);

typedef enum{
	ACKERROR = 0,           /*��վӦ���쳣*/
	GATEPARAM = 0x01,       /*���ز�������*/
	LIGHTPARAM = 0x02,      /*�Ʋ�������*/
	STRATEGY = 0x03,        /*��������*/
	DIMMING = 0x04,         /*�Ƶ������*/
	LAMPSWITCH = 0x05,      /*�ƿ��ؿ���*/
	READDATA = 0x06,        /*������������*/
	LOOPCONTROL = 0x07,     /*���ػ�·����*/
	DATAQUERY = 0x08,       /*�������ݲ�ѯ*/
	TIMEQUERY = 0x09,       /*���ؿ��ص�ʱ���ѯ*/
	AUTOWORK = 0x0A,        /*����������*/
	TIMEADJUST = 0x0B,      /*Уʱ*/
	VERSIONQUERY = 0x0C,    /*��������汾�Ų�ѯ*/ 
  ELECTRICGATHER = 0x0E,  /*�����ɼ�����汾�Ų�ѯ*/	
	LUXUPLOAD = 0x0F,       /*���ն��ϴ�*/
	SMSSEND = 0x10,         /*���ŷ���*/
	ADDRESSQUERY = 0x11,    /*���ص�ַ��ѯ*/
	SETSERVERIP = 0x14,     /*��������Ŀ�������IP*/
	GATEUPGRADE = 0x15,     /*����Զ������*/
	GATHERUPGRADE = 0x1E,   /*�����ɼ�ģ��Զ������*/
	BALLASTUPGRADE= 0x2A,   /*������Զ������*/
	RESTART = 0x3F,         /*�豸��λ*/
	RETAIN,                 /*����*/
} GatewayType;

typedef struct {
	unsigned char BCC[2];
	unsigned char x01;
} ProtocolTail;

unsigned char *ProtocolRespond(unsigned char address[10], unsigned char  type, const char *msg, int *size) {
	int i;
	unsigned int verify = 0;
	unsigned char *p, *ret;
	unsigned char hexTable[] = "0123456789ABCDEF";
	int len = ((msg == NULL) ? 0 : strlen(msg));
	*size = sizeof(ProtocolHead) + len + sizeof(ProtocolTail);
	type = (type | 0x80);
	ret = pvPortMalloc(*size);
	{
		ProtocolHead *h = (ProtocolHead *)ret;
		h->header = 0x02;	
		strcpy((char *)&(h->addr[0]), (const char *)address);
		h->contr[0] = hexTable[(type >> 4) & 0x0F];
		h->contr[1] = hexTable[type & 0x0F];
		h->lenth[0] = hexTable[(len >> 4) & 0x0F];
		h->lenth[1] = hexTable[len & 0x0F];
	}
	
	if (msg != NULL) {
		strcpy((char *)(ret + sizeof(ProtocolHead)), msg);
	}
	
	p = ret;
	for (i = 0; i < (len + sizeof(ProtocolHead)); ++i) {
		verify ^= *p++;
	}
	
	*p++ = hexTable[(verify >> 4) & 0x0F];
	*p++ = hexTable[verify & 0x0F];
	*p++ = 0x03;
	*p = 0;
	return ret;
}


void ReadGWIPPort(GMSParameter *g){
	int len;
	short tmp[64];
	unsigned char *buf;
	
	NorFlashRead(NORFLASH_MANAGEM_ADDR, tmp, sizeof(GMSParameter));

	buf = pvPortMalloc(sizeof(GMSParameter) + 1);
	for(len=0; len<(sizeof(GMSParameter) + 1); len++){
		buf[len] = (char)tmp[len];
	}
	sscanf((const char *)buf, "%10s", g->GWAddr);
	sscanf((const char *)buf, "%*10s%16s", g->serverIP);
	g->serverPORT = ((*(buf+29)) << 8) + *(buf + 28);
	vPortFree(buf);	
}

void ProtocolDestroyMessage(const char *p) {
	vPortFree((void *)p);
}

//static void Analysis(ProtocolHead *data, const char *p){
//	sscanf(&p[1], "%10s", data->addr);
//	sscanf(p, "%*11s%2s", data->contr);
//	sscanf(p, "%*13s%2s", data->lenth);
//}

static void HandleGatewayParam(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	int len;
	unsigned char *buf, msg[2];
	short tmp[255];
	
//	if((strlen(p) != 27) || ((strlen(p) != 50)) || (strlen(p) != 78)){
//		return;
//	}
//	len = strlen(p);

	if(strlen(p) == 50){
		GatewayParam1 g;
		sscanf(p, "%*1s%6s", g.GatewayID);
		sscanf(p, "%*7s%9s", g.Longitude);
		sscanf(p, "%*16s%9s", g.Latitude);
	//	sscanf(p, "%*13s%1s", g->FrequPoint);
		g.FrequPoint = p[26];
		sscanf(p, "%*25s%2s", g.IntervalTime);
		sscanf(p, "%*27s%2s", g.TransfRatio);
		buf = (unsigned char *)&g;
		for(len=0; len<(sizeof(GatewayParam1)+1); len++){
			tmp[len] = buf[len];
		}
		NorFlashWrite(NORFLASH_MANAGEM_BASE, (const short *)tmp, sizeof(GatewayParam1));
	} else if(strlen(p) == 27){
		GatewayParam2 *g;
		sscanf(p, "%*1s%2s", g->OpenOffsetTime1);
		sscanf(p, "%*2s%2s", g->OpenOffsetTime2);
		sscanf(p, "%*4s%2s", g->CloseOffsetTime1);
		sscanf(p, "%*8s%2s", g->CloseOffsetTime2);
	//	NorFlashWriteChar(NORFLASH_MANAGEM_TIMEOFFSET, (const char *)g, sizeof(GatewayParam2));
		buf = (unsigned char *)&g;
		for(len=0; len<(sizeof(GatewayParam2)+1); len++){
			tmp[len] = buf[len];
		}
		NorFlashWrite(NORFLASH_MANAGEM_TIMEOFFSET, (const short *)tmp, sizeof(GatewayParam2));
	} else if(strlen(p) == 78){
		GatewayParam3 *g;
		sscanf(p, "%*1s%12s", g->HVolLimitVal);
		sscanf(p, "%*13s%12s", g->LVolLimitVal);
		sscanf(p, "%*25s%16s", g->NoloadCurLimitVal);
		sscanf(p, "%*41s%16s", g->PhaseCurLimitVal);
//		sscanf(p, "%*16s%2s", g->NumbOfCNBL);
		g->NumbOfCNBL = p[57];
		sscanf(p, "%*58s%2s", g->OtherWarn);
//		NorFlashWriteChar(NORFLASH_MANAGEM_WARNING, (const char *)g, sizeof(GatewayParam3));
		buf = (unsigned char *)&g;
		for(len=0; len<(sizeof(GatewayParam3)+1); len++){
			tmp[len] = buf[len];
		}
		NorFlashWrite(NORFLASH_MANAGEM_WARNING, (const short *)tmp, sizeof(GatewayParam3));
	}
	
	sprintf((char *)msg, "%c", p[0]);
	
	buf = ProtocolRespond(AddrOfGate, type, (const char *)msg, &len);
  GsmTaskSendTcpData((const char *)buf, len);
	ProtocolDestroyMessage((const char *)buf);
}

typedef struct{
	unsigned char AddrOfZigbee[4];   /*ZIGBEE��ַ*/
	unsigned char NorminalPower[4];  /*��ƹ���*/ 
	unsigned char Loop;              /*������·*/ 
	unsigned char LightPole[4];      /*�����Ƹ˺�*/ 
	unsigned char LightSourceType;   /*��Դ����*/ 
	unsigned char LoadPhaseLine;     /*��������*/ 
	unsigned char Attribute[2];      /*��/��/Ͷ����*/ 
}Lightparam;

static void HandleLightParam(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	int len, i;
	unsigned char *buf, msg[5];
	Lightparam g;
	short tmp[255];

	if(p[15] == 0x01){          /*����һյ��*/
		sscanf(p, "%*5s%4s", g.AddrOfZigbee);
		len = atoi((const char *)&(g.AddrOfZigbee));
		sscanf(p, "%*9s%4s", g.NorminalPower);
//		sscanf(p, "%*16s%12s", g->Loop);
		g.Loop = p[13];
		sscanf(p, "%*14s%4s", g.LightPole);
//		sscanf(p, "%*16s%12s", g->LightSourceType);
		g.LightSourceType = p[18];
//		sscanf(p, "%*16s%12s", g->LoadPhaseLine);
		g.LoadPhaseLine = p[19];
		sscanf(p, "%*19s%2s", g.Attribute);
		
	//	NorFlashWriteChar(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const char *)g, sizeof(Lightparam));
		buf = (unsigned char *)&g;
		for(i=0; i<(sizeof(Lightparam)+1); i++){
			tmp[i] = buf[i];
		}
		NorFlashWrite(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)tmp, sizeof(Lightparam));
		
	} else if (p[15] == 0x02){   /*ɾ��һյ��*/
		sscanf(p, "%*5s%4s", g.AddrOfZigbee);
		len = atoi((const char *)&(g.AddrOfZigbee));
		NorFlashEraseParam(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE);
	} else  if (p[15] == 0x03){  /*����һյ��*/
		sscanf(p, "%*5s%4s", g.AddrOfZigbee);
		len = atoi((const char *)&(g.AddrOfZigbee));
		NorFlashEraseParam(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE);
		sscanf(p, "%*9s%4s", g.AddrOfZigbee);
		len = atoi((const char *)&(g.AddrOfZigbee));
		sscanf(p, "%*13s%4s", g.NorminalPower);
//		sscanf(p, "%*16s%12s", g->Loop);
		g.Loop = p[17];
		sscanf(p, "%*18s%4s", g.LightPole);
//		sscanf(p, "%*16s%12s", g->LightSourceType);
		g.LightSourceType = p[22];
//		sscanf(p, "%*16s%12s", g->LoadPhaseLine);
		g.LoadPhaseLine = p[23];
		sscanf(p, "%*24s%2s", g.Attribute);
//		NorFlashWriteChar(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const char *)g, sizeof(Lightparam));
	} else if (p[15] == 0x04){
		for(len = 0; len < 250; len++){
			NorFlashEraseParam(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE);
		}
	}
	
	sprintf((char *)msg, "%s%c", g.AddrOfZigbee, p[0]);
	
	buf = ProtocolRespond(AddrOfGate, type, (const char *)msg, &len);
  GsmTaskSendTcpData((const char *)buf, len);
	ProtocolDestroyMessage((const char *)buf);	
}

typedef struct{
	unsigned char SyncTime[12];    /*Zigbee��ַ*/
	unsigned char SchemeType[2];   /*��������*/
	unsigned char DimmingNOS;      /*�������*/
	unsigned char FirstDCTime[4];  /*��һ�ε������ʱ��*/
	unsigned char FirstDPVal[2];   /*��һ�ε��⹦��ֵ*/
	unsigned char SecondDCTime[4]; /*�ڶ��ε������ʱ��*/
	unsigned char SecondDPVal[2];  /*�ڶ��ε��⹦��ֵ*/
	unsigned char ThirdDCTime[4];  /*�����ε������ʱ��*/
	unsigned char ThirdDPVal[2];   /*�����ε��⹦��ֵ*/
	unsigned char FourthDCTime[4]; /*���Ķε������ʱ��*/
	unsigned char FourthDPVal[2];  /*���Ķε��⹦��ֵ*/
	unsigned char FifthDCTime[4];  /*����ε������ʱ��*/
	unsigned char FifthDPVal[2];   /*����ε��⹦��ֵ*/
}StrategyParam;

static void HandleStrategy(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	int len;
	char NubOfZig;
	unsigned char *buf, msg[7], time[12];
	StrategyParam *g;
	DateTime dateTime;
	uint32_t second;	
	short tmp[255];	
	
	sscanf(p, "%*4s%2s", g->SchemeType);
//	sscanf(p, "%*15s%4s", g->DimmingNOS);
	second = RtcGetTime();
	SecondToDateTime(&dateTime, second);
	buf = (unsigned char *)&dateTime;
	for(len= 0; len < 12; len++){
		if(len%2 == 0){
			time[len] = (buf[len/2] >> 4) + '0';
		} else {
			time[len] = (buf[len/2] & 0x0F) + '0';
		}
	}
	strcpy((char *)g->SyncTime, (const char *)time);
	g->DimmingNOS = p[6];
	sscanf(p, "%*7s%4s", g->FirstDCTime);
	sscanf(p, "%*11s%2s", g->FirstDPVal);
	sscanf(p, "%*13s%4s", g->SecondDCTime);
	sscanf(p, "%*17s%2s", g->SecondDPVal);
	sscanf(p, "%*19s%4s", g->ThirdDCTime);
	sscanf(p, "%*23s%2s", g->ThirdDPVal);
	sscanf(p, "%*25s%4s", g->FourthDCTime);
	sscanf(p, "%*29s%2s", g->FourthDPVal);
	sscanf(p, "%*31s%4s", g->FifthDCTime);
	sscanf(p, "%*35s%2s", g->FifthDPVal);
	
	//NorFlashWriteShort(NORFLASH_STRATEGY_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)g, sizeof(StrategyParam));
	
	buf = (unsigned char *)&g;
	for(len=0; len<(sizeof(StrategyParam)+1); len++){
		tmp[len] = buf[len];
	}
	
	memset(msg, 0, 7);
	sscanf(p, "%4s", msg);
	NubOfZig = atoi((const char *)msg);
	
	NorFlashWrite(NORFLASH_STRATEGY_BASE + NubOfZig * NORFLASH_SECTOR_SIZE, (const short *)tmp, sizeof(StrategyParam));
	
	sscanf(p, "%6s", msg);
	
	buf = ProtocolRespond(AddrOfGate, type, (const char *)msg, &len);
  GsmTaskSendTcpData((const char *)buf, len);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleLightDimmer(unsigned char AddrOfGate[10], unsigned char type, const char *p)  {
	int len;
	unsigned char DataFlag[4], Dim[2];
	
	sscanf(p, "%4s", DataFlag);
	sscanf(p, "%*4s%2s", Dim);
	
	if(DataFlag[0] == 0x0A){          /*�����ص��⣬�����������е�*/
		
	} else if(DataFlag[0] == 0x08){   /*���ջ�·/����/����/Ͷ����⣬����Ͷ���*/
		
	} else if(DataFlag[0] == 0x09){   /*���ջ�·/����/�������⣬������Ͷ���*/
		
	} else if(DataFlag[0] == 0x0B){   /*���յ��Ƶ���*/
	}
	
}

static void HandleLightOnOff(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleReadBSNData(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleGWloopControl(unsigned char AddrOfGate[10], unsigned char type, const char *p){
	
}

static void HandleGWDataQuery(unsigned char AddrOfGate[10], unsigned char type, const char *p) {     /*���ػ�·���ݲ�ѯ*/
	
	
}

static void HandleGWTurnTimeQuery(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleLightAuto(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleAdjustTime(unsigned char AddrOfGate[10], unsigned char type, const char *p) {    /*Уʱ*/
	DateTime ServTime;
	uint32_t second;
	
	second = RtcGetTime();
	
	ServTime.year = (p[0] - '0') * 10 + p[1] - '0';
	ServTime.month = (p[2] - '0') * 10 + p[3] - '0';
	ServTime.date = (p[4] - '0') * 10 + p[5] - '0';
	ServTime.hour = (p[6] - '0') * 10 + p[7] - '0';
	ServTime.minute = (p[8] - '0') * 10 + p[9] - '0';
	ServTime.second = (p[10] - '0') * 10 + p[11] - '0';
	
	if(DateTimeToSecond(&ServTime) > second){
		if((DateTimeToSecond(&ServTime) - second) > 300) {
			RtcSetTime(DateTimeToSecond(&ServTime));
		}	
	} else if (DateTimeToSecond(&ServTime) < second){
		if((second - DateTimeToSecond(&ServTime)) > 300){
			RtcSetTime(DateTimeToSecond(&ServTime));
		}
	}
	
}

static void HandleGWVersQuery(unsigned char AddrOfGate[10], unsigned char type, const char *p) {      /*����������汾��*/
	int len;
	unsigned char *buf;
	
	buf = ProtocolRespond(AddrOfGate, type, Version(), &len);
  GsmTaskSendTcpData((const char *)buf, len);
	ProtocolDestroyMessage((const char *)buf);	
	
}

static void HandleEGVersQuery(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleIllumUpdata(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleSMSSend(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleGWAddrQuery(unsigned char AddrOfGate[10], unsigned char type, const char *p) {   /*���ص�ַ��ѯ*/
	int len;
	unsigned char *buf, msg[5];	
	
	sscanf(p, "%4s", msg);
	buf = ProtocolRespond(AddrOfGate, type, (const char *)msg, &len);
  GsmTaskSendTcpData((const char *)buf, len);
	printf("%s.\r\n", buf);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleSetGWServ(unsigned char AddrOfGate[10], unsigned char type, const char *p) {      /*��������Ŀ�������IP*/
	int len;
	unsigned char *buf;
	unsigned char tmp[255];
	GMSParameter g;
	
	sscanf(p, "%[^,]", g.serverIP);
	sscanf(p, "%*[^,]%*c%d", &(g.serverPORT));

//	g.serverPORT = atoi((const char *)msg);
	
	buf = (unsigned char *)&g;
	for(len=0; len<(sizeof(GMSParameter)+1); len++){
			tmp[len] = buf[len];
	}
	NorFlashWrite(NORFLASH_MANAGEM_WARNING, (const short *)tmp, sizeof(GMSParameter));
	
	buf = ProtocolRespond(AddrOfGate, type, NULL, &len);
  GsmTaskSendTcpData((const char *)buf, len);
	ProtocolDestroyMessage((const char *)buf);	
	
}

static void HandleGWUpgrade(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleEGUpgrade(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleBSNUpgrade(unsigned char AddrOfGate[10], unsigned char type, const char *p) {
	
}

static void HandleRestart(unsigned char AddrOfGate[10], unsigned char type, const char *p){            /*�豸��λ*/
	unsigned char *buf, msg[2] = {3};
	int len;
	
	if((p[0] == 1) ||(p[0] == 2)){
		NVIC_SystemReset();
	} else if(p[0] == 3){
		buf = ProtocolRespond(AddrOfGate, type, (const char *)msg, &len);
		EleComSendString((char *)buf);
		ProtocolDestroyMessage((const char *)buf);	
	}
	
}

typedef void (*ProtocolHandleFunction)(unsigned char AddrOfGate[10], unsigned char type, const char *p);
typedef struct {
	unsigned char type;
	ProtocolHandleFunction func;
} ProtocolHandleMap;


/*GW: gateway  ����*/
/*EG: electric quantity gather �����ɼ���*/
/* illuminance �� ���ն�*/
/*BSN: �Ƶ�������*/
void ProtocolHandler(unsigned char tmp, char *p) {
	int i;
	GMSParameter g;
	char hexTable[] = "0123456789ABCDEF";
	char *ret;
	char verify = 0;
	
	
	const static ProtocolHandleMap map[] = {  
		{GATEPARAM,      HandleGatewayParam},     /*0x01; ���ز�������*/           ///
		{LIGHTPARAM,     HandleLightParam},       /*0x02; �Ʋ�������*/             /// 
		{STRATEGY,       HandleStrategy},         /*0x03; ��������*/               ///
		{DIMMING,        HandleLightDimmer},      /*0x04; �Ƶ������*/
		{LAMPSWITCH,     HandleLightOnOff},       /*0x05; �ƿ��ؿ���*/
		{READDATA,       HandleReadBSNData},      /*0x06; ������������*/
		{LOOPCONTROL,    HandleGWloopControl},    /*0x07; ���ػ�·����*/
		{DATAQUERY,      HandleGWDataQuery},      /*0x08; �������ݲ�ѯ*/
		{TIMEQUERY,      HandleGWTurnTimeQuery},  /*0x09; ���ؿ��ص�ʱ���ѯ*/
		{AUTOWORK,       HandleLightAuto},        /*0x0A; ���Զ�����*/
		{TIMEADJUST,     HandleAdjustTime},       /*0x0B; Уʱ*/                   ///          
		{VERSIONQUERY,   HandleGWVersQuery},      /*0x0C; ����������汾��*/       ///
		{ELECTRICGATHER, HandleEGVersQuery},      /*0x0E; ������ɼ�����汾��*/
		{LUXUPLOAD,      HandleIllumUpdata},      /*0x0F; ���ն��ϴ�*/
		{SMSSEND,        HandleSMSSend},          /*0x10; ���ŷ���*/
		{ADDRESSQUERY,   HandleGWAddrQuery},      /*0x11; ���ص�ַ��ѯ*/           ///
		{SETSERVERIP,    HandleSetGWServ},        /*0x14; ��������Ŀ�������IP*/   ///
		{GATEUPGRADE,    HandleGWUpgrade},        /*0x15; ����Զ������*/
		{GATHERUPGRADE,  HandleEGUpgrade},        /*0x1E; �����ɼ�ģ��Զ������*/
		{BALLASTUPGRADE, HandleBSNUpgrade},       /*0x2A; ������Զ������*/
		{RESTART,        HandleRestart},          /*0x3F; �豸��λ*/              
	};

	ret = p;
	for (i = 0; i < (strlen(p) - 3); ++i) {
		verify ^= *ret++;
	}
	
	if(*ret++ != hexTable[(verify >> 4) & 0x0F]){
		tmp = tmp | 0x40;
	}
	
	if((*ret++ != hexTable[verify & 0x0F])){
		tmp = tmp | 0x40;
	}
	
	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
			if ((map[i].type == tmp)) {
				ReadGWIPPort(&g);	
				map[i].func(g.GWAddr, tmp, p + 15);
				break;
			}
	}
}
