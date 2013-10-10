#ifndef __CNXIQP_H__
#define __CNXIQP_H__

/// \brief  CPU����ı߽�.
#define ALIGNED_SIZE 4

/// \brief  ����һ������ı߽�����Ĵ�С.
/// ���һ�������ʵ�ʴ�СΪ n*ALIGNED_SIZE+1, n*ALIGNED_SIZE+2 ... (n+1)*ALIGNED_SIZE,
/// �ú��������Ĵ�СΪ(n+1)*ALIGNED_SIZE.
#define ALIGNED_SIZEOF(a) (((sizeof(a) + ALIGNED_SIZE - 1) / ALIGNED_SIZE) * ALIGNED_SIZE)

/// \brief ���������Ա������.
/// \note  array����������.
#define ARRAY_MEMBER_NUMBER(array) (sizeof(array)/sizeof(array[0]))

#endif
