////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ��TransDataBuffer.h
// Project Code ����Ⱥ����ת��ʱ�����η�װ�Ļ�����
// Abstract     ��
// Reference    ��
//
// Version      ��1.0
// Author       ��LiuJun
// Accomplished date �� 04 14, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#ifndef __LIUJUN_20150414_TRANS_DATA_BUFFER_H
#define __LIUJUN_20150414_TRANS_DATA_BUFFER_H
#include "globaldef.h"
#include <assert.h>
#include "ServiceThread.h"
#include <memory.h>
#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif
////////////////////////////////////////////////////////////////////////////
//����ת������ʱ����󻺴����,��������ײ�Ĺ����̸߳���,���ֵ32��������ײ��߳�Ĭ��ΪCPU*2
#define CLUSTER_TRANS_DATA_MEM_NUM				64
typedef struct tagTransDataBuffer
{
	WORD		wBufferSize;
	WORD		wDataSize;
	char		*pBuffer;

	tagTransDataBuffer()
	{
		wBufferSize = 0;
		wDataSize = 0;
		pBuffer = NULL;
	}
	~tagTransDataBuffer()
	{
		if(pBuffer!= NULL)
			free(pBuffer);
		pBuffer = NULL;
		wBufferSize = 0;
		wDataSize = 0;
	}
	//�����Ҫת�ֽ������ڵ���ǰת
	void SetData(char *pData,WORD wSize)
	{
		if(wBufferSize < wDataSize + wSize)
		{ 
			wBufferSize += (wSize < 1024)?1024:wSize;
			char *pTmp = (char*)malloc(wBufferSize);
			if(pBuffer != NULL)
			{
				memcpy(pTmp,pBuffer,wDataSize);
				free(pBuffer);
			}
			pBuffer = pTmp; 
		}
		ASSERT(pBuffer != NULL);
		memcpy(pBuffer + wDataSize,pData,wSize);
		wDataSize += wSize;
	} 
	void Release()
	{
		wDataSize = 0;
	}
}TRANSDATABUFFER;
////////////////////////////////////////////////////////////////////////////
class CTransDataBuffer
{
private:
	TRANSDATABUFFER			m_transDataBuffer[CLUSTER_TRANS_DATA_MEM_NUM];
	volatile DWORD			m_dwTransDataBufferIndex;

public:
	CTransDataBuffer();
	~CTransDataBuffer();

public:
	DWORD GetBufferTotalSize();
	TRANSDATABUFFER* GetTransDataBuffer();
private:
	CThreadLock m_Lock;
};
#endif //__LIUJUN_20150414_TRANS_DATA_BUFFER_H
