#ifndef __SHT1X_H__
#define __SHT1X_H__

#include <stdbool.h>

/// \brief   ��ʪ�ȴ�������ʼ��;
void SHT10Init(void);

/// \brief   ��ȡ��ʪ��;
/// \param   temp      ���ָ��, ��Ŷ�ȡ�����¶�, ��λ0.1���϶�;
/// \param   hmui      ���ָ��, ��Ŷ�ȡ����ʪ��, ��λ0.1;
/// \return  true      �ɹ�;
/// \return  false     ʧ��;
bool SHT10ReadTemperatureHumidity(int *temp, int *humi);

#endif
