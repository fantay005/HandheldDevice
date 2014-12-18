#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_flash.h"
#include "misc.h"
#include "version.h"

static void PreSetupHardware(void) {
	extern unsigned int *__Vectors;
	ErrorStatus HSEStartUpStatus;
	/* RCC system reset(for debug purpose) */
	RCC_DeInit();
	/* Enable HSE */
	RCC_HSEConfig(RCC_HSE_ON);
	/* Wait till HSE is ready */
	HSEStartUpStatus = RCC_WaitForHSEStartUp();
	if (HSEStartUpStatus == SUCCESS) {
		/* Enable Prefetch Buffer */
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
		/* Flash 2 wait state */
		FLASH_SetLatency(FLASH_Latency_2);
		/* HCLK = SYSCLK */
		RCC_HCLKConfig(RCC_SYSCLK_Div1);
		/* PCLK2 = HCLK */
		RCC_PCLK2Config(RCC_HCLK_Div1);
		/* PCLK1 = HCLK/2 */
		RCC_PCLK1Config(RCC_HCLK_Div2);
		/* PLLCLK = 8MHz * 9 = 72 MHz */
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
		/* Enable PLL */
		RCC_PLLCmd(ENABLE);
		/* Wait till PLL is ready */
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {}
		/* Select PLL as system clock source */
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
		/* Wait till PLL is used as system clock source */
		while (RCC_GetSYSCLKSource() != 0x08) {}
	}
	/* Enable FSMC, GPIOD, GPIOE, GPIOF, GPIOG and AFIO clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
						   RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
						   RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF |
						   RCC_APB2Periph_GPIOG | RCC_APB2Periph_AFIO |
						   RCC_APB2Periph_USART1 
						   , ENABLE);
	/* Enable peripheral clocks --------------------------------------------------*/

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_TIM3, ENABLE);

	NVIC_SetVectorTable((unsigned int)&__Vectors, 0x0);

}
/*-----------------------------------------------------------*/

extern void UartDebugInit(void);
extern void WatchdogInit(void);
extern void feedbackInit(void);
extern void commuInit(void);


int main(void) {
	PreSetupHardware();
	UartDebugInit();

	feedbackInit();
	commuInit();
	printf("\n==============================\n");
	printf("���������\r\n");
	printf("\n==============================\n");
	vTaskStartScheduler();
	return 0;
}



