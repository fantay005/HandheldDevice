#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#include <stdbool.h>
#include <stdint.h>
#include "fsmc_nor.h"

/********************************************************************************************************
* NOR_FLASH��ַӳ���
********************************************************************************************************/
//NOR_FLASH
#define NORFLASH_SECTOR_SIZE   				          ((uint32_t)0x00001000)/*������С*/ 
#define NORFLASH_MANAGEM_BASE  				          ((uint32_t)0x00001000)/*���ز���-������ݱ�ʶ����γ�ȡ�ZIGBEEƵ�㡢�Զ��ϴ�����ʱ����*/
#define NORFLASH_BALLAST_NUM   				          ((uint32_t)0x00002000)/*��������Ŀ*/
#define NORFLASH_ONOFFTIME1   				          ((uint32_t)0x00003000)/*���ص�ʱ��1*/
#define NORFLASH_ONOFFTIME2   				          ((uint32_t)0x00004000)/*���ص�ʱ��2*/
#define NORFLASH_CHIP_ERASE                     ((uint32_t)0x00005000)/*D*/ 

#define NORFLASH_END_LIGHT_ADDR                 ((uint32_t)0x00016000)/*����ĩ�˵�ZIGBEE��ַ��Ŵ�*/
#define NORFLASH_LIGHT_NUMBER                   ((uint32_t)0x00017000)/*���صĵƲ������������Ƶĵ�����*/
#define NORFLASH_BALLAST_BASE  				          ((uint32_t)0x00018000)/*Zigbee1 ������������ַ*/
#define NORFLASH_BSN_PARAM_BASE                 ((uint32_t)0x00218000)/*Zigbee2 ������������ַ*/
#define NORFLASH_MANAGEM_ADDR                   ((uint32_t)0x00400000)/*���ص�ַ��������IP��ַ���˿ں�*/
#define NORFLASH_MANAGEM_TIMEOFFSET 		        ((uint32_t)0x00401000)/*���ز���-����ƫ�ơ��ص�ƫ��*/
#define NORFLASH_MANAGEM_WARNING   		          ((uint32_t)0x00402000)/*���ز���-�澯*/
#define NORFLASH_RESET_TIME           		    	((uint32_t)0x00404000)/*����ʱ��*/
#define NORFLASH_RESET_COUNT                    ((uint32_t)0x00405000)/*��������*/
#define NORFLASH_ELEC_UPDATA_TIME               ((uint32_t)0x00406000)/*�����ϴ�ʱ��*/
#define NORFLASH_STRATEGY_BASE 				          ((uint32_t)0x00418000)/*Zigbee1 ���Բ�����ַ*/
#define NORFLASH_STY_PARAM_BASE                 ((uint32_t)0x00618000)/*Zigbee2 ���Բ�����ַ*/
#define NORFLASH_STRATEGY_OK_OFFSET 		       	((uint32_t)0x00000d00)/*����������ʶ*/
#define NORFLASH_PARAM_OFFSET   				        ((uint32_t)0x00001000)
#define NORFLASH_STRATEGY_OFFSET        		    ((uint32_t)0x00001000)

/*����������������������ƫ��*/
#define PARAM_ZIGBEE_ADDR_OFFSET                ((uint32_t)0x00000000) /*��4*2���ֽ�ASCII�룬�洢ZIGBEE��ַ*/
#define PARAM_TIME_FALG_OFFSET                  ((uint32_t)0x00000008) /*��12*2���ֽ�ASCII�룬�洢����ͬ����ʶ*/
#define PARAM_RATED_POWER_OFFSET                ((uint32_t)0x00000020) /*��4*2���ֽ�ASCII�룬�洢��ƹ���ֵ*/
#define PARAM_LOOP_NUM_OFFSET                   ((uint32_t)0x00000028) /*��1*2���ֽ�ASCII�룬�洢������·*/
#define PARAM_LAMP_POST_NUM_OFFSET              ((uint32_t)0x0000002A) /*��4*2���ֽ�ASCII�룬�洢�����Ƹ˺�*/
#define PARAM_LAMP_TYPE_OFFSET                  ((uint32_t)0x00000032) /*��1*2���ֽ�ASCII�룬�洢��Դ����*/
#define PARAM_PHASE_OFFSET                      ((uint32_t)0x00000034) /*��1*2���ֽ�ASCII�룬�洢��������*/
#define PARAM_PORPERTY_OFFSET                   ((uint32_t)0x00000036) /*��2*2���ֽ�ASCII�룬�洢����Ͷ����*/
#define STRATEGY_ZIGBEE_ADDR_OFFSET             ((uint32_t)0x00000000) /*��4*2���ֽ�ASCII�룬�洢ZIGBEE��ַ*/
#define STRATEGY_TIME_FALG_OFFSET               ((uint32_t)0x00000008) /*��12*2���ֽ�ASCII�룬�洢����ͬ����ʶ*/
#define STRATEGY_TYPE_OFFSET                    ((uint32_t)0x00000020) /*��2*2���ֽ�ASCII�룬�洢��������*/
#define STRATEGY_STAGE_NUM_OFFSET               ((uint32_t)0x00000024) /*��1*2���ֽ�ASCII�룬�洢�������*/
#define STRATEGY_FIRST_STATE_OFFSET             ((uint32_t)0x00000026) /*��6*2���ֽ�ASCII�룬�洢���⹦�ʼ�ʱ��*/
#define STRATEGY_SECOND_STATE_OFFSET            ((uint32_t)0x00000032) /*��6*2���ֽ�ASCII�룬�洢���⹦�ʼ�ʱ��*/
#define STRATEGY_THIRD_STATE_OFFSET             ((uint32_t)0x0000003E) /*��6*2���ֽ�ASCII�룬�洢���⹦�ʼ�ʱ��*/
#define STRATEGY_FOURTH_STATE_OFFSET            ((uint32_t)0x0000004A) /*��6*2���ֽ�ASCII�룬�洢���⹦�ʼ�ʱ��*/
#define STRATEGY_FIFTH_STATE_OFFSET             ((uint32_t)0x00000056) /*��6*2���ֽ�ASCII�룬�洢���⹦�ʼ�ʱ��*/


#define UPDATA_FLAG_STORE_SECTOR                ((uint32_t)0x0800F800) /*�Ƿ���Ҫ�����ṹ�屣�����ڲ�FLASH��*/


#define UNICODE_TABLE_ADDR (0x0E0000)
#define UNICODE_TABLE_END_ADDR (UNICODE_TABLE_ADDR + 0x3B2E)
#define GBK_TABLE_OFFSET_FROM_UNICODE (0x3B30)
#define GBK_TABLE_ADDR (UNICODE_TABLE_ADDR + GBK_TABLE_OFFSET_FROM_UNICODE)
#define GBK_TABLE_END_ADDR (UNICODE_TABLE_END_ADDR + GBK_TABLE_OFFSET_FROM_UNICODE)

void NorFlashInit(void);
void NorFlashWrite(uint32_t flash, const short *ram, int len);
void NorFlashEraseParam(uint32_t flash);
void NorFlashRead(uint32_t flash, short *ram, int len);
void NorFlashEraseChip(void);

bool NorFlashMutexLock(uint32_t time);
void NorFlashMutexUnlock(void);


#endif
