#include "soundcontrol.h"
#include "stm32f10x_gpio.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

static xSemaphoreHandle __semaphore = NULL;
static unsigned char __channelsEnable;

#define GSM_PIN GPIO_Pin_0
#define XFS_PIN GPIO_Pin_6
#define FM_PIN GPIO_Pin_2
#define MP3_PIN GPIO_Pin_1
#define AP_PIN  GPIO_Pin_6
#define MUTE_PIN GPIO_Pin_6
#define ALL_PIN (GSM_PIN | XFS_PIN | FM_PIN | MP3_PIN)

static void initHardware(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;

	GPIO_ResetBits(GPIOE, GPIO_Pin_0);
	GPIO_ResetBits(GPIOE, GPIO_Pin_1);
	GPIO_ResetBits(GPIOE, GPIO_Pin_2);
	GPIO_ResetBits(GPIOE, GPIO_Pin_6);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);                          //XFS,MP3,FM,GSM��Ƶ������ƽ�	
	
	GPIO_ResetBits(GPIOC, GPIO_Pin_1);
	GPIO_ResetBits(GPIOC, GPIO_Pin_2);
	GPIO_ResetBits(GPIOC, GPIO_Pin_3);
	GPIO_ResetBits(GPIOC, GPIO_Pin_4);
	GPIO_ResetBits(GPIOC, GPIO_Pin_6);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}


void inline __MUTE_DIR_OUT(void) {
    GPIO_InitTypeDef  GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin =  MUTE_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_50MHz;
    GPIO_Init(GPIOG, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOG, MUTE_PIN);
}

#define MUTE_DIR_OUT  __MUTE_DIR_OUT()

void inline __MUTE_DIR_IN(void) {
    GPIO_InitTypeDef  GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin =  MUTE_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//  GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_50MHz;
    GPIO_Init(GPIOG, &GPIO_InitStructure);
}

#define MUTE_DIR_IN  __MUTE_DIR_IN()


void SoundControlInit(void) {
	if (__semaphore == NULL) {
		MUTE_DIR_OUT;
		vSemaphoreCreateBinary(__semaphore);
		initHardware();
	}
}

static inline void __takeEffect(void) {
	if (__channelsEnable & SOUND_CONTROL_CHANNEL_GSM) {
		GPIO_ResetBits(GPIOE, FM_PIN | XFS_PIN | MP3_PIN);
		GPIO_SetBits(GPIOE, GSM_PIN);
		return;	
	}
		
	if (__channelsEnable & SOUND_CONTROL_CHANNEL_XFS) {
		GPIO_ResetBits(GPIOE, FM_PIN | GSM_PIN | MP3_PIN);
		GPIO_SetBits(GPIOE, XFS_PIN);
		return;	
	}
		
	if (__channelsEnable & SOUND_CONTROL_CHANNEL_MP3) {
		GPIO_ResetBits(GPIOE, FM_PIN | GSM_PIN | XFS_PIN);
		GPIO_SetBits(GPIOE, MP3_PIN);
		return;	
	}	
		
	if (__channelsEnable & SOUND_CONTROL_CHANNEL_FM) {
		GPIO_ResetBits(GPIOE, MP3_PIN | GSM_PIN | XFS_PIN);
		GPIO_SetBits(GPIOE, FM_PIN);
		return;
	}

	GPIO_ResetBits(GPIOE, FM_PIN | MP3_PIN | GSM_PIN | XFS_PIN);
}

void SoundControlSetChannel(uint32_t channels, bool isOn) {
	xSemaphoreTake(__semaphore, portMAX_DELAY);
	if (isOn) {
		__channelsEnable |= channels;
	} else {
		__channelsEnable &= ~channels;
	}
	__takeEffect();
	if (__channelsEnable != 0) {
		if ((GPIO_ReadInputDataBit(GPIOC, AP_PIN) == 0) && (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0)){
			// �򿪹��ŵ�Դ
			MUTE_DIR_IN;
	    GPIO_SetBits(GPIOC, AP_PIN);
			vTaskDelay(configTICK_RATE_HZ * 5);
		} else if ((GPIO_ReadInputDataBit(GPIOC, AP_PIN) == 0) && (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 1)) {
			MUTE_DIR_OUT;
		} else if ((GPIO_ReadInputDataBit(GPIOC, AP_PIN) == 1) && (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0)) {
			MUTE_DIR_IN;		
		} else if ((GPIO_ReadInputDataBit(GPIOC, AP_PIN) == 1) && (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 1)) {
			MUTE_DIR_IN;
			GPIO_ResetBits(GPIOB, GPIO_Pin_12);
		}
	} else {
		MUTE_DIR_OUT;
	  GPIO_ResetBits(GPIOC, AP_PIN);
	} 
	
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 1) {
	}
	xSemaphoreGive(__semaphore);
}
