/*
    FreeRTOS V7.5.2 - Copyright (C) 2013 Real Time Engineers Ltd.

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>! NOTE: The modification to the GPL is included to allow you to distribute
    >>! a combined work that includes FreeRTOS without being obliged to provide
    >>! the source code for proprietary components outside of the FreeRTOS
    >>! kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION		1  //是否配置成抢先先多任务内核，是1的时候，优先级高的任务优先执行。 为0任务就没有优先级之说，用时间片轮流执行
#define configUSE_IDLE_HOOK			1  // IDLE任务的HOOK函数，用于OS功能扩展，需要你自己编相应函数， 名字是void vApplicationIdleHook( void )
#define configUSE_TICK_HOOK			0  // SYSTEM TICK的HOOK函数，用于OS功能扩展，需要你自己编相应函数， 名字是 void vApplicationTickHook( void )
#define configCPU_CLOCK_HZ			( ( unsigned long ) 72000000 )  // 系统CPU频率，单位是Hz
#define configTICK_RATE_HZ			( ( portTickType ) 500 )  // 系统SYSTEM TICK每秒钟的发生次数， 数值越大系统反应越快，但是CPU用在任务切换的开销就越多
#define configMAX_PRIORITIES		( ( unsigned portBASE_TYPE ) 5 )  // 系统任务优先级数。5 说明任务有5级优先度。这个数目越大耗费RAM越多
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 64 )  //系统最小堆栈尺寸，注意128不是128字节，而是128个入栈。比如ARM32位，128个入栈就是512字节
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 44 * 1024 ) )   /* 系统可用内存。一般设成除了操作系统和你的程序所用RAM外的最大RAM。 比如20KRAM你用了2K，系统用了3K， 剩下15就是最大HEAP 尺寸。你可以先设小然后看编译结果往大里加*/
#define configMAX_TASK_NAME_LEN		( 16 )  //任务的PC名字最大长度，因为函数名编译完了就不见了，所以追踪时不知道哪个名字。16表示16个char
#define configUSE_TRACE_FACILITY	0   //是否设定成追踪， 由PC端TraceCon.exe记录， 也可以转到系统显示屏上
#define configUSE_16_BIT_TICKS		0  //就是SYSTEM TICK的长度，16是16位，如果是16位以下CPU， 一般选1；如果是32位系统，一般选0。
#define configIDLE_SHOULD_YIELD		1   //简单理解以下就是和IDLE TASK同样优先级的任务执行情况。建议设成1，对系统影响不大。 
#define configUSE_MALLOC_FAILED_HOOK 1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		0    //是用用协程。协程公用堆栈，节省RAM， 但是没有任务优先级高，也无法和任务通讯
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )    //所有协程的最大优先级数， 协程优先级永远低于任务。就是系统先执行任务，所有任务执行完了才执行协程。 

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet		1  //设定可以改变任务优先度
#define INCLUDE_uxTaskPriorityGet		1  //设定可以查询任务优先度
#define INCLUDE_vTaskDelete				1     //设定可以删除任务
#define INCLUDE_vTaskCleanUpResources	0   //据说是可以回收删除任务后的资源（RAM等）
#define INCLUDE_vTaskSuspend			1     //设置可以把任务挂起
#define INCLUDE_vTaskDelayUntil			1  //设置任务延迟的绝对时间， 比如现在4：30，延迟到5：00。时间都是绝对时间
#define INCLUDE_vTaskDelay				1    //设置任务延时， 比如延迟30分钟，相对的时间，现在什么时间，不需要知道
#define INCLUDE_xTaskGetCurrentTaskHandle 1   //设置 当前任务是由哪个任务开启的
#define INCLUDE_pcTaskGetTaskName 1

/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
(lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY 		255  // 系统内核的中断优先级，中断优先级越低，越不会影响其他中断。一般设成最低
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191 /* equivalent to 0xb0, or priority 11. */   // 系统SVC中断优先级，这两项都在在M3和PIC32上应用


/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15


#endif /* FREERTOS_CONFIG_H */
