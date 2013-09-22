#ifndef __SOUNDCONTROL_H__
#define __SOUNDCONTROL_H__

#include <stdbool.h>
#include <stdint.h>

/// ��������ͨ��.
typedef enum {
	/// GSMͨ��.
	SOUND_CONTROL_CHANNEL_GSM = 0x01,
	/// Ѷ������ģ��ͨ��.
	SOUND_CONTROL_CHANNEL_XFS = 0x02,
	/// MP3����ͨ��.
	SOUND_CONTROL_CHANNEL_MP3 = 0x04,
	/// FM����ͨ��.
	SOUND_CONTROL_CHANNEL_FM = 0x08,
} SoundControlChannel;

/// ��ʼ����������.
void SoundControlInit(void);
/// ��������.
/// \param channels    ��Ҫ���Ƶ�ͨ��, ������SoundControlChannel����������.
/// \param isOn        !=false ����Ӧ��ͨ��.
/// \param isOn        =false  �ر���Ӧ��ͨ��.
void SoundControlSetChannel(uint32_t channels, bool isOn);

#endif
