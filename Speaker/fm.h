#ifndef __FM_H__
#define __FM_H__


typedef enum OPERA_MODE {
	READ = 1,
	WRITE = 2
} T_OPERA_MODE;

typedef enum ERROR_OP {
	Si_ERROR = 1,
	I2C_ERROR ,
	LOOP_EXP_ERROR ,
	OK
} T_ERROR_OP;

typedef enum POWER_UP_TYPE {
	FM_RECEIVER = 1,
	FM_TRNSMITTER = 2,
	AM_RECEIVER = 3
} T_POWER_UP_TYPE;

typedef enum SEEK_MODE {
	SEEKDOWN_HALT = 1,
	SEEKDOWN_WRAP = 2,
	SEEKUP_HALT = 3,
	SEEKUP_WRAP = 4
} T_SEEK_MODE;

//void fmopen(int freq);

#endif
