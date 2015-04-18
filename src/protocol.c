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
#include "stm32f10x_gpio.h"
#include "norflash.h"
#include "zklib.h"
#include "libupdater.h"
#include "norflash.h"
#include "sms_cmd.h"

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
	RETAIN,          /*����*/
} GatewayType;

typedef struct {
	unsigned char header;
	unsigned char addr[10];
	unsigned char contr[2];
	unsigned char lenthH;
	unsigned char lenthL;
} ProtocolHead;

typedef struct {
	unsigned char BCC[2];
	unsigned char x01;
} ProtocolTail;

typedef struct {
	unsigned char Teleph[4];
	unsigned char AreaID[2];
	unsigned char GateID[4];
} AddrField;

char *ProtocolSend(AddrField *address, GatewayType type, const char *msg, int *size) {
	int i;
	unsigned int verify = 0;
	unsigned char *p, *ret;
	unsigned char hexTable[] = "0123456789ABCDEF";
	int len = (msg == NULL ? 0 : strlen(msg));
	*size = sizeof(ProtocolHead) + len + sizeof(ProtocolTail);
	ret = pvPortMalloc(*size);
	{
		ProtocolHead *h = (ProtocolHead *)ret;
		h->header = 0x02;	
		h->addr[0] = address->Teleph[0];
		h->addr[1] = address->Teleph[1];
		h->addr[2] = address->Teleph[2];
		h->addr[3] = address->Teleph[3];
		h->addr[4] = address->AreaID[0];
		h->addr[5] = address->AreaID[1];
		h->addr[6] = address->GateID[0];
		h->addr[7] = address->GateID[1];
		h->addr[8] = address->GateID[2];
		h->addr[9] = address->GateID[3];
		h->contr[0] = hexTable[(type >> 4) & 0x0F];
		h->contr[1] = hexTable[type & 0x0F];
		h->lenthH = hexTable[(len >> 4) & 0x0F];
		h->lenthL = hexTable[len & 0x0F];
	}
	
	if (msg != NULL) {
		strcpy((char *)(ret + sizeof(ProtocolHead)), msg);
	}
	
	p = ret;
	for (i = 0; i < len + sizeof(ProtocolHead); ++i) {
		verify ^= *p++;
	}
	
	*p++ = hexTable[(verify >> 4) & 0x0F];
	*p++ = hexTable[verify & 0x0F];
	*p++ = 0x03;
	return ret;
}

void ProtocolDestroyMessage(const char *p) {
	vPortFree((void *)p);
}


static void HandleGatewayParam(const char *p) {
	
}

static void HandleLightParam(const char *p) {
	
}

static void HandleStrategy(const char *p) {
	
}

static void HandleLightDimmer(const char *p) {
	
}

static void HandleLightOnOff(const char *p) {
	
}

static void HandleReadBSNData(const char *p) {
	
}

static void HandleGWloopControl(const char *p) {
	
}

static void HandleGWDataQuery(const char *p) {
	
}

static void HandleGWTurnTimeQuery(const char *p) {
	
}

static void HandleLightAuto(const char *p) {
	
}

static void HandleAdjustTime(const char *p) {
	
}

static void HandleGWVersQuery(const char *p) {
	
}

static void HandleEGVersQuery(const char *p) {
	
}

static void HandleIllumUpdata(const char *p) {
	
}

static void HandleSMSSend(const char *p) {
	
}

static void HandleGWAddrQuery(const char *p) {
	
}

static void HandleSetGWServ(const char *p) {
	
}

static void HandleGWUpgrade(const char *p) {
	
}

static void HandleEGUpgrade(const char *p) {
	
}

static void HandleBSNUpgrade(const char *p) {
	
}

typedef void (*ProtocolHandleFunction)(const char *p);
typedef struct {
	unsigned char type;
	ProtocolHandleFunction func;
} ProtocolHandleMap;


void ProtocolHandler(unsigned char tmp, char *p) {
	int i;
	const static ProtocolHandleMap map[] = {  /*GW: gateway  ����*/
		{GATEPARAM,      HandleGatewayParam},   /*EG: electric quantity gather �����ɼ���*/
		{LIGHTPARAM,     HandleLightParam},     /* illuminance �� ���ն�*/
		{STRATEGY,       HandleStrategy},       /*BSN: �Ƶ�������*/
		{DIMMING,        HandleLightDimmer},
		{LAMPSWITCH,     HandleLightOnOff},
		{READDATA,       HandleReadBSNData},
		{LOOPCONTROL,    HandleGWloopControl},
		{DATAQUERY,      HandleGWDataQuery},
		{TIMEQUERY,      HandleGWTurnTimeQuery},
		{AUTOWORK,       HandleLightAuto},
		{TIMEADJUST,     HandleAdjustTime},
		{VERSIONQUERY,   HandleGWVersQuery},
		{ELECTRICGATHER, HandleEGVersQuery},
		{LUXUPLOAD,      HandleIllumUpdata},
		{SMSSEND,        HandleSMSSend},
		{ADDRESSQUERY,   HandleGWAddrQuery},
		{SETSERVERIP,    HandleSetGWServ},
		{GATEUPGRADE,    HandleGWUpgrade},
		{GATHERUPGRADE,  HandleEGUpgrade},
		{BALLASTUPGRADE, HandleBSNUpgrade},
		{NULL, NULL}
	};

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
			if ((map[i].type == tmp)) {
				map[i].func(p + sizeof(ProtocolHead));
				break;
			}
	}
}
