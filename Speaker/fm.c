#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "misc.h"
#include "fm.h"
#include "xfs.h"
#include "soundcontrol.h"

#define FM_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE + 32)
#define FM_SWITCH_TIME      (configTICK_RATE_HZ * 2)
static xQueueHandle          __queue;
static xSemaphoreHandle      __asemaphore = NULL;

typedef enum{
  TYPE_FM_OPEN_CHANNEL,
	TYPE_FM_CLOSE,
	TYPE_FM_NEXT_CHANNEL,
}FMTaskMessageType;

#define DURATION_INIT_1 	60
#define DURATION_INIT_2	    60
#define DURATION_INIT_3 	60

#define DURATION_START_1	360
#define DURATION_START_2	360
#define DURATION_START_3	480

#define DURATION_STOP_1  	480
#define DURATION_STOP_2	    360
#define DURATION_STOP_3	    780

#define DURATION_HIGH		1000
#define DURATION_LOW		2000
#define POWER_SETTLING		66


#define WRITE_ADDR 0xC6
#define READ_ADDR 0xC7

#define RST_PIN			GPIO_Pin_5
#define SDIO_PIN		GPIO_Pin_11
#define SCLK_PIN		GPIO_Pin_10

#define RST_LOW		GPIO_ResetBits(GPIOB, RST_PIN)			
#define RST_HIGH	GPIO_SetBits(GPIOB, RST_PIN)

void inline __SDIO_DIR_OUT(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin =  SDIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
#define SDIO_DIR_OUT  __SDIO_DIR_OUT()

void inline __RST_DIR_OUT(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = RST_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
#define RST_DIR_OUT   __RST_DIR_OUT()

void inline __SCLK_DIR_OUT(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = SCLK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
#define	SCLK_DIR_OUT   __SCLK_DIR_OUT()

void inline __SDIO_DIR_IN(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = SDIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
#define SDIO_DIR_IN  __SDIO_DIR_IN()

#define SDIO_LOW	GPIO_ResetBits(GPIOB, SDIO_PIN)	
#define SDIO_HIGH	GPIO_SetBits(GPIOB, SDIO_PIN)	
#define READ_SDIO	GPIO_ReadInputDataBit(GPIOB,SDIO_PIN) 
#define SCLK_LOW	GPIO_ResetBits(GPIOB, SCLK_PIN)	
#define SCLK_HIGH	GPIO_SetBits(GPIOB, SCLK_PIN)

void inline __RST_PIN_INIT(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = RST_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, RST_PIN);
}
#define	RST_PIN_INIT  __RST_PIN_INIT()

void inline __SDIO_PIN_INIT(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = SDIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, SDIO_PIN);
}
#define	SDIO_PIN_INIT   __SDIO_PIN_INIT()

void inline  __SCLK_PIN_INIT(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = SCLK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, SCLK_PIN);
}
#define	SCLK_PIN_INIT  __SCLK_PIN_INIT()

#define DELAY(DURATION)		{vu16 i; for(i = 1; i <DURATION; i++){}}

static void __initGpio() {
	GPIO_InitTypeDef  GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = SCLK_PIN | SDIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, SCLK_PIN);

	GPIO_InitStructure.GPIO_Pin = RST_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, RST_PIN);
}

void ResetSi4731_2w(void) {
	__initGpio();
	SDIO_LOW;	
	SDIO_HIGH;
	RST_LOW;
	SCLK_HIGH;
	DELAY(DURATION_INIT_1);
	RST_HIGH;
	DELAY(DURATION_INIT_2);
	SDIO_HIGH;
	DELAY(DURATION_INIT_3);
}

static void __txByte(unsigned char byte) {
	int i;
	for (i = 7; i >= 0; i--) {                   //发送从机地址
		if ((byte >> i) & 0x01) {
			SDIO_HIGH;
		} else {
			SDIO_LOW;
		}
		DELAY(DURATION_LOW / 2);
		SCLK_HIGH;
		DELAY(DURATION_HIGH);
		SCLK_LOW;
		DELAY(DURATION_LOW / 2);
	}
}

static bool __waitAck() {
	unsigned char ret;
	SDIO_DIR_IN;
	DELAY(DURATION_LOW / 2);
	SCLK_HIGH;
	DELAY(DURATION_HIGH);
	ret = READ_SDIO;
	SCLK_LOW;
	DELAY(DURATION_LOW / 2);
	return ret == 0;
}

uint8_t OperationSi4731_2w(T_OPERA_MODE operation, uint8_t *data, uint8_t numBytes) {
	uint8_t controlWord,  j, error = 0;
	int i;

	SCLK_HIGH;
	SDIO_HIGH;
	DELAY(DURATION_START_1);
	SDIO_LOW;
	DELAY(DURATION_START_2);
	SCLK_LOW;
	DELAY(DURATION_START_3);

	if (operation == READ) {                      //READ 说明主机准备接受模块发送数据
		controlWord = 0xC7;
	} else {                                      //WRITE 说明模块准备接受主机发送数据
		controlWord = 0xC6;
	}

	__txByte(controlWord);

	if (!__waitAck()) {
		goto STOP;
	}


	for (j = 0; j < numBytes; j++, data++) {
		if (operation == WRITE) {
			SDIO_DIR_OUT;
		} else {
			SDIO_DIR_IN;
		}
		for (i = 7; i >= 0; i--) {
			if (operation == WRITE)
				if ((*data >> i) & 0x01) {
					SDIO_HIGH;
				} else {
					SDIO_LOW;
				}
			DELAY(DURATION_LOW / 2);
			SCLK_HIGH;
			DELAY(DURATION_HIGH);
			if (operation == READ) {
				*data = (*data << 1) | READ_SDIO;    //地址data所指向的数值被赋予从端口督导的值
			}
			SCLK_LOW;
			DELAY(DURATION_LOW / 2);
		}
		if (operation == WRITE) {
			SDIO_DIR_IN;
		} else {
			SDIO_DIR_OUT;
			if (j == (numBytes - 1)) {
				SDIO_HIGH;
			} else {
				SDIO_LOW;
			}
		}
		DELAY(DURATION_LOW / 2);
		SCLK_HIGH;
		DELAY(DURATION_HIGH);
		if (operation == WRITE)
			if (READ_SDIO != 0) {
				error = 1;
				goto STOP;
			}
		SCLK_LOW;
		DELAY(DURATION_LOW / 2);
	}
STOP:
	SDIO_DIR_OUT;
	SDIO_LOW;
	DELAY(DURATION_STOP_1);
	SCLK_HIGH;
	DELAY(DURATION_STOP_2);
	SDIO_HIGH;
	DELAY(DURATION_STOP_3);
	return (error);
}

static T_ERROR_OP Si4731_Power_Down(void) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_power_down[] = {0x11};

	error_ind = OperationSi4731_2w(WRITE, &(Si4731_power_down[0]), 1);
	if (error_ind) {
		return I2C_ERROR;
	}
	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}



static  T_ERROR_OP Si4731_Power_Up(T_POWER_UP_TYPE power_up_type) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_power_up[] = {0x01, 0xC1, 0x05};

	switch (power_up_type) {
	case FM_RECEIVER: {
		Si4731_power_up[1] = 0xD0;
		Si4731_power_up[2] = 0x05;
		break;
	}
	case FM_TRNSMITTER: {
		Si4731_power_up[1] = 0xC2;
		Si4731_power_up[2] = 0x50;
		break;
	}
	case AM_RECEIVER: {
		Si4731_power_up[1] = 0xC1;
		Si4731_power_up[2] = 0x05;
		break;
	}
	}

	ResetSi4731_2w();
	vTaskDelay(configTICK_RATE_HZ / 5);
	error_ind = OperationSi4731_2w(WRITE, &(Si4731_power_up[0]), 3);
	if (error_ind) {
		return I2C_ERROR;
	}
	vTaskDelay(configTICK_RATE_HZ / 2);;
	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff)); 
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}


static T_ERROR_OP Si4731_Set_Property_GPO_IEN(void) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_set_property[] = {0x12, 0x00, 0x00, 0x01, 0x00, 0xCD};	//set STCIEN,CTSIEN

	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if (error_ind) {
		return I2C_ERROR;
	}
	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}


static T_ERROR_OP Si4731_Get_INT_status(void)
{
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];	
	uint8_t error_ind = 0;
	uint8_t Si4731_Get_INT_status[] = {0x14};	

	error_ind = OperationSi4731_2w(WRITE, &(Si4731_Get_INT_status[0]), 1);
	if(error_ind)
		return I2C_ERROR;
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	
	return OK;
}

static T_ERROR_OP Si4731_Set_Property_FM_Seek_Band_Bottom(void) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_set_property[] = {0x12, 0x00, 0x14, 0x00, 0x22, 0x2E};	//0x222E = 8750

	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if (error_ind) {
		return I2C_ERROR;
	}

	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff)); 
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}

static T_ERROR_OP Si4731_Set_Property_FM_Seek_Band_Top(void) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_set_property[] = {0x12, 0x00, 0x14, 0x01, 0x2A, 0x26};	//0x2A26 = 10790

	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if (error_ind) {
		return I2C_ERROR;
	}

	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff)); 
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}

static T_ERROR_OP Si4731_Set_Property_FM_Seek_Space(void) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_set_property[] = {0x12, 0x00, 0x14, 0x02, 0x00, 0x0A};	//seek space = 0x0A = 10 = 100KHz

	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if (error_ind) {
		return I2C_ERROR;
	}
	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}

static T_ERROR_OP Si4731_Wait_STC(void) {
	uint16_t loop_counter = 0, loop_counter_1 = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_get_int_status[] = {0x14, 0x00};	//读中断位

	do {
	//	vTaskDelay(configTICK_RATE_HZ);
		error_ind = OperationSi4731_2w(WRITE, &(Si4731_get_int_status[0]), 1);

		if (error_ind) {
			return I2C_ERROR;
		}
		do {
			error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
			if (error_ind) {
				return I2C_ERROR;
			}
			loop_counter_1++;
		} while (((Si4731_reg_data[0] & 0x80) == 0) && (loop_counter_1 < 0xff));
		if (loop_counter_1 >= 0xff) {
			return LOOP_EXP_ERROR;
		}
		loop_counter_1 = 0;
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0] & 0x01) == 0) && (loop_counter < 0xfff));
	if (loop_counter == 0xfff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}


static T_ERROR_OP Si4731_FM_Tune_Freq(uint16_t channel_freq) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_tune_freq[] = {0x20, 0x00, 0x27, 0xF6, 0x00};	//0x27F6=10230KHz

	Si4731_tune_freq[2] = (channel_freq & 0xff00) >> 8;
	Si4731_tune_freq[3] = (channel_freq & 0x00ff);
	//send CMD
	error_ind = OperationSi4731_2w(WRITE, &(Si4731_tune_freq[0]), 5);
	if (error_ind) {
		return I2C_ERROR;
	}
	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}

static T_ERROR_OP Si4731_Set_Property_FM_DEEMPHASIS(void)
{
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];	
	uint8_t error_ind = 0;
	uint8_t Si4731_set_property[] = {0x12,0x00,0x11,0x00,0x00,0x01};	//FM deemphasis is 50us
        //0x12 设置特性值；0x1100 FM_DEEMPHASIS的属性； FM_DEEMPHASIS的Default值为75us(0x0002) ，50us(0x0001)
	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1(清除发送下一个指令）
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}


static T_ERROR_OP Si4731_Set_Property_FM_SNR_Threshold(void)
{
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];	
	uint8_t error_ind = 0;
	uint8_t Si4731_set_property[] = {0x12,0x00,0x14,0x03,0x00,0x02};	//SNR threshold = 0x0003 = 3dB
        //0x1403为FM_SEEK_TUNE_SNR_THERSHOLD的属性；缺省值为0x0003=3db
	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	
	return OK;

}


static T_ERROR_OP Si4731_Set_Property_FM_RSSI_Threshold(void)
{
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];	
	uint8_t error_ind = 0;
	uint8_t Si4731_set_property[] = {0x12,0x00,0x14,0x04,0x00,0x08};	//RSSI threshold = 0x0014 = 20dBuV
        //0x1404为FM_SEEK_TUNE_RSSI_TRESHOLD的属性；缺省值为0x0014=20dBuV
	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}


static T_ERROR_OP Si4731_Set_FM_Frequency(uint16_t channel_freq) {
	if (Si4731_FM_Tune_Freq(channel_freq) != OK) {
		return Si_ERROR;
	}
	if (Si4731_Wait_STC() != OK) {
		return Si_ERROR;
	}
	return OK;
}

/**************************************

static Si4731_FM_Seek_Start()

***************************************/

static T_ERROR_OP Si4731_FM_Seek_Start(T_SEEK_MODE seek_mode)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_seek_start[] = {0x21,0x0C};
			
	
	switch(seek_mode)
	{
		case SEEKDOWN_HALT:
		{
			Si4731_seek_start[1] = 0x00;
			break;
		}
    case SEEKDOWN_WRAP:
    {
    	Si4731_seek_start[1] = 0x04;
    	break;
    }
    case SEEKUP_HALT:
    {
    	Si4731_seek_start[1] = 0x08;
    	break;
    }
    case SEEKUP_WRAP:
    {
    	Si4731_seek_start[1] = 0x0C;
    	break;
    }
  }
	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_seek_start[0]), 2);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}

/**************************************

static Si4731_FM_Tune_Status()

***************************************/

static T_ERROR_OP Si4731_FM_Tune_Status(unsigned short *pChannel_Freq, unsigned char *SeekFail, unsigned char *valid_channel)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_fm_tune_status[] = {0x22,0x01};		

	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_fm_tune_status[0]), 2);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	
		
	//read tune status: you should read out: {0x80,0x01,0x27,0xF6,0x2D,0x33,0x00,0x00} //Freq=0x27F6=10230KHz, RSSI=0x2D=45dBuV, SNR=0x33=51dB
	error_ind = OperationSi4731_2w(READ, &Si4731_reg_data[0], 8);	
	if(error_ind)
		return I2C_ERROR;
		
	if(((Si4731_reg_data[1]&0x80) != 0))
		*SeekFail = 1;
	else
		*SeekFail = 0;
		
	if(((Si4731_reg_data[1]&0x01) != 0))
		*valid_channel = 1;
	else
		*valid_channel = 0;
		
	*pChannel_Freq = ((Si4731_reg_data[2] << 8) | Si4731_reg_data[3]);

	return OK;

}

/*********************************************

Si4731_FM_Seek()

SeekFail:"no any station" or not when in WRAP mode

*********************************************/

T_ERROR_OP Si4731_FM_Seek(T_SEEK_MODE seek_mode, unsigned short *pChannel_Freq, unsigned char *SeekFail)
{
	unsigned char valid_channel;
	unsigned short loop_counter = 0;

	do
	{
		if(Si4731_FM_Seek_Start(seek_mode) != OK) return ERROR;
		if(Si4731_Wait_STC() != OK) return ERROR;
		//read seek result:
		if(Si4731_FM_Tune_Status(pChannel_Freq, SeekFail, &valid_channel) != OK) return ERROR;	
		
		loop_counter++;
	}	while((valid_channel == 0) && (loop_counter < 0xff) && (*SeekFail == 0)); 

	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;
		
	if((seek_mode == SEEKDOWN_WRAP) || (seek_mode == SEEKUP_WRAP))
		if((valid_channel == 1) && (*SeekFail == 1))
			*SeekFail = 0;

	return OK;
}

/**************************************

Si4731_FM_Seek_All()

***************************************/

T_ERROR_OP Si4731_FM_Seek_All(unsigned short *pChannel_All_Array, unsigned char Max_Length, unsigned char *pReturn_Length)
{
	unsigned char SeekFail;
	unsigned short Channel_Result, Last_Channel = 8750;
	
	if(Si4731_Set_FM_Frequency(8750) != OK) return ERROR;
	
	while(*pReturn_Length < Max_Length)
	{
		vTaskDelay(configTICK_RATE_HZ / 10);
		if(Si4731_FM_Seek(SEEKUP_WRAP, &Channel_Result, &SeekFail) != OK) return ERROR;
			
		if(SeekFail)
			return OK;
		
		if((Channel_Result) <= Last_Channel)	
		{
			if((Channel_Result) == 8750)
			{
				*pChannel_All_Array++ = Channel_Result;
				(*pReturn_Length)++;
			}
			return OK;
		}
		else
		{
			*pChannel_All_Array++ = Last_Channel = Channel_Result;
			(*pReturn_Length)++;
		}
	}
	
	return OK;
}

void SI4731Init(void){
	RST_PIN_INIT;
	SDIO_PIN_INIT;
	SCLK_PIN_INIT;
	RST_LOW;
}
/*
void EXTI3_INTI(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOG, &GPIO_InitStructure);		  


	EXTI_InitStructure.EXTI_Line = EXTI_Line13; 
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; 
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; 
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                            
    EXTI_Init(&EXTI_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOG, GPIO_PinSource13);	  

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;     //选择中断通道1
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占式中断优先级设置为0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        //响应式中断优先级设置为0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                                   //使能中断
    NVIC_Init(&NVIC_InitStructure);
}

static char NEXT = 0;
static unsigned short pChannel[30];
static portTickType lastT = 0;
static char choose = 0;
static portTickType upT = 0;
static portTickType downT = 0xFFFFFFFF;

void EXTI15_10_IRQHandler (void)
{
	int curT;
	portBASE_TYPE msg;
	EXTI_ClearITPendingBit(EXTI_Line13);
	if (pdTRUE == xSemaphoreGiveFromISR(__asemaphore, &msg)) {
		curT = xTaskGetTickCount();
		if ((curT - lastT) >= FM_SWITCH_TIME){
		   NEXT ++;
		   if(NEXT > 30){
		      NEXT = 0;
       }
			 
			 if (msg) {
			    taskYIELD();
		   }
		 }
		 
		 if(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_13) == 0){
			  downT = xTaskGetTickCount();
		 }

		 if(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_13) == 1){
			  upT = xTaskGetTickCount();
			  if(upT > downT){
					if ((upT - downT) >= FM_SWITCH_TIME){
           choose = 1;					
				  }
				}
        downT = 0xFFFFFFFF;					
		 } 		 

		lastT = curT;
	}
}

void auto_seek_Property(void){
	vTaskDelay(configTICK_RATE_HZ / 10);
	RST_HIGH;
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Power_Down();
	vTaskDelay(configTICK_RATE_HZ / 5);
	Si4731_Power_Up(FM_RECEIVER);
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Set_Property_GPO_IEN();
	vTaskDelay(configTICK_RATE_HZ / 10);
  Si4731_Set_Property_FM_DEEMPHASIS();
  vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Set_Property_FM_Seek_Band_Bottom();
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Set_Property_FM_Seek_Band_Top();
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Set_Property_FM_Seek_Space();
	vTaskDelay(configTICK_RATE_HZ / 10);
  Si4731_Set_Property_FM_SNR_Threshold();
  vTaskDelay(configTICK_RATE_HZ / 10);
  Si4731_Set_Property_FM_RSSI_Threshold();
}
	

static  char memory = 0;

void Broadcast(unsigned short para) {
	int i;
	char buf[8];
	char tune[27] = {0xFD, 0x00, 0x18, 0x01, 0x01, '[', 'm', '5', '1', ']', 's', 'o', 'u', 'n', 'd', '1', '1', '4',//sound114
	                	',', 'f', 'm', ',', '9', '0', 0xB5, 0xE3, '8'	};	  //FM90.8!
  sprintf(buf, "%d", para);
  if(para >= 10000){
		tune[21] = buf[0];
		tune[22] = buf[1];
		tune[23] = buf[2];
		tune[26] = buf[3];
	}	else {
		tune[22] = buf[0];
		tune[23] = buf[1];
		tune[26] = buf[2];
	}
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
  for (i = 0; i < 27; i++) {
		USART_SendData(USART3, tune[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 5);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
	recover();
}

void ServerCMD(char *p){
	
}

void __FMTask(void) {
	unsigned char Return_Length = 0;
	portBASE_TYPE rc;
	__asemaphore = xQueueGenericCreate(1, semSEMAPHORE_QUEUE_ITEM_LENGTH, queueQUEUE_TYPE_BINARY_SEMAPHORE );
	memset(&pChannel[0], 0, 30);
	auto_seek_Property();
	EXTI3_INTI();
	Si4731_FM_Seek_All(&(pChannel[0]), 30, &Return_Length);
	for (;;) {
	  if (__asemaphore != NULL) {
			 xSemaphoreTake(__asemaphore, portMAX_DELAY);
			 if(choose == 1){
				 choose = 0;
				 if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_2) == 0){
			       SoundControlSetChannel(SOUND_CONTROL_CHANNEL_FM, 1);
		     } else {
				     SoundControlSetChannel(SOUND_CONTROL_CHANNEL_FM, 0);
         }
			 }
       if(memory == NEXT) continue;
			 if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_2) == 1){
				 if(pChannel[NEXT] != 0){
					  Broadcast(pChannel[NEXT]);
			   }
			 }
	     if(NEXT < Return_Length){
				  printf("%d=%5d\n", NEXT, pChannel[NEXT]);
	        Si4731_Set_FM_Frequency(pChannel[NEXT]);
       } else {
				  NEXT = 0;
				  Broadcast(pChannel[NEXT]);
				  printf("%d=%5d\n", NEXT, pChannel[NEXT]);
				  Si4731_Set_FM_Frequency(pChannel[NEXT]);
       }
			 
			 memory = NEXT;
	  }
  }
}



void FMInit(void) {
	SI4731Init();
	xTaskCreate(__FMTask, (signed portCHAR *) "FM", FM_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 10, NULL);
}
*/

void fmopen(int freq) {
	if ((freq < 875) || (freq > 1080)) {
		return;
	}
	freq = freq * 10;
	RST_PIN_INIT;
	SDIO_PIN_INIT;
	SCLK_PIN_INIT;
	RST_LOW;
	vTaskDelay(configTICK_RATE_HZ / 10);
	RST_HIGH;
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Power_Down();
	vTaskDelay(configTICK_RATE_HZ / 5);
	Si4731_Power_Up(FM_RECEIVER);
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Set_Property_GPO_IEN();
	vTaskDelay(configTICK_RATE_HZ / 10);
  Si4731_Set_Property_FM_DEEMPHASIS();
  vTaskDelay(configTICK_RATE_HZ / 10);
  Si4731_Set_Property_FM_SNR_Threshold();
  vTaskDelay(configTICK_RATE_HZ / 10);
  Si4731_Set_Property_FM_RSSI_Threshold();
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_FM, 1);
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Set_FM_Frequency(freq);
}