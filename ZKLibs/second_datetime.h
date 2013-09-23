#ifndef __SECOND_DATE_H__
#define __SECOND_DATE_H__

#include <stdint.h>

typedef struct {
	uint16_t year; //<! ��, ��1��ʾ 2001��
	uint8_t month; //<! ��.
	uint8_t date; //<! ��.
	uint8_t day;  //<! ����.
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} DateTime;


/// \param second ��2000/1/1-00:00:00��֮��ʼ��ʱ������.
void SecondToDateTime(DateTime *dateTime, uint32_t second);
/// \return ��2000/1/1-00:00:00��֮��ʼ��ʱ������.
uint32_t DateTimeToSecond(const DateTime *dateTime);


#endif
