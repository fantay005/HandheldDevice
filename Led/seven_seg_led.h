#ifndef __SEVEN_SEG_LEG_HH__
#define __SEVEN_SEG_LEG_HH__

#include <stdbool.h>
#include <stdint.h>

/// \brief  ����ܵ�����.
#define SEVEN_SEG_LED_NUM 20

/// \brief  ��ʼ��7��������ʾ.
void SevenSegLedInit(void);

/// \brief  ����Ҫ�ر�ĳ�������ʱ, SevenSegLedSetContent��what�����������ֵ.
#define SEVEN_SEG_LED_OFF 0xFF

/// \brief  ����7��������ʾ������.
/// \param  index   ����ܵı��, ������{ 0 - (SEVEN_SEG_LED_NUM-1) }.
/// \param  what    ��Ҫ��ʾ������, ȡֵ��Χ��{ 0-9, SEVEN_SEG_LED_OFF }.
/// \return true    �ɹ�,
/// \return false   ʧ��.
bool SevenSegLedSetContent(unsigned int index, uint8_t what);

/// \brief  ��ʾ�����õ�����.
void SevenSegLedDisplay(void);

#endif
