/******************************************************************************/

/******************************************************************************/


/******************************************************************************/
/******************************************************************************/

// �û���Ϣ���뷽ʽ
#define GSM_7BIT 0
#define GSM_8BIT 4
#define GSM_UCS2 8
// ����Ϣ�����ṹ������ / ���빲��

const char	PhoneNum[32]; // Ŀ������ظ����� (TP-DA �� TP-RA)
const char	TP_DCS; // �û���Ϣ���뷽ʽ (TP-DCS)
bit	    	   	message_ture = 0;	//��ִȷ��

/******************************************************************************/

/******************************************************************************/

// Bit7 ����
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ����봮ָ��
// nSrcLength: Դ�ַ�������
// ���� : Ŀ����봮����
int EncodeBit7(const char	*pSrc,const char *pDst, int nSrcLength) {
	int nSrc; // Դ�ַ����ļ���ֵ
	int nDst; // Ŀ����봮�ļ���ֵ
	int nChar; // ��ǰ���ڴ���������ַ��ֽڵ���ţ���Χ�� 0-7
	int nLeft; // ��һ�ֽڲ��������
	// ����ֵ��ʼ��
	nSrc = 0;
	nDst = 0;
	// ��Դ��ÿ 8 ���ֽڷ�Ϊһ�飬ѹ���� 7 ���ֽ�
	// ѭ���ô�����̣�ֱ��Դ����������
	// ������鲻�� 8 �ֽڣ�Ҳ����ȷ����
	while(nSrc<nSrcLength) {
		nChar = nSrc & 7;  // ȡԴ�ַ����ļ���ֵ����� 3 λ
		if(nChar == 0) {   // ����Դ����ÿ���ֽ�
			nLeft = *pSrc; 		 // ���ڵ�һ���ֽڣ�ֻ�Ǳ�����������������һ���ֽ�ʱʹ��
		} else {
			*pDst = (*pSrc << (8-nChar)) | nLeft;  	// ���������ֽڣ������ұ߲��������������ӣ��õ�һ��Ŀ������ֽ�
			nLeft = *pSrc >> nChar; 				// �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
			pDst++;					// �޸�Ŀ�괮��ָ��ͼ���ֵ
			nDst++;
		}
		pSrc++; 	 // �޸�Դ����ָ��ͼ���ֵ
		nSrc++;
	}
	*pDst = nLeft;

	return (nSrcLength - nSrcLength/8); 	 // ����Ŀ����봮����
}

/******************************************************************************/
// 7-bit ����
// pSrc: Դ���봮ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ���봮����
// ���� : Ŀ���ַ�������
int DecodeBit7(const char	*pSrc,const char	*pDst, int nSrcLength) {
	int nDst; // Ŀ����봮�ļ���ֵ
	int nByte; // ��ǰ���ڴ���������ַ��ֽڵ���ţ���Χ�� 0-7
	int nLeft; // ��һ�ֽڲ��������

	// ����ֵ��ʼ��
	nDst = 0; // �����ֽ���źͲ������ݳ�ʼ��
	nByte = 0;
	nLeft = 0;

	// ��Դ����ÿ 7 ���ֽڷ�Ϊһ�飬��ѹ���� 8 ���ֽ�
	// ѭ���ô�����̣�ֱ��Դ���ݱ�������
	// ������鲻�� 7 �ֽڣ�Ҳ����ȷ����
	while(nDst<nSrcLength) {
		*pDst = ((*pSrc << nByte) | nLeft) & 0x7f;    // ��Դ�ֽ��ұ߲��������������ӣ�ȥ�����λ���õ�һ��Ŀ������ֽ�
		nLeft = *pSrc >> (7-nByte);  	 // �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
		pDst++; 			// �޸�Ŀ�괮��ָ��ͼ���ֵ
		nDst++;
		nByte++;  			// �޸��ֽڼ���ֵ
		if(nByte == 7) {	// ����һ������һ���ֽ�
			*pDst = nLeft;	// ����õ�һ��Ŀ������ֽ�
			pDst++; 		// �޸�Ŀ�괮��ָ��ͼ���ֵ
			nDst++;
			nByte = 0; 	  	// �����ֽ���źͲ������ݳ�ʼ��
			nLeft = 0;
		}
		pSrc++; 		    // �޸�Դ����ָ��ͼ���ֵ
	}
	*pDst = 0;
	return (nSrcLength); 		  // ����Ŀ�괮����
}

/******************************************************************************/
// BetyChange ����
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ����봮ָ��
// nSrcLength: Դ�ַ�������
// ���� : Ŀ����봮����
UINT EncodeBetyChange(const char	*pSrc,const char	*pDst, int nSrcLength) {
	int	i;
	UINT	xdata	len;
	int	code	tab[]= {"0123456789ABCDEF"}; // 0x0-0xf ���ַ����ұ�

	for(i=0; i<nSrcLength; i++) {
		*pDst++ = tab[*pSrc >> 4]; // ����� 4 λ
		*pDst++ = tab[*pSrc & 0x0f];// ����� 4 λ
		pSrc++;
	}
	len = ((UINT)nSrcLength) * 2;
	return (len); 	 // ����Ŀ����봮����
}

/******************************************************************************/
// BetyChange ����
// pSrc: Դ���봮ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ���봮����
// ���� : Ŀ���ַ�������
int	DecodeBetyChange(const char	*pSrc,const char	*pDst, int nSrcLength) {
	int	i;

	for(i=0; i<nSrcLength; i++) {
		if(*pSrc>='0' && *pSrc<='9') {   // ����� 4 λ
			*pDst = (*pSrc - '0') << 4;
		} else {
			*pDst = (*pSrc - 'A' + 10) << 4;
		}
		pSrc++;

		if(*pSrc>='0' && *pSrc<='9') {   // ����� 4 λ
			*pDst |= *pSrc - '0';
		} else {
			*pDst |= *pSrc - 'A' + 10;
		}
		pSrc++;
		pDst++;
	}

	return (nSrcLength); 	   // ����Ŀ���ַ�������
}

/******************************************************************************/
// ����˳����ַ���ת��Ϊ�����ߵ����ַ�����������Ϊ�������� 'F' �ճ�ż��
// �磺 "8613851872468" --> "683158812764F8"
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ�ַ�������
// ���� : Ŀ���ַ�������
int InvertNumbers(const char	*pSrc,const char	*pDst,int nSrcLength) {
	int nDstLength; // Ŀ���ַ�������
	int ch,i;

	nDstLength = nSrcLength;

	for(i=0; i<nSrcLength; i+=2) { // �����ߵ�
		ch = *pSrc++; // �����ȳ��ֵ��ַ�
		*pDst++ = *pSrc++; // ���ƺ���ֵ��ַ�
		*pDst++ = ch; // �����ȳ��ֵ��ַ�
	}

	if(nSrcLength & 1) {  // Դ��������������
		*(pDst-2) = 'F'; // �� 'F'
		nDstLength++; // Ŀ�괮���ȼ� 1
	}

	return(nDstLength); 	  // ����Ŀ���ַ�������
}

/******************************************************************************/
// �����ߵ����ַ���ת��Ϊ����˳����ַ���
// �磺 "683158812764F8" --> "8613851872468"
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ�ַ�������
// ���� : Ŀ���ַ�������
int Invert_Return(const char	*pSrc,const char	*pDst,int nSrcLength) {
	int nDstLength; // Ŀ���ַ�������
	int ch,i;

	nDstLength = nSrcLength;

	for(i=0; i<nSrcLength; i+=2) { // �����ߵ�
		ch = *pSrc++; // �����ȳ��ֵ��ַ�
		*pDst++ = *pSrc++; // ���ƺ���ֵ��ַ�
		*pDst++ = ch; // �����ȳ��ֵ��ַ�
	}

	if(*(pDst-1) == 'F') {
		pDst--;
		nDstLength--; // Ŀ���ַ������ȼ� 1
	}

	return(nDstLength); 	  // ����Ŀ���ַ�������
}

/******************************************************************************/

// ������ PDU ȫ���ı����ģ�顣Ϊ�򻯱�̣���Щ�ֶ����˹̶�ֵ��
// PDU ���룬���ڱ��ơ����Ͷ���Ϣ
// pSrc: Դ PDU ����ָ��
// pDst: Ŀ�� PDU ��ָ��
// ���� : Ŀ�� PDU ������
UINT gsmEncodePdu(const char	*pSrc,const char	*pDst) {
	UINT 	xdata		nDstLength; // Ŀ�� PDU ������
	int 	xdata		code_char[160];	  //�����ַ�������
	const char 		buf[5]; // �ڲ��õĻ�����
	const char 		m;

	buf[0] = 0x00;								 //Ĭ�����ĺ���
	nDstLength = EncodeBetyChange(buf, pDst, 1); // ת�� 1 ���ֽڵ�Ŀ�� PDU ��

	if(message_ture == 1)
		buf[0] = 0x31; //��Ҫ��ִ
	else
		buf[0] = 0x11;	  // �Ƿ��Ͷ��� (TP-MTI=01) �� TP-VP ����Ը�ʽ (TP-VPF=10)

	buf[1] = 0x00; // TP-MR=0
	m = strlen(PhoneNum);
	buf[2] = m; // Ŀ���ַ���ָ��� (TP-DA ��ַ�ַ�����ʵ���� )
	if((PhoneNum[0] == '8')&&(PhoneNum[1] == '6'))
		buf[3] = 0x91; //�ù��ʸ�ʽ����
	else
		buf[3] = 0x81; //�ù��ڸ�ʽ����

	nDstLength += EncodeBetyChange(buf, pDst + nDstLength, 4); // ת�� 4 ���ֽڵ�Ŀ�� PDU ��

	nDstLength += InvertNumbers(PhoneNum, pDst + nDstLength, m); // ת�� TP-DA ��Ŀ�� PDU ��

	// TPDU ��Э���ʶ�����뷽ʽ���û���Ϣ��
	buf[0] = 0; // Э���ʶ (TP-PID)
	buf[1] = TP_DCS; // �û���Ϣ���뷽ʽ (TP-DCS)
	buf[2] = 255; // ��Ч�� (TP-VP)=0 Ϊ 5 ���� ; =255Ϊ���
	buf[3] = strlen(pSrc);  //��Ϣ����

	if(data_gsm == 1)
		buf[3] = data_len;

	if(buf[1] == GSM_7BIT) {
		buf[4] = EncodeBit7(pSrc, code_char, buf[3]);
	} else	if(buf[1] == GSM_8BIT) {
		buf[4] = buf[3];
		memcpy(code_char, pSrc, buf[3]);
	}

	nDstLength += EncodeBetyChange(buf, pDst + nDstLength, 4); // ת�� 4 ���ֽڵ�Ŀ�� PDU ��

	nDstLength += EncodeBetyChange(code_char, pDst + nDstLength, buf[4]); // ת����Ϣ���ݵ�Ŀ�� PDU ��

	*(pDst + nDstLength) = 0x1a; // �� Ctrl-Z ����
	nDstLength++;

	return (nDstLength); 		   // ����Ŀ���ַ�������
}

/******************************************************************************/

// PDU ���룬���ڽ��ա��Ķ�����Ϣ
// pSrc: Դ PDU ��ָ��
// pDst: Ŀ�� PDU ����ָ��
// ���� : �û���Ϣ������
int gsmDecodePdu(const char	*pSrc,const char	*pDst) {
	int 	nDstLength; // Ŀ�� PDU ������
	int 	xdata	code_char[165];	  //�����ַ�������
	int 	code	receive_ture[] = {"ok"};	  //�����ַ�������
	const char	i,m;

	m=(*(pSrc+1))&0xf; // ȡSMSC ���봮����
	pSrc += m*2 + 2;   // ָ�����

	DecodeBetyChange(pSrc,&m,1);		  // ��������
	pSrc += 2; // ָ�����

	if((m & 0x02) == 0x02) {	//���Ž���ȷ����Ϣ
		pSrc += 2;

		DecodeBetyChange(pSrc, &m, 1);	// ȡ����
		pSrc += 4;			// ָ����ƣ������˻ظ���ַ(TP-RA)��ʽ

		nDstLength = Invert_Return(pSrc,pDst,(m+1)&0xfe);	 // ȡ TP-RA ����
		pSrc +=	(m+1)&0xfe;		// ָ�����

		*(pDst+nDstLength) = ',';
		nDstLength++;

		pSrc += 14;
		nDstLength += Invert_Return(pSrc,pDst+nDstLength,12);	// ����ʱ����ַ���

		*(pDst+nDstLength) = ',';
		nDstLength++;

		memcpy(pDst+nDstLength, receive_ture, 2);
		nDstLength += 2;

		return  (nDstLength);
	}
	//08 91683108505905F0 06 1D 0D 91683158703261F6 505021908480 23 505021908411 23 00 FF  //��ִPDU

	//08 91683108200505F0 24    0D 91683158714209F8 00 00 400152803535 00 04 D4F29C0E	   //����PDU

	DecodeBetyChange(pSrc,code_char,1);
	m = code_char[0];						//�ظ���ַ��Ϣ���� (TP-UDL)

	pSrc += 4; // ָ�����

	nDstLength = Invert_Return(pSrc,pDst,(m+1)&0xfe);	 // ȡ TP-RA ����
	pSrc +=	(m+1)&0xfe;

	*(pDst+nDstLength) = ',';
	nDstLength++;

	pSrc += 2;

	TP_DCS = *(pSrc+1) & 0x0f; 								   //���뷽ʽ
	pSrc += 2;

	nDstLength += Invert_Return(pSrc,pDst+nDstLength,12);	// ����ʱ����ַ���
	pSrc += 14;

	*(pDst+nDstLength) = ',';
	nDstLength++;

	DecodeBetyChange(pSrc,code_char,1);
	m = code_char[0];						// �û���Ϣ���� (TP-UDL)
	pSrc += 2;

	DecodeBetyChange(pSrc,code_char,m);					// ��ʽת��

	if(TP_DCS == GSM_7BIT) {
		nDstLength += DecodeBit7(code_char,pDst+nDstLength,m);				// 7-bit ����
	} else	if(TP_DCS == GSM_8BIT) {
		memcpy(pDst+nDstLength, code_char, m); 					   	// 8-bit ����
		nDstLength += m;
	} else	if(TP_DCS == GSM_UCS2) {
		for(i = 0; i < m;  ) {
			*(pDst+nDstLength) = *(code_char + i + 1); 					   	// 16-bit ����
			nDstLength ++;
			i += 2;
		}
	}

	*(pDst+nDstLength) = '\n';
	nDstLength++;
	*(pDst+nDstLength) = '\0';
	nDstLength++;

	return (nDstLength);
}

/******************************************************************************/

// ���Ͷ���Ϣ
// pSrc: Դ PDU ����ָ��
bit	SendMessage(const char	*pSrc) {
	UINT  xdata	PduLength; // PDU ������
	int xdata	SmsLength; // SMS ������
	UINT  xdata	r_len;	 // �����յ������ݳ���
	int m,i,j;
	int code	cmd0[9]= {"AT+CMGS="}; // ���
	int xdata	cmd[9]; // ���
	int xdata	pdu[500]; // PDU ��
	int xdata ans[64]; // Ӧ��

	while(gsm_busy)
		os_wait(K_TMO,1,0);

	gsm_busy = 1;

	PduLength = gsmEncodePdu(pSrc, pdu); // ���� PDU ���������� PDU ��

	SmsLength = (PduLength - 2)/2; // ȡ PDU ���е� SMS ��Ϣ����

	for(i=0; i<8; i++)
		cmd[i]=cmd0[i];

	get_string_clear1( );				//��ջ�����

	put_string1(cmd, 8);

	i = SmsLength/100;
	if(i != 0)
		put_char1(i +0x30);		   //����ָ��

	SmsLength = SmsLength%100;
	m = SmsLength/10;
	if((m != 0)||(i != 0))
		put_char1(m +0x30);		   //����PDU����

	put_char1((SmsLength%10)+0x30);
	put_char1(0x0d);
	put_char1(0x0a);

	for(i = 2; i>0; i--) {
		r_len = get_string1(ans);	 // ��Ӧ������
		if(r_len != 0) {
			break;
		}
	}
	if(r_len == 0) {
		gsm_busy = 0;
		return(FALSE);
	}

	if(ans[r_len-2] != '>') {
		gsm_busy = 0;
		return(FALSE);
	}
	put_string1(pdu,PduLength);		//����PDU

	for(j=0; j<4; j++) {
		for(i=0; i<25; i++) {
			memset(ans,0x00,64);
			r_len = get_string1(ans);	 // ��Ӧ������
			if(r_len > 0) {
				break;
			}
		}

		ans[r_len] = '\0';

		if((r_len > 3) && (strstr(ans,"OK") != NULL)) {	 //����OK
			gsm_busy = 0;
			day_inc = 0;
			return(TRUE);
		}
	}
	gsm_busy = 0;
	return(FALSE);
}

/******************************************************************************/
// ɾ������Ϣ
// index: ����Ϣ��ţ��� 1 ��ʼ
void DeleteMessage(int index) {
	int m,i;
	int code	cmd0[9]= {"AT+CMGD="}; // ���
	int xdata	cmd[9]; 			// ���

	for(i=0; i<8; i++)
		cmd[i]=cmd0[i];

	get_string_clear1( );				//��ջ�����

	put_string1(cmd, 8);
	m = index/10;
	if(m != 0)
		put_char1(m +0x30);
	put_char1((index%10)+0x30);
	put_char1(0x0d);
	put_char1(0x0a);

	get_string_clear1( );				//��ջ�����
}

/******************************************************************************/

// ��ȡ����Ϣ
// ��CMGL
//һ�ζ���һ������Ϣ
// pDrc �����ַ���ָ��
// index �������
//���أ��ַ�������
int ReadMessage(const char	*pDrc) {
	int m,i,index;
	UINT  xdata	r_len; 		 // �����յ������ݳ���
	int code	cmd0[16]= {"AT+CMGL=4\r\n"}; // ���
	int xdata	cmd[16]; 			// ���
	int xdata	buf[500]; // ���ܻ���
	int xdata	*ptr;

	while(gsm_busy)
		os_wait(K_TMO,1,0);

	ptr = buf;
	gsm_busy = 1;

	for(i=0; i<12; i++)
		cmd[i]=cmd0[i];

	get_string_clear1( );				//��ջ�����

	put_string1(cmd, 11);

	for(i = 8; i>0; i--) {
		r_len = get_string1(buf);	 // ��Ӧ������
		if(r_len > 30)
			break;
	}

	if(r_len < 30) {
		gsm_busy = 0;
		return(0);
	}

	buf[r_len] = '\0';

	if((ptr = strstr(buf, "CMGL:")) != NULL) {
		ptr = strchr(buf,',');		// ����"+CMGL:"
		index = ((*(ptr-2))&0x0f)*10 + ((*(ptr-1))&0x0f);	// ��ȡ���

		ptr = strstr(ptr, "\r\n");	// ����һ��
		ptr += 2;		// ����"\r\n"

		m = gsmDecodePdu(ptr, pDrc);	// PDU������

		DeleteMessage(index);

		gsm_busy = 0;
		return(m);
	}
	gsm_busy = 0;
	return (0);
}

/******************************************************************************/
void read_storage(int	n) {
	int code	cmd0[]= {"AT+CPMS= ME , SM , SM \r\n"}; // ���
	int code	cmd1[]= {"AT+CPMS= SM , SM , SM \r\n"}; // ���
	int xdata	cmd[25]; 			// ���
	int i;

	while(gsm_busy)
		os_wait(K_TMO,1,0);
	gsm_busy = 1;

	if(n == 1) {
		for(i=0; i<24; i++)
			cmd[i]=cmd1[i];
	} else {
		for(i=0; i<24; i++)
			cmd[i]=cmd0[i];
	}
	cmd[8] = '"';
	cmd[11] = '"';
	cmd[13] = '"';
	cmd[16] = '"';
	cmd[18] = '"';
	cmd[21] = '"';
	get_string_clear1( );				//��ս��ջ�����
	put_string1(cmd, 24);   				//���ڽ��ջظ�����
	os_wait(K_TMO,10,0);
	get_string_clear1( );				//��ս��ջ�����

	gsm_busy = 0;
}

/******************************************************************************
// ��ȡ�绰��
// ��CPBR
//һ�ζ���һ���绰����
// pDrc �����ַ���ָ��
// index �绰�������
//���أ��ַ�������
int read_phonebook(const char	*pDrc,int	index)
{
	int m,i;
	int r_len; 		 // �����յ������ݳ���
	int code	cmd0[]={"AT+CPBR="}; // ���
	int code	cmd1[]={"AT+CPBS=ME\r\n"}; // ���
	int xdata	cmd[16]; 			// ���
	int xdata	buf[128]; // ���ܻ���
	int xdata	*ptr;

   	while(gsm_busy)
		os_wait(K_TMO,1,0);
	gsm_busy = 1;

	for(i=0;i<12;i++)
		cmd[i]=cmd1[i];

 	get_string1(buf);				//��շ��ͻ������ͽ��ջ�����
	put_string1(cmd, 12);

	os_wait(K_TMO,10,0);

	for(i=0;i<8;i++)
		cmd[i]=cmd0[i];

 	get_string1(buf);				//��շ��ͻ������ͽ��ջ�����
	put_string1(cmd, 8);

	m = index/10;
	if(m != 0)
		put_char1(m +0x30);
	put_char1((index%10)+0x30);
	put_char1(0x0d);
	put_char1(0x0a);

	for(i = 2; i>0; i--)
	{
		r_len = get_string1(buf);	 // ��Ӧ������
		if(r_len >15)
			break;
	}
	if(r_len <15)
	{
		gsm_busy = 0;
		return(0);
	}

	buf[127] = '\0';
	if((ptr = strchr(buf,'"')) != NULL)
	{
			ptr++;
			r_len = 0;
			m = *ptr;

			while(m != '"')
			{
				m = *ptr++;
				*pDrc++ = m;

			   	r_len++;
			}
			gsm_busy = 0;
			return(r_len-1);
	}
	gsm_busy = 0;
	return (0);
}

/******************************************************************************
// д�绰��
// ��CPBW
//һ��дһ���绰����
//pSrc: Դ�ַ���ָ��
//index �绰�������  	turn(TRUE); return(FALSE);
//len �绰���볤��
//���أ��ɹ�/ʧ��  		AT+CPBW=4,13459553766,129,CENTER
bit write_phonebook(const char	*pSrc,int	len,int	index)
{
	int m,i,j;
	int r_len; 		 // �����յ������ݳ���
	int code	cmd0[]={"AT+CPBW="}; // ���
	int code	cmd0_[]={",129,strong\r\n"}; // ���
	int code	cmd1[]={"AT+CPBS=ME\r\n"}; // ���
	int xdata	cmd[16]; 			// ���
	int xdata	buf[128]; // ���ܻ���
	int xdata	*ptr;

   	while(gsm_busy)
		os_wait(K_TMO,1,0);
	gsm_busy = 1;

	for(i=0;i<12;i++)
		cmd[i]=cmd1[i];

 	get_string1(buf);				//��շ��ͻ������ͽ��ջ�����
	put_string1(cmd, 12);

	os_wait(K_TMO,10,0);

	for(i=0;i<8;i++)
		cmd[i]=cmd0[i];

 	get_string1(buf);				//��շ��ͻ������ͽ��ջ�����
	put_string1(cmd, 8);

	m = index/10;
	if(m != 0)
		put_char1(m +0x30);
	put_char1((index%10)+0x30);
	put_char1(',');

	put_string1(pSrc, len);

	for(i=0;i<13;i++)
		cmd[i]=cmd0_[i];
	put_string1(cmd, 13);

	os_wait(K_TMO,1,0);

	for(j = 2; j>0; j--)
	{
		for(i = 2; i>0; i--)
		{
			r_len = get_string1(buf);	 // ��Ӧ������
			if(r_len > 0)
				break;
		}
		if(r_len == 0)
		{
			gsm_busy = 0;
			return(FALSE);
		}

		buf[127] = '\0';
		if((ptr = strstr(buf,"OK")) != NULL)
		{
			gsm_busy = 0;
			return(TRUE);
		}
	}

	gsm_busy = 0;
	return (FALSE);
}

/******************************************************************************/
bit  ReadTime(void) {
	UINT  xdata	r_len; 		 // �յ������ݳ���
	int 	i;
	int code	cmd0[ ]= {"AT+CCLK?\r\n"}; // ���
	int xdata	cmd[11]; 			// ���
	int xdata	buf[50]; // ���ܻ���
	int xdata	*ptr;
	int xdata  *pDrc;

	while(gsm_busy)
		os_wait(K_TMO,1,0);

	ptr = buf;
	pDrc = time;
	gsm_busy = 1;

	for(i=0; i<10; i++)
		cmd[i]=cmd0[i];

	get_string_clear1( );				//��ջ�����

	put_string1(cmd, 10);

	for(i = 2; i>0; i--) {
		r_len = get_string1(buf);	 // ��Ӧ������
		if(r_len > 12)
			break;
	}

	if(r_len < 12) {
		gsm_busy = 0;
		return(FALSE);
	}

	buf[127] = '\0';
	if((ptr = strstr(buf, "CCLK:")) != NULL) {
		ptr += 7;		// ����(+CCLK: ")

		for(i=0; i<6; i++) {
			*pDrc = ((*ptr&0x0f)<<4) + (*(ptr+1)&0x0f);
			pDrc++;
			ptr +=3;
		}

		gsm_busy = 0;
		return(TRUE);
	}

	gsm_busy = 0;
	return(FALSE);
}

/******************************************************************************/
bit		TimeCompare_(const char	*sp1,const char	*sp2) {
	if(*sp1>*sp2)	return(1);
	if(*sp1<*sp2)	return(0);
	sp1++;
	sp2++;

	if(*sp1>*sp2)	return(1);
	if(*sp1<*sp2)	return(0);
	sp1++;
	sp2++;

	if(*sp1>*sp2)	return(1);
	if(*sp1<*sp2)	return(0);
	sp1++;
	sp2++;

	if(*sp1>*sp2)	return(1);
	if(*sp1<*sp2)	return(0);
	sp1++;
	sp2++;

	if(*sp1>*sp2)	return(1);
	if(*sp1<*sp2)	return(0);
	sp1++;
	sp2++;

	if(*sp1>*sp2)	return(1);
	if(*sp1<*sp2)	return(0);

	return(0);
}
/******************************************************************************/
void  SetupTime(int xdata  *pSrc) {
	int xdata	buf1[33];
	int code	cmd[ ]= {"AT+CCLK= 00/00/00,00:00:00+00 \r\n"}; // ���
	int xdata	*sp;

	if((sp = strchr(pSrc,',')) != NULL) {
		sp++;
		buf1[0] = (((*sp) & 0x0f)<<4) + (*(sp+1) & 0x0f);
		sp += 2;
		buf1[1] = (((*sp) & 0x0f)<<4) + (*(sp+1) & 0x0f);
		sp += 2;
		buf1[2] = (((*sp) & 0x0f)<<4) + (*(sp+1) & 0x0f);
		sp += 2;
		buf1[3] = (((*sp) & 0x0f)<<4) + (*(sp+1) & 0x0f);
		sp += 2;
		buf1[4] = (((*sp) & 0x0f)<<4) + (*(sp+1) & 0x0f);
		sp += 2;
		buf1[5] = (((*sp) & 0x0f)<<4) + (*(sp+1) & 0x0f);

		if((TimeCompare_(buf1,time) == 1)||((memcmp(time,buf1,4) == 0)&&((time[4]-buf1[4]) == 1))) {
			memcpy(buf1,cmd,32);
			sp = strchr(pSrc,',') + 1;

			buf1[8] = '"';

			buf1[9]  = *sp++;
			buf1[10] = *sp++;

			buf1[12] = *sp++;
			buf1[13] = *sp++;

			buf1[15] = *sp++;
			buf1[16] = *sp++;

			buf1[18] = *sp++;
			buf1[19] = *sp++;

			buf1[21] = *sp++;
			buf1[22] = *sp++;

			buf1[24] = *sp++;
			buf1[25] = *sp++;

			buf1[29] = '"';

			while(gsm_busy)
				os_wait(K_TMO,1,0);

			gsm_busy = 1;

			get_string1(buf1);

			put_string1(buf1, 32);

			get_string1(buf1);				//��ս��ջ�����

			gsm_busy = 0;
		}
	}
}

/******************************************************************************/
void  gsm_status_init(void) {
	int 	i;
	int code	cmd0[ ]= {"AT+CMGF=0\r\n"}; 		// ��� :
	int code	cmd1[ ]= {"ATE0\r\n"}; 		       	// ��� : ���ڽ��ջظ�����
	int code	cmd2[ ]= {"AT+CNMI=3,1,0,2,0\r\n"}; 	// ��� : ������ʾ����
	int xdata	cmd[20]; 			// ���

	while(gsm_busy)
		os_wait(K_TMO,1,0);
	gsm_busy = 1;

	for(i=0; i<11; i++)
		cmd[i]=cmd0[i];
	get_string_clear1( );				//��ս��ջ�����
	put_string1(cmd, 11);   			//

	os_wait(K_TMO,100,0);

	for(i=0; i<6; i++)
		cmd[i]=cmd1[i];
	get_string_clear1( );				//��ս��ջ�����
	put_string1(cmd, 6);   				//���ڽ��ջظ�����

	os_wait(K_TMO,100,0);

	for(i=0; i<19; i++)
		cmd[i]=cmd2[i];
	get_string_clear1( );				//��ս��ջ�����
	put_string1(cmd, 19);   			//������ʾ����

	os_wait(K_TMO,10,0);
	get_string_clear1( );				//��ս��ջ�����

	gsm_busy = 0;
}

/******************************************************************************/
void  gsm_init(void) {
	gsm_igt_0( );

	os_wait( K_TMO, 250, 0);

	gsm_igt_1( );

	TP_DCS = GSM_8BIT;
}

/******************************************************************************/
void	gsm_power_down(void) {
	int 	i;
	int code	cmd0[ ]= {"AT+CFUN=0\r\n"}; 		// ��� : power down
	int xdata	cmd[11]; 			// ���

	while(gsm_busy)
		os_wait(K_TMO,1,0);
	gsm_busy = 1;

	for(i=0; i<11; i++)
		cmd[i]=cmd0[i];
	put_string1(cmd,11);   			// power down

	os_wait(K_TMO,250,0);
	os_wait(K_TMO,250,0);
	os_wait(K_TMO,250,0);
	os_wait(K_TMO,250,0);
	get_string_clear1( );				//��ս��ջ�����

	gsm_busy = 0;
}

/******************************************************************************/
bit	gprs_init(void) {
	int 	i,r_len;
	int xdata	buf[30];

	get_string_clear1( );				//��ս��ջ�����

	put_string1("at*e2ipa=1,1\r\n",14);   // ����GR47 PDP����

	for(i = 50; i>0; i--) {		  //10s
		r_len = get_string1(buf);	 // ��Ӧ������
		buf[r_len] = '\0';
		if(r_len > 2)
			break;
	}
	if(strstr(buf,"OK") != NULL) {
		return(TRUE);
	}
	return(FALSE);
}

/******************************************************************************/
void	cmd_deal_gprs(int xdata *);
/******************************************************************************/
bit	gprs_connect(void) {
	int 	i,r_len;
	int xdata	buf[400];

	get_string_clear1( );				//��ս��ջ�����

	put_string1("at*e2ipo?\r\n",11);   // ���ӣ�

	for(i = 10; i>0; i--) {			  //2s
		r_len = get_string1(buf);	 // ��Ӧ������
		buf[r_len] = '\0';
		if(r_len > 6)
			break;
	}
	if(strchr(buf,'1') != NULL) { //�Ѿ�����
		get_string_clear1( );				//��ս��ջ�����
		put_string1("ATO\r\n",5);
		for(i = 10; i>0; i--) {			  //2s
			r_len = get_string1(buf);	 // ��Ӧ������
			buf[r_len] = '\0';
			if(r_len > 6)
				break;
		}
		cmd_deal_gprs(buf);
		return(TRUE);
	}

	put_string1("at*e2ipo=1,",11);   // ����
	put_char1('"');
	put_string1(gprs_ip,strlen(gprs_ip));   	// ip
	put_char1('"');
	put_char1(',');
	put_string1(gprs_port,strlen(gprs_port));	//port
	put_string1("\r\n",2);

	for(i = 50; i>0; i--) {	  //10s
		r_len = get_string1(buf);	 // ��Ӧ������
		buf[r_len] = '\0';
		if(r_len > 6)
			break;
	}
	if(strstr(buf,"CONNECT") != NULL) { //�ɹ�
		return(TRUE);
	}
	return(FALSE);
}

/******************************************************************************/
void	gprs_close(void) {
	gprs_dtr = 0;
	os_wait(K_TMO,30,0);
	gprs_dtr = 1;
}

/******************************************************************************/
bit	gprs_send(const char *sp) {
	int 	m,r_len;
	int xdata	buf[400];

	while(gsm_busy)
		os_wait(K_TMO,1,0);
	gsm_busy = 1;

	m = 5;
	while( gprs_init( )==FALSE ) {
		gprs_close( );
		m--;
		if(m == 0) {
			gsm_busy = 0;
			return(FALSE);
		}
	}
	m = 5;
	while( gprs_connect( )==FALSE ) {
		gprs_close( );
		m--;
		if(m == 0) {
			gsm_busy = 0;
			return(FALSE);
		}
	}
	put_string1(sp,strlen(sp));

	for(m = 150; m>0; m--) {	  //30s
		r_len = get_string1(buf);	 // ��Ӧ������
		buf[r_len] = '\0';
		if(r_len > 1)
			break;
	}
	if(strstr(buf,"OK") != NULL) { //�ɹ�
		gprs_close( );
		gsm_busy = 0;
		return(TRUE);
	}
	gprs_close( );
	gsm_busy = 0;
	return(FALSE);
}

/******************************************************************************/
void	gprs_rcv(void) {
	int 	i,r_len;
	int xdata	buf[400];

	while(gsm_busy)
		os_wait(K_TMO,1,0);
	gsm_busy = 1;

	get_string_clear1( );				//��ս��ջ�����
	put_string1("at*e2ipo?\r\n",11);   // ���ӣ�

	for(i = 10; i>0; i--) {			  //2s
		r_len = get_string1(buf);	 // ��Ӧ������
		buf[r_len] = '\0';
		if(r_len > 6)
			break;
	}
	if(strchr(buf,'1') != NULL) { //�Ѿ�����
		get_string_clear1( );				//��ս��ջ�����
		put_string1("ATO\r\n",5);
		for(i = 10; i>0; i--) {			  //2s
			r_len = get_string1(buf);	 // ��Ӧ������
			buf[r_len] = '\0';
			if(r_len > 6)
				break;
		}
		cmd_deal_gprs(buf);
		os_wait(K_TMO,50,0);
		gprs_close( );
	}
	gsm_busy = 0;
}

/******************************************************************************/

