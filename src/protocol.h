#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "gsm.h"

#define PIN_CRTL_EN   GPIO_Pin_0
#define GPIO_CTRL_EN  GPIOC

#define PIN_CTRL_1    GPIO_Pin_4
#define GPIO_CTRL_1   GPIOA

#define PIN_CTRL_2    GPIO_Pin_5
#define GPIO_CTRL_2   GPIOA

#define PIN_CTRL_3    GPIO_Pin_2
#define GPIO_CTRL_3   GPIOC

#define PIN_CTRL_4    GPIO_Pin_3
#define GPIO_CTRL_4   GPIOC

#define PIN_CTRL_5    GPIO_Pin_6
#define GPIO_CTRL_5   GPIOA

#define PIN_CTRL_6    GPIO_Pin_7
#define GPIO_CTRL_6   GPIOA

#define PIN_CTRL_7    GPIO_Pin_4
#define GPIO_CTRL_7   GPIOC

#define PIN_CTRL_8    GPIO_Pin_1
#define GPIO_CTRL_8   GPIOB

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
	char Success[7];
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
