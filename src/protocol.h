#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

typedef enum{
	DIMMING = 0x04,         /*����*/
	LAMPSWITCH = 0x05,      /*���ص�*/
	READDATA = 0x06,        /*������������*/
	RETAIN,                 /*����*/
} GatewayType;

#endif
