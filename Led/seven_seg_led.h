#ifndef __SEVEN_SEG_LEG_HH__
#define __SEVEN_SEG_LEG_HH__

#include <stdbool.h>

#define SEVEN_SEG_LED_NUM 20

/// ��ʼ��7��������ʾ.
void SevenSegLedInit(void);

/// ����7��������ʾ������.
/// \param index ����ܵı��, ������0 -- (SEVEN_SEG_LED_NUM-1).
/// \param waht ��Ҫ��ʾ������.
/// \return true �ɹ�,
/// \return false ʧ��.
bool SevenSegLedSetContent(unsigned int index, char what);

/// ��ʾ�����õ�����.
void SevenSegLedDisplay(void);

#endif
