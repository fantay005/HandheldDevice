#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "key.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "zklib.h"
#include "ili9320.h"
#include "ConfigZigbee.h"
#include "sdcard.h"
#include "math.h"

#define KEY_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 512)

static xQueueHandle __KeyQueue;

#define KEY_STATE_0    0       //��ʼ״̬
#define KEY_STATE_1    1       //����ȷ��״̬
#define KEY_STATE_2    2       //�ȴ������ͷ�״̬

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

#define KEY_1        GPIO_ReadInputDataBit(GPIO_KEY_6,Pin_Key_6) 
#define KEY_4        GPIO_ReadInputDataBit(GPIO_KEY_7,Pin_Key_7)
#define KEY_5        GPIO_ReadInputDataBit(GPIO_KEY_8,Pin_Key_8)
#define KEY_6        GPIO_ReadInputDataBit(GPIO_KEY_9,Pin_Key_9)
#define KEY_7        GPIO_ReadInputDataBit(GPIO_KEY_UP,Pin_Key_UP)
#define KEY_8        GPIO_ReadInputDataBit(GPIO_KEY_DOWN,Pin_Key_DOWN)
#define KEY_9        GPIO_ReadInputDataBit(GPIO_KEY_0,Pin_Key_0)
#define KEY_INPUT    GPIO_ReadInputDataBit(GPIO_KEY_1,Pin_Key_1)
#define KEY_0        GPIO_ReadInputDataBit(GPIO_KEY_2,Pin_Key_2)
#define KEY_CONF     GPIO_ReadInputDataBit(GPIO_KEY_3,Pin_Key_3)
#define KEY_DN       GPIO_ReadInputDataBit(GPIO_KEY_4,Pin_Key_4)
#define KEY_2        GPIO_ReadInputDataBit(GPIO_KEY_5,Pin_Key_5)
#define KEY_LF       GPIO_ReadInputDataBit(GPIO_KEY_DEL,Pin_Key_DEL)
#define KEY_RT       GPIO_ReadInputDataBit(GPIO_KEY_OK,Pin_Key_OK)
#define KEY_MENU     GPIO_ReadInputDataBit(GPIO_KEY_MENU,Pin_Key_MENU)
#define KEY_OK       GPIO_ReadInputDataBit(GPIO_KEY_LF,Pin_Key_LF)
#define KEY_3        GPIO_ReadInputDataBit(GPIO_KEY_INPT,Pin_Key_INPT)
#define KEY_UP       GPIO_ReadInputDataBit(GPIO_KEY_MODE,Pin_Key_MODE)
#define KEY_RET      GPIO_ReadInputDataBit(GPIO_KEY_RT,Pin_Key_RT)

/*���¶԰������ж���*/


typedef enum{
	KEY_SEND_DATA,
	KEY_NULL,
}KeyTaskMsgType;

typedef struct {
	/// Message type.
	KeyTaskMsgType type;
	/// Message lenght.
	unsigned char length;
} KeyTaskMsg;

static KeyTaskMsg *__KeyCreateMessage(KeyTaskMsgType type, const char *dat, unsigned char len) {
  KeyTaskMsg *message = pvPortMalloc(ALIGNED_SIZEOF(KeyTaskMsg) + len);
	if (message != NULL) {
		message->type = type;
		message->length = len;
		memcpy(&message[1], dat, len);
	}
	return message;
}

static inline void *__KeyGetMsgData(KeyTaskMsg *message) {
	return &message[1];
}

void key_gpio_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure; 

	GPIO_InitStructure.GPIO_Pin = Pin_Key_0;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_0, &GPIO_InitStructure);
	
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//PB3������ͨIO
	
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

void TIM3_Init(void){
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
 
	TIM_TimeBaseStructure.TIM_Period = 500; 
	TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1; 
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 
 
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); 
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);  
 
	TIM_Cmd(TIM3, ENABLE);  
}

static char IME = 0;                    //��ĸ���������뷨�л�����Ϊ1ʱ��1~6���ΪA~F

KeyPress keycode(void){                 //����б仯�������豻ȷ��
	if(KEY_0 == 0){
		return KEY0;
	} else if(KEY_1 == 0){
		return KEY1;
	} else if(KEY_2 == 0){
		return KEY2;
	} else if(KEY_3 == 0){
		return KEY3;
	} else if(KEY_4 == 0){
		return KEY4;
	} else if(KEY_5 == 0){
		return KEY5;
	} else if(KEY_6 == 0){
		return KEY6;
	} else if(KEY_7 == 0){
		return KEY7;
	} else if(KEY_8 == 0){
		return KEY8;
	} else if(KEY_9 == 0){
		return KEY9;
	} else if(KEY_UP == 0){
		return KEYUP;
	} else if(KEY_DN == 0){
		return KEYDN;
	} else if(KEY_LF == 0){
		return KEYLF;
	} else if(KEY_RT == 0){
		return KEYRT;
	} else if(KEY_CONF == 0){
		return KEYCONF;
	} else if(KEY_MENU == 0){
		return KEYMENU;
	} else if(KEY_INPUT == 0){
		return KEYINPT;
	} else if(KEY_OK == 0){
		return KEYOK;
	} else if(KEY_RET == 0){
		return KEYRETURN;
	} else {
		return NOKEY;
	}
}

static KeyPress KeyConfirm = NOKEY;         //�ϵ�󰴼��ĳ�ʼ״̬
static Dis_Type  InterFace = Open_GUI;      //�ϵ����ʾ����ĳ�ʼ״̬
static Dis_Type  LastFace = Open_GUI;       //��һ����ʾ����

char StatusOfInterface(void){
	return InterFace;
}

void key_driver(KeyPress code)              //�����Ч����
{
	static uint32_t key_state = KEY_STATE_0;
	static KeyPress	code_last = NOKEY;
	

	switch (key_state)
	{
		case KEY_STATE_0: //��ʼ״̬
			if (code != NOKEY){
				code_last = code;
				key_state = KEY_STATE_1; //����м������£���¼��ֵ������������������ȷ��״̬
			}
			break;
		case KEY_STATE_1: //����ȷ��״̬
			if (code != code_last)
				key_state = KEY_STATE_2; //�ȴ������ͷ�״̬
			break;
		case KEY_STATE_2:
			if (code != code_last) {//�ȵ������ͷ� �����س�ʼ״̬
				KeyConfirm = code_last; //���ؼ�ֵ
				key_state = KEY_STATE_0;
			}
			break;
	}
}

static void InputChange(void){                   //��ʾ��ǰ���뷨
	char tmp[5];
	if(IME == 0)
		strcpy(tmp, "123");
	else
		strcpy(tmp, "ABC");
	
	if(!HexSwitchDec)
		strcpy(tmp, "   ");
	Ili9320TaskInputDis(tmp, strlen(tmp) + 1);
}

static char times = 1;             			//��ʱ���жϼ���
static char count = 0;            	 	  //�������ּ��ĸ���
static char dat[9];               	 	  //�������ּ����
static unsigned char wave = 1;    	 	  //����
static unsigned char page = 1;    	 	  //ҳ��
static unsigned char MaxPage = 1; 	 	  //���ҳ��
static unsigned char MaxLine = 1;     	//һҳ���������������ѡ��
static unsigned char OptDecide = 0; 		//ȷ��ѡ��
pro Project = Pro_Null;          //��ʼ����ĿΪ��
unsigned char FrequencyDot = 0;  //��ʼƵ��Ϊ��
unsigned int  ZigBAddr = 1;     	//��ʼZigBee��ֵַ
char Config_Enable = 0;          //���ü�ʹ������ģ�鹦�ܣ�1Ϊ���ã�2Ϊ

char Digits = 1;             //�������޸ĵ�ַ��λ��
char MaxBit = 4;             //���λ��
char BaseBit = 40;           //��ʼ���ֵ� ��ʼλ

static char lastData = 0;    //��һ���޸ĵ�ZigBee��ַ��ֵ


extern unsigned char NumOfPage(void);

unsigned char ProMaxPage(void){         //ȷ����ʾ���͵����ҳ��
	MaxPage = NumOfPage() / 15 + 1;
	return page;
}

extern unsigned char *GWname(void);      //��������

bool DisStatus(char type, char param){              //�ж�Dis_Type��������������ֵ�ڰ���ȷ�ϼ�����Ҫ����������
	char tmp[40];
	KeyTaskMsg *msg;
	
	sprintf((char *)tmp, "%s", GWname());
	switch (type){
		case 2:
			return true;
		case 3:
			return true;
		case 4:
			switch(param){
				case 1:
					return true;
				case 2:
					if(tmp[0] == 0)           //û��ѡ������״����
						return false;
					if(NumOfFrequ < 2)        //ֻ��һ��Ƶ��״����
						return false;
					return true;
				case 3:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 4:
					msg = __KeyCreateMessage(KEY_SEND_DATA, "SHUNCOM", 8);
					xQueueSend(__KeyQueue, &msg, 10);				
					return true;
				case 5:
					return true;
				default:
					return false;
				
			}
		case 5:
			switch(param){
				case 1:
					return true;
				case 2:
					if(tmp[0] == 0)           //û��ѡ������״����
						return false;
					if(NumOfFrequ < 2)   //ֻ��һ��Ƶ��״����
						return false;
					return true;
				case 3:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 4:
					if(ZigBAddr == 0)         //û��ѡ��ZigBee��ַ״����
						return false;
					return true;
				case 5:
					if(ZigBAddr == 0)         //û��ѡ��ZigBee��ַ״����
						return false;
					return true;
				case 6:
					if(ZigBAddr == 0)         //û��ѡ��ZigBee��ַ״����
						return false;
					return true;
				default:
					return false;
				
			}
		case 6:
			switch(param){
				case 1:
					return true;
				case 2:
					if(tmp[0] == 0)           //û��ѡ������״����
						return false;
					if(NumOfFrequ < 2)   //ֻ��һ��Ƶ��״����
						return false;
					return true;
				case 3:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 4:
					if(ZigBAddr == 0)         //û��ѡ��ZigBee��ַ״����
						return false;
					return true;
				case 5:
					if(ZigBAddr == 0)         //û��ѡ��ZigBee��ַ״����
						return false;
					return true;
				case 6:
					return true;
				default:
					return false;
				
			}
		case 7:
			return true;
		case 8:
			if(Project == Pro_Null)     //û��ѡ����Ŀ״����
				return false;
			return true;
		case 9:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 12:                     //û��ѡ����Ŀ״����
			if(Project == Pro_Null)
				return false;
			
			return true;
		case 13:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 14:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 15:
			if(Project == Pro_Null)    //û��ѡ����Ŀ״����
				return false;
			return true;
		case 16:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 17:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 18:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 19:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 20:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 21:
			if(tmp[0] == 0)
				return false;
			return true;
		case 22:
			if(tmp[0] == 0)
				return false;
			return true;
		default:
			return false;
	}	
}

char JudgeMaxNum(void){
	if(MaxPage > page)
		MaxLine = 15;
	else
		MaxLine = NumOfPage()%15;
	
	return MaxLine;
}



void ChooseLine(void){                         //ȷ����������ҳ��
	unsigned char tmp[2], i;
	
	JudgeMaxNum();
	
	if(KeyConfirm == KEYUP){
		wave--;
		if(wave < 1)
			wave = MaxLine;
	} else if(KeyConfirm == KEYDN){
		wave++;
		if(wave > MaxLine)
			wave = 1;
	} else if(KeyConfirm == KEYLF){	
		if((InterFace != GateWay_Set) && (InterFace != GateWay_Choose) && (InterFace != GateWay_Decide))
			return;
		if(page > 1){
			page--;
			wave = 1;
		} else {
			page = MaxPage;
			if(MaxPage != 1)
				wave = 1;
		}
		
		tmp[0] = InterFace;
		tmp[1] = KeyConfirm;
		SDTaskHandleKey((const char *)tmp, 2);
		
		return;
	} else if(KeyConfirm == KEYRT){
		if((InterFace != GateWay_Set) && (InterFace != GateWay_Choose) && (InterFace != GateWay_Decide))
			return;
		
		if(page < MaxPage){
			wave = 1;
			page++;
		} else {
			page = 1;
			if(MaxPage != 1)
				wave = 1;
		}
		
		tmp[0] = InterFace;
		tmp[1] = KeyConfirm;
		SDTaskHandleKey((const char *)tmp, 2);
		
		return;
	} else if(KeyConfirm == KEYOK){
		
		if(DisStatus(InterFace, wave)){
					
			tmp[0] = InterFace;
			tmp[1] = wave;
			SDTaskHandleKey((const char *)tmp, 2);             //���Ͱ�����������
			OptDecide = 1;
		}
						
		return;
	}
	
	tmp[0] = wave;
	tmp[1] = 0;
	i = InterFace;
	if((i != Intro_GUI) && (i != Config_Set) && (i != Config_DIS) && (i != Read_Data) && (i != Debug_Option) && (i != On_And_Off))
		Ili9320TaskLightLine((const char *)tmp, strlen((const char *)tmp));
}


	
void __handleAdvanceSet(void){                          //�߼����������£�����������
	portBASE_TYPE xHigherPriorityTaskWoken;
	char tmp[12] = {0};
	
	KeyTaskMsg *msg;
	char hex2char[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'L'};

	
	if((KeyConfirm >= KEY0) && (KeyConfirm <= KEYL)){		
		dat[count++] = hex2char[KeyConfirm - 1];
		dat[count] = 0;
		if(count > 6){
			count = 0;
			dat[0] = 0;
		}
	
		Ili9320TaskOrderDis(dat, strlen(dat) + 1);
		KeyConfirm = NOKEY;
		return;
	} else if(KeyConfirm == KEYOK){
		count = 0;
  } else if(KeyConfirm == KEYRT){
		strcpy(dat, "SHUNCOM ");
  } else if(KeyConfirm == KEYINPT){
		InputChange();
		KeyConfirm = NOKEY;
		return;
  } else if(KeyConfirm == KEYUP){
		strcpy(tmp, "UP");
		Ili9320TaskUpAndDown(tmp, strlen(tmp) + 1);
		KeyConfirm = NOKEY;
		return;
  } else if(KeyConfirm == KEYDN){
		strcpy(tmp, "DOWN");
		Ili9320TaskUpAndDown(tmp, strlen(tmp) + 1);
		KeyConfirm = NOKEY;
		return;
  } else if(KeyConfirm == KEYLF){
		if(count > 0){
			count--;
			dat[count] = 0;
		}
		Ili9320TaskOrderDis(dat, strlen(dat) + 1);
		KeyConfirm = NOKEY;
		return;
  } else {
		KeyConfirm = NOKEY;
		return;
	}
	
	msg = __KeyCreateMessage(KEY_SEND_DATA, dat, strlen(dat) + 1);
	KeyConfirm = NOKEY;
	
	Ili9320TaskOrderDis(dat, strlen(dat) + 1);
	if (pdTRUE == xQueueSendFromISR(__KeyQueue, &msg, &xHigherPriorityTaskWoken)) {
		if (xHigherPriorityTaskWoken) {
			portYIELD();
		}
	}
  memset(dat, 0, 9);
} 

void DisplayInformation(void){                       //��ʾѡ�����Ŀ�����أ�Ƶ���
	char buf[80], tmp[40], dat[10] = {0}, para[20] = {0};
	
	if(Project == Pro_Null)
		return;

  if((FrequencyDot == 1) && (NumOfFrequ == 1)){      //�ж�����Ƶ��
		sprintf(tmp, "/ΨһƵ��:%02X/����ID:%02X/", FrequPoint1, NetID1);
	} else if(FrequencyDot == 1){
		sprintf(tmp, "/��һƵ��:%02X/����ID:%02X/", FrequPoint1, NetID1);
	} else if(FrequencyDot == 2){
		sprintf(tmp, "/�ڶ�Ƶ��:%02X/����ID:%02X/", FrequPoint2, NetID2);
	} else if((FrequencyDot == 0) && (NumOfFrequ != 1)){
		tmp[0] = 0;
	} else {
		tmp[0] = 0;
	}
	
	if(tmp[0] != 0)
		if(HexSwitchDec)
			sprintf(dat, "��ַ:%04X", ZigBAddr);
		else
			sprintf(dat, "��ַ:%04d", ZigBAddr);
	
	if(Project == Pro_BinHu){
		sprintf(para, "%s", "�Ϸ�/����/");
	} else if(Project == Pro_ChanYeYuan){
		sprintf(para, "%s", "�Ϸ�/��ɽ��ҵ԰/");
	} else if(Project == Pro_DaMing){
		sprintf(para, "%s", "�Ϸ�/��������/");
	}
	
	sprintf(buf, "%s%s%s%s", para, GWname(), tmp, dat);
	Ili9320TaskDisGateWay(buf, strlen(buf) + 1);
}

void __AddrConfig(void){                     //����ZigBee��ַ������ʾ
	char buf[5];
	
	if(HexSwitchDec)
		sprintf(buf, "\r\n%04X", ZigBAddr);
	else
		sprintf(buf, "\r\n%04d", ZigBAddr);
	Ili9320TaskClear("C", 1);
	Ili9320TaskOrderDis(buf, strlen(buf) + 1);
	
	buf[0] = Digits + BaseBit;
	buf[1] = 0;
	Ili9320TaskLightByte(buf, strlen(buf) + 1);
}

void __handleOpenOption(void){                 //��ֵ����TFT��ʾ
	unsigned char dat;
	unsigned char tmp[2];
	
  ChooseLine();	
	
	if(OptDecide == 0)                //����Ƿ�����Ҫ�����ȷ�ϼ����£�Ȼ��ı���ʾ����ֵ
		return;
	
	OptDecide = 0;                    //ȡ��ȷ��
	
	dat = wave;

	if(InterFace == Main_GUI){                   //����ʾҳ��Ϊ���˵�ʱ����ֵ�����ı�ҳ��
		if(dat == 1){
			InterFace = Project_Dir;
			
		} else if(dat == 2){
			InterFace = Config_GUI;
			
		} else if(dat == 3){
			InterFace = Service_GUI;
			
		} else if(dat == 4){
			InterFace = Test_GUI;
			
		} else if(dat == 5){
			InterFace = Intro_GUI;
			
		}
		
	} else if (InterFace == Config_GUI){         //����ʾҳ��Ϊ���ý���ʱ����ֵ�����ı�ҳ��
		
		if(dat == 1){
			InterFace = GateWay_Set;
			
		} else if(dat == 2){
			if(NumOfFrequ > 1){
				InterFace = Frequ_Set;
			}else
				return;
			
		} else if(dat == 3){
			InterFace = Address_Set;
			__AddrConfig();
		} else if(dat == 4){
			InterFace = Config_Set;
		
		} else if(dat == 5){
			InterFace = Config_DIS;
		
		}
		
	} else if (InterFace == Service_GUI){        //����ʾҳ��Ϊά�޽���ʱ����ֵ�����ı�ҳ��

		if(dat == 1){
			InterFace = GateWay_Choose;
			
		} else if(dat == 3){
			InterFace = Address_Choose;
			
		} else if(dat == 2){
			if(NumOfFrequ > 1){
				InterFace = Frequ_Choose;
			} else
				return;
			
		} else if(dat == 4){
			InterFace = Read_Data;
			
		}
		
	} else if (InterFace == Test_GUI){           //����ʾҳ��Ϊ���Խ���ʱ����ֵ�����ı�ҳ��
		
		if(dat == 1){
			InterFace = GateWay_Decide;
			
		} else if(dat == 3){
			InterFace = Address_Option;
			
		} else if(dat == 2){
			if(NumOfFrequ > 1){
				InterFace = Frequ_Option;
				
			} else
				return;
			
		} else if(dat == 4){
			InterFace = Debug_Option;
			
		} else if(dat == 5){
			InterFace = Light_Dim;
			
		} else if(dat == 6){
			InterFace = On_And_Off;
			
		}
		
	} else if (InterFace == Project_Dir){            //����ʾҳ��Ϊ��Ŀѡ�����ʱ����ֵ�����ı�ҳ��
		
		if(dat == 1){
			Project = Pro_BinHu;
			
		} else if(dat == 2){
			Project = Pro_ChanYeYuan;
			
		} else if(dat == 3){
			Project = Pro_DaMing;
			
		}
	
		tmp[0] = Open_GUI;                              /*������һ������ʾ���˵�*/
	  tmp[1] = KEYMENU;	
		SDTaskHandleKey((const char *)tmp, 2);
		InterFace = Main_GUI;
	}  else if (InterFace == GateWay_Set){            //����ʾҳ��Ϊ����ѡ�����ʱ����ֵ�����ı�ҳ��   

		tmp[0] = InterFace;
		tmp[1] = wave;
		SDTaskHandleWGOption((const char *)tmp, 2);	
		
		tmp[0] = Main_GUI;                              /*������һ������ʾ���ý���*/
	  tmp[1] = 2;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Config_GUI;
		
	} else if(InterFace == GateWay_Choose) {           //����ʾҳ��Ϊ����ѡ�����ʱ����ֵ�����ı�ҳ��   
		
		tmp[0] = InterFace;
		tmp[1] = wave;
		SDTaskHandleWGOption((const char *)tmp, 2);	
	
		tmp[0] = Main_GUI;                               /*������һ������ʾά�޽���*/
	  tmp[1] = 3;
	  SDTaskHandleKey((const char *)tmp, 2);

		InterFace = Service_GUI;
		
	} else if(InterFace == GateWay_Decide) {           //����ʾҳ��Ϊ����ѡ�����ʱ����ֵ�����ı�ҳ��   

		tmp[0] = InterFace;
		tmp[1] = wave;
		SDTaskHandleWGOption((const char *)tmp, 2);	
		
		tmp[0] = Main_GUI;                                /*������һ������ʾ���Խ���*/
	  tmp[1] = 4;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Test_GUI;
	} else if(InterFace == Frequ_Set) {                 //����ʾҳ��ΪƵ��ѡ�����ʱ����ֵ����

		
		tmp[0] = Main_GUI;                                /*������һ������ʾ���ý���*/
	  tmp[1] = 2;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Config_GUI;
		
	} else if(InterFace == Frequ_Choose) {              //����ʾҳ��ΪƵ��ѡ�����ʱ����ֵ����

		tmp[0] = Main_GUI;															  /*������һ������ʾά�޽���*/
	  tmp[1] = 3;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Service_GUI;
		
	} else if(InterFace == Frequ_Option) {              //����ʾҳ��ΪƵ��ѡ�����ʱ����ֵ����
			
		tmp[0] = Main_GUI;														    /*������һ������ʾ���Խ���*/
	  tmp[1] = 4;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Test_GUI;
	} 
		
	wave = 1;                                            //ҳ���л���������һ��
	KeyConfirm = NOKEY;
}

void __DisplayWGInformation(void){
	if(times%20 == 0){
		DisplayInformation();
	}
}

void __handleSwitchInput(void){                       //1~7���ڡ�1~7���롰A~L���������л�
	if(KeyConfirm == KEYINPT){
		if(IME == 0)
			IME = 1;
		else
			IME = 0;
	}
	
	if(times%10 == 0){
		InputChange();
	}
		
	if(IME == 1){
		if(KeyConfirm == KEY1){
			KeyConfirm = KEYA;
		} else if (KeyConfirm == KEY2){
			KeyConfirm = KEYB;
		} else if (KeyConfirm == KEY3){
			KeyConfirm = KEYC;
		} else if (KeyConfirm == KEY4){
			KeyConfirm = KEYD;
		} else if (KeyConfirm == KEY5){
			KeyConfirm = KEYE;
		} else if (KeyConfirm == KEY6){
			KeyConfirm = KEYF;
		}	else if (KeyConfirm == KEY7){
			if(InterFace == Config_Set)
				KeyConfirm = KEYL;
			else 
				KeyConfirm = KEY7;
		}
	}	
}

static char U3IRQ_Enable = 0;             //ʹ�ܴ���3��������

char Com3IsOK(void){
	if(InterFace == Address_Set)
		U3IRQ_Enable = 1;
	else if(InterFace == Config_Set)
		U3IRQ_Enable = 2;
	else
		U3IRQ_Enable = 0;
	return U3IRQ_Enable;
}   

bool HexSwitchDec = 0;                               //16������10�����л��� 0λ10���ƣ�1Ϊ16����

void __handleAddrValue(void){	                            //����ZigBee��ַ����
	char buf[8];
	
	if(KeyConfirm == KEYLF){
		Digits--;
		if(Digits < 1)
			Digits = MaxBit;
	} else if(KeyConfirm == KEYRT){
		Digits++;
		if(Digits > MaxBit)
			Digits = 1;
	} else if(KeyConfirm == KEYUP){
		ZigBAddr--;
		if(ZigBAddr < 1)
			ZigBAddr = 1;
	} else if(KeyConfirm == KEYDN){
		if(HexSwitchDec){
			if((ZigBAddr & 0xFFF) == 0xFFF)
				ZigBAddr = ZigBAddr & 0xF001;		
		} else{
			if((ZigBAddr % 1000) == 999)
				ZigBAddr = ZigBAddr / 1000 * 1000 + 1;
		}
		ZigBAddr++;
	} else if(KeyConfirm == KEYOK){
		Config_Enable = 1;
		ConfigTaskSendData("1", 2);
	} else if(KeyConfirm == KEYCONF){
		if(HexSwitchDec)
			HexSwitchDec = 0;
		else 
			HexSwitchDec = 1;
	}
	
	if((KeyConfirm >= KEY0) && (KeyConfirm <= KEYF)){		
		if(HexSwitchDec){
			ZigBAddr &= ~(0xF << ((4 - Digits) * 4));                //����Ҫ�ı��λ����0
			ZigBAddr |= ((KeyConfirm - 1) << ((4 - Digits) * 4));    //�޸ĵ�ֵַΪ����ļ�ֵ
		} else{	
			ZigBAddr = ZigBAddr / (int)pow(10, (5 - Digits)) * (int)pow(10, (5 - Digits)) + ZigBAddr % (int)pow(10, (4 - Digits)) + (KeyConfirm - 1) * pow(10, (4 - Digits));				
		}
		
		Digits++;
			if(Digits > MaxBit)
				Digits = 1;
	}
	
	if(HexSwitchDec){
		if(!(ZigBAddr & 0x0FFF))
			ZigBAddr |= 0x0001;
		sprintf(buf, "%04X", ZigBAddr);
	} else {
		if(!(ZigBAddr % 1000))
			ZigBAddr += 1;
		sprintf(buf, "%04d", ZigBAddr);
	}
	buf[4] = 0;
	
	Line = 2;
	
	if(lastData != ZigBAddr){
		Ili9320TaskClear("1", 1);
		Ili9320TaskOrderDis(buf, strlen(buf) + 1);
	}
	
	buf[0] = Digits + BaseBit;
	buf[1] = 0;
	Ili9320TaskLightByte(buf, strlen(buf) + 1);
	
}

void TIM3_IRQHandler(void){	
	unsigned char dat[2];
	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); 
		times++;
		if(times > 100)
			times = 1;
	}
	
	key_driver(keycode()); 
	
	if((InterFace != GateWay_Set) && (InterFace != GateWay_Choose) && (InterFace != GateWay_Decide) && (InterFace != Config_Set) && (InterFace != Intro_GUI))
		__DisplayWGInformation();
	
	if((InterFace == Config_Set) || (InterFace == Address_Set))
		__handleSwitchInput();
	
	if(KeyConfirm == NOKEY)
		return;
	
	if(InterFace != Address_Set)
		Config_Enable = 0;
	
	if(KeyConfirm == KEYMENU){
		
	  dat[0] = Open_GUI;
	  dat[1] = KEYMENU;
	  SDTaskHandleKey((const char *)dat, 2);
	  InterFace = Main_GUI;
		KeyConfirm = NOKEY;
	  return;
  } else if(KeyConfirm == KEYRETURN){
		if((InterFace == Project_Dir) || (InterFace == Config_GUI) || (InterFace == Service_GUI) || (InterFace == Test_GUI) || (InterFace == Intro_GUI)){
			InterFace = Main_GUI;
			
	  	dat[0] = Open_GUI;
	    dat[1] = KEYMENU;
	    SDTaskHandleKey((const char *)dat, 2);
		
		} else if(InterFace == GateWay_Set || InterFace == Address_Set || InterFace == Config_Set || InterFace == Config_DIS || InterFace == Frequ_Set){
			InterFace = Config_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 2;
	    SDTaskHandleKey((const char *)dat, 2);
		} else if(InterFace == GateWay_Choose || InterFace == Address_Choose || InterFace == Read_Data || InterFace == Frequ_Choose){
			InterFace = Service_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 3;
	    SDTaskHandleKey((const char *)dat, 2);
		} else if(InterFace == GateWay_Decide || InterFace == Address_Option || InterFace == Debug_Option || InterFace == Light_Dim || InterFace == On_And_Off || InterFace == Frequ_Option){
			InterFace = Test_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 4;
	    SDTaskHandleKey((const char *)dat, 2);
			
		} 
		
		wave = 1;
		page = 1;
		KeyConfirm = NOKEY;
	}
	
	if(LastFace != InterFace){
		LastFace = InterFace;
		page = 1;
	}
	
	ProMaxPage();
	
	if(InterFace == Main_GUI){

		__handleOpenOption();	
	} else if(InterFace == Config_GUI){

		__handleOpenOption();
	} else if(InterFace == Service_GUI){

		__handleOpenOption();
	} else if(InterFace == Test_GUI){

		__handleOpenOption();
	} else if(InterFace == Intro_GUI){

		__handleOpenOption();
	} else if(InterFace == Project_Dir){

		__handleOpenOption();
	} else if(InterFace == Light_Dim){

		__handleOpenOption();
	} else if(InterFace == Config_Set){
		
		__handleAdvanceSet();		
	} else if(InterFace == GateWay_Set){
		__handleOpenOption();
	} else if(InterFace == GateWay_Choose){
		__handleOpenOption();
	} else if(InterFace == GateWay_Decide){
		__handleOpenOption();
	} else if(InterFace == Frequ_Set){

		__handleOpenOption();
	} else if(InterFace == Frequ_Choose){

		__handleOpenOption();
	} else if(InterFace == Frequ_Option){

		__handleOpenOption();
	} else if(InterFace == Address_Set){

		__handleAddrValue();
	}
	
	KeyConfirm = NOKEY;
}

void __HandleConfigKey(KeyTaskMsg *dat){
	char *p =__KeyGetMsgData(dat);
	ConfigTaskSendData(p, strlen(p));
}

typedef struct {
	KeyTaskMsgType type;
	void (*handlerFunc)(KeyTaskMsg *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ KEY_SEND_DATA, __HandleConfigKey },
	{ KEY_NULL, NULL },
};

static void __KeyTask(void *parameter) {
	portBASE_TYPE rc;
	KeyTaskMsg *message;

	for (;;) {
	//	printf("Key: loop again\n");
		rc = xQueueReceive(__KeyQueue, &message, configTICK_RATE_HZ);
		if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			for (; map->type != KEY_NULL; ++map) {
				if (message->type == map->type) {
					map->handlerFunc(message);
					break;
				}
			}
			vPortFree(message);
		} 
	}
}


void KeyInit(void) {
	key_gpio_init();
	TIM3_Init();
	__KeyQueue = xQueueCreate(10, sizeof(KeyTaskMsg *));
	xTaskCreate(__KeyTask, (signed portCHAR *) "KEY", KEY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}

