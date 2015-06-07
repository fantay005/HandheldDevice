#ifndef __ATCMD_H__
#define __ATCMD_H__

#include <stdbool.h>
#include "FreeRTOS.h"

/// \brief  ���ATCommand�����κλ�Ӧ, prefix��Ҫ�����������.
#define ATCMD_ANY_REPLY_PREFIX  ((const char *)0xFFFFFFFF)

/// \brief  ��ʼ��.
/// ��ʼ��ATCommand��غ���������Ҫ�Ļ���.
void ATCommandRuntimeInit(void);

/// \brief  ����AT����ȴ�����.
/// \param  cmd[in]     ��Ҫ���͵�AT����, ��Ҫ����'\r'�ַ�.
/// \param  prefix[in]  ��Ҫ�ȴ��Ļ�Ӧ�Ŀ�ʼ�ַ���.
/// \param  timoutTick  �ȴ���Ӧ���ʱ��.
/// \return !=NULL      ����������ȷ, �����Ѿ��ȵ���Ӧ.
/// \return ==NULL      �����������, ��ȴ���Ӧ��ʱ.
/// \note               ������ֵ��Ϊ��ʱ, ʹ���귵��ֵ֮����Ҫ����AtCommandDropReplyLine�ͷ��ڴ�.
char *ATCommand(const char *cmd, const char *prefix, int timeoutTick);

/// \brief  �ͷ�AT���������.
/// \param  line[in]   ��Ҫ�ͷŵ�AT���������.
void AtCommandDropReplyLine(char *line);

/// \brief  ����AT�������Ӧ.
/// \param  cmd[in]     ��Ҫ���͵�AT����, ��Ҫ����'\r'�ַ�.
/// \param  prefix[in]  ��Ҫ�ȴ��Ļ�Ӧ�Ŀ�ʼ�ַ���.
/// \param  timoutTick  �ȴ���Ӧ���ʱ��.
/// \return true        �ڵȴ�ʱ�����յ���ȷ��Ӧ.
/// \return false       �ڵȴ�ʱ����δ�յ���ȷ��Ӧ.
bool ATCommandAndCheckReply(const char *cmd, const char *prefix, int timeoutTick);


/// \brief  ����AT�������Ӧ, ֱ���յ���ȷ��Ӧ.
/// \param  cmd[in]     ��Ҫ���͵�AT����, ��Ҫ����'\r'�ַ�.
/// \param  prefix[in]  ��Ҫ�ȴ��Ļ�Ӧ�Ŀ�ʼ�ַ���.
/// \param  timoutTick  ÿ�εȴ���Ӧ���ʱ��.
/// \param  times       ���ȴ��Ĵ���.
/// \return true        �ڵȴ�ʱ�����յ���ȷ��Ӧ.
/// \return false       �ڵȴ�ʱ����δ�յ���ȷ��Ӧ.
bool ATCommandAndCheckReplyUntilOK(const char *cmd, const char *prefix, int timeoutTick, int times);

/// \brief �ú����� ���ڽ��յ��Իس�����������֮����Ҫ���õ�.
/// �ú����ǰɴ��ڽ��յ������ݷ���AT�����Ķ�����.
/// \param  line[in]    �յ�������.
/// \param  len[in]     �յ������ݵĳ���.
/// \param  pxHigherPriorityTaskWoken[out]  ����Ƿ���Ҫ������ȵı�־.
/// \return true        ���ݳɹ�����AT��������.
/// \return false       ���ݷ���AT��������ʧ��.
bool ATCommandGotLineFromIsr(const char *line, unsigned char len, portBASE_TYPE *pxHigherPriorityTaskWoken);

#endif
