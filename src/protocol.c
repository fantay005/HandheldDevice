#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gsm.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
#include "norflash.h"
#include "zklib.h"
#include "libupdater.h"
#include "norflash.h"
#include "second_datetime.h"
#include "rtc.h"
#include "version.h"
#include "elegath.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_flash.h"
#include "shuncom.h"
#include "time.h"
#include "semphr.h"
#include "inside_flash.h"

static xSemaphoreHandle __ProSemaphore;

typedef enum{
	ACKERROR = 0,           /*从站应答异常*/
	GATEPARAM = 0x01,       /*网关参数下载*/
	LIGHTPARAM = 0x02,      /*灯参数下载*/
	DIMMING = 0x04,         /*灯调光控制*/
	LAMPSWITCH = 0x05,      /*灯开关控制*/
	READDATA = 0x06,        /*读镇流器数据*/
	LOOPCONTROL = 0x07,     /*网关回路控制*/
	DATAQUERY = 0x08,       /*网关数据查询*/
	TIMEADJUST = 0x0B,      /*校时*/
	VERSIONQUERY = 0x0C,    /*网关软件版本号查询*/ 
  ELECTRICGATHER = 0x0E,  /*电量采集软件版本号查询*/	
	GATEUPGRADE = 0x37,     /*网关远程升级*/
	LUXVALUE = 0x38,        /*接收到光强度值*/
	RESTART = 0x3F,         /*设备复位*/
	RETAIN,                 /*保留*/
} GatewayType;

typedef struct {
	unsigned char BCC[2];
	unsigned char x03;
} ProtocolTail;

void CurcuitContrInit(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CRTL_EN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_EN, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_EN, PIN_CRTL_EN);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_1, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_1, PIN_CTRL_1);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_2, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_2, PIN_CTRL_2);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_3, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_3, PIN_CTRL_3);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_4, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_4, PIN_CTRL_4);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_5, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_5, PIN_CTRL_5);

	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_6, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_6, PIN_CTRL_6);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_7, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_7, PIN_CTRL_7);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_8, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_8, PIN_CTRL_8);	

	GPIO_ResetBits(GPIO_CTRL_EN, PIN_CRTL_EN);
}

unsigned char *DataSendToBSN(unsigned char control[2], unsigned char address[4], const char *msg, unsigned char *size) {
	unsigned char i;
	unsigned int verify = 0;
	unsigned char *p, *ret, tmp[5];
	unsigned char hexTable[] = "0123456789ABCDEF";
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	*size = 9 + len + 3 + 2;
	
	ret = pvPortMalloc(*size + 1);
	
	
	if(strncmp((const char *)address, "FFFF", 4) == 0){
		*ret = 0xFF;
		*(ret + 1) = 0xFF;
	} else {
		
#if defined (__HEXADDRESS__)	
		sscanf((const char *)address, "%4s", tmp);
		verify = strtol((const char *)tmp, NULL, 16);
		*ret = verify >> 8;
		*(ret + 1) = verify & 0xFF;
#else		
		*ret = (address[0] << 4) | (address[1] & 0x0f);
		*(ret + 1) = (address[2] << 4) | (address[3] & 0x0f);
#endif		
	}
	{
		FrameHeader *h = (FrameHeader *)(ret + 2);
		h->FH = 0x02;	
		strcpy((char *)&(h->AD[0]), (const char *)address);
		h->CT[0] = control[0];
		h->CT[1] = control[1];
		h->LT[0] = hexTable[(len >> 4) & 0x0F];
		h->LT[1] = hexTable[len & 0x0F];
	}

	if (msg != NULL) {
		strcpy((char *)(ret + 11), msg);
	}
	
	p = ret + 2;
	
	verify = 0;
	for (i = 0; i < (len + 9); ++i) {
		verify ^= *p++;
	}
	
	*p++ = hexTable[(verify >> 4) & 0x0F];
	*p++ = hexTable[verify & 0x0F];
	*p++ = 0x03;
	*p = 0;
	return ret;
}

extern char HexToAscii(char hex);

unsigned char *ProtocolRespond(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	unsigned char i;
	unsigned int verify = 0;
	unsigned char *p, *ret;
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	*size = 15 + len + 3;
	if(type[1] > '9'){
		i = (unsigned char)(type[0] << 4) | (type[1] - 'A' + 10);
	} else{
		i = (unsigned char)(type[0] << 4) | (type[1] & 0x0f);
	}
	i = i | 0x80;
	ret = pvPortMalloc(*size + 1);
	{
		ProtocolHead *h = (ProtocolHead *)ret;
		h->header = 0x02;	
		strcpy((char *)h->addr, (const char *)address);
		h->contr[0] = HexToAscii(i >> 4);
		h->contr[1] = HexToAscii(i & 0x0F);
		h->lenth[0] = HexToAscii((len >> 4) & 0x0F);
		h->lenth[1] = HexToAscii(len & 0x0F);
	}

	if (msg != NULL) {
		strcpy((char *)(ret + 15), msg);
	}
	
	p = ret;
	for (i = 0; i < (len + 15); ++i) {
		verify ^= *p++;
	}
	
	*p++ = HexToAscii((verify >> 4) & 0x0F);
	*p++ = HexToAscii(verify & 0x0F);
	*p++ = 0x03;
	*p = 0;
	return ret;
}

unsigned char *ProtocolToElec(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	int i;
	unsigned int verify = 0;
	unsigned char *p, *ret;
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	
	*size = 15 + len + 3;
	i = (unsigned char)(type[0] << 4) + (type[1] & 0x0f);
	
	ret = pvPortMalloc(*size + 1);
	{
		ProtocolHead *h = (ProtocolHead *)ret;
		h->header = 0x02;
		strcpy((char *)h->addr, (const char *)address);
		h->contr[0] = HexToAscii(i >> 4);
		h->contr[1] = HexToAscii(i & 0x0F);
		h->lenth[0] = HexToAscii((len >> 4) & 0x0F);
		h->lenth[1] = HexToAscii(len & 0x0F);
	}

	if (msg != NULL) {
		strcpy((char *)(ret + 15), msg);
	}
	
	p = ret;
	for (i = 0; i < (len + 15); ++i) {
		verify ^= *p++;
	}
	
	*p++ = HexToAscii((verify >> 4) & 0x0F);
	*p++ = HexToAscii(verify & 0x0F);
	*p++ = 0x03;
	*p = 0;
	return ret;
}


void ProtocolDestroyMessage(const char *p) {
	vPortFree((void *)p);
}

//static void Analysis(ProtocolHead *data, const char *p){
//	sscanf(&p[1], "%10s", data->addr);
//	sscanf(p, "%*11s%2s", data->contr);
//	sscanf(p, "%*13s%2s", data->lenth);
//}

static void HandleGatewayParam(ProtocolHead *head, const char *p) {
	unsigned char size;
	unsigned char *buf, msg[2];
	
//	if((strlen(p) != 27) || ((strlen(p) != 50)) || (strlen(p) != 78)){
//		return;
//	}
//	len = strlen(p);

	if(strlen(p) == (50 - 15)){
		GatewayParam1 g;
		sscanf(p, "%*1s%6s", g.GatewayID);
		sscanf(p, "%*7s%10s", g.Longitude);
		sscanf(p, "%*17s%10s", g.Latitude);
	//	sscanf(p, "%*13s%1s", g->FrequPoint);
		g.FrequPoint = p[27];
		sscanf(p, "%*28s%2s", g.IntervalTime);
		sscanf(p, "%*30s%2s", g.Ratio);
		g.EmbedInformation = 1;
		NorFlashWrite(NORFLASH_MANAGEM_BASE, (const short *)&g, (sizeof(GatewayParam1) + 1) / 2);
	} else if(strlen(p) == (27 - 15)){
		GatewayParam2 g;
		sscanf(p, "%*1s%2s", g.OpenOffsetTime1);
		sscanf(p, "%*3s%2s", g.OpenOffsetTime2);
		sscanf(p, "%*5s%2s", g.CloseOffsetTime1);
		sscanf(p, "%*7s%2s", g.CloseOffsetTime2);
		g.SetFlag = 1;
	//	NorFlashWriteChar(NORFLASH_MANAGEM_TIMEOFFSET, (const char *)g, sizeof(GatewayParam2));
		NorFlashWrite(NORFLASH_MANAGEM_TIMEOFFSET, (const short *)&g, (sizeof(GatewayParam2) + 1) / 2);
	} else if(strlen(p) == (78 - 15)){
		GatewayParam3 g;
		sscanf(p, "%*1s%4s", g.HVolLimitValL1);
		sscanf(p, "%*5s%4s", g.HVolLimitValL2);
		sscanf(p, "%*9s%4s", g.HVolLimitValL3);
		
		sscanf(p, "%*13s%4s", g.LVolLimitValL1);
		sscanf(p, "%*17s%4s", g.LVolLimitValL2);
		sscanf(p, "%*21s%4s", g.LVolLimitValL3);

		sscanf(p, "%*25s%4s", g.NoloadCurLimitValL1);
		sscanf(p, "%*29s%4s", g.NoloadCurLimitValL2);
		sscanf(p, "%*33s%4s", g.NoloadCurLimitValL3);
		sscanf(p, "%*37s%4s", g.NoloadCurLimitValN);
		
		sscanf(p, "%*41s%4s", g.PhaseCurLimitValL1);
		sscanf(p, "%*45s%4s", g.PhaseCurLimitValL2);
		sscanf(p, "%*49s%4s", g.PhaseCurLimitValL3);
		sscanf(p, "%*53s%4s", g.PhaseCurLimitValN);
//		sscanf(p, "%*16s%2s", g->NumbOfCNBL);
		g.NumbOfCNBL = p[57];
		sscanf(p, "%*58s%2s", g.OtherWarn);
		g.SetWarnFlag = 1;
//		NorFlashWriteChar(NORFLASH_MANAGEM_WARNING, (const char *)g, sizeof(GatewayParam3));
		NorFlashWrite(NORFLASH_MANAGEM_WARNING, (const short *)&g, (sizeof(GatewayParam3) + 1) / 2);
	}
	
	sprintf((char *)msg, "%c", p[0]);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);
}

void StoreZigbAddr(unsigned char rank, short Address){
	unsigned short End[96], i;
	NorFlashRead(NORFLASH_END_LIGHT_ADDR , (short *)End, sizeof(End)/sizeof(short));
	for(i = 0; i < 6; i++){
		if(End[rank * 6 + i] == 0xFFFF){
			End[rank * 6 + i] = Address;
			break;
		}
	}
	NorFlashWrite(NORFLASH_END_LIGHT_ADDR , (short *)End, sizeof(End)/sizeof(short));
}

static void __ParamWriteToFlash(const char *p){
	int i;
	unsigned char msg[5];
	unsigned short len;
	DateTime dateTime;
	uint32_t second;	
	Lightparam g;
	
	second = RtcGetTime();
	SecondToDateTime(&dateTime, second);
	
	sscanf(p, "%4s", g.AddrOfZigbee);

		sscanf(p, "%*4s%4s", g.NorminalPower);
//		sscanf(p, "%*16s%12s", g->Loop);
		g.Loop = p[8];
		
		sscanf(p, "%*9s%4s", g.LightPole);		
		len = atoi((const char *)g.LightPole);
		
		if(len >= 9000){
			
		}
//		sscanf(p, "%*16s%12s", g->LightSourceType);
		g.LightSourceType = p[13];
//		sscanf(p, "%*16s%12s", g->LoadPhaseLine);
		g.LoadPhaseLine = p[14];
		sscanf(p, "%*16s%2s", g.Attribute);
		
		sprintf((char *)g.TimeOfSYNC, "%2d%2d%2d%2d%2d%2d", dateTime.year, dateTime.month, dateTime.date, dateTime.hour, dateTime.minute, dateTime.second);
    for(i = 0; i < 12; i++){
			if(g.TimeOfSYNC[i] == 0x20){
				g.TimeOfSYNC[i] = '0';
			}
		}
		g.CommState = 0x04;
		g.InputPower = 0;
		
	//	NorFlashWriteChar(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const char *)g, sizeof(Lightparam));
		
		sscanf(p, "%4s",msg);

#if defined(__HEXADDRESS__)
		len = strtol((const char *)msg, NULL, 16);
#else				
		len = atoi((const char *)msg);
#endif		
		
		NorFlashWrite(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(Lightparam) + 1) / 2);	
}

static void HandleLightParam(ProtocolHead *head, const char *p) {
	int len, i, lenth;
	unsigned char size;
	DateTime dateTime;
	uint32_t second;	
	unsigned char *buf, msg[42], tmp[5];
	Lightparam g;
	
	second = RtcGetTime();
	SecondToDateTime(&dateTime, second);

	if(p[0] == '1'){          /*增加一盏灯*/
		
		len = (strlen(p) - 18) / 17;
		memset(msg, 0, 41);
		for(i = 0; i < len; i++) {
			__ParamWriteToFlash(&p[1 + i * 17]);
			sscanf(&p[1 + i * 17], "%4s", &(msg[i * 4]));
		}
		msg[i * 4 + 1] = p[0];
		
	} else if (p[0] == '2'){   /*删除一盏灯*/
		len = (strlen(p) - 18) / 17;
		memset(msg, 0, 41);
		for(i = 0; i < len; i++) {
			sscanf(&p[1 + i * 17], "%4s", &(msg[i * 4]));
			sscanf(&p[1 + i * 17], "%4s", tmp);
		
#if defined(__HEXADDRESS__)
			lenth = strtol((const char *)tmp, NULL, 16);
#else				
			lenth = atoi((const char *)tmp);
#endif		
		
			NorFlashEraseParam(NORFLASH_BALLAST_BASE + lenth * NORFLASH_SECTOR_SIZE);
		}
		msg[i * 4 + 1] = p[0];
		
	} else  if (p[0] == '3'){  /*更改一盏灯*/
		sscanf(p, "%*5s%4s", g.AddrOfZigbee);
		sscanf(p, "%*5s%4s",msg);
	//	msg[4] = 0;
		len = atoi((const char *)msg);
		NorFlashEraseParam(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE);
		sscanf(p, "%*9s%4s", g.AddrOfZigbee);
		sscanf(p, "%*5s%4s",msg);
	//	msg[4] = 0;
		
#if defined(__HEXADDRESS__)
		len = strtol((const char *)msg, NULL, 16);
#else				
		len = atoi((const char *)msg);
#endif		
		
		sscanf(p, "%*13s%4s", g.NorminalPower);
//		sscanf(p, "%*16s%12s", g->Loop);
		g.Loop = p[17];
		sscanf(p, "%*18s%4s", g.LightPole);
//		sscanf(p, "%*16s%12s", g->LightSourceType);
		g.LightSourceType = p[22];
//		sscanf(p, "%*16s%12s", g->LoadPhaseLine);
		g.LoadPhaseLine = p[23];
		sscanf(p, "%*24s%2s", g.Attribute);
		
    sprintf((char *)g.TimeOfSYNC, "%2d%2d%2d%2d%2d%2d", dateTime.year, dateTime.month, dateTime.date, dateTime.hour, dateTime.minute, dateTime.second);
	  for(i = 0; i < 12; i++){
			if(g.TimeOfSYNC[i] == 0x20){
				g.TimeOfSYNC[i] = '0';
			}
		}
		
		g.CommState = 0x04;
	  g.InputPower = 0;
		
		NorFlashWrite(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(Lightparam) + 1) / 2);
		

	} else if (p[0] == '4'){
		for(len = 0; len < 1000; len++){
			NorFlashEraseParam(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE);
		}
	}
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

typedef struct{	
	unsigned short ArrayAddr[600];
	unsigned char SendFlag;
	unsigned char ProtectFlag;
	unsigned char Command;
	unsigned char NoReply;
	unsigned char NumberOfLoop;
	unsigned short Lenth;
	unsigned char Answer;
	unsigned char Sort;
}ReadBSNData;

static ReadBSNData __msg = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void ProtocolInit(void) {
	memset(__msg.ArrayAddr, 0, 600);
	if (__ProSemaphore == NULL) {
		vSemaphoreCreateBinary(__ProSemaphore);
	}
}

void *DataFalgQueryAndChange(char Obj, unsigned short Alter, char Query){
	if (xSemaphoreTake(__ProSemaphore, configTICK_RATE_HZ * 5) == pdTRUE) {
		if(Query == 0){
			switch (Obj){
				case 1:
					__msg.ArrayAddr[(__msg.Sort)++] = Alter;
					break;
				case 2:
					__msg.Command = Alter;
					break;
				case 3:
					__msg.NoReply = Alter;
					break;
				case 4:
					__msg.SendFlag = Alter;
					break;
				case 5:
					__msg.ProtectFlag = Alter;
					break;
				case 6:
					__msg.NumberOfLoop = Alter;
					break;
				case 7:
					__msg.Lenth = Alter;
				case 8:
					__msg.Sort = 0;
					break;
				default:
					break;
			}
		} 
		xSemaphoreGive(__ProSemaphore);
		if(Query == 1){
			switch (Obj){
				case 1:
					return __msg.ArrayAddr;
				case 2:
					return &(__msg.Command);
				case 3:
					return &(__msg.NoReply);
				case 4:
					return &(__msg.SendFlag);
				case 5:
					return &(__msg.ProtectFlag);
				case 6:
					return &(__msg.NumberOfLoop);
				case 7:
					return &(__msg.Lenth);
				case 8:
					return &(__msg.Answer);
				default:
					break;
			}
		}
	}
	return false;
}


void __DataFlagJudge(const char *p){
	int i = 0, j, len = 0, m, n;
	unsigned char tmp[5], *ret, *msg;
	Lightparam k;
	
	msg = DataFalgQueryAndChange(2, 0, 1);
	if(*msg == 1){
		ret = (unsigned char *)p + 4;
	} else if(*msg == 3) {
		ret = (unsigned char *)p + 5;
	} else if(*msg == 2) {
		ret = (unsigned char *)p + 6;
	}

	if(p[0] == 'A'){
		memset(__msg.ArrayAddr, 0, 600);
		for(len = 0; len < 599; len++) {
			NorFlashRead(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
			if(k.AddrOfZigbee[0] != 0xff){
				sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
				__msg.ArrayAddr[i++] = atoi((const char *)tmp);
			}
		}
	} else if(p[0] == '8'){
		memset(__msg.ArrayAddr, 0, 600);
		for(len = 0; len < 599; len++) {
			NorFlashRead(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
			if(k.Loop == 0xFF){
				continue;
			}
			
			for(j = 0; j < (p[1] - '0'); j++){
				if(k.Loop != *(ret + j)){
					continue;
				}
				
				if(k.Attribute[0] == '0'){                               /*主道灯*/
					for(m = 0; m < (p[2] - '0'); m++){
						if(k.Attribute[1] == *(ret + p[1] - '0' + m)){
							sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
							
							#if defined(__HEXADDRESS__)
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
			
							break;
						}
					}
				} else if(k.Attribute[0] == '1'){	                      /*辅道灯*/
					for(n = 0; n < (p[3] - '0'); n++){
						if(k.Attribute[1] == *(ret + p[1] - '0'  + p[2] - '0' + n)){
							sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
							
							#if defined(__HEXADDRESS__)
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
							
							break;
						}
					}
				} else {                                               /*投光灯*/
						sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
					
							#if defined(__HEXADDRESS__)						
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
					
					  break;
				}
				j = p[1] - '0';				
			}
		}
	} else if(p[0] == '9'){
		memset(__msg.ArrayAddr, 0, 600);
		for(len = 0; len < 599; len++) {
			NorFlashRead(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
		  if(k.Loop == 0xFF){
				continue;
			}
			
			for(j = 0; j < (p[1] - '0'); j++){
				if(k.Loop != *(ret + j)){
					continue;
				}
				
				if(k.Attribute[0] == '0'){                           /*主道灯*/
					for(m = 0; m < (p[2] - '0'); m++){
						if(k.Attribute[1] == *(ret + p[1] - '0' + m)){
							sscanf((const char *)k.AddrOfZigbee, "%4s", tmp);
							
							#if defined(__HEXADDRESS__)
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
							
							break;
						}
					}
				} else if(k.Attribute[0] == '1'){	                   /*辅道灯*/
					for(n = 0; n < (p[3] - '0'); n++){
						if(k.Attribute[1] == *(ret + p[1] - '0'  + p[2] - '0' + n)){
							sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
							
							#if defined(__HEXADDRESS__)
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
							
							break;
						}
					} 
				}
				j = p[1] - '0';		
			}
		}			
	} else if(p[0] == 'B'){
		  memset(__msg.ArrayAddr, 0, 600);
			sscanf((const char *)ret, "%4s", tmp);
		
			#if defined(__HEXADDRESS__)
					__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
			#else		
					__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
			#endif		
		
	}
	__msg.ArrayAddr[i] = 0;
	__msg.Lenth = i;
} 


static unsigned char DataMessage[32];

unsigned char *__datamessage(void){
	return DataMessage;
}

static void HandleLightDimmer(ProtocolHead *head, const char *p){
	unsigned char *buf, msg[8], *ret, size;
	
	ret = DataFalgQueryAndChange(5, 0, 1);
	while(*ret != 0){
		vTaskDelay(configTICK_RATE_HZ / 500);
	}
	
	DataFalgQueryAndChange(5, 1, 0);
	
	DataFalgQueryAndChange(2, 2, 0);
	sscanf(p, "%6s", msg);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
	ret = pvPortMalloc(strlen(p) + 1);
	strcpy((char *)ret, p);
	ret[strlen(p) - 3] = 0;
	
	strcpy((char *)DataMessage, (const char *)ret);
	vPortFree(ret);
	
	DataFalgQueryAndChange(3, 1, 0);
	__DataFlagJudge(p);
}

static void HandleLightOnOff(ProtocolHead *head, const char *p) {
	unsigned char msg[8];
	unsigned char *buf, *ret, size;
	
	ret = DataFalgQueryAndChange(5, 0, 1);
	while(*ret != 0){
		vTaskDelay(configTICK_RATE_HZ / 500);
	}
	
	DataFalgQueryAndChange(5, 1, 0);
	
	DataFalgQueryAndChange(2, 3, 0);
	sscanf(p, "%5s", msg);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
	ret = pvPortMalloc(strlen(p) + 1);
	strcpy((char *)ret, p);
	ret[strlen(p) - 3] = 0;
	
	strcpy((char *)DataMessage, (const char *)ret);
	vPortFree(ret);
	
	DataFalgQueryAndChange(3, 1, 0);
	__DataFlagJudge(p);
}

static void HandleReadBSNData(ProtocolHead *head, const char *p) {
	unsigned char *buf, msg[8], size;	
	
	buf = DataFalgQueryAndChange(5, 0, 1);
	while(*buf != 0){
		vTaskDelay(configTICK_RATE_HZ / 500);
	}
	
	DataFalgQueryAndChange(5, 1, 0);
	
	DataFalgQueryAndChange(2, 1, 0);
	sscanf(p, "%4s", msg);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
	__DataFlagJudge(p);

}

static void HandleGWloopControl(ProtocolHead *head, const char *p) {
	unsigned char tmp[5] = {0}, a, b, TurnFlag = 0;
	unsigned char *buf, size;
	int len;
	
//	memset(tmp, 0, 3);
	if(p[0] == '0'){              /*强制开*/  
		TurnFlag = 1;
	}
	
	sscanf(p, "%*1s%2s", tmp);
	if((tmp[0] >= '0') && (tmp[0] <= '9')){
		b = (tmp[0] - '0') << 4;
	} else if ((tmp[0] >= 'A') && (tmp[0] <= 'F')){
		b = (tmp[0] - '7') << 4;
	} else {
		return;
	}
	
	if((tmp[1] >= '0') && (tmp[1] <= '9')){
		b |= (tmp[1] - '0');
	} else if ((tmp[1] >= 'A') && (tmp[1] <= 'F')){
		b |= (tmp[1] - '7');
	} else {
		return;
	}

	GPIO_SetBits(GPIO_CTRL_EN, PIN_CRTL_EN);
	
	if(TurnFlag == 1){
		if(b & 0x01){		
				GPIO_SetBits(GPIO_CTRL_1, PIN_CTRL_1);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_1, PIN_CTRL_1);
		}
		if(b & 0x02) {
				GPIO_SetBits(GPIO_CTRL_2, PIN_CTRL_2);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_2, PIN_CTRL_2);
		}
		if(b & 0x04) {
				GPIO_SetBits(GPIO_CTRL_3, PIN_CTRL_3);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_3, PIN_CTRL_3);
		}
		if(b & 0x08) {
				GPIO_SetBits(GPIO_CTRL_4, PIN_CTRL_4);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_4, PIN_CTRL_4);
		}
		if(b & 0x10) {
				GPIO_SetBits(GPIO_CTRL_5, PIN_CTRL_5);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_5, PIN_CTRL_5);
		}
		if(b & 0x20) {
				GPIO_SetBits(GPIO_CTRL_6, PIN_CTRL_6);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_6, PIN_CTRL_6);
		}
		if(b & 0x40) {
				GPIO_SetBits(GPIO_CTRL_7, PIN_CTRL_7);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_7, PIN_CTRL_7);
		}
		if(b & 0x80) {
				GPIO_SetBits(GPIO_CTRL_8, PIN_CTRL_8);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_8, PIN_CTRL_8);
		}
	}
	
	GPIO_ResetBits(GPIO_CTRL_EN, PIN_CRTL_EN);
	TurnFlag = 0;
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)tmp, &size);
  GsmTaskSendTcpData((const char *)buf, size);
 	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleGWDataQuery(ProtocolHead *head, const char *p) {     /*网关回路数据查询*/
	char *buf;
	
	buf = DataFalgQueryAndChange(5, 0, 1);
	while(*buf != 0){
		vTaskDelay(configTICK_RATE_HZ / 500);
	}
	
	DataFalgQueryAndChange(2, 4, 0);
	DataFalgQueryAndChange(6, p[0], 0);
 // ElecTaskSendData((const char *)(p - sizeof(ProtocolHead)), (sizeof(ProtocolHead) + strlen(p)));

	DataFalgQueryAndChange(5, 1, 0);
}

static void HandleGWTurnTimeQuery(ProtocolHead *head, const char *p) {
	int len;
	unsigned char *buf, msg[18], size;
	unsigned short tmp[1465];
	DateTime dateTime;
	
	if(p[0] != '1'){
		buf = ProtocolRespond(head->addr, head->contr, "0", &size);
		GsmTaskSendTcpData((const char *)buf, size);
		ProtocolDestroyMessage((const char *)buf);	
		return;
	}
	
	sscanf(p, "%*1s%4s", msg);
	dateTime.year = atoi((const char *)msg) - 2000;
	sscanf(p, "%*5s%2s", msg);
	dateTime.month =  atoi((const char *)msg);
	sscanf(p, "%*7s%2s", msg);
	dateTime.date = atoi((const char *)msg);
	
	len = __OffsetNumbOfDay(&dateTime);
	
	if(dateTime.year % 2){
		NorFlashRead(NORFLASH_ONOFFTIME1, (short *)tmp, len * 4);
	} else {
		NorFlashRead(NORFLASH_ONOFFTIME2, (short *)tmp, len * 4);
	}
	
	memset(msg, 0, 18);
	sprintf((char *)msg, "1%4x%4x%4x%4x", (unsigned short)tmp[len * 4 - 2], (unsigned short)tmp[len * 4 - 1], (unsigned short)tmp[len * 4 - 4], (unsigned short)tmp[len * 4 - 3]);
	
	for(len = 0; len < sizeof(msg); len++){
		if(msg[len] == 0x20){
			msg[len] = 0x30;
		}
	}
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleLightAuto(ProtocolHead *head, const char *p) {
	unsigned char *buf, size;
	
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleAdjustTime(ProtocolHead *head, const char *p) {    /*校时*/
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

static void HandleGWVersQuery(ProtocolHead *head, const char *p) {      /*查网关软件版本号*/
	unsigned char *buf, size;
	
	buf = ProtocolRespond(head->addr, head->contr, Version(), &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
}

static void HandleEGVersQuery(ProtocolHead *head, const char *p) {
	unsigned char *buf, size;
	
	buf = ProtocolRespond((unsigned char *)"9999999999", head->contr, NULL, &size);
	ElecTaskSendData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleGWUpgrade(ProtocolHead *head, const char *p) {             //FTP远程升级
	FirmwareUpdaterMark *mark;
	char size[6];
	int len, type;
	
	sscanf(p, "%2s", size);
	type = strtol((const char *)size, NULL, 16);
	
	sscanf(&p[2], "%6s", size);
	len = atoi(size);
	mark = pvPortMalloc(sizeof(*mark));
	if (mark == NULL) {
		return;
	}
	
	if (FirmwareUpdateSetMark(mark, len, type)) {
		NVIC_SystemReset();
	}
	vPortFree(mark);
}


static void HandleEGUpgrade(ProtocolHead *head, const char *p) {
	
}

#define BALLAST_UPGRADE_AREA    (0x0805B000)     //  300K + 64K 

static char Ballast_Upgrade_Flag[320] = {0};
static char Ballast_Count = 0;

static void HandleBSNUpgrade(ProtocolHead *head, const char *p) {
	char msg[10];
	int Total, Segment, i;
	unsigned char size, *buf;
	
	sscanf(p, "%4s", msg);
	Total = strtol(msg, NULL, 16);
	
	if(Total > 320){
		return;
	}
	
	sscanf(p, "%*4s%4s", msg);
	Segment = strtol(msg, NULL, 16);
	
	if(Segment > Total){
		return;
	}
	
	Ballast_Upgrade_Flag[Segment - 1] = 1;
	
	STMFLASH_Write((BALLAST_UPGRADE_AREA + (Segment - 1) * 200), (uint16_t *)p, (uint16_t)head->lenth);
	
	sscanf(p, "%8s", msg);
	msg[8] = '0';
	msg[9] = 0;
	
	buf = ProtocolRespond(head->addr, head->contr, msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	
	for(i = 0; i < Total; i++){
		if(Ballast_Upgrade_Flag[i] == 1){
			Ballast_Count++;
		}
	}
	
	if(Ballast_Count == Total){
		DataFalgQueryAndChange(2, 9, 0);
	}
	
}

static void HandleLuxGather(ProtocolHead *head, const char *p) {
	
	
}

static void HandleRestart(ProtocolHead *head, const char *p){            /*设备复位*/
	unsigned char *buf, msg[11], size;
	
	if((p[0] == '1')){
		NVIC_SystemReset();
	} else if(p[0] == '3'){
		sscanf(p, "%10s", msg);
		buf = ProtocolRespond("9999999999", head->contr, (const char *)msg, &size);
		ElecTaskSendData((const char *)buf, size);
		ProtocolDestroyMessage((const char *)buf);	
	}
}

typedef void (*ProtocolHandleFunction)(ProtocolHead *head, const char *p);
typedef struct {
	unsigned char type;
	ProtocolHandleFunction func;
} ProtocolHandleMap;


/*GW: gateway  网关*/
/*EG: electric quantity gather 电量采集器*/
/* illuminance ： 光照度*/
/*BSN: 钠灯镇流器*/
void ProtocolHandler(ProtocolHead *head, char *p) {
	int i;
	char tmp[3], mold, buf[20];
	char *ret;
	char verify = 0;
	unsigned char len;
	
	const static ProtocolHandleMap map[] = {  
		{GATEPARAM,      HandleGatewayParam},     /*0x01; 网关参数下载*/           ///
		{LIGHTPARAM,     HandleLightParam},       /*0x02; 灯参数下载*/             /// 
		{DIMMING,        HandleLightDimmer},      /*0x04; 灯调光控制*/
		{LAMPSWITCH,     HandleLightOnOff},       /*0x05; 灯开关控制*/
		{READDATA,       HandleReadBSNData},      /*0x06; 读镇流器数据*/
		{LOOPCONTROL,    HandleGWloopControl},    /*0x07; 网关回路控制*/           ///
		{DATAQUERY,      HandleGWDataQuery},      /*0x08; 网关数据查询*/           
		{TIMEADJUST,     HandleAdjustTime},       /*0x0B; 校时*/                   ///          
		{VERSIONQUERY,   HandleGWVersQuery},      /*0x0C; 查网关软件版本号*/       ///
		{GATEUPGRADE,    HandleGWUpgrade},        /*0x37; 网关远程升级*/
		{LUXVALUE,       HandleLuxGather},        /*0x38; 接收到光照度强度值*/
		{RESTART,        HandleRestart},          /*0x3F; 设备复位*/               ///  
	};

	ret = p;
	
	for (i = 0; i < (strlen(p) - 3); ++i) {
		verify ^= *ret++;
	}
	
	sscanf(p, "%*13s%2s", tmp);
	len = strtol((const char *)tmp, NULL, 16);  
	
	sprintf(buf, "%%*%ds%%2s", 15 + len);
	
	sscanf(p, buf, tmp);
	len = strtol((const char *)tmp, NULL, 16);  
	
	if(verify != len)
		return;
	
	sscanf(p, "%*11s%2s", tmp);
	if(tmp[1] > '9'){
		mold = ((tmp[0] & 0x0f) << 4) | (tmp[1] - 'A' + 10);
	} else {
		mold = ((tmp[0] & 0x0f) << 4) | (tmp[1] & 0x0f);
	}
	
	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
			if (map[i].type == mold) {
				map[i].func(head, p + 15);
				break;
			}
	}
}

