#ifndef __COMMU_H__
#define __COMMU_H__

typedef enum{
	Number_1 = 1,
	Number_2,
	Number_3,
	Number_4,
	Number_5,
	Number_6,
	Number_7,
	Number_8,
	Number_9,
	Number_10,
	Number_11,
	Number_12,
	Number_13,
	Number_14,
	Number_15,
	Number_16,
}machineNumber;

typedef struct {
	unsigned char addr;
	unsigned char mach;
	unsigned char act;
	unsigned char sum[2];
} comm_pack;

void max_send(machineNumber msg);


#endif
