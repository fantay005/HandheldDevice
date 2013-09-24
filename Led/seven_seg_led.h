#ifndef __SEVEN_SEG_LEG_HH__
#define __SEVEN_SEG_LEG_HH__

#include <stdbool.h>
#include <stdint.h>

#define SEVEN_SEG_LED_NUM 20

/// ��ʼ��7��������ʾ.
void SevenSegLedInit(void);

#define SEVEN_SEG_LED_OFF 0xFF

/// ����7��������ʾ������.
/// \param index ����ܵı��, ������0 -- (SEVEN_SEG_LED_NUM-1), or SEVEN_SEG_LED_OFF.
/// \param waht ��Ҫ��ʾ������.
/// \return true �ɹ�,
/// \return false ʧ��.
bool SevenSegLedSetContent(unsigned int index, uint8_t what);

/// ��ʾ�����õ�����.
void SevenSegLedDisplay(void);

#endif
