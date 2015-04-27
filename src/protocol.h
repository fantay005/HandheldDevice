#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "gsm.h"

typedef struct {
	unsigned char header;
	unsigned char addr[10];
	unsigned char contr[2];
	unsigned char lenth[2];
} ProtocolHead;

typedef struct{
	unsigned char GatewayID[6];          /*������ݱ�ʶ*/
	unsigned char Longitude[10];         /*����*/
	unsigned char Latitude[10];          /*γ��*/
	unsigned char FrequPoint;            /*ZIGBEEƵ��*/
	unsigned char IntervalTime[2];       /*�Զ��ϴ�����ʱ����*/
	unsigned char TransfRatio[2];        /*����������*/
}GatewayParam1;                        /*���ز�������֡1*/

typedef struct{
	unsigned char OpenOffsetTime1[2];    /*����ƫ��ʱ��1*/
	unsigned char OpenOffsetTime2[2];    /*����ƫ��ʱ��1*/
	unsigned char CloseOffsetTime1[2];   /*�ص�ƫ��ʱ��1*/
	unsigned char CloseOffsetTime2[2];   /*�ص�ƫ��ʱ��2*/
}GatewayParam2;                        /*���ز�������֡1*/ 

typedef struct{
	unsigned char HVolLimitVal[12];      /*�ܻ�·L1/L2/L3�ߵ�ѹ�޶�ֵ*/
	unsigned char LVolLimitVal[12];      /*�ܻ�·L1/L2/L3�͵�ѹ�޶�ֵ*/ 
	unsigned char NoloadCurLimitVal[16]; /*�ܻ�·L1/L2/L3/N����ص����޶�ֵ*/
	unsigned char PhaseCurLimitVal[16];  /*�ܻ�·A/B/C/N������޶�ֵ*/
	unsigned char NumbOfCNBL;            /*��������������*/
	unsigned char OtherWarn[2];          /*��������*/ 
}GatewayParam3;

void ProtocolHandler(ProtocolHead *head, char *p);

#endif
