/*
 * =====================================================================================
 *
 *       Filename:  sms.h
 *
 *    Description:  sms decode and encode headers
 *
 *        Version:  1.0
 *        Created:  2008-6-24 9:10:50
 *       Revision:  none
 *       Compiler:  C51
 *
 *         Author:  xiqingping(xiqingping@gmail.com)
 *        Company:  Beijin Cabletech development LTD.
 *
 * =====================================================================================
 */
#ifndef __SMS_H__
#define __SMS_H__

#define PDU_NUMBER_TYPE_INTERNATIONAL	0x91
#define PDU_NUMBER_TYPE_NATIONAL		0x81
#define GPRS_NUMBER_TYPE				0xFF

#define ENCODE_TYPE_GBK  0
#define ENCODE_TYPE_UCS2 1

typedef struct {
	unsigned char numberType;
	unsigned char encodeType;
	unsigned char contentLen;
	char number[15];
	char time[15];
	char content[700];
} SMSInfo;

/// \brief  ����PDU����.
/// \param  pdu[in]     ��Ҫ�����PDU.
/// \param  sms[out]    ���ڴ�Ž���֮��Ķ�����Ϣ.
void SMSDecodePdu(const char *pdu, SMSInfo *sms);

/// \brief  ��8bit��ʽ�����ݱ����PDU.
/// \param  pdu[out]    ���ڴ�ű���֮���PDU����.
/// \param  destNum[in] ���ŷ��͵�Ŀ�����.
/// \param  dat[in]     ��Ҫ������ַ�������.
/// \return �����PDU�����ֽڳ���.
/// \note   ֻ�ܱ���ASCII�ַ���.
int SMSEncodePdu8bit(char *pdu, const char *destNum, const char *dat);

/// \brief  ��UCS2��ʽ�����ݱ����PDU.
/// \param  pdu[out]    ���ڴ�ű���֮���PDU����.
/// \param  destNum[in] ���ŷ��͵�Ŀ�����.
/// \param  dat[in]     ��Ҫ������ַ�������.
/// \param  len[in]     ��Ҫ������ַ������ݵ��ֽڳ���.
/// \return �����PDU�����ֽڳ���.
/// \note   ֻ�ܱ���UCS2����.
int SMSEncodePduUCS2(char *pdu, const char *destNum, const char *ucs2, int len);

#endif // ifndef __SMS_H__


