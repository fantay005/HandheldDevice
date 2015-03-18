#include <stdio.h>
#include "FreeRTOS.h" 
#include "queue.h" 
#include "task.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "feedback.h"
#include "commu.h"

#if 0
// static xQueueHandle __queue;

#define  FEEDBACK_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 64)

extern void status(machineNumber msg);

static uint32_t EXTI_Numb[] = {
	EXTI_Line0, EXTI_Line1, EXTI_Line2, EXTI_Line3, EXTI_Line4, EXTI_Line5, EXTI_Line6, EXTI_Line7, EXTI_Line8, EXTI_Line9, 
	EXTI_Line10, EXTI_Line11, EXTI_Line12, EXTI_Line13, EXTI_Line14, EXTI_Line15
};

static machineNumber Count[] = {
	Number_1, Number_2, Number_3, Number_4, Number_5,Number_6, Number_7, Number_8, Number_9, Number_10, Number_11, Number_12,
	Number_13, Number_14, Number_15, Number_16
};

static uint8_t Exti_Sounrce[] = {
	GPIO_PinSource0, GPIO_PinSource1, GPIO_PinSource2, GPIO_PinSource3, GPIO_PinSource4, GPIO_PinSource5, GPIO_PinSource6, GPIO_PinSource7,
	GPIO_PinSource8, GPIO_PinSource9, GPIO_PinSource10, GPIO_PinSource11, GPIO_PinSource12, GPIO_PinSource13, GPIO_PinSource14, GPIO_PinSource15
};

static IRQn_Type Exti_Channel[] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn, EXTI9_5_IRQn, EXTI15_10_IRQn
};

void Freedback_Init(void){
	int i;
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	for(i = 0; i < sizeof(Exti_Sounrce) / sizeof(uint8_t); i++){
			GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, Exti_Sounrce[i]);
	}
	
	EXTI_ClearITPendingBit(EXTI_Line0 | EXTI_Line1 | EXTI_Line2 | EXTI_Line3 | EXTI_Line4 | EXTI_Line5
	                       | EXTI_Line6 | EXTI_Line7 | EXTI_Line8 | EXTI_Line9 | EXTI_Line10 | EXTI_Line11
	                       | EXTI_Line12 | EXTI_Line13 | EXTI_Line14 | EXTI_Line15);
												 
	EXTI_InitStructure.EXTI_Line = (EXTI_Line0 | EXTI_Line1 | EXTI_Line2 | EXTI_Line3 | EXTI_Line4 | EXTI_Line5
	                               | EXTI_Line6 | EXTI_Line7 | EXTI_Line8 | EXTI_Line9 | EXTI_Line10 | EXTI_Line11
	                               | EXTI_Line12 | EXTI_Line13 | EXTI_Line14 | EXTI_Line15);
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; 
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;                       
	EXTI_Init(&EXTI_InitStructure);
	
	for(i = 0; i < sizeof(Exti_Channel) / sizeof(IRQn_Type); i++){
			NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	
			NVIC_InitStructure.NVIC_IRQChannel = Exti_Channel[i];	 
			NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; 
			NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;    
			NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;      
			NVIC_Init(&NVIC_InitStructure);
	}
}

void EXTI_X_IRQHandler(void){
	machineNumber message;
	int i;
	for(i = 0; i < sizeof(EXTI_Numb) / sizeof(uint32_t); i++){
		if(EXTI_GetITStatus(EXTI_Numb[i])!= RESET){
			message = Count[i];
			EXTI_ClearITPendingBit(EXTI_Numb[i]);
			status(message);
		}
	}
}

// static void __FeedbackTask(void *parameter) {
// 	portBASE_TYPE rc;
// 	machineNumber msg;
// 	__queue = xQueueCreate(5, sizeof(machineNumber));
// 	printf("Feedback start.\n");

// 	for (;;) {
// 		rc = xQueueReceive(__queue, &msg, portMAX_DELAY);
// 		printf("+1.\n");
// 		if (rc == pdTRUE) {
// 			printf("FeedbackTask recv: %d\n", msg);
// 		}
// 	}
// }

void feedbackInit(void) {
	Freedback_Init();
//	TimerInit();
//	xTaskCreate(__FeedbackTask, (signed portCHAR *) "FEEDBACK", FEEDBACK_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
#endif
