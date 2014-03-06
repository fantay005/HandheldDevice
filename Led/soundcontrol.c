#include "soundcontrol.h"
#include "stm32f10x_gpio.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

static xSemaphoreHandle __semaphore = NULL;
static unsigned char __channelsEnable;

#define GSM_PIN GPIO_Pin_3
#define XFS_PIN GPIO_Pin_11
#define FM_PIN GPIO_Pin_10
#define MP3_PIN GPIO_Pin_9
#define ALL_PIN (GSM_PIN | XFS_PIN | FM_PIN | MP3_PIN)

static void initHardware(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;

	GPIO_ResetBits(GPIOF, GPIO_Pin_9);
	GPIO_ResetBits(GPIOF, GPIO_Pin_10);
	GPIO_ResetBits(GPIOF, GPIO_Pin_11);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOD, GPIO_Pin_3);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void SoundControlInit(void) {
	if (__semaphore == NULL) {
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
	xSemaphoreGive(__semaphore);
}
