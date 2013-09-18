#ifndef __SHT1X_H__
#define __SHT1X_H__

/// \brief   ��ʪ�ȴ�������ʼ��;
void SHT11Init(void);

/// \brief   ��ȡ��ʪ��;
/// \param   temp      ���ָ��, ��Ŷ�ȡ�����¶�, ��λ0.1���϶�;
/// \param   hmui      ���ָ��, ��Ŷ�ȡ����ʪ��, ��λ0.1;
/// \return  !=0       �ɹ�;
/// \return  =0        ʧ��;
int SHT11ReadTemperatureHumidity(int *temp, int *humi);

#endif
