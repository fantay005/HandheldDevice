#include "key.h"
#include "stm32f10x_gpio.h"

#define GPIO_KEY_0     GPIOA
#define Pin_Key_0      GPIO_Pin_4

#define GPIO_KEY_1     GPIOA
#define Pin_Key_1      GPIO_Pin_1

#define GPIO_KEY_2     GPIOA
#define Pin_Key_2      GPIO_Pin_0

#define GPIO_KEY_3     GPIOC
#define Pin_Key_3      GPIO_Pin_3

#define GPIO_KEY_4     GPIOC
#define Pin_Key_4      GPIO_Pin_2

#define GPIO_KEY_5     GPIOC
#define Pin_Key_5      GPIO_Pin_1

#define GPIO_KEY_6     GPIOA
#define Pin_Key_6      GPIO_Pin_5

#define GPIO_KEY_7     GPIOA
#define Pin_Key_7      GPIO_Pin_6

#define GPIO_KEY_8     GPIOA
#define Pin_Key_8      GPIO_Pin_7

#define GPIO_KEY_9     GPIOC
#define Pin_Key_9      GPIO_Pin_4

#define GPIO_KEY_UP    GPIOC
#define Pin_Key_UP     GPIO_Pin_5            //���ϼ�

#define GPIO_KEY_DOWN  GPIOB
#define Pin_Key_DOWN   GPIO_Pin_0            //���¼�

#define GPIO_KEY_LF    GPIOB
#define Pin_Key_LF     GPIO_Pin_1            //�����

#define GPIO_KEY_RT    GPIOB
#define Pin_Key_RT     GPIO_Pin_2            //���Ҽ�

#define GPIO_KEY_MODE  GPIOB
#define Pin_Key_MODE   GPIO_Pin_13           //ģʽ��

#define GPIO_KEY_MENU  GPIOB
#define Pin_Key_MENU   GPIO_Pin_14           //���ܼ�

#define GPIO_KEY_INPT  GPIOB
#define Pin_Key_INPT   GPIO_Pin_15           //�����л��� 

#define GPIO_KEY_DEL   GPIOF                         
#define Pin_Key_DEL    GPIO_Pin_7            //ɾ����

#define GPIO_KEY_OK     GPIOF
#define Pin_Key_OK      GPIO_Pin_6           //ȷ����

/*����Ķ�������ԶŹ������嶨��*/

#define KEY_1        GPIO_ReadInputDataBit(GPIO_KEY_0,Pin_Key_0) 
#define KEY_4        GPIO_ReadInputDataBit(GPIO_KEY_1,Pin_Key_1)
#define KEY_5        GPIO_ReadInputDataBit(GPIO_KEY_2,Pin_Key_2)
#define KEY_6        GPIO_ReadInputDataBit(GPIO_KEY_3,Pin_Key_3)
#define KEY_7        GPIO_ReadInputDataBit(GPIO_KEY_4,Pin_Key_4)
#define KEY_8        GPIO_ReadInputDataBit(GPIO_KEY_5,Pin_Key_5)
#define KEY_9        GPIO_ReadInputDataBit(GPIO_KEY_6,Pin_Key_6)
#define KEY_RET      GPIO_ReadInputDataBit(GPIO_KEY_7,Pin_Key_7)
#define KEY_0        GPIO_ReadInputDataBit(GPIO_KEY_8,Pin_Key_8)
#define KEY_OK       GPIO_ReadInputDataBit(GPIO_KEY_9,Pin_Key_9)
#define KEY_DN       GPIO_ReadInputDataBit(GPIO_KEY_UP,Pin_Key_UP)
#define KEY_2        GPIO_ReadInputDataBit(GPIO_KEY_DOWN,Pin_Key_DOWN)
#define KEY_LF       GPIO_ReadInputDataBit(GPIO_KEY_LF,Pin_Key_LF)
#define KEY_RT       GPIO_ReadInputDataBit(GPIO_KEY_RT,Pin_Key_RT)
#define KEY_MENU     GPIO_ReadInputDataBit(GPIO_KEY_MENU,Pin_Key_MENU)
#define KEY_INPUT    GPIO_ReadInputDataBit(GPIO_KEY_DEL,Pin_Key_DEL)
#define KEY_3        GPIO_ReadInputDataBit(GPIO_KEY_INPUT,Pin_Key_INPUT)
#define KEY_UP       GPIO_ReadInputDataBit(GPIO_KEY_MODE,Pin_Key_MODE)
#define KEY_DEL      GPIO_ReadInputDataBit(GPIO_KEY_OK,Pin_Key_OK)

/*���¶԰������ж���*/

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


void key_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure; 

	GPIO_InitStructure.GPIO_Pin = Pin_Key_0;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_0, &GPIO_InitStructure);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//PB3������ͨIO
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_1;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_1, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_2;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_2, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_3;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_3, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_4;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_4, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_5;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_5, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_6;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_6, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_7;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_7, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_8;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_8, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_9;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_9, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_UP;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_UP, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_DOWN;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_DOWN, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_MODE;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_MODE, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_MENU;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_MENU, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_DEL;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_DEL, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_OK;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_OK, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_LF;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_LF, &GPIO_InitStructure);
		
	GPIO_InitStructure.GPIO_Pin = Pin_Key_RT;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_RT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_INPT;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_INPT, &GPIO_InitStructure);
}

/*
static void Key_Handle(void){
	Dis_Type Status;

	switch(Status){
		case Open_GUI: 
			
			break;
		case Main_GUI:
			
			break;
		case Config_GUI:
			
			break;
		case Service_GUI:
			break;
		case Test_GUI:
			break;
		case Close_GUI:
			break;
		case GateWay_Set:
			break;
		case Address_Set
		
	}
	
}
*/



