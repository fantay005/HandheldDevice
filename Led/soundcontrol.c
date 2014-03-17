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
#define AMP_EN  GPIO_Pin_0

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
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	

	GPIO_InitStructure.GPIO_Pin = AMP_EN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_ResetBits(GPIOB, AMP_EN);
}

void SoundControlInit(void) {
	initHardware();
	if (__semaphore == NULL) {
		vSemaphoreCreateBinary(__semaphore);
	}
}

static inline void __takeEffect(void) {
	if (__channelsEnable & SOUND_CONTROL_CHANNEL_GSM) {
		GPIO_ResetBits(GPIOF, FM_PIN | XFS_PIN | MP3_PIN);
		GPIO_SetBits(GPIOD, GSM_PIN);
		return;	
	}
		
	if (__channelsEnable & SOUND_CONTROL_CHANNEL_XFS) {
		GPIO_ResetBits(GPIOF, FM_PIN | MP3_PIN);
		GPIO_ResetBits(GPIOD, GSM_PIN);
		GPIO_SetBits(GPIOF, XFS_PIN);
		return;	
	}
		
	if (__channelsEnable & SOUND_CONTROL_CHANNEL_MP3) {
		GPIO_ResetBits(GPIOF, FM_PIN | XFS_PIN);
		GPIO_ResetBits(GPIOD, GSM_PIN);
		GPIO_SetBits(GPIOF, MP3_PIN);
		return;	
	}	
		
	if (__channelsEnable & SOUND_CONTROL_CHANNEL_FM) {
		GPIO_ResetBits(GPIOF, MP3_PIN| XFS_PIN);
		GPIO_ResetBits(GPIOD, GSM_PIN);
		GPIO_SetBits(GPIOF, FM_PIN);
		return;
	}

	GPIO_ResetBits(GPIOF, FM_PIN | MP3_PIN | XFS_PIN);
	GPIO_ResetBits(GPIOD, GSM_PIN);
	GPIO_ResetBits(GPIOB, AMP_EN);
}

void SoundControlSetChannel(uint32_t channels, bool isOn) {
	xSemaphoreTake(__semaphore, portMAX_DELAY);
	if (isOn) {
		GPIO_SetBits(GPIOB, AMP_EN);
		__channelsEnable |= channels;
	} else {
		__channelsEnable &= ~channels;
	}
	__takeEffect();
	xSemaphoreGive(__semaphore);
}
