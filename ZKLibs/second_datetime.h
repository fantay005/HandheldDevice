#ifndef __SECOND_DATE_H__
#define __SECOND_DATE_H__

#include <stdint.h>

typedef struct {
	uint16_t year; //<! ��,
	uint8_t month; //<! ��, 1-12
	uint8_t date; //<! ��, 1-31
	uint8_t day;  //<! ����, 1-7
	uint8_t hour; //<! 0-23
	uint8_t minute; //<! 0-59
	uint8_t second; //<! 0-59
} DateTime;


/// \param second ��2000/1/1-00:00:00��֮��ʼ��ʱ������.
void SecondToDateTime(DateTime *dateTime, uint32_t second);
/// \return ��2000/1/1-00:00:00��֮��ʼ��ʱ������.
uint32_t DateTimeToSecond(const DateTime *dateTime);


#endif
