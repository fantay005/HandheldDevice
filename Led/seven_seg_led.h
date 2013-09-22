#ifndef __SEVEN_SEG_LEG_HH__
#define __SEVEN_SEG_LEG_HH__

#include <stdbool.h>

/// ��ʼ��7��������ʾ.
void SevenSegLedInit(void);
/// ����7��������ʾ������.
/// \param index ����ܵı��.
/// \param waht ��Ҫ��ʾ������.
/// \return true �ɹ�,
/// \return false ʧ��.
bool SevenSegLedSetContent(unsigned int index, char what);
void SevenSegLedDisplay(void);

#endif
