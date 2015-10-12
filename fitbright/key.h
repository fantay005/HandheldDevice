#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x_gpio.h"

#define  KEY_IO_0     GPIOB     
#define  KEY_Pin_0    GPIO_Pin_8 

#define  KEY_IO_1     GPIOB     
#define  KEY_Pin_1    GPIO_Pin_0 

#define  KEY_IO_2     GPIOC     
#define  KEY_Pin_2    GPIO_Pin_1  

#define  KEY_IO_3     GPIOC   
#define  KEY_Pin_3    GPIO_Pin_10 

#define  KEY_IO_4     GPIOB    
#define  KEY_Pin_4    GPIO_Pin_1

#define  KEY_IO_5     GPIOB   
#define  KEY_Pin_5    GPIO_Pin_2  

#define  KEY_IO_6     GPIOB  
#define  KEY_Pin_6    GPIO_Pin_3

#define  KEY_IO_7     GPIOB     
#define  KEY_Pin_7    GPIO_Pin_4 

#define  KEY_IO_8     GPIOB   
#define  KEY_Pin_8    GPIO_Pin_5   

#define  KEY_IO_9     GPIOB   
#define  KEY_Pin_9    GPIO_Pin_6 

#define  KEY_IO_Lf    GPIOC  
#define  KEY_Pin_Lf   GPIO_Pin_2 

#define  KEY_IO_Rt    GPIOC   
#define  KEY_Pin_Rt   GPIO_Pin_3 

#define  KEY_IO_Up    GPIOA  
#define  KEY_Pin_Up   GPIO_Pin_5 

#define  KEY_IO_Dn    GPIOC  
#define  KEY_Pin_Dn   GPIO_Pin_0 

typedef enum{
	KEY0,
	KEY1,
	KEY2,
	KEY3,
	KEY4,
	KEY5,
	KEY6,
	KEY7,
	KEY8,
	KEY9,
	KEYA,
	KEYB,
	KEYC,
	KEYD,
	KEYE,
	KEYF,
	KEYL,
	KEYUP,
	KEYDN,
	KEYLF,
	KEYRT,
	KEYOK,
	KEYMENU,
	KEYINPT,
	KEYDEL,
	KEYCLEAR,
	NOKEY,
}KeyPress;

typedef enum{
	Open_GUI,        //��������
	Main_GUI,        //������	
	Config_GUI,      //���ý���
	Service_GUI,     //ά�޽���	
	Test_GUI,        //���Խ���
	Close_GUI,       //�ػ�����
	
	GateWay_Set,     //����ģʽ�£�����ѡ��
	Address_Set,     //����ģʽ�£�����ZigBee��ַ
	Config_Set,      //����ģʽ�£��߼����ã����������в���
	Config_DIS,      //����ģʽ�£�������ʾ
	
	GateWay_Choose,  //ά��ģʽ�£�����ѡ��
	Address_Choose,  //ά��ģʽ�£���ַѡ��
	Read_Data,       //ά��ģʽ�£���ȡ����������
	Ballast_Operate, //ά��ģʽ�£�����������  /*�������ơ��ص�*/
	Diagn_Reason,    //ά��ԭ��    /*�������������⡢�ƹ����⡢�������⡢ZigBeeģ�����⡢��Դ���⡢�Ӵ��������⡢ZigBee��ַ����*/
	
	GateWay_Decide,  //����ģʽ�£�����ѡ��
	Address_Option,  //����ģʽ�£�ZigBee��ַѡ��
	Debug_Option,    //����ģʽ�£����������� /*�������ơ��صơ����⣬������������*/
	
}Dis_Type;


void key_init(void);
void KEY(void);

#endif /* __KEY_H */
